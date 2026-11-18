---
name: Firmware CH32V203
description: |
  Building and flashing the CH32V203 firmware for arrow-stand-gen2: toolchain
  setup, project layout, the hand-written minimal SX1262 LoRa P2P driver,
  SPI bus sharing between SX1262 and SC7A20, sleep/wake budget, Flash/RAM
  limits, host tests, and the coding conventions for the firmware tree.
  Triggers on: "firmware", "build the firmware", "flash", "CH32V203", "RISC-V",
  "driver for SX1262", "driver for SC7A20", "sleep mode", "read accelerometer".
argument-hint: "[task]"
---

# Firmware — CH32V203F8U6 (arrow-stand-gen2)

## Layout

```
firmware/
├── src/          application: main.c, app.c, event.c, protocol.c
├── include/      shared headers
├── platform/     HAL: board.h (pin map), hal_ch32v203, hal_sx1262,
│                 hal_sc7a20, hal_power
├── linker/       ld script (16 KB flash / 2 KB RAM)
└── tests/        host tests (no hardware)
```

Rules:
- `platform/` knows nothing about the application; the application never
  touches registers.
- Every module that can be tested on the host must be.

## Pipeline (ADR-0008): все на пристрої

1. **Сенсор** — SC7A20 у FIFO-режимі (32 зразки), threshold-detector → INT1.
2. **Захоплення** — burst-читання FIFO (96 байт) по SPI, DMA.
3. **Ознаки** — модуль |a|, пік, RMS, імпульс Σ|a|Δt, zero-crossings,
   час наростання. int32, буфер 32×3×2 B.
4. **Класифікація** — дерево рішень або мала int8-MLP на ознаках:
   стріла / легкий дотик / похитування мішені / шум.
5. **Звіт** — пакет v2 (16 Б): клас, пік, імпульс, батарея.

> **Порядок робіт:** спершу **логгер** (збір відрізків удару в flash/USB для
> R-03), і лише потім класифікатор. Пороги без виміряних даних — вигадка.

## Hard limits

| Resource | Size | Note |
|---|---|---|
| Flash | 64 KB | Target ≤ 38 KB. `make firmware` prints `size` — watch it. |
| SRAM | 20 KB | Budget ≤ 10 KB for DSP buffers + stack |
| Stack | ~1 KB | No deep recursion |

We write our own minimal SX1262 driver (no Semtech LoRa Basics Modem — it
would need 40 KB Flash / 6 KB RAM). Driver + SC7A20 + app + algorithm must
fit in 64 KB. If it does not fit, shrink the algorithm — not the drivers.

## Toolchain

```bash
riscv-none-elf-gcc --version     # MounRiver SDK / WCH
make toolchain-check
make firmware                    # -> firmware/build/arrow-stand-gen2.bin
```

Flash: WCH-LinkE, 2-wire WCH ISP over **PDI + SCLK** (+ GND, VCC). The board
needs those two test pads — they are part of the schematic, do not "optimise"
them away.

Alternatively CH32V203 has USB FS — a USB-C programming port is an option,
but it costs pins and a connector; keep WCH 2-wire ISP as the primary path.

## SPI bus

One controller, two targets, separate CS:

| Signal | SX1262 | SC7A20 |
|---|---|---|
| SCK | PA5 | PA5 |
| MOSI | PA7 | PA7 |
| MISO | PA6 | PA6 |
| CS | PA2 | PA3 |
| IRQ | PA4 (BUSY), PA8 (DIO1) | PB0 (INT1) |
| RESET | PA9 | — |

Pin map lives in `firmware/platform/board.h` and must match the
schematic exactly. CH32V203 SPI1 is fixed by the peripheral:
PA5 = SCK, PA6 = MISO, PA7 = MOSI.

Always toggle CS around a transaction. The SX1262 needs a `BUSY` poll between
commands — respect the datasheet timing, not a fixed delay.

## Power modes

| Mode | What runs | Target current |
|---|---|---|
| Init | peripherals up | — |
| Wait | SC7A20 standby + threshold interrupt | ~30 µA |
| Event | read peaks, build packet | — |
| TX | SX1262 transmit | ~130 mA peak |
| Sleep | everything off | ≤ 2 µA |

The device must spend nearly all its life in Wait/Sleep. A design that polls
the accelerometer continuously is a bug, not a style choice.

## Driver notes

**SX1262** — command-based state machine, not registers:
`setStandby` → `setPacketType(LoRa)` → `setRfFrequency` → `setTxPower`
→ `setDioIrqParams` → `setTx`. Read `getIrqStatus` / `getRxBufferStatus`
after `waitOnIrq`. Poll `BUSY` before every command.

**SC7A20** — LIS2DH12-compatible register map:
init → `CTRL_REG1` (ODR) → `CTRL_REG4` (range ±16 g) → `CTRL_REG6` (BDU)
→ `WHITE_MSM`/`TAP_CFG` threshold → `INT1_CFG`/`INT1_DUR` → enable INT1 on PB2.

## Shared with the hub

The same code base serves both roles (ADR-0008):

| | Target | Hub |
|---|---|---|
| SX1262 | TX, SF11, after an impact | always RX |
| MCU | deep sleep, wake on INT1 | always on |
| Output | LoRa packet | USB FS (CDC) to the PC |
| Accel / charger | yes | DNP |

Keep the HAL, the SX1262 driver and the packet format identical; only the
role and the power policy differ.

## Conventions

- English for identifiers, comments and logs.
- `snake_case` functions, `UPPER_SNAKE` macros, `Module_State` types.
- No dynamic allocation.
- `HAL_Delay` only where the datasheet demands it — never in a polling loop
  that could instead wait on an interrupt.
- Every public function documented with units.

## Tests

Host tests live in `firmware/tests/` and must not include hardware headers.
Cover at minimum: CRC8, packet packing, the impact filter, debounce.

## Definition of done for firmware

- [ ] `make firmware-lint` clean (-Wall -Wextra -Werror)
- [ ] `size` output: flash ≤ 12 KB, RAM ≤ 1.5 KB
- [ ] Host tests pass
- [ ] Measured sleep current ≤ 5 µA (R-05)
- [ ] Radio link verified over 300 m (R-06)
