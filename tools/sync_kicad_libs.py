#!/usr/bin/env python3
"""Синхронізація бібліотек KiCAD між проєктами «мішень» і «хаб».

Кожен KiCAD-проєкт має **власну** копію бібліотеки `arrowstand`
(символи + футпрінти, імпортовані з LCSC/EasyEDA). Причина: Copperpour MCP
пише бібліотеку в каталог проєкту (`<project_dir>/<lib>.pretty`), тому
спільна директорія `../common` для нього недоступна.

`hardware/target` — **майстер**. `hardware/hub` — копія, синхронізується цим
скриптом.

Використання:
    python3 tools/sync_kicad_libs.py            # hub <- target (master)
    python3 tools/sync_kicad_libs.py --to hub   # те саме
    python3 tools/sync_kicad_libs.py --check     # лише перевірити, чи збігаються
    python3 tools/sync_kicad_libs.py --to target  # master <- hub
"""
from __future__ import annotations

import argparse
import filecmp
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HW = ROOT / "hardware"
PROJECTS = {"target": HW / "target", "hub": HW / "hub"}
LIB_NAME = "arrowstand"


def lib_index(project: Path) -> dict[str, Path]:
    """Файли бібліотеки, індексовані за шляхом відносно каталогу проєкту."""
    out: dict[str, Path] = {}
    sym = project / f"{LIB_NAME}.kicad_sym"
    if sym.exists():
        out[sym.relative_to(project).as_posix()] = sym
    pretty = project / f"{LIB_NAME}.pretty"
    if pretty.exists():
        for mod in sorted(pretty.glob("*.kicad_mod")):
            out[mod.relative_to(project).as_posix()] = mod
    return out


def compare(a: Path, b: Path) -> list[str]:
    """Розбіжності, які заважають: відсутні або різні файли.

    Зайві файли в копії (наприклад, локальні експерименти) — не помилка:
    вони не ламають нічого, але про них варто знати.
    """
    ia, ib = lib_index(a), lib_index(b)
    diffs: list[str] = []
    for missing in sorted(set(ia) - set(ib)):
        diffs.append(f"у {b.name}: відсутня {missing}")
    for name in sorted(set(ia) & set(ib)):
        if not filecmp.cmp(ia[name], ib[name], shallow=False):
            diffs.append(f"різний вміст: {name}")
    return diffs


def extras(a: Path, b: Path) -> list[str]:
    """Файли, які є лише в копії."""
    return sorted(set(lib_index(b)) - set(lib_index(a)))


def sync(src: Path, dst: Path) -> int:
    copied = 0
    for rel, source in lib_index(src).items():
        target = dst / rel
        if not target.exists() or not filecmp.cmp(source, target, shallow=False):
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
            copied += 1
    return copied


def main() -> int:
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            try:
                stream.reconfigure(encoding="utf-8", errors="replace")
            except (ValueError, OSError):
                pass

    ap = argparse.ArgumentParser(description="Синхронізація бібліотек KiCAD")
    ap.add_argument("--to", choices=sorted(PROJECTS), default="hub",
                    help="куди копіювати (джерело — решта)")
    ap.add_argument("--check", action="store_true",
                    help="лише перевірити, без копіювання")
    args = ap.parse_args()

    dst = PROJECTS[args.to]
    src = next(p for name, p in PROJECTS.items() if name != args.to)

    diffs = compare(src, dst)
    extra = extras(src, dst)
    if args.check:
        if extra:
            print(f"Локальні файли лише в {dst.name} (не помилка):")
            for e in extra:
                print("  - " + e)
        if diffs:
            print(f"Розбіжності ({src.name} -> {dst.name}):")
            for d in diffs:
                print("  - " + d)
            return 1
        print(f"Бібліотеки збігаються: {src.name} == {dst.name}")
        return 0

    if not dst.exists():
        print(f"Каталог проєкту відсутній: {dst}", file=sys.stderr)
        return 2

    n = sync(src, dst)
    print(f"Скопійовано {n} файл(ів): {src.name} -> {dst.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
