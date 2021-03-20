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
        else
        {
            printf("Setting memory address block %i with %i, %x\n", i, (uint8_t)data, (uint8_t)data);
            memory[i] = (uint8_t)data;
        }
    }
    fclose(f);
}

int handleEvents()
{
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
                //handleKey(event.key.keysym.sym);
                return 1;
        }
    }
    return 1;
}

int main(int argc, char ** argv)
{
    initDisplay(SCREEN_WIDTH, SCREEN_HEIGHT); // Create screen, etc.
    setfont(memory); // Set CHIP-8 font in memory.
    load_into_mem("roms/test_opcode.ch8"); // Load in program.

    render(SCREEN_WIDTH, SCREEN_HEIGHT, display);

    while (handleEvents()) // As long as there's no quit event, handle other events and do..
    {
        //printf("\n%i\n", programcounter);
        cpu();
        //sleep(1);
    }

    SDL_Quit();
    return 0;
}
