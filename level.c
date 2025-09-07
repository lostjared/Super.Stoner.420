#include "level.h"
#include "smx.h"
#define FIRE_COOLDOWN_MS 250
Level *level;
Hero  hero;
Evil  evil[50];
EvilGfx evil_gfx[4];
int offset = 0;
static int jump = 0, jump_ok = 0;
int hero_ani = 0;
void logic();
static int shoot_ani = 0;
static Uint32 last_fire_ticks = 0;
static void fill_evil(Evil *e, int vpos, int type) {
	e->vpos = vpos;
	e->type = type;
	e->dir = 0;
	e->egfx = &evil_gfx[e->type];
	switch(type) {
		case 0:
			break;
	}
}

Level *load_level(const char *src) {
	Level *lvl = 0;
	FILE *fptr = 0;
	fptr = fopen(src, "rb");
	if(!fptr)
		return 0;
	lvl = (Level*) malloc ( sizeof( Level ) );
	memset( lvl, 0, sizeof(Level) );
	fread( (char*)lvl, 1, sizeof(Level), fptr);
	fclose(fptr);
	hero.hpos = lvl->start_pos;
	hero.cur_ani = 0;
	hero.dir = 1;
	{
		unsigned int i = 0;
		memset(evil, 0, sizeof(evil));
		for(i = 0; i < 50; i++)
			if(lvl->grandma[i] != 0) {
				fill_evil(&evil[i], lvl->grandma[i], 0); 
			}
			else evil[i].type = -1;
	}
	return lvl;
}

void release_level(Level *lvl) {
	free(lvl);
}

void render_pause() {
    SDL_Rect rc = { 50,50,640-100,480-100 };
    
    if(stick && SDL_JoystickGetButton(stick, 1)) {
        cur_scr = ID_GAME;
    }
    if(stick && SDL_JoystickGetButton(stick, 0)) {
        game_over();
    }
    SDL_BlitSurface(bg, 0, front, 0);
    SDL_FillRect(front, &rc, 0);
    SDL_PrintText(front, font, 75, 75, SDL_MapRGB(front->format, 255, 255, 255), "Paused - Press Circle to continue ");
    SDL_PrintText(front, font, 75, 110, SDL_MapRGB(front->format, 255, 0, 0), "Press Triangle to Return to Menu ");
}

