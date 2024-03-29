.text
.global main

main:
	# 1) task1 call BIOS print string "It's bootblock!"
  la $a0, msg
  lw $t0, printstr
  jalr $t0
	
	# 2) task2 call BIOS read kernel in SD card and jump to kernel start
  lw $a0, kernel
  li $a1, 0x200
  lw $a2, os_size
  lhu $a2, ($a2)
  sll $a2, 9
  lw $ra, kernel_main
  lw $t0, read_sd_card
  j $t0

# while(1) --> stop here
stop:
	j stop

.data

msg: .ascii "Loading kernel...\n"

os_size: .word 0xa08001fc

# 1. PMON read SD card function address
# read_sd_card();
read_sd_card: .word 0x8007b1cc

# 2. PMON print string function address
# printstr(char *string)
printstr: .word 0x8007b980

# 3. PMON print char function address
# printch(char ch)
printch: .word 0x8007ba00

# 4. kernel address (move kernel to here ~)
kernel : .word 0xa0800200

# 5. kernel main address (jmp here to start kernel main!)
kernel_main : .word 0xa0800200
