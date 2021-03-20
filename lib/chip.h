#include <stdint.h>
#include <stdio.h>

uint8_t memory[4096]; // 4KB, packed per 8 bits
uint8_t display[SCREEN_WIDTH][SCREEN_HEIGHT]; // Could possibly be optimized by dividing by 8 and doing it bitwise.
uint16_t programcounter = 0x200; // Programs start at hexadecimal 200, dec 512.
uint16_t indexreg = 0;
//uint16_t stack[16];
//uint8_t delaytimer = 0;
//uint8_t soundtimer = 0;
uint8_t registers[16];

// Clear screen
void cls()
{
    memset(display, 0, sizeof(display));
}

// Jump to index, e.g. set programcounter to index
void jump(uint16_t index)
{
    programcounter = index;
}

void setreg(uint8_t reg, uint8_t val)
{
    registers[reg] = val;
}

void addreg(uint8_t reg, uint8_t val)
{
    registers[reg] += val;
}

void setindex(uint16_t index)
{
    indexreg = index;
}

void draw(uint8_t x, uint8_t y, uint8_t n)
{
    // Value of register y. If greater than the screenheight, 'overflow' the screen.
    uint8_t vy = registers[y] % SCREEN_HEIGHT;
    // Reset the F register, as per hardware definition.
    registers[0xF] = 0;

    // Loop through n, as per hardware definition.
    for(; n > 0; n--)
    {
        // Value of register x. If greater than the screenwidth, 'overflow' the screen.
        uint8_t vx = registers[x] % SCREEN_WIDTH;

        // Get row of bytes to print to the screen.
        const uint8_t spriteRow = memory[indexreg];
        // Too lazy to turn this into a function.
        // Turn the spriteRow into an array of 'bits'.
        uint8_t spriteArray[8] = {
            (spriteRow >> 7) % 2,
            (spriteRow >> 6) % 2,
            (spriteRow >> 5) % 2,
            (spriteRow >> 4) % 2,
            (spriteRow >> 3) % 2,
            (spriteRow >> 2) % 2,
            (spriteRow >> 1) % 2,
            spriteRow % 2
        };
        indexreg++;

        for(uint8_t i = 0; i < 8; i++)
        {
            if(spriteArray[i] == 1 && display[vx][vy] == 1)
            {
                display[vx][vy] = 0;
                registers[0xF] = 1;
            }
            if(spriteArray[i] == 1 && display[vx][vy] == 0)
            {
                display[vx][vy] = 1;
            }

            ++vx; // Next column.
            if(vx >= SCREEN_WIDTH) break; // Prevent going past screenlimits
        }
        ++vy; // Next row.
        if(vy >= SCREEN_HEIGHT) break; // Prevent going past screenlimits
    }

    render(SCREEN_WIDTH, SCREEN_HEIGHT, display);
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

void decode(uint16_t op)
{
    printf("Got op %x\n", op);

    // Deconstruct instruction.
    const uint8_t i = op >> 12; // first nibble, instruction
    const uint8_t x = (op >> 8) - ((uint16_t)i << 4); // second nibble, register
    const uint8_t y = (op >> 4) - ((uint16_t)i << 8) - ((uint16_t)x << 4); // third nibble, register
    const uint8_t n = op - ((uint16_t)i << 12) - ((uint16_t)x << 8) - ((uint16_t)y << 4); //fourth nibble, variable
    const uint8_t nn = op - ((uint16_t)i << 12) - ((uint16_t)x << 8); // variable
    const uint16_t nnn = op - ((uint16_t)i << 12); // variable

    //printf(" Running op %x %x %x %x\n", i, x, y, n);

    switch(i)
    {
    case(0x0):
        //if(x != 0) sys(nnn);
        if(n == 0) {
            cls();
            puts("cls");
        }
        //if(n == 0xE) ret();
        break;
    case(0x1):
        jump(nnn);
        puts("jump");
        break;
    case(0x6):
        setreg(x,nn);
        puts("setreg");
        break;
    case(0x7):
        addreg(x,nn);
        puts("addreg");
        break;
    case(0xA):
        setindex(nnn);
        puts("setindex");
        break;
    case(0xD):
        draw(x, y, n);
        puts("draw");
        break;
    default:
        printf("Not implemented: %x\n", i);
        break;
    }
}

// Emulate one 'tick' of the CPU.
void cpu()
{
    decode(fetch());
}