# SPDX-License-Identifier: GPL-2.0

include scripts/Makefile.include

PHONY := all clean

SUBDIRS		:= startup kernel
CLEAN_DIRS	:= $(addprefix _clean_, $(SUBDIRS))

PHONY += $(SUBDIRS)

all: $(SUBDIRS) startup.bin
	@cp ./startup.bin ../xemu/image/payload.bin
	@cp ./System.map ../xemu/image/System.map

$(SUBDIRS):
	@$(MAKE) -f ./scripts/Makefile.build obj=$@

startup.bin: startup.elf System.map
	@printf "LD\t$@\n"
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

startup.elf: startup/startup.ko $(LDS_FILE)
	@printf "LD\t$@\n"
	$(LD) $(LDFLAGS) -T $(LDS_FILE) -o $@ $<

System.map: startup.elf
	$(NM) -n $< | \
		grep -v '\( [aNUw] \)\|\(__crc_\)\|\( \$[adt]\)\|\( \.L\)' > $@

PHONY += $(CLEAN_DIRS)

$(CLEAN_DIRS):
	@$(MAKE) -f ./scripts/Makefile.clean obj=$@

clean: $(CLEAN_DIRS)
	@rm -f ./*.bin ./*.elf

dump:
	$(OBJDUMP) -D -m riscv:rv64 -EL -b binary ./startup.bin

.PHONY: $(PHONY)
