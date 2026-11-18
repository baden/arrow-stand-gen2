# Специфікація схеми — arrow-stand-gen2 (варіант «мішень»)

> Статус: **v1.2** · 2026-11-18 · **синхронізовано зі збудованою схемою**
> `hardware/kicad/arrow-stand-gen2.kicad_sch` (ERC 0/0, orphans 0, shorts 0,
> 38 компонентів, 28 мереж).
> Це канон: будь-який вихід за специфікацію — тільки з ADR.

## 0. Одностороння PCB, 2 шари

- Розмір: орієнтовно 35 × 45 мм (з запасом під SMA- і USB-C-роз'єми).
- Усі деталі — SMT. SMA і USB-C — edge-mount.
- Акумулятор LiPo 1S — **припаюється на контакти TP5/TP6** (роз'єму J2 немає).

## 1. Шини живлення

| Мережа | Джерело | Споживачі |
|---|---|---|
| `VBAT` | LiPo+ на TP5 (3.7 В, 4.2 В max) | TP4056 BAT, LDO IN, EN-резистор, дільник батареї |
| `VCC_3V3` | LDO ME6211C33M5G-N OUT | SX1262 (VDD_IN, VBAT, VBAT_IO), CH32V203 (VDD), SC7A20 (VDD), pull-ups |
| `CHG_IN` | USB-C VBUS (4 контакти A4/A9/B4/B9) | TP4056 VCC |
| `GND` | — | все |

## 2. Список деталей (38 позицій у схемі)

| Ref | Деталь | LCSC | Значення | Корпус | Примітка |
|---|---|---|---|---|---|
| U1 | SX1262IMLTRT | C191341 | — | QFN-24-EP 4×4 | LoRa TX, +22 dBm, 433 МГц |
| U2 | CH32V203F8U6 | C7570477 | — | QFN-20 3×3 | 64 KB Flash / 20 KB SRAM / 144 МГц |
| U3 | SC7A20HTR | C19274408 | — | LGA-12 2×2 | ±16 g, SPI, INT1 |
| U4 | ME6211C33M5G-N | C82942 | 3.3 В / 500 мА | SOT-23-5 | LDO з EN |
| U5 | TP4056-42-ESOP8 | C16581 | — | ESOP-8 | заряд LiPo 1 А |
| Y1 | E3SB32E00000KE | C1021665 | 32 МГц, CL 12 pF | SMD3225-4P | такт SX1262 |
| L1 | LQW15AN56NG80D | C1330587 | **56 нГ** | 0402 | choke VR_PA, 340 мА |
| J1 | M2-SMA-42M | C55608210 | SMA | edge-mount | антенний вхід |
| J3 | TYPE-C-31-M-12 | C165948 | USB-C 16P | SMD | зарядка (лише живлення) |
| R1 | 0805W8F1003T5E | C149504 | 100 кОм | 0805 | pull-up NRST |
| R2, R3 | 0805W8F1003T5E | C149504 | 100 кОм | 0805 | дільник батареї |
| R4 | 0402WGF1002TCE | C25744 | 10 кОм | 0402 | VBAT → EN LDO |
| R6 | 0402WGF1002TCE | C25744 | 10 кОм | 0402 | PROG TP4056 → 100 мА |
| R7, R8 | 0402WGF5101TCE | C25905 | 5.1 кОм | 0402 | USB-C CC pull-down |
| SW1 | B3U-1000P | C231329 | — | SMD | **DNP** (ADR-0010) |
| R5 | 0402WGF1002TCE | C25744 | 10 кОм | 0402 | **DNP** разом із SW1 |
| C1, C5, C8–C11, C13 | CL05B104KO5NNNC | C1525 | 100 нФ | 0402 | bypass |
| C2 | CL21A106KAYNNNE | C15850 | 10 мкФ | 0805 | VDDA SX1262 |
| C3, C4 | 0402CG120J500NT | C1547 | 12 пФ | 0402 | кристал |
| C6, C7, C14, C15 | CL05A105KA5NQNC | C52923 | 1 мкФ | 0402 | LDO / VR_PA / VREG |
| C12 | TBD | — | TBD | 0402 | shunt π-мережі (R-15) |
| C16 | TBD | — | TBD | 0402 | DC block RFO→ANT_RF (R-15) |
| TP1, TP2 | TestPoint_Pad | — | TP_PDI / TP_SCLK | 1×1 мм | WCH ISP |
| TP5, TP6 | TestPoint_Pad | — | BAT+ / BAT− | 1×1 мм | припаюваний акумулятор |
| ANT1 | AILF002 | C22467622 | 433 МГц | — | поза PCB: антена + кабель + SMA-штекер |

## 3. Підключення U1 — SX1262IMLTRT (QFN-24)

Імена виводів — за Semtech SX1261/2 DS Rev 1.2, Table 2-1.

| Вивід | Функція | До | Примітка |
|---|---|---|---|
| 1 | VDD_IN | `VCC_3V3` | + C1 100 нФ ≤5 мм |
| 2, 5, 8, 20, 25 | GND (EP) | `GND` | термальна площадка |
| 3 | XTA | `XTAL_A` | Y1.1 + C3 12 пФ |
| 4 | XTB | `XTAL_B` | Y1.3 + C4 12 пФ |
| 6 | DIO3 | NC | вивід TCXO — не використовуємо (кварц) |
| 7 | VREG | `VREG_RADIO` | + C15 1 мкФ; з'єднаний з DCC_SW (режим LDO) |
| 9 | DCC_SW | `VREG_RADIO` | напряму з VREG, без індуктора |
| 10 | VBAT | `VCC_3V3` | + C2 10 мкФ |
| 11 | VBAT_IO | `VCC_3V3` | + C12… (див. §2 — VBAT_IO bypass 100 нФ) |
| 12 | DIO2 | NC | TXEN/RXEN зовнішнього RF-комутатора — немає |
| 13 | DIO1 | `DIO1_RADIO` | IRQ TxDone |
| 14 | BUSY | `BUSY_RADIO` | poll перед командою |
| 15 | NRESET | `NRST_RADIO` | GPIO + R1 100 кОм + C10 100 нФ |
| 16 | MISO | `SPI1_MISO` | |
| 17 | MOSI | `SPI1_MOSI` | |
| 18 | SCK | `SPI1_SCK` | |
| 19 | NSS | `CS_RADIO` | окремий CS |
| 21, 22 | RFI_P, RFI_N | NC | входи RX — варіант «мішень» лише TX |
| 23 | RFO | `RFO` | через L1 → C16 → `ANT_RF` |
| 24 | VR_PA | `VR_PA` | + C13 100 нФ + C14 1 мкФ; через L1 → RFO |

## 4. Підключення U3 — SC7A20HTR (LGA-12)

| Вивід | Функція | До |
|---|---|---|
| 1 | SDO | `SPI1_MISO` |
| 2 | SDx | `SPI1_MOSI` |
| 3 | VDDIO | `VCC_3V3` |
| 4 | NC | NC |
| 5 | INT1 | `INT_ACC` (вхід, EXTI) |
| 6 | INT2 | NC |
| 7 | VDD | `VCC_3V3` (+ C9 100 нФ) |
| 8 | GNDIO | `GND` |
| 9 | GND | `GND` |
| 10 | CS | `CS_ACC` |
| 11 | NC | NC |
| 12 | SCx | `SPI1_SCK` |

## 5. Підключення U2 — CH32V203F8U6 (QFN-20)

У корпусі QFN20 **немає виводів NRST і BOOT0** (BOOT0 = GND внутрішньо);
**немає і PA11/PA12**, тобто USB FS недоступний.

| Вивід | Сигнал | Мережа | Куди |
|---|---|---|---|
| 1 | PA0 | `BTN` | SW1.1 + R5 ↑ VCC_3V3 (DNP) |
| 2 | PA1 | `VBAT_ADC` | R2/R3 дільник + C11 100 нФ |
| 3 | PA2 | `CS_RADIO` | U1 NSS (19) |
| 4 | PA3 | `CS_ACC` | U3 CS (10) |
| 5 | PA4 | `BUSY_RADIO` | U1 BUSY (14), вхід |
| 6 | PA5 | `SPI1_SCK` | U1 SCK (18) + U3 SCx (12) |
| 7 | PA7 | `SPI1_MOSI` | U1 MOSI (17) + U3 SDx (2) |
| 8 | PB0 | `INT_ACC` | U3 INT1 (5), вхід/wake-up |
| 9 | PB1 | NC | — |
| 10 | PB10 | NC | резерв під п'єзо-канал (ADR-0003) |
| 11 | PB11 | NC | — |
| 12 | PB14 | NC | — |
| 13 | PB15 | NC | — |
| 14 | PA8 | `DIO1_RADIO` | U1 DIO1 (13) |
| 15 | PA9 | `NRST_RADIO` | U1 NRESET (15) |
| 16 | PA13 | `TP_PDI` | TP1 (WCH ISP PDI) |
| 17 | PA14 | `TP_SCLK` | TP2 (WCH ISP SCLK) |
| 18 | PA10 | NC | — |
| 19 | VDD | `VCC_3V3` | + C8 100 нФ |
| 20 | PA6 | `SPI1_MISO` | U1 MISO (16) + U3 SDO (1) |
| 21 (EP) | GND | `GND` | термальна площадка |

## 6. Живлення

| Елемент | Підключення |
|---|---|
| TP5 / TP6 | акумулятор LiPo 1S припаюється: BAT+ → `VBAT`, BAT− → `GND` |
| U5 TP4056 | VCC(4) ← `CHG_IN`, BAT(5) ← `VBAT`, TEMP(1) ← GND, PROG(2) ← `PROG` (R6 10 кОм → GND, 100 мА), GND(3) ← GND, STDBY#(6) → NC, CHRG#(7) → NC, CE(8) ← GND, EP(9) ← GND |
| U4 LDO | IN(1) ← `VBAT`, VSS(2) ← GND, CE(3) ← `LDO_EN` (R4 10 кОм ← VBAT), NC(4) → NC, VOUT(5) → `VCC_3V3`; C6 1 мкФ на IN, C7 1 мкФ на OUT |
| R2, R3 | 100 кОм: VBAT → midpoint → GND; midpoint → ADC (PA1) |

## 7. Програмування

2-дротовий WCH ISP: **TP1 = PA13 (PDI)**, **TP2 = PA14 (SCLK)**. GND і
`VCC_3V3` беруться з плати.

## 8. Зарядка (USB-C)

- J3 (TYPE-C-31-M-12, C165948), футпрінт
  `Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12`.
- **VBUS: 4 контакти (A4, A9, B4, B9) → `CHG_IN`** (усі чотири — інакше зарядка
  не працюватиме).
- GND: A1, A12, B1, B12 + SHIELD → `GND`.
- CC1 (A5) → R7 5.1 кОм → GND; CC2 (B5) → R8 5.1 кОм → GND.
- D+/D− (A6, B6, A7, B7) і SBU (A8, B8) → NC: на QFN20 немає USB
  (живлення лише).

## 9. Антена та RF-мережа

- Ланцюг: `VR_PA(24)` → **L1 56 нГ** → `RFO(23)` → **C16** (DC block) →
  `ANT_RF` → J1 центр; J1 земля → `GND`.
- C12 — shunt π-мережі `ANT_RF` → `GND`.
- Значення C12/C16 — **TBD**, підбираються на стенді (ADR-0009, R-15).
- Траса 50 Ом мікроstrip; під нею — суцільна GND-площа; PCB-антена не
  розводиться; L1 **перпендикулярно** RF-трасі (AN1200.66).

## 10. Правила розводки

1. C1 (100 нФ) і C2 (10 мкФ) — ≤ 5 мм від SX1262; C13/C14 (VR_PA) — ≤ 5 мм
   від виводу 24.
2. Кристал Y1 — ≤ 5 мм від SX1262; C3/C4 — ≤ 2 мм від кристалу.
3. GND-площа під U1 (QFN-EP) — не менше 9 vias.
4. SPI-шина — коротка, ≤ 30 мм; окремі CS.
5. Антенна зона — без міді знизу під трасою.
6. Зона U5/TP3..TP6/J3 — розсунути при layout (AGENT: SHIELD J3 ≈ 2.5 мм
   від U5).

## 11. DNP

`SW1` (кнопка) і `R5` (її підтягування) — нативний атрибут KiCad `dnp`
(ADR-0010, варіант C). На прототипі можна поставити вручну.

## 12. Що НЕ входить у цю схему (хаб)

Хаб — **окремий виріб з окремим ТЗ** (ADR-0008). Дані пристрою: SX1262 у
режимі RX, той самий MCU, USB-C (але на QFN-20 немає USB FS — див. ADR-0008).

## 13. Відкриті питання по схемі

| # | Питання | Статус |
|---|---|---|
| S1 | Значення C12 (shunt) і C16 (DC block) | TBD — R-15 |
| S2 | Сумісність футпрінта `SMA_Amphenol_132289_EdgeMount` із клоном M2-SMA-42M | перевірити перед замовленням PCB |
| S3 | Чи вистачить запасу по пінах USB-C (4 VBUS + 4 GND + SHIELD) на краю плати | вирішити на layout |
