#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "lib/font.h"

// Screen size must be defined before loading display and cpu!
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#include "lib/display.h"
#include "lib/chip.h"

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

        printf("Setting memory address block %i with %i, %x\n", i, (uint8_t)data, (uint8_t)data);
        memory[i] = (uint8_t)data;
    }
    fclose(f);
}

void handleKey(SDL_Keycode symbol, int state)
{
    if(symbol == SDLK_x)
        keys[0] = state;
    if(symbol == SDLK_1)
        keys[1] = state;
    if(symbol == SDLK_2)
        keys[2] = state;
    if(symbol == SDLK_3)
        keys[3] = state;
    if(symbol == SDLK_q)
        keys[4] = state;
    if(symbol == SDLK_w)
        keys[5] = state;
    if(symbol == SDLK_e)
        keys[6] = state;
    if(symbol == SDLK_a)
        keys[7] = state;
    if(symbol == SDLK_s)
        keys[8] = state;
    if(symbol == SDLK_d)
        keys[9] = state;
    if(symbol == SDLK_z)
        keys[0xA] = state;
    if(symbol == SDLK_c)
        keys[0xB] = state;
    if(symbol == SDLK_4)
        keys[0xC] = state;
    if(symbol == SDLK_r)
        keys[0xD] = state;
    if(symbol == SDLK_f)
        keys[0xE] = state;
    if(symbol == SDLK_v)
        keys[0xF] = state;
}


int handleEvents()
{
    //for(int i = 0; i < 16; i++)
    //    keys[i] = 0;

    // Get the next event
    SDL_Event event;
    if (SDL_PollEvent(&event))
    {
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
            return 0; // Break out of the gameloop on quit

        if (event.type == SDL_KEYDOWN)
        {
            if(event.key.keysym.sym == SDLK_ESCAPE)
                return 0; // Break out of the gameloop on quit
            else
                handleKey(event.key.keysym.sym, 1);
            return 1;
        }
        if (event.type == SDL_KEYUP)
            handleKey(event.key.keysym.sym, 0);

    }
    return 1;
}

int main(int argc, char ** argv)
{
    if(argc < 2 || argc > 2)
    {
        printf("Please run this command as follows: %s [programname.ch8]\n", argv[0]);
        return 1;
    }

    load_into_mem(argv[1]); // Load in program.
    setfont(memory); // Set CHIP-8 font in memory.

    initDisplay(SCREEN_WIDTH, SCREEN_HEIGHT); // Create screen, etc.
    render(SCREEN_WIDTH, SCREEN_HEIGHT, display); // Set screen to black.

    srand(time(NULL)); // Needed for CXNN instruction in CHIP-8 CPU.

    Uint64 nowTime = SDL_GetPerformanceCounter();
    Uint64 lastTime = 0;
    double deltaTime = 0;

    while (handleEvents()) // As long as there's no quit event, handle other events and do..
    {
        lastTime = nowTime;
        nowTime = SDL_GetPerformanceCounter();
        deltaTime = (double)((nowTime - lastTime)*1000 / (double)SDL_GetPerformanceFrequency());
        //printf("\n%i\n", programcounter);
        if(deltaTime > (0.003 )) cpu();
        if(deltaTime > (0.016 )) cpuDecTimers();
        //sleep(1);
    }

    SDL_Quit();
    return 0;
}
