#!/usr/bin/env python3
"""Звіряє кожен артикул у BOM із локальною базою LCSC.

Навіщо: у 2026-11-19 в BOM опинився артикул M2-SMA-42M (C55608210) — це
звичайний ДІОД у корпусі SMA (DO-214AC), а не SMA-роз'єм. Помилка пройшла,
бо код і назву ніхто не звіряв із категорією LCSC.

Інструмент:
  * бере кожен рядок BOM із колонкою `lcsc`;
  * шукає цей код у локальній базі LCSC (~/.konnect або copperpour/jlcpcb.db);
  * порівнює MFR_Part із колонкою `mpn` у BOM;
  * друкує рядки, де є розбіжність, і рядки без деталі в базі;
  * Exit code 1, якщо є хоч одна розбіжність — тоді `make bom-check` падає.

Додатково (--full) друкує категорію та опис деталі з бази поруч із BOM —
саме так 2026-11-19 було виявлено, що J1 у BOM — це діод.
"""
from __future__ import annotations

import argparse
import csv
import pathlib
import sqlite3
import sys

DB_CANDIDATES = [
    pathlib.Path(r"C:\Users\ПК\AppData\Roaming\copperpour\jlcpcb.db"),
    pathlib.Path.home() / ".konnect" / "jlcpcb.db",
]
BOM_DIR = pathlib.Path(__file__).resolve().parent.parent / "hardware" / "bom"


def open_db() -> sqlite3.Connection | None:
    for db in DB_CANDIDATES:
        if db.exists():
            con = sqlite3.connect(f"file:{db}?mode=ro", uri=True)
            cols = [r[1] for r in con.execute("PRAGMA table_info(components)")]
            if "LCSC" in cols:
                return con
            con.close()
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description="Звірка артикулів BOM із базою LCSC")
    ap.add_argument("--full", action="store_true", help="друкувати категорію та опис")
    args = ap.parse_args()

    con = open_db()
    if con is None:
        print("БАЗА LCSC НЕ ЗНАЙДЕНА — перевірку пропущено")
        return 0
    cur = con.cursor()

    bad = 0
    for bom in sorted(BOM_DIR.glob("bom*.csv")):
        print(f"\n=== {bom.name} ===")
        for row in csv.DictReader(bom.open(encoding="utf-8")):
            code = (row.get("lcsc") or "").strip()
            if not code.startswith("C"):
                continue
            found = cur.execute(
                "SELECT MFR_Part, Description, Manufacturer, Library_Type, Category "
                "FROM components WHERE LCSC = ? LIMIT 1", (code,)).fetchone()
            if not found:
                print(f"  {row['ref']:6s} {code}: НЕМАЄ В БАЗІ")
                bad += 1
                continue
            mpn_db = str(found[0] or "")
            mpn_bom = (row.get("mpn") or "").strip()
            if mpn_db and mpn_bom and mpn_db.upper() != mpn_bom.upper():
                print(f"  {row['ref']:6s} {code}: BOM '{mpn_bom}' != БД '{mpn_db}'")
                bad += 1
            if args.full:
                print(f"  {row['ref']:6s} {code:12s} {mpn_db:22s} "
                      f"[{found[4]}] {(str(found[1] or ''))[:66]}")
    print(f"\nРОЗБІЖНОСТЕЙ: {bad}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
