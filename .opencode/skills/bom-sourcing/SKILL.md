---
name: BOM Sourcing
description: |
  Selecting, verifying and importing components for arrow-stand-gen2: LCSC/JLCPCB
  search, price and stock checks, alternative comparison, datasheet retrieval,
  import into KiCAD, and keeping hardware/bom/bom-draft.csv in sync.
  Triggers on: "pick a part", "find a component", "price of", "is it in stock",
  "cheaper alternative", "BOM", "import footprint", "LCSC", "JLCPCB part".
argument-hint: "[component / question]"
---

# BOM Sourcing — arrow-stand-gen2

Selecting the cheapest part that actually works, and proving it with data.

## Non-negotiables

1. **Never state a price, LCSC code or stock count from memory.** Every number
   comes from a tool call in this session, with the date recorded.
   **The price in the local DB is the 1-piece price — it is 1.4–1.7× the real
   price at volume.** For the budget, always use the LCSC ladder.
2. **Cheapest ≠ selected.** A part must satisfy the requirement first
   (range, interface, package, availability). Price breaks ties.
3. **One unique position is worth ~$3** in setup fee at an SMT assembler.
   Prefer an existing value/package already in the BOM over a new one.
4. **Record everything.** Every candidate that was considered and rejected goes
   into the ADR alternative table with its price.

## Workflow

### 1. Define the requirement

Before searching, write down: electrical need, package constraint, target
price, quantity, and what the part is competing against.

### 2. Search the local database

```js
await tools.copperpour.load_toolset({ name: "integration" })
await tools.copperpour.search_jlcpcb_parts({ query: "SX1262", limit: 12 })
await tools.copperpour.get_jlcpcb_part({ lcsc_id: "C191341" })
```

Notes:
- The DB is a local SQLite cache. If missing:
  `download_jlcpcb_database({})` (~187 MB, takes a minute).
- `query` is AND over words, in any order, across LCSC / MPN / package /
  manufacturer / description.
- **Value words match as whole values:** `100nF` will not find `0.1uF`;
  `20pF` will not find `220pF`. Unit spellings are not normalized.
- `library_type` matters for cost: `Basic` and `Preferred` carry no setup
  fee, `Extended` costs ~$3 per unique position at JLCPCB.

### 3. Get the real (volume) price

```bash
python3 tools/lcsc_prices.py --codes C191341,C5187096 --qty 100
python3 tools/lcsc_prices.py --from-bom hardware/bom/bom-draft.csv --qty 100 --update-bom
```

LCSC has a **ladder** per part, and the break points differ between parts
(SX1262: 1/10/30/100/500/1000; CH32V003: 5/50/150/500/2500/4000). The price
at 100 pcs is the price of the tier you land in — not necessarily the
"100" tier. The tool resolves this correctly; the local DB does not.

### 4. Compare candidates

Build a table with: LCSC, MPN, price @qty, package, library type, stock, key
electrical params. Then apply the project's rules in order:

1. Does it meet the electrical requirement?
2. Is it in stock with headroom (≥ 3× the order quantity)?
3. Is the package solderable by our chosen assembly model (ADR-0006)?
4. Is it the cheapest of the survivors?

### 5. Import into KiCAD

```js
await tools.copperpour.import_jlcpcb_part({
  lcsc_id: "C191341",
  project_dir: "<abs path>/hardware/target",   # або hardware/hub
  library_name: "jlcpcb",
  schematic: "<abs path>/hardware/target/arrow-stand-gen2.kicad_sch",
  x: 100, y: 60,
  reference: "U1"
})
```

This pulls symbol **and** footprint from EasyEDA, registers both library
tables, and can place the part on the schematic in one call. Use it instead of
hand-made symbols wherever the part exists in the DB.

### 6. Update the BOM

`hardware/bom/bom-draft.csv` is the source of truth. Every row needs:

`ref, qty, value, mpn, manufacturer, lcsc, package, unit_price_usd,
library_type, stock, status, rationale, datasheet_url, price_basis`

`price_basis` records the quantity the price was taken at (e.g. "100 шт.").

- `status` ∈ `chosen | candidate | optional | open`
- `rationale` — one sentence, why this part and not the next cheapest.
- `datasheet_url` — from `get_datasheet_url` or the search result. Never invent.

Then re-run the budget: `docs/cost/budget.md`.

### 7. Datasheets

```js
await tools.copperpour.get_datasheet_url({ mpn: "SX1262IMLTRT" })
await tools.copperpour.enrich_datasheets({ schematic: "<abs>.kicad_sch" })
```

Keep a local copy under `docs/components/<part>/` when the datasheet is the
source of a schematic decision (pinout, recommended decoupling, register
values).

## Project-specific gotchas

| Gotcha | Handling |
|---|---|
| `SX1262IMLTRT` is QFN-24-EP 4×4, not a simple QFN | Hand-soldering is impractical → drives ADR-0006 |
| `CH32V003F4P6` is TSSOP-20 (not TSSOP-16) | `A4M6` is SOP-16, `J4M6` is SOP-8 |
| `SC7A20HTR` (LGA-12 2×2) is the LIS2DH12 clone | Write the HAL against the LIS2DH12 datasheet |
| `SX1262` is 62 % of the BOM cost | Any radio swap must beat $1.91 @100 pcs |
| All key parts are Extended/Preferred, none Basic | Minimize unique positions |
| Prices in the DB are for 1 unit | Use `tools/lcsc_prices.py` for volume prices |

## Reference

- `references/candidate-table.md` — the comparison table format and the
  candidates already evaluated for each block.
- `docs/adr/0001-radio.md`, `0002-mcu.md`, `0003-accelerometer.md` — decisions.
- `docs/cost/budget.md` — the budget this sourcing must respect.
