#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <time.h>
#include <stdlib.h>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#endif

void render(int screenwidth, int screenheight, uint8_t disp[][screenheight]);

uint8_t memory[4096]; // 4KB, packed per 8 bits
uint8_t display[SCREEN_WIDTH][SCREEN_HEIGHT]; // Could possibly be optimized by dividing by 8 and doing it bitwise.
uint16_t programcounter = 0x200; // Programs start at hexadecimal 200, dec 512.
uint16_t indexreg = 0;
uint8_t delaytimer = 0;
uint8_t soundtimer = 0;
uint8_t registers[16]; // General use registers
uint8_t keys[16]; // All keys on the keypad. Get set by SDL if key is pressed.

uint16_t stack[16]; // Used for subroutines
uint8_t stackp = 0; // Pointer to last/top item in stack.

// Helper function: Push on subroutine stack
void push(uint16_t val)
{
    ++stackp;
    if(stackp == 16) puts("STACK OVERFLOW");
    stack[stackp] = val;
}

// Helper function: Pop from subroutine stack
uint16_t pop()
{
    --stackp;
    if(stackp < 0) puts("STACK UNDERFLOW");
    return stack[stackp+1];
}

// 00E0: Clear screen
void cls()
{
    for(int x = 0; x < SCREEN_WIDTH; x++)
        for(int y = 0; y < SCREEN_HEIGHT; y++)
            display[x][y] = 0;
    render(SCREEN_WIDTH, SCREEN_HEIGHT, display);
}

// 00EE: Pop subroutine from stack
void ret()
{
    programcounter = pop();
}

// 1NNN: Jump to index, e.g. set programcounter to index
void jump(uint16_t index)
{
    programcounter = index;
}

// 2NNN: Run subroutine
void call(uint16_t index)
{
    push(programcounter);
    programcounter = index;
}

// 3XNN: Jump one instruction if value of registers[reg] == val
void jumpeq(uint8_t reg, uint8_t val)
{
    if(registers[reg] == val)
        programcounter += 2;
}

// 4XNN: Jump one instruction if value of registers[reg] != val
void jumpnq(uint8_t reg, uint8_t val)
{
    if(registers[reg] != val)
        programcounter += 2;
}

// 5XY0: Jump one instruction if value of registers[x] == registers[y]
void jumpreq(uint8_t x, uint8_t y)
{
    if(registers[x] == registers[y])
        programcounter += 2;
}

// 6XNN: Set register X to value NN
void setreg(uint8_t reg, uint8_t val)
{
    registers[reg] = val;
}

// 7XNN: Add NN to the value of registers[x]
void addreg(uint8_t reg, uint8_t val)
{
    registers[reg] += val;
}

// 8XY0: Set register X to the value of register Y
void bwset(uint8_t x, uint8_t y)
{
    registers[x] = registers[y];
}

// 8XY1: Bitwise OR
void bwor(uint8_t x, uint8_t y)
{
    registers[x] = registers[x] | registers[y];
}

// 8XY2: Bitwise AND
void bwand(uint8_t x, uint8_t y)
{
    registers[x] = registers[x] & registers[y];
}

// 8XY3: Bitwise XOR
void bwxor(uint8_t x, uint8_t y)
{
    registers[x] = registers[x] ^ registers[y];
}

// 8XY4: Add register y to register x. Set register 0xF to 1 if register x overflows.
void addcheck(uint8_t x, uint8_t y)
{
    uint16_t c = (uint16_t)registers[x] + registers[y];
    if(c > 255) registers[0xF] = 1;
    else registers[0xF] = 0;
    registers[x] = c;
}

// 8XY5: reg x - reg y, set 0xF if X > Y, otherwise unset 0xF.
void sub(uint8_t x, uint8_t y)
{
    if(registers[x] > registers[y])
        registers[0xF] = 1;
    else
        registers[0xF] = 0;

    registers[x] = registers[x] - registers[y];
}

