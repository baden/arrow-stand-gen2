# arrow-stand-gen2 — Makefile
# GNU Make. Команди розраховані на Linux/WSL/macOS; на Windows — через WSL
# або MSYS2.

SHELL   := /bin/bash
PROJ    := arrow-stand-gen2
KICAD_D := hardware/kicad
SCH     := $(KICAD_D)/$(PROJ).kicad_sch
PCB     := $(KICAD_D)/$(PROJ).kicad_pcb
PRO     := $(KICAD_D)/$(PROJ).kicad_pro

FW_DIR  := firmware
FW_SRC  := $(wildcard $(FW_DIR)/src/*.c) $(wildcard $(FW_DIR)/platform/*.c)
FW_BUILD:= $(FW_DIR)/build
FW_ELF  := $(FW_BUILD)/$(PROJ).elf
FW_BIN  := $(FW_BUILD)/$(PROJ).bin

PREFIX  := riscv-none-elf-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size

# KiCAD CLI
KICAD_CLI ?= kicad-cli

.PHONY: help all firmware firmware-lint toolchain-check bom erc drc cost prices upgrade fmt-check lint clean snapshot

help:
	@echo "arrow-stand-gen2 — available targets"
	@echo "  make firmware        build the CH32V203 firmware"
	@echo "  make firmware-lint   compile with -Wall -Wextra -Werror (no link)"
	@echo "  make toolchain-check verify riscv-none-elf-gcc is available"
	@echo "  make bom             export BOM from the schematic (kicad-cli)"
	@echo "  make erc             run ERC on the schematic"
	@echo "  make drc             run DRC on the board"
	@echo "  make cost            recalculate the BOM cost model"
	@echo "  make prices          fetch the LCSC price ladder and update BOM prices"
	@echo "  make upgrade         upgrade KiCAD files to the newest file format"
	@echo "  make fmt-check       verify the KiCAD file format version"
	@echo "  make lint            all static checks"
	@echo "  make snapshot        export schematic + PCB PDFs (checkpoint)"

all: firmware bom

# --- Прошивка ---

toolchain-check:
	@command -v $(CC) >/dev/null 2>&1 || { \
	  echo "ERROR: $(CC) not found in PATH."; \
	  echo "  Install MounRiver SDK (WCH) or riscv-none-elf toolchain."; \
	  exit 1; }
	@$(CC) --version | head -1

firmware: toolchain-check
	@mkdir -p $(FW_BUILD)
	$(CC) -march=rv32imac -mabi=ilp32 -Os -ffunction-sections -fdata-sections \
	  -I$(FW_DIR)/include -I$(FW_DIR)/platform \
	  -DCH32V203 \
	  $(FW_SRC) $(FW_DIR)/linker/ch32v203.ld -o $(FW_ELF) -Wl,--gc-sections -lm
	$(SIZE) $(FW_ELF)
	$(OBJCOPY) -O binary $(FW_ELF) $(FW_BIN)
	@echo "OK: $(FW_BIN)"

firmware-lint:
	@mkdir -p $(FW_BUILD)
	$(CC) -march=rv32imac -mabi=ilp32 -Os -Wall -Wextra -Werror -fsyntax-only \
	  -I$(FW_DIR)/include -I$(FW_DIR)/platform -DCH32V203 \
	  $(FW_SRC)

# --- KiCAD ---

bom:
	$(KICAD_CLI) sch export bom --output $(KICAD_D)/../bom/bom.csv $(SCH)
	@echo "NOTE: hardware/bom/bom.csv is generated — edit bom-draft.csv instead"

erc:
	$(KICAD_CLI) sch erc --output $(FW_BUILD)/erc.json $(SCH)

drc:
	$(KICAD_CLI) pcb drc --output $(FW_BUILD)/drc.json $(PCB)

snapshot:
	$(KICAD_CLI) sch export pdf --output $(FW_BUILD)/schematic.pdf $(SCH)
	$(KICAD_CLI) pcb export pdf --output $(FW_BUILD)/board.pdf $(PCB)

# --- Формат файлів KiCAD ---
#
# KiCAD 10 зберігає схему у форматі 20260306, а PCB — 20260206. Якщо файл
# було створено/перезаписано старішим форматом, KiCAD при наступному відкритті
# переконвертує його сам — і зміна потрапить у git уже з новим форматом.
# Тому після будь-якої правки через Copperpour MCP перевіряємо версію.

upgrade:
	$(KICAD_CLI) sch upgrade $(SCH)
	$(KICAD_CLI) pcb upgrade $(PCB)

fmt-check:
	@echo "Очікуваний формат: sch 20260306 / pcb 20260206 (KiCad 10.0.7)"
	@grep -m1 'version' $(SCH) | sed 's/^/  схема: /'
	@grep -m1 'version' $(PCB) | sed 's/^/  плата: /'

# --- Вартість ---

# Зняти реальну цінову сходинку LCSC і оновити ціни в BOM.
QTY ?= 100
prices:
	python3 tools/lcsc_prices.py --from-bom hardware/bom/bom-draft.csv \
	  --qty $(QTY) --out hardware/bom/lcsc-prices.csv --update-bom

cost: prices
	python3 tools/cost_model.py --bom hardware/bom/bom-draft.csv --qty $(QTY) --check

# --- Перевірки ---

lint: firmware-lint
	@echo "lint OK"

clean:
	rm -rf $(FW_BUILD)
