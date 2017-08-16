#define WHITE rgba(255, 255, 255, 255)
#define DARKGRAY rgba(50, 50, 50, 255)
#define LIGHTGRAY rgba(100, 100, 100, 255)
#define BLACK rgba(0, 0, 0, 255)

#define RED rgba(255, 0, 0, 255)
#define DARKRED rgba(150, 0, 0, 255)
#define LIGHTRED rgba(255, 150, 150, 255)

#define GREEN rgba(0, 255, 0, 255)
#define DARKGREEN rgba(0, 150, 0, 255)
#define LIGHTGREEN rgba(150, 255, 150, 255)

#define BLUE rgba(0, 0, 255, 255)
#define DARKBLUE rgba(0, 0, 150, 255)
#define LIGHTBLUE rgba(150, 150, 255, 255)

SDL_Color rgba(int r, int g, int b, int a)
{
    SDL_Color color = {r, g, b, a};
    return color;
}