void render_map(SDL_Surface *surf, Level *lvl) {
    static int startby;
    static int bx,by;
    int gcount = 0;
    Uint32 i;
    startby = 75;
    bx = 75; by = startby;   
    
#ifdef FOR_PSP
    if(stick == NULL) {
        try_init_joystick();
    }
    
    if(stick && SDL_JoystickGetButton(stick, 10)) {
        cur_scr = ID_PAUSED;
        return;
    }
#endif

#ifndef FOR_PSP
        SDL_BlitSurface(bg, 0, surf, 0);
#endif
		logic();
		for(i = 0; i < 700-4+24; i++)
		{
			SDL_Rect rc = { bx, by, 16, 16 };
			SDL_BlitSurface(gfx[lvl->tiles[offset+i].block], 0, surf, &rc);			
			by = by + 16;
			gcount++;
			if(gcount > 23)
			{
				gcount = 0;
				by = startby;
				bx = bx + 16;
			}			
		}
		{
			SDL_Rect rc = { 25, 15, 640-50, 30 };
			SDL_FillRect(surf,&rc, 0);
		}
		SDL_PrintText(surf, font, 25, 25, SDL_MapRGB(surf->format, 255, 255, 255), lvl->level_name);
		{
			static char buf[256];
			memset(buf, 0, sizeof(buf));
			snprintf(buf,255, "Score: %d Lives: %d", score, lives);
			SDL_PrintText(surf,cfont,350, 25, SDL_MapRGB(surf->format, rand()%255, rand()%255, 255), buf);
		}
		startby = 75;
		bx = 75; by = startby;
		gcount = 0;
		hero.x = hero.y = 0;
		if(hero_ani == 1) {
			static int w = 0;
			if(w++ >= 3) w = 0;
			hero.cur_ani = w;
		}
		for( i = 0; i < 700-4+24; i++) {
			SDL_Rect rc = { bx, by, hgfx[hero.cur_ani]->w-1, hgfx[hero.cur_ani]->h };
			if(i == hero.hpos) {
				hero.x = bx, hero.y = by;
				SDL_SetColorKey(hgfx[hero.cur_ani], SDL_TRUE, SDL_MapRGB(hgfx[hero.cur_ani]->format, 255, 255, 255));
				if(hero.dir == 1)
				SDL_BlitSurface( hgfx[hero.cur_ani], 0, surf, &rc);
				else {
					SDL_Rect rc2 = { 1, 1, hgfx[hero.cur_ani]->w-1, hgfx[hero.cur_ani]->h-1 };
					SDL_ReverseBlt(hgfx[hero.cur_ani], &rc2, surf, &rc, SDL_MapRGB(hgfx[hero.cur_ani]->format, 255, 255, 255));
				}
			}
			{
				Uint8 pos = 0;
				for(pos = 0; pos < MAX_PARTICLE; pos++) {
					if(emiter.p[pos].type != 0 && emiter.p[pos].vpos == i+offset) {
						SDL_Rect rcX = { bx, by, particles[0]->w, particles[0]->h };
						SDL_SetColorKey(particles[0], SDL_TRUE, SDL_MapRGB(particles[0]->format, 255, 255, 255));
						SDL_BlitSurface(particles[0], 0, surf, &rcX );
						emiter.p[pos].x = bx, emiter.p[pos].y = by;
					}
				}
				for(pos = 0; pos < 50; pos++) {
					if(evil[pos].type != -1 && evil[pos].vpos == i+offset) {
						SDL_Rect erc = { 0,  0, evil[pos].egfx->gfx[evil[pos].type]->w-1, evil[pos].egfx->gfx[evil[pos].type]->h };
						SDL_Rect prc = { bx, by, evil[pos].egfx->gfx[evil[pos].type]->w, evil[pos].egfx->gfx[evil[pos].type]->h };
						if(evil[pos].dir == 1) {
							SDL_SetColorKey(evil[pos].egfx->gfx[evil[pos].cur_ani], SDL_TRUE, SDL_MapRGB(evil[pos].egfx->gfx[evil[pos].cur_ani]->format, 255, 255, 255));
							SDL_BlitSurface(evil[pos].egfx->gfx[evil[pos].cur_ani], 0, surf, &prc);
						}
						else
							SDL_ReverseBlt(evil[pos].egfx->gfx[evil[pos].cur_ani], &erc, surf, &prc, SDL_MapRGB(evil[pos].egfx->gfx[evil[pos].cur_ani]->format, 255, 255, 255));
						evil[pos].x = bx, evil[pos].y = by;
					}
					if(level->items[pos].type != 0 && level->items[pos].vpos == i+offset) {
						SDL_Rect rc = { bx, by, collect[level->items[pos].type]->w, collect[level->items[pos].type]->h };
						SDL_SetColorKey(collect[level->items[pos].type], SDL_TRUE, SDL_MapRGB(collect[level->items[pos].type]->format, 255, 255, 255));
						SDL_BlitSurface(collect[level->items[pos].type], 0, surf, &rc);
					}
				}
			}
			by = by + 16;
			gcount++;
			if(gcount > 23) {
				gcount = 0;
				by = startby;
				bx = bx + 16;
			}
		}
	if (level != NULL && level->tiles[hero.hpos+offset].block == 14) {
		reload_level();
	}
	if(lives < 0)
		game_over();
}

void scroll_left() {
	if(offset > 0)
		offset -= 24;
}

void scroll_right() {
	if(offset < MAX_TILE)
		offset += 24;
}

