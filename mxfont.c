#include "SDL_mxf.h"
#include<stdio.h>
#include<stdlib.h>
int SDL_GetFX(struct SDL_Font *m,int x, int nw) {
	float xp = (float)x * (float)m->mx / (float)nw;		
	return (int)xp;
}
int SDL_GetFZ(struct SDL_Font *m, int y, int nh) {
	float yp = (float)y * (float)m->my / (float)nh;
	return (int)yp;
}
void InitFont(struct SDL_Font *m, int width, int height, int color) {
	int l,p,i,z;
	m->mx = width;
	m->my = height;
	for(l = 0; l <= 127; l++) {
		m->letters[l].fnt_ptr = (int**) malloc ( sizeof(int*) * width);
		for(p = 0; p < width; p++) {
			m->letters[l].fnt_ptr[p] = (int*) malloc ( sizeof(int) * height);
		}
		for(i = 0; i < width; i++) {
			for(z = 0; z < height; z++) {
				m->letters[l].fnt_ptr[i][z] = color;
			}
		}
	}
	m->tcolor = 0x0;
}
void dump_font(struct SDL_Font *fnt) {
}
int littleToBig(int i)
{
	return((i&0xff)<<24)+((i&0xff00)<<8)+((i&0xff0000)>>8)+((i>>24)&0xff);
}
struct SDL_Font *SDL_InitFont(const char *src) {
	FILE *fptr = fopen(src, "rb");
	int q = 0,mx = 0,my = 0;
	struct SDL_Font *fnt;
	int i,z,p;
	if(!fptr) return 0;
	int mode = 0;
	fnt = (struct SDL_Font*)malloc(sizeof(struct SDL_Font));
	if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
	{
		mode = 1;
	}
	fread((char*)&q, sizeof(int), 1, fptr);
	if(mode == 1)
	q = littleToBig(q);
	if(q == -100) {
		fread((char*)&mx, sizeof(int), 1, fptr);
		fread((char*)&my, sizeof(int), 1, fptr);
		fread((char*)&fnt->tcolor, sizeof(int), 1, fptr);
		if(mode == 1) {
			mx = littleToBig(mx);
			my = littleToBig(my);
			fnt->tcolor = littleToBig(fnt->tcolor);
		}
		if(mx < 0 || mx < 0 || mx > 250 || my > 250) {
			fprintf(stderr, "Error invalid font file format!\n");
			free(fnt);
			return 0;
		}
		InitFont(fnt, mx, my, fnt->tcolor);
		for(p = 0; p <= 127; p++)
		for(i = 0; i < fnt->mx; i++) {
			for(z = 0; z < fnt->my; z++) {
				fread((char*)&fnt->letters[p].fnt_ptr[i][z], sizeof(int), 1, fptr);
				if(mode == 1) fnt->letters[p].fnt_ptr[i][z] = littleToBig(fnt->letters[p].fnt_ptr[i][z]);
			}
		}
	}
	fclose(fptr);
	return fnt;
}
void SDL_FreeFont(struct SDL_Font *m) {
	int l,i;
	for(l = 0; l <= 127; l++) {
		if(m->letters[l].fnt_ptr) {
			for(i = 0; i < m->mx; i++) 
				free(m->letters[l].fnt_ptr[i]);
			free(m->letters[l].fnt_ptr);
		}
	}
	free(m);
}
int SDL_PrintText(struct SDL_Surface *surf, struct SDL_Font *fnt, int x, int y, Uint32 color, const char *src) {
	int prev_x = x;
	int offset_x = prev_x, offset_y = y;
	int width = 0, height = 0;
	int i,z,p;
	void *pbuf = lock(surf, surf->format->BitsPerPixel);
	for(p = 0; p < (int)strlen(src);  p++) {
		if(src[p] == '\n') {
			offset_x = prev_x;
			offset_y += height;
			continue;
		}
		for(i = 0; i < fnt->mx; i++) {
			for(z = 0; z < fnt->my; z++) {
				if(fnt->letters[src[p]].fnt_ptr[i][z] != fnt->tcolor) {
					if(offset_y+z > 0 && offset_x+i > 0 && offset_y+z < surf->h && offset_x+i < surf->w)
					setpixel(pbuf, (Uint32)offset_x+i, (Uint32)offset_y+z, color, surf->format->BitsPerPixel, surf->pitch);
					width=i;
					if(height < z)
						height=z;
				}
			}
		}
		offset_x += width + 3;
		if(offset_x+width >= surf->w) {
			offset_x = prev_x;
			offset_y += height;
		}
		if(offset_y+height > surf->h)
				return 1;
	}
	unlock(surf);
	return 0;
}
void SDL_PrintTextScaled(struct SDL_Surface *surf, struct SDL_Font *fnt, int x, int y, int w, int h, Uint32 color, const char *src) {
		int prev_x = x;
	int offset_x = prev_x, offset_y = y;
	int width = 0, height = 0;
	int i,z,p;
	void *pbuf = lock(surf, surf->format->BitsPerPixel);
	for(p = 0; p < (int)strlen(src);  p++) {
		if(src[p] == '\n') {
			offset_x = prev_x;
			offset_y += height;
			continue;
		}
		for(i = 0; i < w; i++) {
			for(z = 0; z < h; z++) {
				if(fnt->letters[src[p]].fnt_ptr[SDL_GetFX(fnt,i,w)][SDL_GetFZ(fnt,z,h)] != fnt->tcolor) {
					setpixel(pbuf, (Uint32)offset_x+i, (Uint32)offset_y+z, color, surf->format->BitsPerPixel, surf->pitch);
					width=i;
					if(height < z)
						height=z;
				}
			}
		}
		offset_x += width + 2;
	}
	unlock(surf);
}
