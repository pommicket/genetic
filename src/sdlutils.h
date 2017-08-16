// __-prefixed functions, structs, etc. are not advised to be used by other files
#include <stdlib.h>
#include <math.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include "colors.h"

#define FONT_WIDTH_FACTOR 0.585 // Width of text / letter / font size
#define FONT_HEIGHT_FACTOR 1.2 // Height of text / font size

#define FONT_SIZE 12
#define CHAR_WIDTH FONT_WIDTH_FACTOR * FONT_SIZE
#define CHAR_HEIGHT FONT_HEIGHT_FACTOR * FONT_SIZE

#define MAX_OPTIONS 1000

#define ENTRY_SUBMIT 1 << 14

SDL_Window* window;
SDL_Surface* window_surface;
SDL_Renderer* renderer;
TTF_Font* font;
char* editor;
char* __sdlutils_clear;
int __sdlutils_entry_capacity;
int __sdlutils_entry_enabled;

typedef struct
{
    char* text;
    int xpos;
    int ypos;
    int action_id;
    SDL_Color fg;
    SDL_Color bg;
    SDL_Color hl; // BG When highlighted
} __sdlutils_option;

__sdlutils_option** __sdlutils_options;
int __sdlutils_selected = 0;
int __sdlutils_option_count = 0;
int __sdlutils_entry_x, __sdlutils_entry_y;
char* __sdlutils_entry_contents;
int __sdlutils_entry_contents_sz; // Length of string in entry

char* int_to_str(int i)
{
    char* buffer = malloc((int)(log((double)i)/log(10))+3);
    sprintf(buffer, "%d", i);
    return buffer;
}

char* double_to_str(double d)
{
    char* buffer = malloc(32);
    sprintf(buffer, "%f", d);
    return buffer;
}


void write_text(char* text, int x, int y, SDL_Color fg, SDL_Color bg)
{
    if (!(text && text[0])) return;
    SDL_Surface* text_surface = TTF_RenderText_Solid(font, text, fg);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_Rect rect = {x, y, text_surface->w, text_surface->h};
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(renderer, &rect);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    SDL_FreeSurface(text_surface);
    SDL_DestroyTexture(texture);

}

void write_str(char* s, int xpos, int ypos, SDL_Color fg, SDL_Color bg)
{
    // For grid-like letter placement (for text-based applications1)
    // Same as write_text, but writes in terms of the letter positions
    write_text(s, xpos * CHAR_WIDTH, ypos * CHAR_HEIGHT, fg, bg);
}

void hcenter_str(char* s, int cols, int ypos, SDL_Color fg, SDL_Color bg)
{
    // Center string horizontally
    write_str(s, cols / 2 - strlen(s) / 2, ypos, fg, bg);
}

void write_char(char c, int xpos, int ypos, SDL_Color fg, SDL_Color bg)
{
    char* s = malloc(2);
    s[0] = c;
    s[1] = 0;
    write_str(s, xpos, ypos, fg, bg);
}

void create_option(char* text, int action_id, int x, int y, SDL_Color fg, SDL_Color bg, SDL_Color highlighted)
{
    __sdlutils_option* opt = malloc(sizeof(__sdlutils_option));
    opt->text = text;
    opt->action_id = action_id;
    opt->xpos = x;
    opt->ypos = y;
    opt->fg = fg;
    opt->bg = bg;
    opt->hl = highlighted;
    __sdlutils_options[__sdlutils_option_count++] = opt;
}

void draw_options()
{
    int i;
    __sdlutils_option* opt;
    for (i = 0; opt = __sdlutils_options[i]; i++)
    {
        write_str(opt->text, opt->xpos, opt->ypos, opt->fg, __sdlutils_selected == i ? opt->hl : opt->bg);
    }
}

void clear_options()
{
    int i;
    for (i = __sdlutils_option_count = 0; i < MAX_OPTIONS; i++)
    {
        __sdlutils_options[i] = NULL;
    }
    __sdlutils_selected = 0;
    for (i = 0; i < __sdlutils_entry_capacity; i++) __sdlutils_entry_contents[i] = 0;
    __sdlutils_entry_enabled = 0;
    SDL_StopTextInput();
}

int mod(int a, int b)
{
    a %= b;
    while (a < 0) a += b;
    return a;
}

void draw_entry()
{
    if (__sdlutils_entry_enabled)
    {
        write_str(__sdlutils_clear, __sdlutils_entry_x, __sdlutils_entry_y, BLACK, BLACK);
        write_str(__sdlutils_entry_contents, __sdlutils_entry_x, __sdlutils_entry_y, WHITE, BLACK);
    }
}

void setup_entry(char* contents, int x, int y)
{
    memcpy(__sdlutils_entry_contents, contents, strlen(contents)+1);
    __sdlutils_entry_x = x;
    __sdlutils_entry_y = y;
    __sdlutils_entry_enabled = 1;
    SDL_StartTextInput();
    draw_entry();
}

char* get_entry_contents()
{
    return __sdlutils_entry_contents;
}



int sdlutils_process_key(SDL_Keycode kc)
{
    int len;
    switch (kc)
    {
    case SDLK_UP:
        if (__sdlutils_options[0])
        {
            __sdlutils_selected--;
            __sdlutils_selected = mod(__sdlutils_selected, __sdlutils_option_count);
            draw_options();
        }
        break;
    case SDLK_DOWN:
        if (__sdlutils_options[0])
        {
            __sdlutils_selected++;
            __sdlutils_selected = mod(__sdlutils_selected, __sdlutils_option_count);
            draw_options();
        }
        break;
    case SDLK_RETURN:
        if (__sdlutils_entry_enabled) return ENTRY_SUBMIT;
        if (__sdlutils_options[0]) return __sdlutils_options[__sdlutils_selected]->action_id;
        break;
    case SDLK_BACKSPACE:
        if (!__sdlutils_entry_enabled) break;
        len = strlen(__sdlutils_entry_contents);
        __sdlutils_entry_contents[(len ? len : 1)-1] = 0;
        break;
    }
    if (__sdlutils_options[0]) __sdlutils_selected = mod(__sdlutils_selected, __sdlutils_option_count);
    draw_entry();
    return 0;
}

void handle_text(char* text)
{
    if (!__sdlutils_entry_enabled) return;
    strcat(__sdlutils_entry_contents, text);
    draw_entry();
}

void quit()
{
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    exit(0);
}

void sdlutils_set_capacity(int capacity)
{
    __sdlutils_entry_contents = malloc((__sdlutils_entry_capacity = capacity)+1);
    __sdlutils_clear = malloc(__sdlutils_entry_capacity+1);
    int i;
    for (i = 0; i < __sdlutils_entry_capacity; i++)
    {
        __sdlutils_clear[i] = 'M'; // Wide letter for non-monospace fonts (although most of this wouldn't work for non-monospace fonts)
    }
    __sdlutils_clear[i] = 0;
}

void sdlutils_init(int width, int height)
{
    __sdlutils_options = malloc(MAX_OPTIONS*sizeof(__sdlutils_option*));

    clear_options();

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }
    if (TTF_Init() == -1)
    {
        fprintf(stderr, "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
    }

    window = SDL_CreateWindow("Life", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        fprintf(stderr, "SDL Error: %s\n", SDL_GetError());
        exit(1);
    }

    window_surface = SDL_GetWindowSurface(window);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // Font
    font = TTF_OpenFont("fonts/DejaVuSansMono.ttf", FONT_SIZE);
}
