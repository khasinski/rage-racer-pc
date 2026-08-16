# rage-pc - Rage Racer (PS1) matching decompilation scaffold.

# The game had one European release, SCES-00650. This tree calls it PAL
# throughout; EUR was a second name for the same build and is gone.
VERSION    ?= PAL
BASENAME   := main

TARGET_SHA_PAL := 2913e15648eddef40821c5f666460abc04155ee6
TARGET_SHA_USA := 2661e8bf18d209c98fd70d33e18ab88b10abd52b
TARGET_SHA := $(TARGET_SHA_$(VERSION))

ROOT       := $(abspath .)
PY         ?= $(if $(wildcard $(ROOT)/.venv/bin/python),$(ROOT)/.venv/bin/python,python3)
SPLAT_CFG  := configs/$(VERSION)/$(BASENAME).yaml

BUILD      := build/$(VERSION)
ASM_DIR    := asm/$(VERSION)/$(BASENAME)
SRC_DIR    := src/$(BASENAME)
LD_SCRIPT  := linkers/$(VERSION)/$(BASENAME).ld
TARGET_BIN := assets/$(VERSION)/$(BASENAME).exe

ELF        := $(BUILD)/$(BASENAME).elf
OUT_BIN    := $(BUILD)/$(BASENAME).exe

CC_WRAPPER := tools/scripts/cc.sh
AS         := mipsel-none-elf-as
LD         := mipsel-none-elf-ld
NM         := mipsel-none-elf-nm
OBJCOPY    := mipsel-none-elf-objcopy
READELF    := mipsel-none-elf-readelf
OBJDIFF    ?= build/toolchain/bin/objdiff-cli

ASM_SRCS := $(shell find $(ASM_DIR) -name '*.s' -not -path '*/nonmatchings/*' 2>/dev/null)
C_SRCS   := $(shell find $(SRC_DIR)/$(VERSION) -name '*.c' 2>/dev/null)

# A .s under src/ is either a translation unit in its own right - the original
# shipped it as assembly and there is no C to write - or the assembly half of a
# unit whose C sits beside it. Only the first kind is assembled on its own; the
# second is pulled in by its .c and would collide with it here.
SRC_ASM_ALL := $(shell find $(SRC_DIR)/$(VERSION) -name '*.s' 2>/dev/null)
SRC_ASM  := $(foreach s,$(SRC_ASM_ALL),$(if $(wildcard $(s:.s=.c)),,$(s)))

ASM_OBJS := $(ASM_SRCS:%.s=$(BUILD)/%.s.o)
C_OBJS   := $(C_SRCS:%=$(BUILD)/%.o)
SRC_ASM_OBJS := $(SRC_ASM:%=$(BUILD)/%.o)
OBJS := $(ASM_OBJS) $(C_OBJS) $(SRC_ASM_OBJS)

# Header dependencies, written by cpp -MD in tools/scripts/cc.sh. Without
# these a change under include/ leaves every dependent object stale, which
# has repeatedly hidden real breakage behind a passing build.
C_DEPS := $(C_OBJS:.o=.o.d)
-include $(C_DEPS)

.PHONY: all setup stage split build check audit-code test progress expected report clean distclean help

all: build check

setup:
	$(PY) -m venv .venv
	.venv/bin/pip install -r requirements.txt

stage:
	$(PY) tools/scripts/stage_discs.py $(STAGE_ARGS)

# Objects go too: a unit that pulls in generated assembly with `.include` has
# no recorded dependency on it, so a re-split would otherwise leave it built
# against the previous disassembly.
split:
	rm -rf $(BUILD)/src $(ASM_DIR) $(LD_SCRIPT) $(UNDEFINED_SYMS) $(UNDEFINED_FUNCS) $(ADDR_ALIASES) $(ADDR_HALVES)
	$(PY) -m splat split $(SPLAT_CFG)
	$(PY) tools/scripts/gen_nonmatching_asm.py --version $(VERSION) --basename $(BASENAME)
	$(PY) tools/scripts/symbolise_data_words.py --version $(VERSION) --basename $(BASENAME)
	$(PY) tools/scripts/symbolise_header.py --version $(VERSION) --basename $(BASENAME)
	$(PY) tools/scripts/strip_nonmatching_markers.py --version $(VERSION) --basename $(BASENAME)

$(BUILD)/asm/%.s.o: asm/%.s
	@mkdir -p $(dir $@)
	$(AS) -EL -G0 -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude -I$(ASM_DIR) -o $@ $<

define compile_c_object
	@mkdir -p $(dir $@)
	$(CC_WRAPPER) $< $@
endef

$(BUILD)/src/%.c.o: src/%.c | $(BUILD)
	$(call compile_c_object)

$(BUILD)/src/%.s.o: src/%.s | $(BUILD)
	@mkdir -p $(dir $@)
	$(AS) -EL -G0 -march=r3000 -mtune=r3000 -no-pad-sections -Iinclude -I$(ASM_DIR) -o $@ $<

