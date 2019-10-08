.text
.global main

main:
	# 1) call BIOS print string
    la   $a0, msg # address of the string
    lw   $t0, printstr
    jalr $t0

	# 2) call BIOS read kernel in SD card and jump to kernel
    lw    $a0, bootloader  # memory address
    li    $a1, 0x200       # offset
                           # the 1-st sector is bootloader
    lui   $a2, 0           # size_high
    ori   $a2, 0           # size_low
                           # using two instructions instead of li
                           # to make sure of there're two instructions in file generated
    lw    $t0, read_sd_card
    lw    $ra, bootloader  # put kernel on the same memory space of bootloader
                           # jump here to start kernel
    jr    $t0
    # instructions after here won't be done
    # kernel shouldn't return

.data

msg: .ascii "Loading kernel...\n"

# 1. PMON read SD card function address
read_sd_card: .word 0x8007b1cc

# 2. PMON print string function address
printstr: .word 0x8007b980

# 3. bootloader address
# BIOS jumps here to start bootloader
# bootloader jumps here to start kernel
bootloader : .word 0xa0800000