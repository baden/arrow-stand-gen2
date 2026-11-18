#!/usr/bin/env python3
"""Калькулятор собівартості для arrow-stand-gen2.

Читає hardware/bom/bom-draft.csv і рахує:
  - вартість позицій (only status in chosen/candidate/planned/optional)
  - вартість по блоках
  - setup-fee за унікальні позиції не з JLCPCB Basic/Preferred
  - порівняння з цілями з docs/cost/budget.md

Використання:
    python3 tools/cost_model.py --bom hardware/bom/bom-draft.csv
    python3 tools/cost_model.py --bom hardware/bom/bom-draft.csv --check
    python3 tools/cost_model.py --bom hardware/bom/bom-draft.csv --markdown
"""

from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

# Ціни за 1 шт., знято з LCSC/JLCPCB.
# Використовуються лише для позицій, яких немає в самому BOM
# (наприклад, PCB або батарея).
REFERENCE_COSTS = {
    "pcb_2layer_45x50": 0.45,
    "lipo_1s_200mah": 2.00,
    "smt_assembly_per_unit": 2.25,
    "setup_fee_per_unique_extended": 3.00,
}

# Цілі (docs/cost/budget.md) — замовлення 100 шт.
GOAL_USD = 4.10
REALISTIC_USD = 4.05
CEILING_USD = 4.50

# Позиції, що не входять у вартість пристрою (навкруг пристрою).
# ANT1 (антенна з кабелем) — не на PCB, але це частина вартості пристрою,
# тому враховується; виключається лише акумулятор (обладнання споживача).
EXCLUDED_REFS = {"BT1"}

COUNTED_STATUSES = {"chosen", "candidate", "planned", "optional"}

BLOCK_BY_PREFIX = {
    "U": "Радіо / MEMS / живлення",
    "Y": "Радіо / MEMS / живлення",
    "L": "Радіо / MEMS / живлення",
    "SW": "Пасивні / ввід",
    "R": "Пасивні / ввід",
    "C": "Пасивні / ввід",
    "D": "Пасивні / ввід",
}

# Точні підказки за реф-десигнатором (іначе — за префіксом).
BLOCK_HINTS = {
    "U1": "Радіо (SX1262 + кварц)",
    "Y1": "Радіо (SX1262 + кварц)",
    "U2": "МК (CH32V203)",
    "U3": "Акселерометр (SC7A20)",
    "U4": "Живлення (LDO)",
    "U5": "Живлення (заряд)",
    "SW1": "Ввід",
    "D1": "Ввід",
}


@dataclass
class Item:
    ref: str
    qty: int
    value: str
    mpn: str
    lcsc: str
    package: str
    unit_price: float
    library_type: str
    status: str
    rationale: str


@dataclass
class Result:
    items: list[Item] = field(default_factory=list)
    total: float = 0.0
    by_block: dict[str, float] = field(default_factory=lambda: defaultdict(float))
    extended_refs: list[str] = field(default_factory=list)
    skipped: list[str] = field(default_factory=list)


