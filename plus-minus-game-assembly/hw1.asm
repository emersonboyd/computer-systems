# Author: Emerson Boyd

.data # let processor know we will be submitting data to program now
game_board:
	.space 33 # make a 33 byte space in memory for a game board with address game_boad, one byte at the end for the null character
	
game_board_size:
	.word 33 # the size of the board to store - 33 bytes for a 32-byte game board with the null character at the end

ask_input:
	.asciiz "Please enter a game board up to size 32:\n"
	
newline_string:
	.asciiz "\n"
	
tell_output:
	.asciiz "\nString: "

winnable:
	.asciiz "Result: Winnable\n"

not_winnable:
	.asciiz "Result: Not Winnable\n"



.text
main:
	la $a0, ask_input
	li $v0, 4
	syscall

	# string = argv[1];
	la $a0, game_board # sets $a0 to point to the space allocated for writing a word
	lw $t0, game_board_size # get the size, in bytes, of the input we want
	move $a1, $t0 # gets the length of the space in $a1 so we can't go over the memory limit, including the null character at the end
	li $v0, 8
	syscall
	la $a0, newline_string
	li $v0, 4
	syscall
	
	# printf("String: %s\n", string);
	la $a0, tell_output
	li $v0, 4
	syscall
	la $a0, game_board
	li $v0, 4
	syscall
	la $a0, newline_string
	li $v0, 4
	syscall
	
	jal is_winnable
	
	# if (isWinnable(string));
	beqz $v0, print_not_winnable
	
# printf ("Result: Winnable\n");
print_winnable:
	la $a0, winnable
	li $v0, 4
	syscall
	j end

# printf ("Result: Not Winnable\n").
print_not_winnable:
	la $a0, not_winnable
	li $v0, 4
	syscall
	j end
	
end:
	li $v0, 10 # load the code to exit the program
	syscall	
		


strlen:
	move $t8, $zero # this is the value of the null character
	addi $t9, $zero, 10 # this is the value of the newline character
	
	move $t0, $zero # start with our i counter equal to zero
	
strlen_loop:
	la $t1, ($a0) # point t1 to the address of the string
	add $t1, $t1, $t0 # point t1 to the address of the current pointer in the string
	lb $t2, ($t1) # load the ascii character at the pointer
	
	beq $t2, $t8, end_strlen_loop # if we have reached one pointer before a the null character, we're done
	beq $t2, $t9, end_strlen_loop # if we have reached one pointer before a newline character, we're done
	
	addi $t0, $t0, 1 # we add one to the i counter
	j strlen_loop

end_strlen_loop:
	move $v0, $t0
	jr $ra



is_winnable:
	# update the stack pointer to be filled with the necessary variables (return address, save registers)
	subi $sp, $sp, 12
	sw $ra, 8($sp)
	sw $s4, 4($sp)
	sw $s0, ($sp)
	
	# int len = strlen(str);
	la $a0, game_board
	jal strlen
	move $s4, $v0
	
	addi $t8, $zero, 43 # this is the value of the + character
	addi $t9, $zero, 45 # this is the value of the - character
	
	# int result = 1;
	addi $v0, $zero, 0xffffffff
	
	# int i = 0; - the initialization of the for loop
	move $s0, $zero # start with our i counter equal to zero

loop:
	# i < len - 1; - the termination check of the foor loop
	subi $t5, $s4, 1
	bge $s0, $t5, end_loop
	
	la $t1, game_board # point t1 to the address of the string
	add $t1, $t1, $s0 # point t1 to the address of the current pointer in the string
	lb $t2, ($t1) # load the ascii character at the pointer
	lb $t3, 1($t1) # load the ascii character at one after the pointer
	
	# if (str[i] == '-' && str[i+1] == '-')
	sub $t2, $t9, $t2 # should be 0 if t3 was -
	sub $t3, $t9, $t3 # should be 0 if t3 was -
	or $t4, $t2, $t3 # this will be 0 if and only if both characters were - to start with
	beqz $t4, update_and_call
	
repeat_loop:
	# i++; - the incrementation of the for loop
	addi $s0, $s0, 1 # we add one to the i counter
	j loop
	
update_and_call:
	# str[i] = str[i+1] = '+';
	sb $t8, ($t1)
	sb $t8, 1($t1)
	
	# isWinnable(str); - recursive call
	jal is_winnable

	# result = !isWinnable(str);
	not $v0, $v0
	
	# str[i] = str[i+1] = '-';
	la $t1, game_board # point t1 to the address of the string
	add $t1, $t1, $s0 # point t1 to the address of the current pointer in the string
	sb $t9, ($t1)
	sb $t9, 1($t1)
	
	# if (result) { return 1; }
	bnez $v0, end_loop
	
	j repeat_loop

end_loop:
	# return result;
	lw $ra, 8($sp)
	lw $s4, 4($sp)
	lw $s0, ($sp)
	addi $sp, $sp, 12
	jr $ra
