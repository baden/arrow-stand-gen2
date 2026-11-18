# Прошивка — CH32V203F8U6

Прошивка датчика влучання: читає акселерометр, ловить удар, рахує ознаки,
**класифікує подію на пристрої** (ADR-0008), формує пакет 17 Б і відправляє
по LoRa.

MCU: **CH32V203C8U6** (64 KB Flash / 20 KB SRAM, 144 МГц, QFN-48) — ADR-0011.
Радіо: власний мінімальний драйвер SX1262 (LoRa P2P), **не** Semtech
LoRa Basics Modem. Діапазон 433 МГц, SF11 / BW 125 кГц.

## Статус

**Каркас створено, логіка ще не написана.** HAL — заглушки; протокол пакета
v2 — готовий і покритий host-тестами.

## Структура

```
firmware/
├── common/                   спільний код для обох пристроїв
│   ├── include/
│   │   ├── protocol.h        формат пакета + перелік класів і прапорців
│   │   └── hal_*.h          HAL: CH32V203, SX1262, SC7A20, power, USB
│   ├── src/
│   │   ├── protocol.c      формування пакета v2, CRC8
│   │   └── hal_*.c
│   ├── platform/             драйвери пристроїв
│   ├── linker/
│   │   └── ch32v203.ld      64 KB Flash @ 0x00000000, 20 KB SRAM @ 0x20000000
│   └── tests/
│       └── test_protocol.c  host-тести формату пакета і CRC8
├── target/                   «мішень»
│   ├── include/
│   │   ├── config.h         константи компіляції (LoRa, пороги, розміри)
│   │   └── app.h
│   ├── src/
│   │   ├── main.c           точка входу, головний цикл
│   │   └── app.c            детекція удару, ознаки, класифікація
│   └── board.h              розподіл виводів плати мішені
└── hub/                      «хаб»
    ├── include/
    │   ├── config.h
    │   └── app.h
    ├── src/
    │   ├── main.c
    │   └── app.c            прийом пакетів, реєстр каналів, USB-CDC
    └── board.h              розподіл виводів плати хаба
```

Збірка: `make firmware DEV=target` або `make firmware DEV=hub`.

## Toolchain

| Компонент | Варіант |
|---|---|
| Компілятор | `riscv-none-elf-gcc` (MounRiver SDK / WCH MounRiver Studio) |
| Прошивальник | WCH-LinkE, 2-дротовий WCH ISP: PDI + SCLK (+ GND, VCC) |
| Відладка | WCH-Link, RV32IMAC |

Встановлення: `make toolchain-check` перевіряє наявність компілятора.

> **Стан на цій машині (Windows, 2026-11-18):** `riscv-none-elf-gcc` і host-`gcc`
> **не встановлені**. Потрібно встановити WCH MounRiver SDK (або
> `riscv-none-elf` toolchain) і будь-який host-компілятор для тестів.

Host-тести можна зібрати будь-яким C-компілятором:

```bash
gcc -I firmware/include firmware/src/protocol.c firmware/tests/test_protocol.c \
    -o firmware/build/test_protocol && ./firmware/build/test_protocol
```

## Обмеження ресурсів

| Ресурс | Обсяг | Застереження |
|---|---|---|
| Flash | 64 KB | Власний драйвер SX1262 (~5–8 KB) + застосунок + алгоритм. Ціль ≤ 38 KB |
| SRAM | 20 KB | Бюджет на буфери DSP: до 10 KB |
| Стек | ~1 KB | Обережно з глибиною рекурсії та локальними масивами |
| Частота | 144 МГц | 52 мкА/МГц у run — перевірити режим сну (R-05) |

Ядро: QingKe V4B (RV32IMAC) — **одноциклове множення, апаратне ділення**,
без FPU (float не потрібен — увесь стек fixed-point).

## Розподіл виводів

Остаточний розподіл — у `firmware/target/board.h` і `firmware/hub/board.h`;
вони мають збігатися зі схемами (`docs/architecture/schematic-spec.md`,
`docs/hub/hub-requirements.md`).

| Функція | CH32V203 | Примітка |
|---|---|---|
| SPI1 SCK / MISO / MOSI | PA5 / PA6 / PA7 | визначено периферією |
| CS радіо | PA2 | окремий CS |
| CS акселерометра | PA3 | окремий CS |
| SX1262 BUSY | PA4 | вхід, poll перед командою |
| SX1262 DIO1 | PA8 | вхід, IRQ |
| SX1262 NRST | PA9 | вихід |
| SC7A20 INT1 | PB0 | вхід, EXTI → пробудження |
| Кнопка | PA0 | вхід, pull-up (DNP у серії, ADR-0010) |
| ADC батареї | PA1 | дільник VBAT |
| USB DM / DP | PA11 / PA12 | CDC (ADR-0011) |
| ISP PDI / SCLK | PA13 / PA14 | тестові площадки TP1/TP2 |
| Резерв: п'єзо-канал | PB10 | цифровий вхід (варіант B, ADR-0003) |

**Вільні GPIO:** PB1, PB11, PB12, PB13, PB14, PB15, PA10, PA15, PB3…PB9.
**Вільний ADC:** PB1 (ADC9). PB2 — strap BOOT1 (на GND), не GPIO.

**Вільні GPIO:** PB1, PB11, PB14, PB15, PA10. **Вільних ADC-каналів немає**
(PA0…PA7 усі зайняті).

Один SPI-контролер на два пристрої. Завжди перемикати CS перед транзакцією;
SX1262 вимагає poll `BUSY` між командами, а не фіксованої затримки.

## Режими живлення

| Режим | Джерело | Опис |
|---|---|---|
| Init | RUN | Ініціалізація SX1262, SC7A20 |
| Wait | STOP | SC7A20 у standby + threshold-detector → INT |
| Event | RUN | Читання FIFO, ознаки, класифікація |
| TX | RUN | SX1262 TX LoRa-пакета |
| Sleep | STOP | Все вимкнено, wake по INT або таймеру |

## Конвеєр обробки (ADR-0008)

1. **Сенсор** — SC7A20 у FIFO-режимі (32 зразки), threshold-detector → INT1.
2. **Захоплення** — burst-читання FIFO (96 Б) по SPI, DMA.
3. **Ознаки** — |a|, пік, RMS, імпульс Σ|a|Δt, zero-crossings, час наростання,
   **час на рейлі** (сигнал насичено на ±16 g).
4. **Класифікація** — стріла / легкий дотик / похитування / шум.
5. **Звіт** — пакет v2 (17 Б).

> Порядок робіт: спершу **логгер** (R-03), потім класифікатор. Пороги без
> виміряних даних — вигадка.

## Планування

- [ ] HAL SPI з обома CS + DMA
- [ ] Драйвер SX1262: setStandby, setPacketType, setRfFrequency, setPaConfig,
      setTxParams, setTx, setRx, setDioIrqParams, getIrqStatus
- [ ] Драйвер SC7A20: init, ODR, ±16 g, BDU, FIFO, threshold, INT1
- [ ] Детектор удару + ознаки (стійкі до насичення)
- [ ] Класифікатор (після R-03)
- [ ] Логгер для R-03 (flash / UART)
- [ ] Вимірювання струму (R-05)

## Правила

- Коментарі та імена — англійською.
- HAL не знає про застосунок; застосунок не лізє в регистри.
- Розподіл виводів — тільки в `board.h`, не розкидати по коду.
- Кожен модуль, який можна, — під host-тестом.
- Перед комітом: `make firmware lint` — має бути чисто.
