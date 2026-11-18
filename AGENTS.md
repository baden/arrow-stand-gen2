# AGENTS.md — інструктаж для ШІ-агента

Цей файл — обов'язковий перед будь-якою роботою в репозиторії.
Якщо правило тут суперечить зовнішньому скілу, пріоритет має **цей файл**.

---

## 0. Жорсткі правила (порушення = зупинка роботи)

1. **KiCAD-файли не редагуються текстовими інструментами.**
   `*.kicad_sch`, `*.kicad_pcb`, `*.kicad_pro`, `*.kicad_sym`, `*.kicad_mod`,
   `sym-lib-table`, `fp-lib-table` — тільки через **Copperpour MCP**.
   Жодних `edit`/`write`/`sed`/`str_replace` по цих файлах. Навіть для «дрібної
   правки».

2. **Вартість — первинний критерій.** Кожна додана деталь має бути записана в
   `hardware/bom/bom-draft.csv` з обґрунтуванням у колонці `rationale`.
   Деталь без обґрунтування — не існує. Бюджет: **≤ $4.10** за пристрій
   при партії 100 шт (поточна конфігурація — ADR-0010 + ADR-0011).

3. **Мінімум унікальних деталей.** Кожна унікальна позиція в BOM — це
   setup-fee на стороні SMT-складальника (~$3 за позицію, якщо деталь не з
   JLCPCB Basic Library) плюс ризик неправильного замовлення. Перед додаванням
   номіналу перевір, чи є вже дешевша вже наявна в схемі.

4. **2 шари. Без внутрішніх шарів.** Без заливки на шарах без живлення.

5. **Пакети: максимум простої пайки.** Пріоритет: `0805/0603/0402` →
   `SOT-23` → `SOP/TSSOP` → `SOIC`. Уникати LGA, QFN, BGA — тільки з явного
   обґрунтування (якщо вже є дешевша альтернатива, брати її).

6. **Не добавляти «про запас».** Кожен резистор/конденсатор має бути в
   даташиті або обґрунтований. Жодних тригерних/EN-лампок у продуктивній
   схемі.

7. **Не вигадувати ціни й наявність.** Ціни, LCSC-коди, наявність — тільки з
   `search_jlcpcb_parts` / `get_jlcpcb_part` (Copperpour `integration` toolset)
   та `tools/lcsc_prices.py`. **Бюджет рахувати за ціновою содинкою LCSC
   (обсяг замовлення), а не за базовою ціною за 1 штуку** — різниця до 1.7×.
   У документи ставити дату зняття ціни та обсяг.

---

8. **Формат файлів KiCAD.** KiCad 10.0.7 пише схему у форматі `20260306`, а
   PCB — `20260206`. Copperpour MCP цей формат **зберігає** (перевірено),
   але файл, створений старим форматом, KiCAD переконвертує сам при
   відкритті. Після будь-якої правки схеми/плати — перевір версію:
   `make fmt-check`. Якщо вона відкотилась — `make upgrade`
   (`kicad-cli sch upgrade` / `kicad-cli pcb upgrade`).
   Нормалізація KiCAD (переписує вбудовані символи в канонічний вигляд) дає
   великий, але **семантично порожній** diff — не плутай його зі зміною схеми.

9. **3D-моделі живуть у двох місцях:** у бібліотеці (`.kicad_mod`) і в
   **копіях футпрінтів усередині `.kicad_pcb`**. Модель, додана до бібліотеки,
   не потрапляє в плату автоматично — після зміни бібліотеки потрібне
   «Update Footprints from Library» (KiCad) або `update_footprints_from_library`
   (MCP, потрібен відкритий pcbnew). Перевірка: `make check-3d`.

10. **KiCad IPC: що насправді потрібно.** Перевага «Enable KiCad API» у
    `kicad_common.json` (`"api": {"enable_server": true}`) — це **не** те, що
    ламає доступ. IPC-сервер піднімає **менеджер проєкту** (`kicad.exe`), і
    він обслуговує лише документи, відкриті **через менеджер**. Перевірено
    2026-11-18:
    - `pcbnew.exe <файл>`, запущений окремо, **не реєструє** плату в IPC →
      усі pcb-інструменти Copperpour відповідають «KiCAD must be running with
      the board loaded (Connection refused)», хоча вікно pcbnew відкрите;
    - виклики зі схемою (eeschema/kicad-side) працюють, поки запущено менеджер;
    - «Connection refused» означає «сервер не слухає», а не «API вимкнено»;
    - `BeginCommit without a document specified is not allowed when multiple
      editors are open` → у цей момент відкриті **два** редактори (напр. pcbnew
      і eeschema) — закрийте один.

    **Як робити PCB-операції через MCP:** відкрити плату **з дерева проєкту** в
    менеджері (подвійний клік по `.kicad_pcb`), тоді `update_pcb_from_schematic`
    працює. Якщо потрібен лише редактор — Tools → Update PCB from Schematic
    у самому pcbnew.

11. **SolidCraft: не чіпати чужі проєкти.** MCP керує **одним** екземпляром
    SolidCraft, і в ньому може бути відкритий чужий проєкт. Перед будь-якою
    роботою з геометрією:
    - викликати `inspect_design` і перевірити `path` та `dirty`;
    - якщо `path` вказує не на наш репозиторій — **не викликати `new_design`**:
      спертися в користувача;
    - якщо `dirty: true` — у пам'яті є незбережені зміни: `new_design` їх
      знищить без можливості відновлення (автозбереження в SolidCraft немає);
    - `new_design` створює документ без шляху (`path: null`) — файл на диску не
      змінюється, але незбережена сесія втрачається;
    - експорт через MCP обмежено теками `C:\Users\ПК\designs` (і recovery/tmp
      SolidCraft) — експортувати туди й **копіювати** в репозиторій
      (`hardware/common/packages3d/`, `hardware/common/models3d/`);
    - власний параметричний код зберігати як `.sc` (SolidCraft container),
      щоб модель можна було переробити.
