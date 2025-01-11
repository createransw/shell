[![Build Status](https://github.com/createransw/shell/actions/workflows/ci.yml/badge.svg)](https://github.com/createransw/shell/actions/workflows/ci.yml)
Shell. 
BNF:
	<order>::= log; order[&,;] ! log& order[&,;] ! log[&,;] 
	<log>::= conv|| log ! conv&& log ! conv <conv>::= cmd| conv ! 
	cmd <cmd>::= name ! name>file ! name>>file ! name<file ! 
	name>file1<file2
		! name>>file1<file2 ! name<file1>file2 ! 
		! name<file1>>file2
	<name>::= (order) ! prog

	prog - name of command with arguments 
	closing ; or & in (...) is not allowed

Program my_shell executes commands set by following BNF. Program contains
	7 modules :
	
	my_shell.c
		Only main() function. Launches code from other modules.

	run.c
		Contains main warking loop and code for executing command
		line seved in tree. Executes function, that bilds the tree,
		then executes command from it. Also cantains prebuild
		commands cd and exit.

	run.h
		Function headers and variables from run.c. Makes my_shell.c
		able to run starting function.

	tree.c
		Builds command tree from list of word. Asks list functions
		for list, untill it returns NULL. Also contains some service
		functions and checks syntax.

	tree.h
		Function headers and variables from tree.c. Makes run.c
		able to get new list and to dispose old one.

	list.c
		Reads line from input and splits it to words. Devides usual
		words from special. Processes ", ' and #(comment). Replaces
		$HOME, $USER, $SHELL and $EUID with relevant values.

	list.h
		Service functions for list.c. Realisation of creating,
		destroying and expending list.
