all:
	gcc -O2 -lSDL2 main.c -o chip8
test:
	gcc -O2 -lSDL2 main.c -o chip8
	./chip8 roms/test_opcode.ch8
