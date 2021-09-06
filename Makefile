# SPDX-License-Identifier: GPL-2.0

include scripts/Makefile.include

PHONY := all clean

SUBDIRS		:= startup kernel
CLEAN_DIRS	:= $(addprefix _clean_, $(SUBDIRS))

PHONY += $(SUBDIRS)

all: $(SUBDIRS) startup/startup.bin
	@cp ./startup/System.map ../xemu/image/System.map
	@cp ./startup/startup.bin ../xemu/image/startup.bin
	@cp ./kernel/kernel.ko ../xemu/image/kernel.ko

$(SUBDIRS):
	@$(MAKE) -f ./scripts/Makefile.build obj=$@

PHONY += $(CLEAN_DIRS)

$(CLEAN_DIRS):
	@$(MAKE) -f ./scripts/Makefile.clean obj=$@

clean: $(CLEAN_DIRS)

dump:
	$(OBJDUMP) -D -m riscv:rv64 -EL -b binary ./startup/startup.bin

.PHONY: $(PHONY)
