#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "font.h"

void render();

SDL_Window *window = 0; // Global window
SDL_Renderer *renderer = 0; // Global renderer

#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 256
#define WINDOW_NAME "CHIP-8"

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
uint8_t memory[4096]; // 4KB, packed per 8 bits
uint8_t display[SCREEN_WIDTH][SCREEN_HEIGHT]; // Could possibly be optimized by dividing by 8 and doing it bitwise.
uint16_t programcounter = 0x200; // Programs start at hexadecimal 200, dec 512.
uint16_t indexreg = 0;
//uint16_t stack[16];
//uint8_t delaytimer = 0;
//uint8_t soundtimer = 0;
uint8_t registers[16];
SDL_Texture *screenTexture = 0;

//Helper function

void load_into_mem(char* filename)
{
    FILE *f = fopen(filename, "rb");
    char data = 0;
    if (!f)
    {
        printf("Unable to open file!\n");
        exit(5);
    }

    for(int i = 512; ; i++) // Bit 512(0x200) is the start of programs.
    {
        data = fgetc(f);
        if (feof(f))
            break;
        else
        {
            printf("Setting memory address block %i with %i, %x\n", i, (uint8_t)data, (uint8_t)data);
            memory[i] = (uint8_t)data;
        }
    }
    fclose(f);
}

//=====CPU=====//

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

// CURRENTLY ONLY SUPPORTS WRITING MAX 1 VALUE TO SCREEN, NO SPRITES!!
void draw(uint8_t x, uint8_t y, uint8_t n)
{
    uint8_t mem = memory[indexreg];

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
        uint8_t spriteRow = memory[indexreg];
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
    render();
}

// Return current instruction to be ran. Also, shift programcounter to next instruction.
uint16_t fetch()
{
    printf("Memory index = %i\n", programcounter);

    uint8_t a = memory[programcounter];
    printf("a = %x\n", a);
    uint8_t b = memory[programcounter + 1];
    printf("b = %x\n", b);

    uint16_t op = (a << 8) + b; // Morph 2 bytes into one 16-bit instruction

    programcounter += 2; // Set programcounter to next op.
    return op;
}

void decode(uint16_t op)
{
    printf("Got op %x\n", op);

    // Deconstruct instruction.
    uint8_t i = op >> 12; // first nibble, instruction
    uint8_t x = (op >> 8) - ((uint16_t)i << 4); // second nibble, register
    uint8_t y = (op >> 4) - ((uint16_t)i << 8) - ((uint16_t)x << 4); // third nibble, register
    uint8_t n = op - ((uint16_t)i << 12) - ((uint16_t)x << 8) - ((uint16_t)y << 4); //fourth nibble, variable
    uint8_t nn = op - ((uint16_t)i << 12) - ((uint16_t)x << 8); // variable
    uint16_t nnn = op - ((uint16_t)i << 12); // variable

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
//=============

void initSDL()
{
    //Initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
        exit(1);
    }
    //Create window
    window = SDL_CreateWindow(WINDOW_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN );
    if( window == 0 )
    {
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        SDL_Quit();
        exit(2);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if( renderer == 0 )
    {
        printf( "Renderer could not be created! SDL_Error: %s\n", SDL_GetError() );
        SDL_Quit();
        exit(3);
    }
    SDL_RenderSetIntegerScale(renderer, 1);
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH,SCREEN_HEIGHT);

    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    if( screenTexture == 0 )
    {
        printf( "map texture could not be created! SDL_Error: %s\n", SDL_GetError() );
        SDL_Quit();
        exit(4);
    }
}

void render()
{
    // Clear everything.
    SDL_DestroyTexture(screenTexture);
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);
    // -----------------

    SDL_SetRenderTarget(renderer, screenTexture);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    for(int x = 0; x < SCREEN_WIDTH; x++)
        for(int y = 0; y < SCREEN_HEIGHT; y++)
            if(display[x][y] == 1)
                SDL_RenderDrawPoint(renderer,x, y);

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    // Present to screen
    SDL_RenderPresent(renderer);
}

int handleEvents()
{
    // Get the next event
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
            // Break out of the gameloop on quit
            return 0;
        if (event.type == SDL_KEYDOWN)
        {
            if(event.key.keysym.sym == SDLK_ESCAPE)
                // Break out of the gameloop on quit
                return 0;
            else
                //handleKey(event.key.keysym.sym);
                return 1;
        }
    }
    return 1;
}

int main(int argc, char ** argv)
{
    initSDL(); // Create screen, etc.
    setfont(memory); // Set CHIP-8 font in memory.
    load_into_mem("ibm.ch8"); // Load in program.
    // Make the texture to render to.
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STATIC, SCREEN_WIDTH, SCREEN_HEIGHT);

    render(); // Clear screen

    while (handleEvents()) // As long as there's no quit event, handle other events and do..
    {
        printf("\n%i\n", programcounter);
        render();
        cpu();
        sleep(1);
    }

    SDL_Quit();
    return 0;
}
