# SPDX-License-Identifier: GPL-2.0

include scripts/Makefile.include

PHONY := all clean

SUBDIRS		:= startup lib early_dt \
	rbtree \
	mm memblock buddy slab kalloc \
	vma ioremap devres \
	of platform kobject \
	dcache fs ramfs rootfs \
	block genhd \
	virtio virtio_mmio virtio_blk \
	init

CLEAN_DIRS := $(addprefix _clean_, $(SUBDIRS))

PHONY += $(SUBDIRS)

all: $(SUBDIRS) startup/startup.bin
	@cp ./startup/System.map ../xemu/image/
	@cp ./startup/startup.bin ../xemu/image/

$(SUBDIRS):
	@$(MAKE) -f ./scripts/Makefile.build obj=$@
	$(if $(filter-out startup, $@), @cp ./$@/*.ko ../xemu/image/)

PHONY += $(CLEAN_DIRS)

$(CLEAN_DIRS):
	@$(MAKE) -f ./scripts/Makefile.clean obj=$@

clean: $(CLEAN_DIRS)

dump:
	$(OBJDUMP) -D -m riscv:rv64 -EL -b binary ./startup/startup.bin

.PHONY: $(PHONY)
