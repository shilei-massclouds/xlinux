# SPDX-License-Identifier: GPL-2.0

include scripts/Makefile.include

PHONY := all clean

SUBDIRS := startup lib early_dt \
	rbtree radix_tree hashtable bitmap xarray scatterlist \
	mm pgalloc gup memblock buddy slab kalloc filemap \
	vma ioremap devres \
	of of_irq platform kobject \
	dcache fs ramfs rootfs ext2 \
	irq softirq intc plic \
	block genhd bio iov_iter readahead backing-dev \
	virtio virtio_mmio virtio_blk \
	workqueue \
	fork \
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
