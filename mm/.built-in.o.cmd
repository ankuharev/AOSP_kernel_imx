cmd_mm/built-in.o :=  arm-eabi-ld -EL    -r -o mm/built-in.o mm/filemap.o mm/mempool.o mm/oom_kill.o mm/fadvise.o mm/maccess.o mm/page_alloc.o mm/page-writeback.o mm/readahead.o mm/swap.o mm/truncate.o mm/vmscan.o mm/shmem.o mm/util.o mm/mmzone.o mm/vmstat.o mm/backing-dev.o mm/mm_init.o mm/mmu_context.o mm/percpu.o mm/slab_common.o mm/compaction.o mm/balloon_compaction.o mm/interval_tree.o mm/fremap.o mm/highmem.o mm/madvise.o mm/memory.o mm/mincore.o mm/mlock.o mm/mmap.o mm/mprotect.o mm/mremap.o mm/msync.o mm/rmap.o mm/vmalloc.o mm/pagewalk.o mm/pgtable-generic.o mm/process_vm_access.o mm/init-mm.o mm/bootmem.o mm/memblock.o mm/bounce.o mm/page_io.o mm/swap_state.o mm/swapfile.o mm/dmapool.o mm/ksm.o mm/slub.o mm/migrate.o mm/page_isolation.o 
