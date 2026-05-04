#ifndef TEXT_RENDER_H
#define TEXT_RENDER_H

#if defined(__has_include)
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#if __has_include(<SDL2/SDL_ttf.h>)
#include <SDL2/SDL_ttf.h>
#else
#include <SDL_ttf.h>
#endif
#else
#include <SDL.h>
#include <SDL_ttf.h>
#endif
#include <string.h>

struct SDL_Font {
    TTF_Font *ttf;
    int pt_size;
};

extern struct SDL_Font *SDL_InitFont(const char *src, int pt_size);
extern void SDL_FreeFont(struct SDL_Font *m);
extern int SDL_PrintText(SDL_Surface *surf, struct SDL_Font *m, int x, int y, Uint32 color, const char *src);
extern void SDL_PrintTextScaled(SDL_Surface *surf, struct SDL_Font *m, int x, int y, int w, int h, Uint32 color, const char *src);

extern void *lock(SDL_Surface *surf);
extern void unlock(SDL_Surface *surf);
extern void setpixel(void *buff, Uint32 x, Uint32 y, Uint32 color, Uint8 type, Uint16 pitch);
extern Uint32 getpixel(SDL_Surface *surf, int x, int y, Uint8 type, Uint16 pitch, SDL_Color *c);

#endif