cmd_/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o := /home/cybersyn/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/riscv64-unknown-linux-musl-gcc -Wp,-MMD,/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/.bash_exe.o.d -nostdinc -isystem /home/cybersyn/duo-buildroot-sdk-v2/host-tools/gcc/riscv64-linux-musl-x86_64/bin/../lib/gcc/riscv64-unknown-linux-musl/10.2.0/include -I/home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include -I./arch/riscv/include/generated -I/home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include -I./include -I/home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/uapi -I./arch/riscv/include/generated/uapi -I/home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi -I./include/generated/uapi -include /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/kconfig.h -include /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/compiler_types.h -D__KERNEL__ -fmacro-prefix-map=/home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/= -Wall -Wundef -Werror=strict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -fno-PIE -Werror=implicit-function-declaration -Werror=implicit-int -Werror=return-type -Wno-format-security -std=gnu89 -mabi=lp64 -march=rv64imac -Wa,-march=rv64imafdcv0p7 -mno-save-restore -DCONFIG_PAGE_OFFSET=0xffffffe000000000 -mcmodel=medany -mstrict-align -fno-delete-null-pointer-checks -Wno-frame-address -Wno-format-truncation -Wno-format-overflow -Wno-address-of-packed-member -Os -Wframe-larger-than=2048 -fstack-protector-strong -Wno-unused-but-set-variable -Wimplicit-fallthrough -Wno-unused-const-variable -fomit-frame-pointer -Wdeclaration-after-statement -Wvla -Wno-pointer-sign -Wno-stringop-truncation -Wno-zero-length-bounds -Wno-array-bounds -Wno-stringop-overflow -Wno-restrict -Wno-maybe-uninitialized -fno-strict-overflow -fno-stack-check -Werror=date-time -Werror=incompatible-pointer-types -Werror=designated-init -Wno-packed-not-aligned -I/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/include  -DMODULE -mno-relax  -DKBUILD_BASENAME='"bash_exe"' -DKBUILD_MODNAME='"vk36n16i"' -c -o /home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o /home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.c

source_/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o := /home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.c

