#!/usr/bin/env python3
"""Зняття реальної цінової сходинки LCSC для деталей BOM.

Локальна база Copperpour (jlcpcb.db) віддає ціну за 1 штуку. Реальна ціна
залежить від обсягу (ladder). Цей інструмент читає сходинку з картки товару
LCSC і оновлює `hardware/bom/bom-draft.csv`.

Використання:
    python3 tools/lcsc_prices.py --codes C191341,C5187096
    python3 tools/lcsc_prices.py --from-bom hardware/bom/bom-draft.csv --qty 100
    python3 tools/lcsc_prices.py --from-bom hardware/bom/bom-draft.csv --qty 100 \\
        --update-bom            # записати ціну за обсяг у BOM

Сховище: https://www.lcsc.com/product-detail/<CODE>.html
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path

UA = ("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/124.0 Safari/537.36")
TIMEOUT = 30


@dataclass
class Tier:
    qty: int
    usd: float


@dataclass
class Part:
    lcsc: str
    mpn: str
    tiers: list[Tier]
    stock: int
    package: str
    library_type: str = ""

    def price_at(self, qty: int) -> float:
        """Ціна за штуку при замовленні qty штук."""
        best = self.tiers[0]
        for t in self.tiers:
            if qty >= t.qty:
                best = t
            else:
                break
        return best.usd


def fetch_product(code: str) -> str:
    url = f"https://www.lcsc.com/product-detail/{code}.html"
    req = urllib.request.Request(url, headers={"User-Agent": UA,
                                               "Accept-Language": "en-US,en;q=0.9"})
    with urllib.request.urlopen(req, timeout=TIMEOUT) as resp:
        return resp.read().decode("utf-8", errors="replace")


def extract_array(html: str, key: str) -> list | None:
    """Витягує JSON-масив із ключа у вбудованому __NEXT_DATA__."""
    marker = f'"{key}":['
    start = html.find(marker)
    if start < 0:
        return None
    i = start + len(marker) - 1
    depth = 0
    in_str = False
    esc = False
    for j in range(i, len(html)):
        ch = html[j]
        if in_str:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                in_str = False
            continue
        if ch == '"':
            in_str = True
        elif ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
            if depth == 0:
                try:
                    return json.loads(html[i:j + 1])
                except json.JSONDecodeError:
                    return None
    return None


def parse_part(code: str, html: str) -> Part | None:
    raw = extract_array(html, "productPriceList")
    if not raw:
        return None
    tiers: list[Tier] = []
    for row in raw:
        try:
            tiers.append(Tier(int(row["ladder"]), float(row["usdPrice"])))
        except (KeyError, TypeError, ValueError):
            continue
    if not tiers:
        return None
    tiers.sort(key=lambda t: t.qty)

    mpn = ""
    package = ""
    stock = 0
    m = re.search(r'"productModel":"([^"]+)"', html)
    if m:
        mpn = m.group(1)
    m = re.search(r'"encapStandard":"([^"]+)"', html)
    if m:
        package = m.group(1)
    m = re.search(r'"stockNumber":(\d+)', html)
    if m:
        stock = int(m.group(1))
    return Part(lcsc=code, mpn=mpn, tiers=tiers, stock=stock, package=package)


def codes_from_bom(path: Path) -> list[tuple[str, str]]:
    out: list[tuple[str, str]] = []
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            code = (row.get("lcsc") or "").strip()
            if code and code.upper().startswith("C") and code[1:].isdigit():
                out.append((code, (row.get("mpn") or "").strip()))
    return out


def fmt_table(parts: list[Part], qty: int) -> str:
    """Таблиця сходинок.

    Розриви сходинки в LCSC різні для кожної деталі (напр. SX1262: 1/10/30/100/500/1000,
    CH32V003: 5/50/150/500/2500/4000), тому кожну деталь друкуємо окремо.
    """
    lines = [f"Ціни LCSC станом на {qty} шт. замовлення", ""]
    for p in parts:
        cells = "  ".join(f"@{t.qty}:${t.usd:.4f}" for t in p.tiers)
        lines.append(f"{p.lcsc}  {p.mpn}")
        lines.append(f"    {cells}")
        lines.append(f"    -> @100 = ${p.price_at(100):.4f}   "
                     f"@1000 = ${p.price_at(1000):.4f}   "
                     f"stock {p.stock}   {p.package}")
        lines.append("")
    return "\n".join(lines)


def update_bom(path: Path, prices: dict[str, float], qty: int) -> None:
    rows: list[dict] = []
    with path.open(newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        fields = reader.fieldnames or []
        for row in reader:
            code = (row.get("lcsc") or "").strip()
            if code in prices:
                row["unit_price_usd"] = f"{prices[code]:.4f}"
                row["price_basis"] = f"{qty} шт. (LCSC ladder)"
                if "price_basis" not in fields:
                    fields.append("price_basis")
            rows.append(row)
    with path.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            try:
                stream.reconfigure(encoding="utf-8", errors="replace")
            except (ValueError, OSError):
                pass

    ap = argparse.ArgumentParser(description="Цінова сходинка LCSC")
    ap.add_argument("--codes", help="коди LCSC через кому")
    ap.add_argument("--from-bom", type=Path, help="взяти коди з BOM")
    ap.add_argument("--qty", type=int, default=100, help="обсяг замовлення")
    ap.add_argument("--update-bom", action="store_true",
                    help="записати ціну за обсяг у BOM")
    ap.add_argument("--out", type=Path, help="CSV з результатами")
    args = ap.parse_args()

    pairs: list[tuple[str, str]] = []
    if args.codes:
        pairs = [(c.strip(), "") for c in args.codes.split(",") if c.strip()]
    elif args.from_bom:
        pairs = codes_from_bom(args.from_bom)
    if not pairs:
        ap.error("вкажи --codes або --from-bom")

    parts: list[Part] = []
    for code, _ in pairs:
        try:
            html = fetch_product(code)
        except (urllib.error.URLError, urllib.error.HTTPError, OSError) as exc:
            print(f"WARN {code}: не вдалося завантажити ({exc})", file=sys.stderr)
            continue
        part = parse_part(code, html)
        if part is None:
            print(f"WARN {code}: цінову сходинку не знайдено", file=sys.stderr)
            continue
        parts.append(part)

    if not parts:
        print("Нічого не вдалося завантажити", file=sys.stderr)
        return 1

    print(fmt_table(parts, args.qty))

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        with args.out.open("w", newline="", encoding="utf-8") as fh:
            w = csv.writer(fh)
            w.writerow(["lcsc", "mpn", "qty", "usd_price", "stock", "package"])
            for p in parts:
                for t in p.tiers:
                    w.writerow([p.lcsc, p.mpn, t.qty, f"{t.usd:.4f}", p.stock, p.package])
        print(f"\nЗаписано: {args.out}")

    if args.update_bom and args.from_bom:
        prices = {p.lcsc: p.price_at(args.qty) for p in parts}
        update_bom(args.from_bom, prices, args.qty)
        print(f"Оновлено ціни в {args.from_bom} (база: {args.qty} шт.)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
