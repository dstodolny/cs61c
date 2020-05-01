.data
shouldben1:	.asciiz "Should be -1, and firstposshift and firstposmask returned: "
shouldbe0:	.asciiz "Should be 0 , and firstposshift and firstposmask returned: "
shouldbe16:	.asciiz "Should be 16, and firstposshift and firstposmask returned: "
shouldbe31:	.asciiz "Should be 31, and firstposshift and firstposmask returned: "

.text
main:
        la	$a0, shouldbe31
        jal	print_str
        lui	$a0, 0x8000	# should be 31
        jal	first1posshift
        move	$a0, $v0
        jal	print_int
        jal	print_space

        lui	$a0, 0x8000
        jal	first1posmask
        move	$a0, $v0
        jal	print_int
        jal	print_newline

        la	$a0, shouldbe16
        jal	print_str
        lui	$a0, 0x0001	# should be 16
        jal	first1posshift
        move	$a0, $v0
        jal	print_int
        jal	print_space

        lui	$a0, 0x0001
        jal	first1posmask
        move	$a0, $v0
        jal	print_int
        jal	print_newline

        la	$a0, shouldbe0
        jal	print_str
        li	$a0, 1		# should be 0
        jal	first1posshift
        move	$a0, $v0
        jal	print_int
        jal	print_space

        li	$a0, 1
        jal	first1posmask
        move	$a0, $v0
        jal	print_int
        jal	print_newline

        la	$a0, shouldben1
        jal	print_str
        move	$a0, $0		# should be -1
        jal	first1posshift
        move	$a0, $v0
        jal	print_int
        jal	print_space

        move	$a0, $0
        jal	first1posmask
        move	$a0, $v0
        jal	print_int
        jal	print_newline

        li	$v0, 10
        syscall

first1posshift:
        addi    $v0, $0, -1
        beq     $a0, $0, return_shift

        addi    $v0, $v0, 1
        addiu   $t0, $0, 1
loop_shift:
        beq     $a0, $t0, return_shift
        addiu   $v0, $v0, 1
        srl     $a0, $a0, 1
        j       loop_shift
return_shift:
        jr      $ra


first1posmask:
        addi    $v0, $0, 31
        li      $t0, 0x80000000
        beq     $a0, $0, return_mask_0
loop_mask:
        and     $t1, $t0, $a0
        bne     $t1, $0, return_mask
        addi    $v0, $v0, -1
        srl     $t0, $t0, 1
        j       loop_mask
return_mask_0:
        addi    $v0, $0, -1
return_mask:
        jr      $ra

print_int:
        move	$a0, $v0
        li	$v0, 1
        syscall
        jr	$ra

print_str:
        li	$v0, 4
        syscall
        jr	$ra

print_space:
        li	$a0, ' '
        li	$v0, 11
        syscall
        jr	$ra

print_newline:
        li	$a0, '\n'
        li	$v0, 11
        syscall
        jr	$ra