## 1. Що це за проєкт

Див. [README.md](README.md). Коротко: датчик влучання в мішень.
**CH32V203F8U6** читає акселерометр (SC7A20), рахує ознаки удару і
**класифікує подію на пристрої** (ADR-0008), формує пакет 16 Б і
відправляє по LoRa (SX1262, 433 МГц, SF11, власний P2P-протокол).
Живлення — LiPo. Хаб — те саме залізо в режимі RX + USB (ADR-0008).

## 2. Структура каталогів

Детально — [docs/02-directory-layout.md](docs/02-directory-layout.md).

```
docs/        вимоги, архітектура, ADR, план дослідження, вартість
             architecture/schematic-spec.md — канон схеми мішені
             hub/hub-requirements.md   — канон ТЗ хаба
hardware/    target/ + hub/ (проєкти KiCAD), bom/, mechanical/, assembly/
firmware/    common/ (спільне), target/ (мішень), hub/ (хаб)
tools/       скрипти
.opencode/   skills/ (проєктні скіли), agent/ (проєктний агент)
```

Правило: **кожна зміна схеми тягне за собою** оновлення `hardware/bom/bom-draft.csv`
і `docs/cost/budget.md`. Це не опція — це частина визначення «готово».

## 3. Маршрутизація інструментів (Copperpour)

| Задача | Toolset | Ключові інструменти |
|---|---|---|
| Створити/відкрити проєкт | `project` | `create_project`, `get_project_info`, `open_schematic_viewer` |
| Розмістити символи | `sch_components` | `add_schematic_component`, `edit_schematic_component` |
| З'єднати | `sch_wiring` | `connect_pins`, `add_wire`, `add_schematic_net_label` |
| Масова правка | `sch_batch` | `sch_batch` (за один виклик) |
| Аналіз схеми | `sch_analysis` | `find_orphan_items`, `find_shorted_nets`, `get_component_nets` |
| Перевірки | `sch_export` + `verification` | `run_erc`, `run_drc` |
| PCB | `pcb_board`, `pcb_components`, `pcb_routing` | `update_pcb_from_schematic`, `place_footprint`, `route` |
| Виробництво | `pcb_export`, `manufacturing` | `export_gerber`, `export_bom`, `estimate_cost` |
| Деталі/ціни | `integration` | `search_jlcpcb_parts`, `get_jlcpcb_part`, `import_jlcpcb_part` |
| Бібліотеки | `library` | створення власних символів/футпрінтів |
| Огляд | `design_review` | `run_design_review` |

`load_toolset` приймає масив: `load_toolset({name: ["sch_components","sch_wiring"]})`
— дешевше, ніж по одному. Після кожного `load_toolset` каталог toolset-ів
перезавантажується — роби `tools/list` лише коли реально щось шукаєш.

## 4. Скіли

**Глобальні (вже встановлені):**

| Скіл | Коли вантажити |
|---|---|
| `copperpour` | Завжди для роботи з KiCAD. Описує канали записи і reliability contract. |
| `kicad-schematic` | Створення/правка схеми |
| `kicad-pcb` | Розводка, stackup, зони, правила |
| `kicad-library` | Власні символи/футпрінти, sym-lib-table/fp-lib-table |
| `kicad-review` | DRC/ERC та повний pre-fab аудит |
| `kicad-manufacture` | Гербери, BOM, замовлення виробництва |

**Проєктні (в цьому репозиторії):**

| Скіл | Коли вантажити |
|---|---|
| `bom-sourcing` | Вибір/перевірка деталей, ціни, наявність, імпорт у KiCAD |
| `cost-gate` | Будь-яка зміна BOM або схеми — перевірка бюджету й setup-fee |
| `firmware-ch32` | Збірка/прошивка CH32V203, розподіл виводів, структура `firmware/` |

## 5. Робочий цикл

1. Прочитати `docs/00-requirements.md` і `docs/01-architecture.md`.
2. Перевірити стан: `get_project_info` + `inspect`/ревізія схеми.
3. Сформувати план: **які деталі, які номінали, який вплив на вартість**.
4. Зміни — через Copperpour MCP, з `sch_batch` для групових операцій.
5. Перевірити: `run_erc` → `find_orphan_items` → `run_drc`.
6. Оновити BOM + бюджет.
7. Зафіксувати рішення в `docs/adr/` (якщо змінилось архітектурне рішення).

## 6. Definition of Done (схема)

- [ ] `run_erc` — 0 помилок, 0 попереджень (або кожне попередження записано в ADR з обґрунтуванням)
- [ ] `find_orphan_items` — порожньо
- [ ] `find_shorted_nets` — порожньо
- [ ] Кожен позиційний номер у схемі має `MPN`, `LCSC`, `Value`, `Datasheet`
- [ ] BOM синхронізовано з `hardware/bom/bom-draft.csv`
- [ ] `docs/cost/budget.md` перераховано
- [ ] Немає деталей «про запас»
- [ ] `run_drc` — 0 помилок

## 7. Відкриті питання

Актуальний список — [docs/03-research-plan.md](docs/03-research-plan.md).
Не приймай рішення з відкритих питань мовчки: зафіксуй ADR або запитай.

## 8. Стиль комітів та документів

- Коміти та ідентифікатори в коді — англійською.
- Документація проєкту — українською (як у цьому файлі).
- ADR — українською, шаблон у `docs/adr/0000-template.md`.
- Файли — snake_case, крім KiCAD-проєкту.