def load_bom(path: Path) -> Result:
    result = Result()
    with path.open(newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        for row in reader:
            ref = (row.get("ref") or "").strip()
            if not ref:
                continue
            status = (row.get("status") or "").strip().lower()
            unit = parse_price(row.get("unit_price_usd"))
            qty = parse_int(row.get("qty"))

            if ref in EXCLUDED_REFS or status in {"open", ""}:
                result.skipped.append(f"{ref} ({status or 'no status'})")
                continue
            if status not in COUNTED_STATUSES:
                result.skipped.append(f"{ref} ({status})")
                continue

            item = Item(
                ref=ref,
                qty=qty,
                value=(row.get("value") or "").strip(),
                mpn=(row.get("mpn") or "").strip(),
                lcsc=(row.get("lcsc") or "").strip(),
                package=(row.get("package") or "").strip(),
                unit_price=unit,
                library_type=(row.get("library_type") or "").strip(),
                status=status,
                rationale=(row.get("rationale") or "").strip(),
            )
            result.items.append(item)
            result.total += item.unit_price * item.qty
            result.by_block[block_of(ref)] += item.unit_price * item.qty

            if item.library_type.lower() == "extended":
                result.extended_refs.append(ref)

    result.extended_refs.sort()
    return result


def load_tiers(path: Path) -> dict[str, list[tuple[int, float]]]:
    """Читає сходинку, зняту tools/lcsc_prices.py: lcsc -> [(qty, usd), ...]."""
    tiers: dict[str, list[tuple[int, float]]] = {}
    if not path.exists():
        return tiers
    with path.open(newline="", encoding="utf-8") as fh:
        for row in csv.DictReader(fh):
            code = (row.get("lcsc") or "").strip()
            try:
                tiers.setdefault(code, []).append(
                    (int(row["qty"]), float(row["usd_price"]))
                )
            except (KeyError, TypeError, ValueError):
                continue
    for code in tiers:
        tiers[code].sort()
    return tiers


def price_from_tiers(ladder: list[tuple[int, float]], qty: int) -> float | None:
    """Ціна за штуку при замовленні qty штук."""
    price = None
    for tier_qty, usd in ladder:
        if qty >= tier_qty:
            price = usd
        else:
            break
    return price


def apply_tiers(res: Result, tiers: dict[str, list[tuple[int, float]]], qty: int) -> int:
    """Перераховує ціни позицій за сходинкою. Повертає кількість змінених."""
    changed = 0
    for item in res.items:
        ladder = tiers.get(item.lcsc)
        if not ladder:
            continue
        usd = price_from_tiers(ladder, qty)
        if usd is None or abs(usd - item.unit_price) < 1e-9:
            continue
        delta = (usd - item.unit_price) * item.qty
        res.total += delta
        res.by_block[block_of(item.ref)] += delta
        item.unit_price = usd
        changed += 1
    return changed


def block_of(ref: str) -> str:
    if ref in BLOCK_HINTS:
        return BLOCK_HINTS[ref]
    for prefix, block in BLOCK_BY_PREFIX.items():
        if ref.startswith(prefix):
            return block
    return "Інше"


def parse_price(raw: str | None) -> float:
    if not raw:
        return 0.0
    cleaned = raw.replace("$", "").replace("~", "").strip()
    if not cleaned:
        return 0.0
    try:
        return float(cleaned)
    except ValueError:
        return 0.0


def parse_int(raw: str | None) -> int:
    try:
        return int(str(raw).strip())
    except (TypeError, ValueError):
        return 1


def setup_fee(refs: list[str]) -> float:
    return len(refs) * REFERENCE_COSTS["setup_fee_per_unique_extended"]


def report(res: Result) -> str:
    lines: list[str] = []
    lines.append("=== arrow-stand-gen2 — модель вартості ===")
    lines.append("")
    lines.append("Позиції:")
    for it in sorted(res.items, key=lambda i: -i.unit_price * i.qty):
        ext = it.unit_price * it.qty
        lines.append(
            f"  {it.ref:<6} {it.mpn or it.value:<24} "
            f"{it.qty} x ${it.unit_price:<6.3f} = ${ext:<6.3f}  [{it.status}]"
        )
    lines.append("")
    lines.append("По блоках:")
    total = res.total or 1.0
    for block, cost in sorted(res.by_block.items(), key=lambda kv: -kv[1]):
        lines.append(f"  {block:<32} ${cost:<7.3f} ({cost / total * 100:4.1f} %)")
    lines.append("")
    lines.append(f"Разом компонентів:            ${res.total:.3f}")
    lines.append(f"PCB 2 шари (референс):        ${REFERENCE_COSTS['pcb_2layer_45x50']:.3f}")
    grand = res.total + REFERENCE_COSTS["pcb_2layer_45x50"]
    lines.append(f"РАЗОМ (пристрій):             ${grand:.3f}")
    lines.append("")
    lines.append(
        f"Setup-fee позицій ({len(res.extended_refs)} Extended):  "
        f"${setup_fee(res.extended_refs):.2f} одноразово "
        f"({', '.join(res.extended_refs) or '—'})"
    )
    lines.append(
        f"Операційні SMT (за пристрій):  ${REFERENCE_COSTS['smt_assembly_per_unit']:.2f}"
    )
    lines.append("")
    lines.append(f"Ціль   ≤ ${GOAL_USD:.2f}:  {'OK' if grand <= GOAL_USD else 'ПЕРЕВИЩЕНО'}")
    lines.append(
        f"Реал.  ≤ ${REALISTIC_USD:.2f}:  "
        f"{'OK' if grand <= REALISTIC_USD else 'понад метою'}"
    )
    lines.append(
        f"Стеля ≤ ${CEILING_USD:.2f}:  {'OK' if grand <= CEILING_USD else 'ПЕРЕВИЩЕНО'}"
    )
    if res.skipped:
        lines.append("")
        lines.append(f"Не враховано ({len(res.skipped)}): {', '.join(res.skipped)}")
    return "\n".join(lines)


def to_markdown(res: Result) -> str:
    grand = res.total + REFERENCE_COSTS["pcb_2layer_45x50"]
    lines = []
    lines.append("<!-- Згенеровано tools/cost_model.py — не редагувати руками -->")
    lines.append("")
    lines.append("| Позиція | MPN | Qty | $/шт | Сума |")
    lines.append("|---|---|---|---|---|")
    for it in sorted(res.items, key=lambda i: -i.unit_price * i.qty):
        lines.append(
            f"| {it.ref} | {it.mpn or it.value} | {it.qty} | "
            f"${it.unit_price:.3f} | ${it.unit_price * it.qty:.3f} |"
        )
    lines.append("")
    lines.append(f"| **Разом компонентів** | | | | **${res.total:.3f}** |")
    lines.append(f"| PCB | | 1 | $0.400 | $0.400 |")
    lines.append(f"| **РАЗОМ** | | | | **${grand:.3f}** |")
    lines.append("")
    lines.append(
        f"Цілі: ≤ ${GOAL_USD:.2f} — "
        f"{'виконано' if grand <= GOAL_USD else '**перевищено**'}; "
        f"стел ${CEILING_USD:.2f}."
    )
    lines.append("")
    lines.append(
        f"Setup-fee: {len(res.extended_refs)} унікальних Extended-позицій × "
        f"${REFERENCE_COSTS['setup_fee_per_unique_extended']:.2f} = "
        f"**${setup_fee(res.extended_refs):.2f}** одноразово."
    )
    return "\n".join(lines)


def main() -> int:
    # Windows consoles default to a legacy code page; force UTF-8 output.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            try:
                stream.reconfigure(encoding="utf-8", errors="replace")
            except (ValueError, OSError):
                pass

    ap = argparse.ArgumentParser(description="Калькулятор вартості BOM")
    ap.add_argument(
        "--bom",
        default="hardware/bom/bom-draft.csv",
        type=Path,
        help="шлях до bom-draft.csv",
    )
    ap.add_argument("--out", type=Path, help="записати markdown-зведення у файл")
    ap.add_argument(
        "--tiers",
        type=Path,
        default=Path("hardware/bom/lcsc-prices.csv"),
        help="файл сходинки LCSC (hardware/bom/lcsc-prices.csv)",
    )
    ap.add_argument(
        "--qty",
        type=int,
        default=100,
        help="обсяг замовлення для перерахунку за сходинкою",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="ненульовий код виходу, якщо перевищено ціль $%.2f" % GOAL_USD,
    )
    args = ap.parse_args()

    if not args.bom.exists():
        print(f"ERROR: BOM не знайдено: {args.bom}", file=sys.stderr)
        return 2

    res = load_bom(args.bom)
    tiers = load_tiers(args.tiers)
    if tiers:
        changed = apply_tiers(res, tiers, args.qty)
        print(f"Сходинка LCSC ({args.tiers}): перераховано {changed} позицій "
              f"за обсягу {args.qty} шт.\n")
    print(report(res))

    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(to_markdown(res) + "\n", encoding="utf-8")
        print(f"\nЗаписано: {args.out}")

    if args.check:
        grand = res.total + REFERENCE_COSTS["pcb_2layer_45x50"]
        if grand > GOAL_USD:
            print(f"\nFAIL: ${grand:.3f} > ціль ${GOAL_USD:.2f}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
