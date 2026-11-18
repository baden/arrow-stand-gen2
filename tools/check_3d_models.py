#!/usr/bin/env python3
"""Перевірка 3D-моделей у РОЗМІЩЕНИХ екземплярах обох PCB.

Різниця між бібліотекою та платою: KiCAD зберігає в .kicad_pcb копію
футпрінта, тому модель, додана до бібліотеки, у вже розміщені екземпляри
не потрапляє — їх треба оновити через «Update Footprints from Library».
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(r"C:\Users\ПК\SDK\lora\arrow-stand-gen2")

EXPECT = {
    "arrowstand:": {
        "QFN-24": "Package_DFN_QFN.3dshapes/QFN-24-1EP_4x4mm_P0.5mm_EP2.6x2.6mm.step",
        "QFN-48": "Package_DFN_QFN.3dshapes/QFN-48-1EP_7x7mm_P0.5mm_EP5.15x5.15mm.step",
        "LGA-12": "Package_LGA.3dshapes/LGA-12_2x2mm_P0.5mm.step",
        "SOT-23-5": "Package_TO_SOT_SMD.3dshapes/SOT-23-5.step",
        "ESOP-8": "Package_SO.3dshapes/SOIC-8-1EP_3.9x4.9mm_P1.27mm_EP2.41x3.81mm.step",
        "CRYSTAL": "Crystal.3dshapes/Crystal_SMD_3225-4Pin_3.2x2.5mm.step",
    },
}

PCBS = [
    ("мішень", ROOT / "hardware" / "target" / "arrow-stand-gen2.kicad_pcb"),
    ("хаб", ROOT / "hardware" / "hub" / "arrow-stand-hub.kicad_pcb"),
]


def main() -> int:
    ok_all = True
    for label, pcb in PCBS:
        txt = pcb.read_text(encoding="utf-8", errors="replace")
        # інстанси футпрінтів: блоки (footprint "lib:name" ...)
        blocks = re.findall(r'\(footprint "([^"]+)"(.*?)\n\t\)', txt, re.S)
        missing: list[str] = []
        have = 0
        for lib_id, body in blocks:
            if not lib_id.startswith("arrowstand:"):
                continue
            models = re.findall(r'\(model\s+"?([^"\r\n]+)"?', body)
            if models:
                have += 1
                continue
            missing.append(lib_id.split(":", 1)[1])
        print(f"[{label}] футпрінтів arrowstand: "
              f"{len([b for b in blocks if b[0].startswith('arrowstand:')])}, "
              f"з моделлю: {have}, без: {len(missing)}")
        if missing:
            ok_all = False
            for m in sorted(missing):
                print(f"    БЕЗ 3D-МОДЕЛІ: {m}")
        else:
            print("    усі з моделлю")
    print()
    print("ПІДСУМОК:", "усе на місці" if ok_all
          else "є футпрінти без моделі — потрібен Update Footprints from Library")
    return 0 if ok_all else 1


if __name__ == "__main__":
    raise SystemExit(main())