deps_/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o := \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/kconfig.h \
    $(wildcard include/config/cc/version/text.h) \
    $(wildcard include/config/cpu/big/endian.h) \
    $(wildcard include/config/booger.h) \
    $(wildcard include/config/foo.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/compiler_types.h \
    $(wildcard include/config/have/arch/compiler/h.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/cc/has/asm/inline.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/compiler_attributes.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/compiler-gcc.h \
    $(wildcard include/config/retpoline.h) \
    $(wildcard include/config/arch/use/builtin/bswap.h) \
  /home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/include/bash_exe.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/umh.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/gfp.h \
    $(wildcard include/config/lockdep.h) \
    $(wildcard include/config/highmem.h) \
    $(wildcard include/config/zone/dma.h) \
    $(wildcard include/config/zone/dma32.h) \
    $(wildcard include/config/zone/device.h) \
    $(wildcard include/config/numa.h) \
    $(wildcard include/config/pm/sleep.h) \
    $(wildcard include/config/contig/alloc.h) \
    $(wildcard include/config/cma.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/mmdebug.h \
    $(wildcard include/config/debug/vm.h) \
    $(wildcard include/config/debug/virtual.h) \
    $(wildcard include/config/debug/vm/pgflags.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/bug.h \
    $(wildcard include/config/generic/bug.h) \
    $(wildcard include/config/bug/on/data/corruption.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/bug.h \
    $(wildcard include/config/generic/bug/relative/pointers.h) \
    $(wildcard include/config/debug/bugverbose.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/stack/validation.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/compiler_types.h \
  arch/riscv/include/generated/asm/rwonce.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/rwonce.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/kasan-checks.h \
    $(wildcard include/config/kasan.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/types.h \
    $(wildcard include/config/have/uid16.h) \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/arch/dma/addr/t/64bit.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/types.h \
  arch/riscv/include/generated/uapi/asm/types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/int-ll64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/int-ll64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/uapi/asm/bitsperlong.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitsperlong.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/bitsperlong.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/posix_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/stddef.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/stddef.h \
  arch/riscv/include/generated/uapi/asm/posix_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/posix_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/kcsan-checks.h \
    $(wildcard include/config/kcsan.h) \
    $(wildcard include/config/kcsan/ignore/atomics.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/const.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/const.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/const.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/asm.h \
    $(wildcard include/config/xip/kernel.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bug.h \
    $(wildcard include/config/bug.h) \
    $(wildcard include/config/smp.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/instrumentation.h \
    $(wildcard include/config/debug/entry.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/kernel.h \
    $(wildcard include/config/preempt/voluntary.h) \
    $(wildcard include/config/debug/atomic/sleep.h) \
    $(wildcard include/config/preempt/rt.h) \
    $(wildcard include/config/mmu.h) \
    $(wildcard include/config/prove/locking.h) \
    $(wildcard include/config/panic/timeout.h) \
    $(wildcard include/config/tracing.h) \
    $(wildcard include/config/ftrace/mcount/record.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/limits.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/limits.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/limits.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/linkage.h \
    $(wildcard include/config/arch/use/sym/annotations.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/stringify.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/export.h \
    $(wildcard include/config/modversions.h) \
    $(wildcard include/config/module/rel/crcs.h) \
    $(wildcard include/config/have/arch/prel32/relocations.h) \
    $(wildcard include/config/modules.h) \
    $(wildcard include/config/trim/unused/ksyms.h) \
    $(wildcard include/config/unused/symbols.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/linkage.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/bitops.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/bits.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/bits.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/build_bug.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/bitops.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/irqflags.h \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/irqsoff/tracer.h) \
    $(wildcard include/config/preempt/tracer.h) \
    $(wildcard include/config/trace/irqflags/support.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/typecheck.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/irqflags.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/processor.h \
    $(wildcard include/config/vector/emu.h) \
    $(wildcard include/config/compat.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/processor.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/vdso/processor.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/barrier.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/barrier.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/ptrace.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/uapi/asm/ptrace.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/csr.h \
    $(wildcard include/config/vector/1/0.h) \
    $(wildcard include/config/riscv/m/mode.h) \
  arch/riscv/include/generated/asm/percpu.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/percpu.h \
    $(wildcard include/config/debug/preempt.h) \
    $(wildcard include/config/have/setup/per/cpu/area.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/threads.h \
    $(wildcard include/config/nr/cpus.h) \
    $(wildcard include/config/base/small.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/percpu-defs.h \
    $(wildcard include/config/debug/force/weak/per/cpu.h) \
    $(wildcard include/config/amd/mem/encrypt.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/__ffs.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/ffz.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/fls.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/__fls.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/fls64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/find.h \
    $(wildcard include/config/generic/find/first/bit.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/sched.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/ffs.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/hweight.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/arch_hweight.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/const_hweight.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/non-atomic.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/le.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/uapi/asm/byteorder.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/byteorder/little_endian.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/byteorder/little_endian.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/swab.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/swab.h \
  arch/riscv/include/generated/uapi/asm/swab.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/swab.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/byteorder/generic.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/bitops/ext2-atomic.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/log2.h \
    $(wildcard include/config/arch/has/ilog2/u32.h) \
    $(wildcard include/config/arch/has/ilog2/u64.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/minmax.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/printk.h \
    $(wildcard include/config/message/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/default.h) \
    $(wildcard include/config/console/loglevel/quiet.h) \
    $(wildcard include/config/early/printk.h) \
    $(wildcard include/config/printk/nmi.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/dynamic/debug.h) \
    $(wildcard include/config/dynamic/debug/core.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/init.h \
    $(wildcard include/config/strict/kernel/rwx.h) \
    $(wildcard include/config/strict/module/rwx.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/kern_levels.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/cache.h \
    $(wildcard include/config/arch/has/cache/line/size.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/kernel.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/sysinfo.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/cache.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/ratelimit_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/param.h \
  arch/riscv/include/generated/uapi/asm/param.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/param.h \
    $(wildcard include/config/hz.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/param.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/spinlock_types.h \
    $(wildcard include/config/debug/spinlock.h) \
    $(wildcard include/config/debug/lock/alloc.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/spinlock_types_up.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/lockdep_types.h \
    $(wildcard include/config/prove/raw/lock/nesting.h) \
    $(wildcard include/config/preempt/lock.h) \
    $(wildcard include/config/lock/stat.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rwlock_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/dynamic_debug.h \
    $(wildcard include/config/jump/label.h) \
  arch/riscv/include/generated/asm/div64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/div64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/mmzone.h \
    $(wildcard include/config/force/max/zoneorder.h) \
    $(wildcard include/config/memory/isolation.h) \
    $(wildcard include/config/zsmalloc.h) \
    $(wildcard include/config/shadow/call/stack.h) \
    $(wildcard include/config/memcg.h) \
    $(wildcard include/config/sparsemem.h) \
    $(wildcard include/config/memory/hotplug.h) \
    $(wildcard include/config/compaction.h) \
    $(wildcard include/config/discontigmem.h) \
    $(wildcard include/config/transparent/hugepage.h) \
    $(wildcard include/config/flat/node/mem/map.h) \
    $(wildcard include/config/page/extension.h) \
    $(wildcard include/config/deferred/struct/page/init.h) \
    $(wildcard include/config/have/memoryless/nodes.h) \
    $(wildcard include/config/need/multiple/nodes.h) \
    $(wildcard include/config/flatmem.h) \
    $(wildcard include/config/sparsemem/vmemmap.h) \
    $(wildcard include/config/sparsemem/extreme.h) \
    $(wildcard include/config/memory/hotremove.h) \
    $(wildcard include/config/have/arch/pfn/valid.h) \
    $(wildcard include/config/holes/in/zone.h) \
    $(wildcard include/config/arch/has/holes/memorymodel.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/spinlock.h \
    $(wildcard include/config/preemption.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/preempt.h \
    $(wildcard include/config/preempt/count.h) \
    $(wildcard include/config/trace/preempt/toggle.h) \
    $(wildcard include/config/preempt/notifiers.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/list.h \
    $(wildcard include/config/debug/list.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/poison.h \
    $(wildcard include/config/illegal/pointer/value.h) \
    $(wildcard include/config/page/poisoning/zero.h) \
  arch/riscv/include/generated/asm/preempt.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/preempt.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/thread_info.h \
    $(wildcard include/config/thread/info/in/task.h) \
    $(wildcard include/config/have/arch/within/stack/frames.h) \
    $(wildcard include/config/hardened/usercopy.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/restart_block.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/time64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/math64.h \
    $(wildcard include/config/arch/supports/int128.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/math64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/time64.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/time.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/time_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/current.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/thread_info.h \
    $(wildcard include/config/set/fs.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/page.h \
    $(wildcard include/config/page/offset.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/pfn.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/memory_model.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/getorder.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/bottom_half.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/lockdep.h \
    $(wildcard include/config/debug/locking/api/selftests.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/smp.h \
    $(wildcard include/config/up/late/init.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/errno.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/errno.h \
  arch/riscv/include/generated/uapi/asm/errno.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/errno.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/asm-generic/errno-base.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/cpumask.h \
    $(wildcard include/config/cpumask/offstack.h) \
    $(wildcard include/config/hotplug/cpu.h) \
    $(wildcard include/config/debug/per/cpu/maps.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/bitmap.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/string.h \
    $(wildcard include/config/binary/printf.h) \
    $(wildcard include/config/fortify/source.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/string.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/string.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/atomic.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/atomic.h \
    $(wildcard include/config/generic/atomic64.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/cmpxchg.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/fence.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/atomic-fallback.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/atomic-long.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/smp_types.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/llist.h \
    $(wildcard include/config/arch/have/nmi/safe/cmpxchg.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/mmiowb.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/mmiowb.h \
    $(wildcard include/config/mmiowb.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/spinlock_up.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rwlock.h \
    $(wildcard include/config/preempt.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/spinlock_api_up.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/wait.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/wait.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/numa.h \
    $(wildcard include/config/nodes/shift.h) \
    $(wildcard include/config/numa/keep/meminfo.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/seqlock.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/mutex.h \
    $(wildcard include/config/mutex/spin/on/owner.h) \
    $(wildcard include/config/debug/mutexes.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/osq_lock.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/debug_locks.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/ww_mutex.h \
    $(wildcard include/config/debug/ww/mutex/slowpath.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/nodemask.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/pageblock-flags.h \
    $(wildcard include/config/hugetlb/page.h) \
    $(wildcard include/config/hugetlb/page/size/variable.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/page-flags-layout.h \
    $(wildcard include/config/numa/balancing.h) \
    $(wildcard include/config/kasan/sw/tags.h) \
  include/generated/bounds.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/mm_types.h \
    $(wildcard include/config/have/aligned/struct/page.h) \
    $(wildcard include/config/userfaultfd.h) \
    $(wildcard include/config/swap.h) \
    $(wildcard include/config/have/arch/compat/mmap/bases.h) \
    $(wildcard include/config/membarrier.h) \
    $(wildcard include/config/aio.h) \
    $(wildcard include/config/mmu/notifier.h) \
    $(wildcard include/config/arch/want/batched/unmap/tlb/flush.h) \
    $(wildcard include/config/iommu/support.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/mm_types_task.h \
    $(wildcard include/config/split/ptlock/cpus.h) \
    $(wildcard include/config/arch/enable/split/pmd/ptlock.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/auxvec.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/auxvec.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/uapi/asm/auxvec.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rbtree.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rcupdate.h \
    $(wildcard include/config/preempt/rcu.h) \
    $(wildcard include/config/tiny/rcu.h) \
    $(wildcard include/config/rcu/stall/common.h) \
    $(wildcard include/config/no/hz/full.h) \
    $(wildcard include/config/rcu/nocb/cpu.h) \
    $(wildcard include/config/tasks/rcu/generic.h) \
    $(wildcard include/config/tasks/rcu.h) \
    $(wildcard include/config/tasks/rcu/trace.h) \
    $(wildcard include/config/tasks/rude/rcu.h) \
    $(wildcard include/config/tree/rcu.h) \
    $(wildcard include/config/debug/objects/rcu/head.h) \
    $(wildcard include/config/prove/rcu.h) \
    $(wildcard include/config/rcu/boost.h) \
    $(wildcard include/config/arch/weak/release/acquire.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rcutree.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rwsem.h \
    $(wildcard include/config/rwsem/spin/on/owner.h) \
    $(wildcard include/config/debug/rwsems.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/err.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/completion.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/swait.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/uprobes.h \
    $(wildcard include/config/uprobes.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/workqueue.h \
    $(wildcard include/config/debug/objects/work.h) \
    $(wildcard include/config/freezer.h) \
    $(wildcard include/config/sysfs.h) \
    $(wildcard include/config/wq/watchdog.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/timer.h \
    $(wildcard include/config/debug/objects/timers.h) \
    $(wildcard include/config/no/hz/common.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/ktime.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/time.h \
    $(wildcard include/config/arch/uses/gettimeoffset.h) \
    $(wildcard include/config/posix/timers.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/time32.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/timex.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/timex.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/timex.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/time32.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/time.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/jiffies.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/jiffies.h \
  include/generated/timeconst.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/vdso/ktime.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/timekeeping.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/timekeeping32.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/debugobjects.h \
    $(wildcard include/config/debug/objects.h) \
    $(wildcard include/config/debug/objects/free.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/arch/riscv/include/asm/mmu.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/page-flags.h \
    $(wildcard include/config/arch/uses/pg/uncached.h) \
    $(wildcard include/config/memory/failure.h) \
    $(wildcard include/config/idle/page/tracking.h) \
    $(wildcard include/config/thp/swap.h) \
    $(wildcard include/config/ksm.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/memory_hotplug.h \
    $(wildcard include/config/arch/has/add/pages.h) \
    $(wildcard include/config/have/arch/nodedata/extension.h) \
    $(wildcard include/config/have/bootmem/info/node.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/notifier.h \
    $(wildcard include/config/tree/srcu.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/srcu.h \
    $(wildcard include/config/tiny/srcu.h) \
    $(wildcard include/config/srcu.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rcu_segcblist.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/srcutree.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/rcu_node_tree.h \
    $(wildcard include/config/rcu/fanout.h) \
    $(wildcard include/config/rcu/fanout/leaf.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/topology.h \
    $(wildcard include/config/use/percpu/numa/node/id.h) \
    $(wildcard include/config/sched/smt.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/arch_topology.h \
    $(wildcard include/config/generic/arch/topology.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/percpu.h \
    $(wildcard include/config/need/per/cpu/embed/first/chunk.h) \
    $(wildcard include/config/need/per/cpu/page/first/chunk.h) \
  arch/riscv/include/generated/asm/topology.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/asm-generic/topology.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/sysctl.h \
    $(wildcard include/config/sysctl.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/uidgid.h \
    $(wildcard include/config/multiuser.h) \
    $(wildcard include/config/user/ns.h) \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/linux/highuid.h \
  /home/cybersyn/duo-buildroot-sdk-v2/linux_5.10/include/uapi/linux/sysctl.h \

/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o: $(deps_/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o)

$(deps_/home/cybersyn/Learn_MilkV_DUOS/my-duos-project/kernelmode/rebuild/vk36n16i_re/src/bash_exe.o):