// 8XY6: Shift bits one to the right. set reg 0xF to the bit that's shifted out
void shiftr(uint8_t x, uint8_t y)
{
    registers[x] = registers[y];
    registers[0xF] = (uint8_t)(registers[x] % 2); // Set VF to 1 if the bit that was shifted out was 1, or 0 if it was 0
    registers[x] = registers[x] >> 1;
}

// 8XY7: reg y - reg x, set 0xF if X > Y, otherwise unset 0xF.
void subrev(uint8_t x, uint8_t y)
{
    if(registers[y] > registers[x]) registers[0xF] = 1;
    else registers[0xF] = 0;

    registers[x] = registers[y] - registers[x];
}

// 8XYE: Shift bits one to the left. set reg 0xF to the bit that's shifted out
void shiftl(uint8_t x, uint8_t y)
{
    registers[x] = registers[y];
    registers[0xF] = (uint8_t)((registers[x] >> 7) % 2); // Set VF to 1 if the bit that was shifted out was 1, or 0 if it was 0
    registers[x] = registers[x] << 1;
}

// 9XY0: Jump one instruction if value of registers[x] != registers[y]
void jumprnq(uint8_t x, uint8_t y)
{
    if(registers[x] != registers[y])
        programcounter += 2;
}

// ANNN: Set index register to NNN
void setindex(uint16_t val)
{
    indexreg = val;
}

// BNNN: Jump to (NNN + reg 0)
void jumpoffset(uint16_t index)
{
    jump(index + registers[0]);
}

// CXNN: Get a random number, awesome! Put it in register x.
void randval(uint8_t x, uint8_t val)
{
    registers[x] = rand() & val;
}

// DXYN: Draw stuff. Look at spec for detailed info.
void draw(uint8_t x, uint8_t y, uint8_t height)
{
    // Value of register y. If greater than the screenheight, 'overflow' the screen.
    const uint8_t vy = registers[y] % SCREEN_HEIGHT;
    // Value of register x. If greater than the screenwidth, 'overflow' the screen.
    const uint8_t vx = registers[x] % SCREEN_WIDTH;

    // Reset the F register, as per hardware definition.
    registers[0xF] = 0;

    // Loop through n, as per hardware definition.
    for (int row = 0; row < height; ++row)
    {
        // Get row of bytes to print to the screen.
        const uint8_t spriteRow = memory[indexreg + row];

        for(uint8_t i = 0; i < 8; i++)
        {
            uint8_t spritePixel = (spriteRow >> (7-i)) % 2; // Get the pixelbit 'i' in row.

            if(spritePixel)
            {
                if (display[vx+i][vy+row])
                    registers[0xF] = 1;

                display[vx+i][vy+row] ^= 1;
            }
            //if(vx+i >= SCREEN_WIDTH) break; // Prevent going past screenlimits
        }
        //if(vy+row >= SCREEN_HEIGHT) break; // Prevent going past screenlimits
    }

    render(SCREEN_WIDTH, SCREEN_HEIGHT, display);
}

// EX9E: Skip an instruction if key x is being pressed.
void jmpress(uint8_t x)
{
    if(keys[x]) programcounter += 2;
}

// EX9E: Skip an instruction if key x is NOT being pressed.
void jmnpress(uint8_t x)
{
    if(!keys[x]) programcounter += 2;
}

// FX0A: Wait till a key is pressed.
void waitkey(uint8_t x)
{
    for(int i = 0; i < 16; i++)
        if(keys[i] == 1)
        {
            registers[x] = i;
            return;
        }

    programcounter -= 2;
}

// FX1E: Add value of reg x to the index register
void addindex(uint8_t x)
{
    indexreg += registers[x];
}

// FX33: Turn the value of reg x into a decimal.
// Put each digit seperately into index, index+1 and index+2.
void toDec(uint8_t x)
{
    memory[indexreg] = (registers[x]/100) % 10;
    memory[indexreg + 1] = (registers[x]/10) % 10;
    memory[indexreg + 2] = registers[x] % 10;
}

// FX55: Store registers to memory
void store(uint8_t x)
{
    for(int i = 0; i <= x; i++)
        memory[indexreg + i] = registers[i];
}

