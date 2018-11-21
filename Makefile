CC = mipsel-linux-gcc

all: clean createimage image asm # floppy

SRC_BOOT 	= $(wildcard ./arch/mips/boot/*.S)

SRC_ARCH	= $(wildcard ./arch/mips/kernel/*.S ./arch/mips/pmon/*.c )
SRC_DRIVER	= $(wildcard ./drivers/*.c)
SRC_INIT 	= $(wildcard ./init/*.c)
SRC_INT		= $(wildcard ./kernel/irq/*.c)
SRC_LOCK	= $(wildcard ./kernel/locking/*.c)
SRC_MM    = $(wildcard ./kernel/mm/*.c)
SRC_SCHED	= $(wildcard ./kernel/sched/*.c)
SRC_SYSCALL	= $(wildcard ./kernel/syscall/*.c)
SRC_LIBS	= $(wildcard ./libs/*.c)

SRC_TEST	= $(wildcard ./test/*.c ./test/test_project4/*.c)

SRC_IMAGE	= ./tools/createimage.c

bootblock: $(SRC_BOOT)
	${CC} -G 0 -O2 -fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800000 -N -o bootblock $(SRC_BOOT) -nostdlib -e main -Wl,-m -Wl,elf32ltsmip -T ld.script

main : $(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_INT) $(SRC_LOCK) $(SRC_SCHED) $(SRC_SYSCALL) $(SRC_LIBS) $(SRC_TEST)
	${CC} -G 0 -O0 -Iinclude -Ilibs -Iarch/mips/include -Idrivers -Iinclude/os -Iinclude/sys -Itest -Itest/test_project2 -Itest/test_project3 \
	-fno-pic -mno-abicalls -fno-builtin -nostdinc -mips3 -Ttext=0xffffffffa0800200 -N -o main -g \
	$(SRC_ARCH) $(SRC_DRIVER) $(SRC_INIT) $(SRC_INT) $(SRC_LOCK) $(SRC_SCHED) $(SRC_SYSCALL) $(SRC_PROC) $(SRC_LIBS) $(SRC_TEST) -nostdlib -Wl,-m -Wl,elf32ltsmip -T ld.script		

createimage: $(SRC_IMAGE)
	gcc $(SRC_IMAGE) -o createimage

image: bootblock main
	./createimage --extended bootblock main

clean:
	rm -rf bootblock image createimage main *.o

floppy:
	sudo fdisk -l /dev/sdc
	sudo dd if=image of=/dev/sdc conv=notrunc

asm:
	mipsel-linux-objdump -d main > kernel.txt
