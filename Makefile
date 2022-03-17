# SPDX-License-Identifier: GPL-2.0

include scripts/Makefile.include

PHONY := all clean

PREDIRS := prebuilt

SUBDIRS := startup lib early_dt \
	rbtree radix_tree hashtable bitmap xarray scatterlist \
	mm pgalloc gup memblock buddy slab kalloc filemap \
	vma ioremap devres mempool \
	of of_irq platform kobject \
	dcache fs ramfs rootfs ext2 \
	irq softirq intc plic \
	block genhd bio iov_iter readahead backing-dev \
	virtio virtio_mmio virtio_blk \
	workqueue \
	fork \
	sched \
	sys \
	init

CLEAN_DIRS := $(addprefix _clean_, $(SUBDIRS))

PHONY += $(SUBDIRS)
PHONY += $(PREDIRS)

all: $(SUBDIRS) startup/startup.bin
	@cp ./startup/System.map ../xemu/image/
	@cp ./startup/startup.bin ../xemu/image/

$(PREDIRS):
	@$(MAKE) -f ./scripts/Makefile.build obj=$@

$(SUBDIRS): $(PREDIRS)
	@$(MAKE) -f ./scripts/Makefile.build obj=$@
	$(if $(filter-out startup, $@), @cp ./$@/*.ko ../xemu/image/)

PHONY += $(CLEAN_DIRS)

$(CLEAN_DIRS):
	@$(MAKE) -f ./scripts/Makefile.clean obj=$@

clean: $(CLEAN_DIRS)
	@rm -f ./prebuilt/*.h ./prebuilt/*.s

dump:
	$(OBJDUMP) -D -m riscv:rv64 -EL -b binary ./startup/startup.bin

.PHONY: $(PHONY)