// FX65: Load registers from memory
void load(uint8_t x)
{
    for(int i = 0; i <=x; i++)
        registers[i] = memory[indexreg + i];
}

// Decode ops into actionable functions.
void decode(uint16_t op)
{
    //printf("Got op %x\n", op);

    // Deconstruct instruction.
    const uint8_t i = op >> 12; // first nibble, instruction
    const uint8_t x = (op >> 8) - ((uint16_t)i << 4); // second nibble, register
    const uint8_t y = (op >> 4) - ((uint16_t)i << 8) - ((uint16_t)x << 4); // third nibble, register
    const uint8_t n = op - ((uint16_t)i << 12) - ((uint16_t)x << 8) - ((uint16_t)y << 4); //fourth nibble, variable
    const uint8_t nn = op - ((uint16_t)i << 12) - ((uint16_t)x << 8); // variable
    const uint16_t nnn = op - ((uint16_t)i << 12); // variable

    printf(" Running op %x %x %x %x\n", i, x, y, n);

    switch(i)
    {
    case(0x0):
        if(nnn == 0x0E0) cls();
        else if(nnn == 0x0EE) ret();
        else puts("This instruction is not implemented, since we're not running on a COSMAC or DREAM computer!");
        break;
    case(0x1):
        jump(nnn);
        break;
    case(0x2):
        call(nnn);
    case(0x3):
        jumpeq(x,nn);
        break;
    case(0x4):
        jumpnq(x,nn);
        break;
    case(0x5):
        jumpreq(x,y);
        break;
    case(0x6):
        setreg(x,nn);
        break;
    case(0x7):
        addreg(x,nn);
        break;
    case(0x8):
        switch(n)
        {
        case(0):
            bwset(x,y);
            break;
        case(1):
            bwor(x,y);
            break;
        case(2):
            bwand(x,y);
            break;
        case(3):
            bwxor(x,y);
            break;
        case(4):
            addcheck(x,y);
            break;
        case(5):
            sub(x,y);
            break;
        case(6):
            shiftr(x,y);
            break;
        case(7):
            subrev(x,y);
            break;
        case(0xE):
            shiftl(x,y);
            break;
        }
        break;
    case(0x9):
        jumprnq(x,y);
        break;
    case(0xA):
        setindex(nnn);
        break;
    case(0xB):
        jumpoffset(nnn);
        break;
    case(0xC):
        randval(x, nn);
        break;
    case(0xD):
        draw(x, y, n);
        break;
    case(0xE):
        if(nn == 0x9e) jmpress(x);
        if(nn == 0xA1) jmnpress(x);
        break;
    case(0xF):
        switch(nn)
        {
        case(0x07):
            registers[x] = delaytimer;
            break;
        case(0x15):
            delaytimer = registers[x];
            break;
        case(0x18):
            soundtimer = registers[x];
            break;
        case(0x1E):
            addindex(x);
            //printf("Not implemented: %x %x %x %x\n", i,x,y,n);
            break;
        case(0x0A):
            waitkey(x);
            //printf("Not tested: %x %x %x %x\n", i,x,y,n);
            break;
        case(0x29):
            indexreg = 0 + (5*registers[x]); // Set index to location of char
            break;
        case(0x33):
            toDec(x);
            break;
        case(0x55):
            store(x);
            break;
        case(0x65):
            load(x);
            break;
        }
        break;
    default:
        printf("Not implemented: %x %x %x %x\n", i,x,y,n);
        break;
    }
}

// Return current instruction to be ran. Also, shift programcounter to next instruction.
uint16_t fetch()
{
    const uint8_t a = memory[programcounter];
    const uint8_t b = memory[programcounter + 1];

    uint16_t op = (a << 8) + b; // Morph 2 bytes into one 16-bit instruction

    programcounter += 2; // Set programcounter to next op.
    return op;
}

// Decrement timers by one. Should happen roughly 60 times per second.
void cpuDecTimers()
{
    if (delaytimer > 0) --delaytimer;
    if (soundtimer > 0) --soundtimer;
}

// Emulate one 'tick' of the CPU.
void cpu()
{
    decode(fetch());
}
