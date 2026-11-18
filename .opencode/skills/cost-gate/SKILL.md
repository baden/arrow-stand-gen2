---
name: Cost Gate
description: |
  The cost-first gate for arrow-stand-gen2. Use before and after any schematic or
  BOM change: check the budget, the setup-fee per unique position, the package
  solderability rules, and produce the cost delta for the ADR.
  Triggers on: "cost", "price", "budget", "how much", "cheaper", "expensive",
  "BOM cost", "setup fee", "add a component".
argument-hint: "[change / question]"
---

# Cost Gate — arrow-stand-gen2

This project exists to be **as cheap as possible**. Cost is a gate, not a
nice-to-have. A change that adds cost without a written justification is not
ready.

## The budget

| Level | Value | Source |
|---|---|---|
| Goal | ≤ $4.10 per device at 100 pcs (PCB + components, no assembly) | `docs/cost/budget.md` |
| Realistic | ≤ $3.60 | |
| Hard ceiling | $4.50 — beyond this needs a new ADR | |
| Current | ≈ $4.05 (після ADR-0011: MCU QFN-48 + більша PCB) | |

The radio is **62 %** of the cost ($1.91 for SX1262IMLTRT at 100 pcs).
Optimising anything else can save at most $0.25. Say this out loud before
proposing micro-optimisations.

## The four gates

### Gate 1 — Requirement first

Does the change meet an actual requirement in `docs/00-requirements.md`? If
nobody asked for it, it does not enter the schematic. "Might be useful
later" is not a requirement.

### Gate 2 — Unique positions

Every new *unique* part number adds a ~$3 setup fee at an SMT assembler
(Extended library). A 10 kΩ and a 20 kΩ resistor are **two** positions.
A 10 kΩ and a 10 kΩ are **one**.

→ Before adding a value, check whether an existing value already fits.
→ Prefer a value already present in the BOM.

### Gate 3 — Package

Preferred order: `0805/0603/0402` → `SOT-23` → `SOP/TSSOP` → `SOIC`.
Avoid: LGA, QFN, BGA.

Exceptions allowed only with a written reason in the ADR. The two LGA/QFN
parts we already carry (SC7A20, SX1262) are the known expensive-to-solder
ones — do not add a third.

### Gate 4 — No "spare" parts

No extra LEDs, no unused GPIO pull-ups, no "just in case" capacitors.
Every part appears in the datasheet or in a deliberate choice recorded in the
ADR.

## How to run the gate

When asked to add or change something:

1. **Name the requirement** it serves (FR-xx / NFR-xx).
2. **Price it**: new part price at the target order quantity + any new setup
   fee. Use `bom-sourcing` / `tools/lcsc_prices.py` for the price — never guess,
   and never use the 1-piece price as the budget figure.
3. **Delta**: state `+$X.XX/шт (підсумок $Y.YY)` explicitly.
4. **Check against the budget** and state whether the goal/ceiling is breached.
5. **Check the assembly model** (ADR-0006) — does the package change the
   soldering cost more than the part costs?
6. **Write the result into** `hardware/bom/bom-draft.csv` and
   `docs/cost/budget.md`.
7. If the delta is negative, say so explicitly — it is the goal.

## Output format

```
Вартість: −$0.036/шт (XC6206P332MR $0.015 замість ME6211C33M5G-N $0.051, @100 шт.)
Позиції: +0 (немає нових унікальних позицій)
Підсумок: $3.02 — в межах мети (≤$3.50)
Пайка: SOT-23-3 — простіша за SOT-23-5, плюс
```

## Hard stops

Stop and ask the user when:

- The change pushes the total above $3.50.
- The change adds a unique position (setup fee) without removing another.
- The change swaps a cheap part for an expensive one for non-electrical
  reasons (aesthetic, "better brand") without a stated benefit.
- The change makes the device unsolderable by the chosen assembly model.

## Reference

- `docs/cost/budget.md` — current numbers, cost distribution, savings ideas.
- `docs/cost/cost-model.md` — setup fee, tier prices, assembly model comparison.
- `docs/adr/0006-assembly-model.md` — what assembly we assumed.
- `references/checks.md` — the checklist as a short list.