static void move_left() {
	hero.dir = 0;
	if(hero.hpos > 0 && offset == 0) {
			Uint8 check[5];
			check[0] = level->tiles[hero.hpos-24].solid;
			check[1] = level->tiles[hero.hpos+1-24].solid;
			check[2] = level->tiles[hero.hpos+2-24].solid;
			check[3] = level->tiles[hero.hpos+3-24].solid;
			check[4] = level->tiles[hero.hpos-24-24].solid;
			if(!check[0] && !check[1] && !check[2] && !check[3] && !check[4]) hero.hpos -= 24;
			hero_ani = 1;
	}
	else {
			Uint8 check[5];
			check[0] = level->tiles[hero.hpos+offset-24].solid;
			check[1] = level->tiles[hero.hpos+offset+1-24].solid;
			check[2] = level->tiles[hero.hpos+offset+2-24].solid;
			check[3] = level->tiles[hero.hpos+offset+3-24].solid;
			check[4] = level->tiles[hero.hpos+offset-24-24].solid;
			if(!check[0] && !check[1] && !check[2] && !check[3] && !check[4]) scroll_left();
			hero_ani = 1;
	}
}

static void move_right() {
	Uint8 check[5];
	hero.dir = 1;
	if(hero.hpos < 24*15) {
		check[0] = level->tiles[hero.hpos + 27].solid;
		check[1] = level->tiles[hero.hpos + 27 + 24].solid;
		check[2] = level->tiles[hero.hpos + 27 + 23].solid;
		check[3] = level->tiles[hero.hpos + 27 + 22].solid;
		check[4] = level->tiles[hero.hpos + 27 + 21].solid;
		if(!check[0] && !check[1] && !check[2] && !check[3] && !check[4]) hero.hpos += 24;
		hero_ani = 1;
		if(jump_ok == 0 && jump == 0 && hero_ani == 0 && shoot_ani == 0)
			hero.cur_ani = 0;
	}
	else {
		check[0] = level->tiles[hero.hpos + 27+offset].solid;
		check[1] = level->tiles[hero.hpos + 27 + 24+offset].solid;
		check[2] = level->tiles[hero.hpos + 27 + 23+offset].solid;
		check[3] = level->tiles[hero.hpos + 27 + 22+offset].solid;
		check[4] = level->tiles[hero.hpos + 27 + 21+offset].solid;
		if(!check[0] && !check[1] && !check[2] && !check[3] && !check[4]) scroll_right();
		hero_ani = 1;
	}
}


static void rls_bullet() {
    Uint32 now = SDL_GetTicks();
    if (now - last_fire_ticks < (Uint32)FIRE_COOLDOWN_MS) {
        return; 
    }
    last_fire_ticks = now;
	if(hero.dir == 1)
	rls_particle(&emiter, offset+hero.hpos+24+24+1, 1, hero.dir);
	else
	rls_particle(&emiter, offset+hero.hpos-24+1, 1, hero.dir);
#ifdef HAS_MIXER
	if(fire_snd != 0)
	Mix_PlayChannel( -1, fire_snd, 0);
#endif
}