# A HANDWRITTEN_ASM unit pulls its assembly in with `.include`, which cpp never
# sees, so -MD does not record it. Without this an edit to the .s leaves the
# object stale and the build silently keeps the previous instructions.
$(BUILD)/src/%.c.o: src/%.s

$(BUILD):
	@mkdir -p $@

UNDEFINED_SYMS := linkers/$(VERSION)/undefined_syms_auto.$(BASENAME).txt
UNDEFINED_FUNCS := linkers/$(VERSION)/undefined_funcs_auto.$(BASENAME).txt
UNDEFINED_MANUAL := linkers/$(VERSION)/undefined_syms_manual.txt
ADDR_ALIASES := linkers/$(VERSION)/undefined_addr_aliases.$(BASENAME).txt
ADDR_HALVES := linkers/$(VERSION)/addr_halves.$(BASENAME).txt

build: $(OUT_BIN)

# splat's undefined_syms_auto / undefined_funcs_auto are NOT linked in: every
# line in them is an assignment, and an assignment overrides a real definition,
# so linking them would pin each address forever. They stay on disk because the
# disassembler reads them for symbol names. This target distils them down to
# the addresses the link actually still needs, and only that file is linked.
$(ADDR_ALIASES): $(OBJS) $(UNDEFINED_MANUAL) $(UNDEFINED_SYMS) $(UNDEFINED_FUNCS) $(ADDR_HALVES)
	$(PY) tools/scripts/gen_undefined_addr_aliases.py --nm $(NM) --output $@ \
	      --source $(UNDEFINED_SYMS) --source $(UNDEFINED_FUNCS) \
	      --manual $(UNDEFINED_MANUAL) --manual $(ADDR_HALVES) $(OBJS)

$(ELF): $(OBJS) $(LD_SCRIPT) $(UNDEFINED_MANUAL) $(ADDR_ALIASES) $(ADDR_HALVES)
	$(LD) -EL -T $(LD_SCRIPT) \
	      -T $(UNDEFINED_MANUAL) -T $(ADDR_ALIASES) -T $(ADDR_HALVES) \
	      -Map $(BUILD)/$(BASENAME).map -o $@

$(OUT_BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

check: $(OUT_BIN)
	@echo "$(TARGET_SHA)  $(OUT_BIN)" | shasum -c -

audit-code:
	$(PY) tools/scripts/code_debt.py --check
	$(PY) -m unittest tools.tests.test_code_debt

# Enumerated rather than discovered: tools/ has no __init__.py, so unittest
# discovery cannot import it, but the namespace package resolves by name.
TESTS := tools.tests.test_code_debt tools.tests.test_gen_expected \
         tools.tests.test_gen_objdiff_config \
         tools.tests.test_progress_report tools.tests.test_strip_nonmatching_markers

test:
	$(PY) -m unittest $(TESTS)

progress:
	$(PY) tools/scripts/progress_report.py --version $(VERSION)

# objdiff compares what this tree builds against objects disassembled from the
# game itself. `expected` produces that second side; `report` scores it and
# writes the file decomp.dev ingests. Both need a build that already passed
# `check`, because the target side is named from the verified build's symbols.
expected: check
	$(PY) tools/scripts/gen_expected.py --version $(VERSION) --basename $(BASENAME) \
	      --python $(PY) --as $(AS) --objcopy $(OBJCOPY) --readelf $(READELF)

report: expected
	$(PY) tools/scripts/gen_objdiff_config.py --version $(VERSION) --basename $(BASENAME)
	$(OBJDIFF) report generate -p . -o $(BUILD)/report.json

clean:
	rm -rf $(BUILD)

distclean: clean
	rm -rf asm/$(VERSION)/$(BASENAME) $(LD_SCRIPT) \
	       $(UNDEFINED_SYMS) $(UNDEFINED_FUNCS) $(ADDR_ALIASES) $(ADDR_HALVES)

help:
	@echo "Targets:"
	@echo "  setup             Create .venv and install Python tooling"
	@echo "  stage             Symlink local disc dumps and extract boot EXEs"
	@echo "  split VERSION=PAL Run splat for PAL or USA"
	@echo "  build VERSION=PAL Build split output"
	@echo "  check VERSION=PAL Verify rebuilt EXE SHA-1"
	@echo "  audit-code        Check that game-code scaffolding debt did not increase"
	@echo "  test              Run the tooling unit tests"
	@echo "  progress          Refresh badge JSON and print the progress table"
	@echo "  expected          Build the objdiff target objects from the game EXE"
	@echo "  report            Write build/$$(VERSION)/report.json for decomp.dev"
	@echo "  clean             Remove build/ for selected VERSION"
	@echo "  distclean         Also remove generated asm/linker output"
