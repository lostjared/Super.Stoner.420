#include "text_render.h"

#include <stdio.h>
#include <stdlib.h>

struct SDL_Font *SDL_InitFont(const char *src, int pt_size) {
    struct SDL_Font *fnt;

    if (!src || pt_size <= 0) {
        return 0;
    }
    if (TTF_WasInit() == 0 && TTF_Init() < 0) {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return 0;
    }

    fnt = (struct SDL_Font *)malloc(sizeof(struct SDL_Font));
    if (!fnt) {
        return 0;
    }
    fnt->ttf = TTF_OpenFont(src, pt_size);
    fnt->pt_size = pt_size;

    if (!fnt->ttf) {
        fprintf(stderr, "TTF_OpenFont failed for %s: %s\n", src, TTF_GetError());
        free(fnt);
        return 0;
    }

    return fnt;
}

void SDL_FreeFont(struct SDL_Font *m) {
    if (!m) {
        return;
    }
    if (m->ttf) {
        TTF_CloseFont(m->ttf);
        m->ttf = 0;
    }
    free(m);
}

int SDL_PrintText(SDL_Surface *surf, struct SDL_Font *fnt, int x, int y, Uint32 color, const char *src) {
    SDL_Color text_color;
    SDL_Surface *text;
    SDL_Rect dst;
    Uint32 wrap_len;

    if (!surf || !fnt || !fnt->ttf || !src) {
        return 1;
    }

    SDL_GetRGB(color, surf->format, &text_color.r, &text_color.g, &text_color.b);
    text_color.a = 255;
    wrap_len = (x < surf->w) ? (Uint32)(surf->w - x) : 0;
    text = TTF_RenderUTF8_Blended_Wrapped(fnt->ttf, src, text_color, wrap_len);
    if (!text) {
        return 1;
    }

    dst.x = x;
    dst.y = y;
    dst.w = text->w;
    dst.h = text->h;
    SDL_BlitSurface(text, 0, surf, &dst);
    SDL_FreeSurface(text);
    return 0;
}

void SDL_PrintTextScaled(SDL_Surface *surf, struct SDL_Font *fnt, int x, int y, int w, int h, Uint32 color, const char *src) {
    SDL_Color text_color;
    SDL_Surface *text;
    SDL_Rect dst;

    if (!surf || !fnt || !fnt->ttf || !src || w <= 0 || h <= 0) {
        return;
    }

    SDL_GetRGB(color, surf->format, &text_color.r, &text_color.g, &text_color.b);
    text_color.a = 255;
    text = TTF_RenderUTF8_Blended(fnt->ttf, src, text_color);
    if (!text) {
        return;
    }

    dst.x = x;
    dst.y = y;
    dst.w = w;
    dst.h = h;
    SDL_BlitScaled(text, 0, surf, &dst);
    SDL_FreeSurface(text);
}
