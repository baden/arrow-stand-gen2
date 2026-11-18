# 02 — Структура каталогів і правила

> Статус: draft v0.1 · Оновлено: 2026-11-18

## 01 — Карта репозиторію

```
arrow-stand-gen2/
├── AGENTS.md                    обов'язковий інструктаж для ШІ-агента
├── README.md                    огляд проєкту, поточний стан, бюджет
├── CHANGELOG.md                 журнал змін
├── Makefile                     збірка прошивки + перевірки
├── .gitignore
├── .editorconfig
│
├── docs/
│   ├── 00-requirements.md       вимоги (DRD)
│   ├── 01-architecture.md       архітектура, блок-схема, режими живлення
│   ├── 02-directory-layout.md   цей файл
│   ├── 03-research-plan.md      план дослідження, відкриті питання
│   ├── architecture/
│   │   └── schematic-spec.md    специфікація схеми мішені (канон побудови)
│   ├── hub/
│   │   └── hub-requirements.md  ТЗ хаба (канон побудови)
│   ├── adr/                     архітектурні рішення
│   │   ├── 0000-template.md
│   │   ├── 0001-radio.md
│   │   ├── 0002-mcu.md
│   │   ├── 0003-accelerometer.md
│   │   ├── 0004-antenna.md
│   │   ├── 0005-radio-band.md
│   │   └── 0006-assembly-model.md
│   ├── components/              нотатки по компонентах, даташити
│   ├── cost/                    budget.md, bom-draft.md, cost-model.md
│   ├── manufacturing/           інструкції складання, перевірка футпрінтів і 3D
│   └── research/                результати досліджень
│
├── hardware/
│   ├── kicad/                   проєкт KiCAD (ТІЛЬКИ через Copperpour MCP!)
│   │   ├── arrow-stand-gen2.kicad_pro
│   │   ├── arrow-stand-gen2.kicad_sch
│   │   ├── arrow-stand-gen2.kicad_pcb
│   │   ├── arrow-stand-gen2.kicad_prl
│   │   ├── .copperpour/          конфіг проєкту + design rules
│   │   ├── sym-lib-table
│   │   └── fp-lib-table
│   ├── bom/
│   │   ├── bom-draft.csv        живий BOM (джерело істини)
│   │   └── bom.csv              експортований BOM (генерується)
│   ├── mechanical/              корпус, кріплення до мішені, 3D
│   └── assembly/                інструкції складання, pick&place
│
│   target/                       KiCAD-проєкт «мішень»
│   │   ├── arrow-stand-gen2.kicad_pro / .kicad_sch / .kicad_pcb
│   │   ├── arrowstand.kicad_sym   бібліотека символів (майстер)
│   │   ├── arrowstand.pretty/     бібліотека футпрінтів (майстер)
│   │   └── .copperpour/project.json  design rules
│
│   hub/                          KiCAD-проєкт «хаб»
│   │   ├── arrow-stand-hub.kicad_pro / .kicad_sch / .kicad_pcb
│   │   ├── arrowstand.kicad_sym   копія бібліотеки (sync_kicad_libs.py)
│   │   ├── arrowstand.pretty/     копія бібліотеки
│   │   └── .copperpour/project.json  design rules
│
├── firmware/
│   ├── common/                   спільний код для обох пристроїв
│   │   ├── include/              protocol.h, hal_*.h
│   │   ├── src/                  protocol.c, hal_*.c
│   │   ├── platform/             HAL: CH32V203, SX1262, SC7A20, power
│   │   ├── linker/               ch32v203.ld (64 KB / 20 KB)
│   │   └── tests/                host-тести (без заліза)
│   ├── target/                   «мішень»
│   │   ├── include/              config.h, app.h
│   │   ├── src/                  main.c, app.c (детекція + класифікація)
│   │   └── board.h               розподіл виводів плати
│   ├── hub/                      «хаб»
│   │   ├── include/              config.h, app.h
│   │   ├── src/                  main.c, app.c (реєстр каналів, USB-CDC)
│   │   └── board.h               розподіл виводів плати
│   └── README.md
│
├── tools/                       допоміжні скрипти (імпорт BOM, калькулятор вартості)
├── ci/                          конфігурація перевірок у CI
└── .opencode/
    ├── skills/                  проєктні скіли
    │   ├── bom-sourcing/
    │   ├── cost-gate/
    │   └── firmware-ch32/
    └── agent/                   проєктний агент
```

## 02 — Правила

### 02.1 Де-що лежить

| Тип | Місце | Приклад |
|---|---|---|
| Вимоги | `docs/0*.md` | вимога, функція |
| Рішення «чому отак» | `docs/adr/NNNN-*.md` | вибір SX1262 замість nRF24 |
| Дослідження | `docs/research/`, `docs/components/` | порівняння акселерометрів |
| Живий BOM | `hardware/bom/bom-draft.csv` | єдине джерело вартості |
| KiCAD | `hardware/target/`, `hardware/hub/` | тільки через MCP |
| Прошивка | `firmware/` | C, RISC-V |

### 02.2 Що заборонено

- Редагувати `*.kicad_*` текстовими інструментами — тільки Copperpour MCP.
- Тримати дві версії BOM (`bom-draft.csv` і `bom.csv`) вручну. `bom.csv` —
  **генерований** артефакт (`make bom`), не редагується руками.
- Тримати ціни без дати зняття. Формат: `2026-11-18`.
- Додавати компонент у схему, не додавши його в `bom-draft.csv`.

### 02.3 Іменування

| Об'єкт | Формат | Приклад |
|---|---|---|
| ADR | `NNNN-short-slug.md` | `0003-accelerometer.md` |
| Мережа живлення | `VCC_3V3`, `VCC_1V8`, `VBAT` | — |
| Земля | `GND` | — |
| Дизайн-атефакт | `kebab-case` | `docs/cost/budget.md` |

## 03 — Інструменти

| Задача | Команда / інструмент |
|---|---|
| Зібрати прошивку | `make firmware DEV=target` / `DEV=hub` (потрібен `riscv-none-elf-gcc`) |
| Синхронізувати бібліотеки KiCAD | `make libs` (target → hub) |
| Оновити BOM з KiCAD | `make bom` (kicad-cli) |
| Перевірити ERC | `make erc` |
| Перевірити DRC | `make drc` |
| Зняти ціни LCSC | `make prices` → `tools/lcsc_prices.py` |
| Перерахувати вартість | `make cost` → `tools/cost_model.py` |
| Перевірити форматирование | `make lint` |

## 04 — Git

Репозиторій ще не ініціалізовано. Рекомендована схема комітів:

```
<type>(<scope>): <subject>
```

Типи: `feat`, `fix`, `docs`, `hw`, `fw`, `chore`, `research`.
Scope: `schematic`, `pcb`, `bom`, `firmware`, `docs`, `cost`.

`hw(...)` та `fw(...)` — зміни, що змінюють залізо чи прошивку відповідно.
