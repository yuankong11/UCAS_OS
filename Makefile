CC = mipsel-linux-gcc

all: clean createimage image asm qemu #floppy

SRC_BOOT 	  = ./arch/mips/boot/bootblock.S

SRC_ARCH	  = ./arch/mips/kernel/asm.S ./arch/mips/lock/lock.S ./arch/mips/pmon/common.c
SRC_DRIVER	  = ./drivers/screen.c ./drivers/mac.c
SRC_INIT 	  = ./init/main.c
SRC_SOFTWARE  = ./kernel/software/shell.c ./kernel/software/io_proc.c ./kernel/software/run.c
SRC_LOCK	  = ./kernel/lock/lock.c ./kernel/lock/barrier.c ./kernel/lock/cond.c ./kernel/lock/sem.c
SRC_MEMORY    = ./kernel/mm/memory.c
SRC_SCHED	  = ./kernel/sched/sched.c ./kernel/sched/queue.c ./kernel/sched/time.c
SRC_EXCEPTION = ./kernel/exception/irq.c ./kernel/exception/syscall.c ./kernel/exception/tlb.c
SRC_FS        = ./kernel/fs/fs.c
SRC_LIBS	  = ./libs/string.c ./libs/printk.c ./libs/debug.c ./libs/mailbox.c ./libs/slist.c
SRC_TEST	  = ./test/test.c ./test/test_fs/test_fs.c ./test/test_bonus/test_bonus.c

SRC_IMAGE	  = ./tools/createimage.c

SRC_KERNEL = $(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_SOFTWARE) $(SRC_LOCK) $(SRC_MEMORY) $(SRC_SCHED) $(SRC_EXCEPTION) $(SRC_FS) $(SRC_LIBS) $(SRC_TEST)

INCLUDE = -Iinclude/exception -Iinclude/fs -Iinclude/mm -Iinclude/sched -Iinclude/software -Iinclude/sync -Ilibs -Iarch/mips/include -Idrivers -Itest -Itest/test_fs -Itest/test_bonus

bootblock: $(SRC_BOOT)
	@${CC} -g -G 0 -O0 -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800000 -N -o bootblock $(SRC_BOOT) -nostdlib -e main -Wl,-m -Wl,elf32ltsmip -T ld.script

main : $(SRC_KERNEL)
	@${CC} -g -G 0 -O0 $(INCLUDE) -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800000 -N -o main $(SRC_KERNEL) -nostdlib -Wl,-m -Wl,elf32ltsmip -T ld.script

createimage: $(SRC_IMAGE)
	@gcc $(SRC_IMAGE) -o createimage

image: bootblock main
	./createimage --extended bootblock main

clean:
	rm -rf bootblock image createimage main kernel.txt print_image *.o bonus_out pkt.txt convert

floppy:
	sudo fdisk -l /dev/sdb
	sudo dd if=image of=/dev/sdb conv=notrunc

asm:
	@mipsel-linux-objdump -D main > kernel.txt

qemu: image ~/QEMULoongson/disk
	dd if=image of=/home/yzl/QEMULoongson/disk conv=notrunc

print_image: ./tools/print_image.c
	gcc -o print_image ./tools/print_image.c

SRC_OUT = ./test/test_bonus/task_bonus_out.c
bonus_out: $(SRC_OUT)
	@${CC} -G 0 -O0 -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0x0 -N -o bonus_out $(SRC_OUT) -nostdlib -Wl,-m -Wl,elf32ltsmip -T ld.script

pkt: bonus_out ./tools/convert.c
	@gcc -o convert ./tools/convert.c
	@./convert