# arrow-stand-gen2 — Makefile
# GNU Make. Команди розраховані на Linux/WSL/macOS; на Windows — через WSL
# або MSYS2. KiCAD-команди — через kicad-cli (PATH).

SHELL   := /bin/bash

# --- Пристрої -----------------------------------------------------------
# firmware/common   — спільний код (HAL, драйвери SX1262/SC7A20, протокол)
# firmware/target   — «мішень»: детекція удару + класифікація
# firmware/hub      — «хаб»: прийом, реєстр каналів, USB-CDC
DEV     ?= target
ifeq ($(DEV),target)
  PROJ   := arrow-stand-gen2
else ifeq ($(DEV),hub)
  PROJ   := arrow-stand-hub
else
  $(error DEV має бути target або hub, а не "$(DEV)")
endif

COMMON  := firmware/common
FW_DIR  := firmware/$(DEV)
FW_SRC  := $(wildcard $(COMMON)/src/*.c) $(wildcard $(COMMON)/platform/*.c) \
           $(wildcard $(FW_DIR)/src/*.c)
FW_INC  := -I$(COMMON)/include -I$(COMMON)/platform -I$(FW_DIR)/include
FW_BUILD:= firmware/build/$(DEV)
FW_ELF  := $(FW_BUILD)/$(PROJ).elf
FW_BIN  := $(FW_BUILD)/$(PROJ).bin

PREFIX  := riscv-none-elf-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size

# --- KiCAD --------------------------------------------------------------
TARGET_DIR ?= hardware/target
HUB_DIR    ?= hardware/hub
SCH        := $(TARGET_DIR)/$(PROJ).kicad_sch
PCB        := $(TARGET_DIR)/$(PROJ).kicad_pcb
SCH_HUB    := $(HUB_DIR)/arrow-stand-hub.kicad_sch
PCB_HUB    := $(HUB_DIR)/arrow-stand-hub.kicad_pcb
KICAD_CLI  ?= kicad-cli

# --- Параметри ----------------------------------------------------------
QTY   ?= 100

.PHONY: help all firmware firmware-lint toolchain-check bom erc drc cost prices \
        upgrade fmt-check libs libs-check lint clean snapshot hub-erc hub-drc

help:
	@echo "arrow-stand-gen2 — available targets"
	@echo "  make firmware DEV=target   build the target firmware (мішень)"
	@echo "  make firmware DEV=hub      build the hub firmware (хаб)"
	@echo "  make firmware-lint         compile with -Wall -Wextra -Werror (no link)"
	@echo "  make toolchain-check       verify riscv-none-elf-gcc is available"
	@echo "  make bom                   export BOM from the schematic (kicad-cli)"
	@echo "  make erc / make drc        ERC / DRC of the target"
	@echo "  make hub-erc / hub-drc     ERC / DRC of the hub"
	@echo "  make cost                  recalculate the BOM cost model"
	@echo "  make prices                fetch the LCSC price ladder and update BOM"
	@echo "  make libs                  sync the KiCad libraries target -> hub"
	@echo "  make libs-check            check that the KiCad libraries match"
	@echo "  make upgrade               upgrade KiCAD files to the newest format"
	@echo "  make fmt-check             verify the KiCAD file format version"

all: firmware

# --- Прошивка -----------------------------------------------------------

toolchain-check:
	@command -v $(CC) >/dev/null 2>&1 || { \
	  echo "ERROR: $(CC) not found in PATH."; \
	  echo "  Install MounRiver SDK (WCH) or riscv-none-elf toolchain."; \
	  exit 1; }
	@$(CC) --version | head -1

firmware: toolchain-check
	@mkdir -p $(FW_BUILD)
	$(CC) -march=rv32imac -mabi=ilp32 -Os -ffunction-sections -fdata-sections \
	  $(FW_INC) -DCH32V203 -DDEVICE_$(shell echo $(DEV) | tr a-z A-Z) \
	  $(FW_SRC) $(COMMON)/linker/ch32v203.ld -o $(FW_ELF) -Wl,--gc-sections -lm
	$(SIZE) $(FW_ELF)
	$(OBJCOPY) -O binary $(FW_ELF) $(FW_BIN)
	@echo "OK: $(FW_BIN)"

firmware-lint:
	@mkdir -p $(FW_BUILD)
	$(CC) -march=rv32imac -mabi=ilp32 -Os -Wall -Wextra -Werror -fsyntax-only \
	  $(FW_INC) -DCH32V203 $(FW_SRC)

# --- KiCAD --------------------------------------------------------------

bom:
	$(KICAD_CLI) sch export bom --output hardware/bom/bom-$(DEV).csv $(SCH)

erc:
	$(KICAD_CLI) sch erc --output firmware/build/erc-$(DEV).json $(SCH)

drc:
	$(KICAD_CLI) pcb drc --output firmware/build/drc-$(DEV).json $(PCB)

hub-erc:
	$(KICAD_CLI) sch erc --output firmware/build/erc-hub.json $(SCH_HUB)

hub-drc:
	$(KICAD_CLI) pcb drc --output firmware/build/drc-hub.json $(PCB_HUB)

snapshot:
	$(KICAD_CLI) sch export pdf --output firmware/build/schematic-$(DEV).pdf $(SCH)
	$(KICAD_CLI) pcb export pdf --output firmware/build/board-$(DEV).pdf $(PCB)

# --- Формат файлів KiCAD ------------------------------------------------
#
# KiCAD 10 зберігає схему у форматі 20260306, а PCB — 20260206. Якщо файл
# було створено/перезаписано старішим форматом, KiCAD при наступному відкритті
# переконвертує його сам — і зміна потрапить у git уже з новим форматом.
# Тому після будь-якої правки через Copperpour MCP перевіряємо версію.

upgrade:
	$(KICAD_CLI) sch upgrade $(SCH)
	$(KICAD_CLI) pcb upgrade $(PCB)
	$(KICAD_CLI) sch upgrade $(SCH_HUB)
	$(KICAD_CLI) pcb upgrade $(PCB_HUB)

fmt-check:
	@echo "Очікуваний формат: sch 20260306 / pcb 20260206 (KiCad 10.0.7)"
	@grep -m1 'version' $(SCH) | sed 's/^/  схема мішені: /'
	@grep -m1 'version' $(PCB) | sed 's/^/  плата мішені: /'
	@grep -m1 'version' $(SCH_HUB) | sed 's/^/  схема хаба:   /'
	@grep -m1 'version' $(PCB_HUB) | sed 's/^/  плата хаба:   /'

# --- Бібліотеки KiCAD ---------------------------------------------------
#
# Кожен проєкт має власну копію бібліотеки arrowstand (MCP пише її в каталог
# проєкту). hardware/target — майстер, hardware/hub — копія.

libs:
	python3 tools/sync_kicad_libs.py --to hub

libs-check:
	python3 tools/sync_kicad_libs.py --check

# --- Вартість -----------------------------------------------------------

# Зняти реальну цінову сходинку LCSC і оновити ціни в BOM.
prices:
	python3 tools/lcsc_prices.py --from-bom hardware/bom/bom-draft.csv \
	  --qty $(QTY) --out hardware/bom/lcsc-prices.csv --update-bom

cost: prices
	python3 tools/cost_model.py --bom hardware/bom/bom-draft.csv --qty $(QTY) --check

# --- Перевірки ----------------------------------------------------------

lint: firmware-lint
	@echo "lint OK"

clean:
	rm -rf firmware/build
