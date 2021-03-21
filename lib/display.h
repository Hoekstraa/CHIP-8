#include <SDL2/SDL.h>

#define WINDOW_WIDTH 512
#define WINDOW_HEIGHT 256
#define WINDOW_NAME "CHIP-8"

SDL_Window *window = 0; // Global window
SDL_Renderer *renderer = 0; // Global renderer
SDL_Texture *screenTexture = 0;

void initDisplay(int screenwidth, int screenheight)
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
    SDL_RenderSetIntegerScale(renderer, (SDL_bool)1);
    SDL_RenderSetLogicalSize(renderer, screenwidth,screenheight);

    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, screenwidth, screenheight);
    if( screenTexture == 0 )
    {
        printf( "map texture could not be created! SDL_Error: %s\n", SDL_GetError() );
        SDL_Quit();
        exit(4);
    }
}

void render(int screenwidth, int screenheight, uint8_t display[][screenheight])
{
    // Clear everything.
    SDL_DestroyTexture(screenTexture);
    screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, screenwidth, screenheight);
    SDL_SetRenderDrawColor(renderer, 0x33, 0x33, 0x55, 0xFF);
    SDL_RenderClear(renderer);
    // -----------------

    SDL_SetRenderTarget(renderer, screenTexture);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    for(int x = 0; x < screenwidth; x++)
        for(int y = 0; y < screenheight; y++)
            if(display[x][y] == 1)
                SDL_RenderDrawPoint(renderer,x, y);

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    // Present to screen
    SDL_RenderPresent(renderer);
}