static int check_input() {
    const Uint8 *keys = SDL_GetKeyboardState(0);
    int axis_x = 0;
    
    
    if(stick != NULL) {
        axis_x = SDL_JoystickGetAxis(stick, 0); 
    }
    
    static int w = 0;
    
    if((keys[SDL_SCANCODE_A] || (stick && SDL_JoystickGetButton(stick, 0))) && jump_ok == 1 && jump == 0)
        jump = 1, shoot_ani = 0;
        
    if((keys[SDL_SCANCODE_S] || (stick && SDL_JoystickGetButton(stick, 1))) && jump_ok == 1 && jump == 0) {
        if(shoot_ani == 0) { shoot_ani = 1, hero.cur_ani = 5; }
    }
    
    
#ifdef FOR_PSP
    if(stick && SDL_JoystickGetButton(stick, 7)) {
#else
    if((keys[SDL_SCANCODE_LEFT] || (stick && axis_x < -8000))) {
#endif
        move_left();
        return 0;
    }
    
#ifdef FOR_PSP
    if(stick && SDL_JoystickGetButton(stick, 9)) {
#else
    if((keys[SDL_SCANCODE_RIGHT] || (stick && axis_x > 8000))) {
#endif
        move_right();
        return 0;
    }
    return 1;
}
static void collect_item(int type) {
	score += 10*type;
	#ifdef HAS_MIXER
	if(collect_snd != 0)
	Mix_PlayChannel( -1, collect_snd, 0);
	#endif
}
static void proc_collect() {
	Uint8 i;
	for(i = 0; i < 50; i++) {
		if(level->items[i].type != 0) {
			unsigned int z = 0;
			for(z = 0; z < 4; z++) {
			if((offset+hero.hpos+24+z == level->items[i].vpos) || (offset+hero.hpos-24-z == level->items[i].vpos) || (offset+hero.hpos+z == level->items[i].vpos) || offset+hero.hpos-z == level->items[i].vpos )
			{
				collect_item(level->items[i].type);
				level->items[i].type = 0;
				level->items[i].vpos = 0;
				return;
			}
			}
		}
	}
}

Uint32 proccess_game(Uint32 interval, void *p) {
	if(level == 0)
		return interval;
	int x = check_input();
	proc_collect();
	proc_particles(&emiter);
	if(jump == 0) {
		if((! level->tiles[hero.hpos+offset+4].solid )&& (!level->tiles[hero.hpos+offset+4+24].solid))
		{
			hero.hpos++;
			hero.cur_ani = 4;
		}
		else {
			if(shoot_ani == 0) {
				jump_ok = 1; jump = 0; 
				hero_ani = 1;
				hero.cur_ani = 0;
			}
		}
	}
	else {
		jump++;
		if(hero.hpos > 0 && !level->tiles[hero.hpos+offset-1].solid)
			hero.hpos--;
		hero.cur_ani = 4;
		if(jump > 8) {
			jump = 0;
			jump_ok = 0;
		}
	}
	if(x == 1)
		hero_ani = 0;
	return interval;
}
void logic() {
	 if(shoot_ani == 0 && hero_ani == 0 && jump_ok == 0 && jump == 0)
		hero.cur_ani = 4;
	else if(shoot_ani == 1 && hero_ani == 0 ) {
		 hero.cur_ani++;
		 if(hero.cur_ani > 8) {
			 rls_bullet();
			 hero.cur_ani = 0;
			 shoot_ani = 0;
			 hero_ani = 0;
		 }
	} else if(hero_ani == 1 && jump_ok == 0) { hero_ani = 0; }
}
void init_particles(Emiter *e) {
	memset(e, 0, sizeof(Emiter));
}
void game_over() {
	init_game();
}
static void hero_die() {
	Level *lvl = level;
	offset = 0;
	hero.hpos = level->start_pos;
	memset(&emiter, 0, sizeof(emiter));
	{
		unsigned int i = 0;
		memset(evil, 0, sizeof(evil));
		for(i = 0; i < 50; i++)
			if(lvl->grandma[i] != 0) {
				fill_evil(&evil[i], lvl->grandma[i], 0); 
			}
			else evil[i].type = -1;
	}
	lives--;
	hero_ani = 0;
	shoot_ani = 0;
	jump = 0;
	jump_ok = 1;
}
void proc_particles(Emiter *e) {
    unsigned int i = 0;
    static int lost_focus[50] = {0};
    for( i = 0; i < MAX_PARTICLE; i++) {
        if(e->p[i].type != 0) {
            if(e->p[i].vpos >= MAX_TILE-24 || e->p[i].vpos <= 24 ||  level->tiles[e->p[i].vpos].solid)
            {
                e->p[i].type = 0;
                continue;
            }
            if(e->p[i].dir == 0) {
                e->p[i].vpos -= 48;
            }
            else e->p[i].vpos += 48;
        }
    }
    for( i = 0; i < 50; i++ ) {
        Uint8 check[5];
        if(evil[i].type != -1) {
            if(!level->tiles[evil[i].vpos+4].solid)
                evil[i].vpos++;
        
        int horizontal_distance = abs((int)evil[i].vpos - (int)(hero.hpos + offset));
        
        int level_width = 24; 
        
        int enemy_tile_x = evil[i].vpos % level_width;
        int enemy_tile_y = evil[i].vpos / level_width;
        int hero_tile_x = (hero.hpos + offset) % level_width;
        int hero_tile_y = (hero.hpos + offset) / level_width;
        
        int vertical_distance = abs(enemy_tile_y - hero_tile_y);
        
        int is_aggressive = (horizontal_distance < 96) &&          
                           (vertical_distance < 3) &&  
                           !lost_focus[i]; 
        
        if(evil[i].die == 0) {
            evil[i].cur_ani ++;
            if(evil[i].cur_ani >= 5)
            evil[i].cur_ani = 0;
        } else {
            evil[i].cur_ani++;
            if(evil[i].cur_ani > 7) {
                evil[i].type = -1;
                lost_focus[i] = 0; 
#ifdef HAS_MIXER
                if(kill_snd != 0)
                    Mix_PlayChannel(-1, kill_snd, 0);
#endif
                break;
            }
        }
        
        int move_speed = is_aggressive ? 48 : 24; 
        
        if(evil[i].dir == 0) {
            check[0] = level->tiles[evil[i].vpos-move_speed].solid;
            check[1] = level->tiles[evil[i].vpos+1-move_speed].solid;
            check[2] = level->tiles[evil[i].vpos+2-move_speed].solid;
            check[3] = level->tiles[evil[i].vpos+3-move_speed].solid;
            check[4] = level->tiles[evil[i].vpos-move_speed-24].solid;
            if(!check[0] && !check[1] && !check[2] && !check[3] && !check[4]) 
                evil[i].vpos -= move_speed; 
            else {
                evil[i].dir = 1; 
                lost_focus[i] = 1; 
            }
        }
        else if(evil[i].dir == 1) {
            int check_pos = move_speed + 3;
            check[0] = level->tiles[evil[i].vpos + check_pos].solid;
            check[1] = level->tiles[evil[i].vpos + check_pos + 24].solid;
            check[2] = level->tiles[evil[i].vpos + check_pos + 23].solid;
            check[3] = level->tiles[evil[i].vpos + check_pos + 22].solid;
            check[4] = level->tiles[evil[i].vpos + check_pos + 21].solid;
            if(!check[0] && !check[1] && !check[2] && !check[3] && !check[4]) 
                evil[i].vpos += move_speed;
            else {
                evil[i].dir = 0; 
                lost_focus[i] = 0; 
            }
        }
        
        if(evil[i].type != -1 && hero.x > 0 && hero.y > 0 && evil[i].x > 0 && evil[i].y > 0)
        {
            int vertical_pixel_distance = abs(hero.y - evil[i].y);    
            if(vertical_pixel_distance < 32) 
            {
                SDL_Rect rcY = { evil[i].x + 2, evil[i].y + 2, evil[i].egfx->gfx[evil[i].cur_ani]->w - 4, evil[i].egfx->gfx[evil[i].cur_ani]->h - 4 };
                SDL_Rect rcX = { hero.x + 2, hero.y + 2, hgfx[hero.cur_ani]->w - 4, hgfx[hero.cur_ani]->h - 4 };
                if(SDL_Colide(&rcX, &rcY)) {
                    hero_die();
                    return;
                }
            }
        }
        
        {
            Uint8 p = 0;
            for( ; p < MAX_PARTICLE; p++ ) {
                if(e->p[p].type != 0) {
                    if(e->p[p].x > 0 && e->p[p].y > 0 && e->p[p].x < 640 && e->p[p].y < 480 && evil[i].x > 0 && evil[i].y > 0 && evil[i].x < 640 && evil[i].y < 480) {
                    SDL_Rect rcX = { e->p[p].x, e->p[p].y, particles[0]->w, particles[0]->h };
                    SDL_Rect rcY = { evil[i].x, evil[i].y, evil[i].egfx->gfx[evil[i].cur_ani]->w, evil[i].egfx->gfx[evil[i].cur_ani]->h };
                    if(SDL_Colide(&rcX, &rcY)) {
                        e->p[p].type = 0;
                        evil[i].die = 1;
                        evil[i].cur_ani = 5;
                        score++;
                    }
                    }
                }
            }
        }
        }
    }
}
static int get_off_particle(Emiter *e) {
	unsigned int i = 0;
	for ( i = 0; i < MAX_PARTICLE; i++ ) {
		if(e->p[i].type == 0)
		return (int)i;
	}
	return -1;
}
void rls_particle(Emiter *e, int vpos, int type, int dir) {
	int off = get_off_particle(e);
	if(off != -1) {
		e->p[off].type = type;
		e->p[off].vpos = vpos;
		e->p[off].dir = dir;
	}
}