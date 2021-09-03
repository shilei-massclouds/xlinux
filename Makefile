# SPDX-License-Identifier: GPL-2.0

PHONY := all clean

SUBDIRS		:= startup kernel
CLEAN_DIRS	:= $(addprefix _clean_, $(SUBDIRS))

CROSS_ := riscv64-linux-gnu-
LD := @$(CROSS_)ld
NM := @$(CROSS_)nm
OBJCOPY := @$(CROSS_)objcopy
OBJDUMP := @$(CROSS_)objdump
OBJCOPYFLAGS := -O binary -R .note -R .note.gnu.build-id -R .comment -S

LDFLAGS := -melf64lriscv --build-id=none --strip-debug

MAKE	:= @make --no-print-directory

PHONY += $(SUBDIRS)

all: $(SUBDIRS) startup.bin
	@cp ./startup.bin ../xemu/image/payload.bin
	@cp ./System.map ../xemu/image/System.map

$(SUBDIRS):
	@$(MAKE) -f ./scripts/Makefile.build obj=$@

startup.bin: startup.elf System.map
	@printf "LD\t$@\n"
	$(OBJCOPY) $(OBJCOPYFLAGS) $< $@

startup.elf: startup/startup.ko startup/module.lds
	@printf "LD\t$@\n"
	$(LD) $(LDFLAGS) -T ./startup/module.lds -o $@ $<

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
