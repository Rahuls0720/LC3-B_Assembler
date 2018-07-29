compile: assembler.c
	gcc -ansi assembler.c -lm
	./a.out source.asm output.obj

debug: assembler.c
	gcc -g -ansi assembler.c -lm
	gdbtui -q ./a.out
