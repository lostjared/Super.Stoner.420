#ifdef __EMSCRIPTEN__
#include<emscripten/emscripten.h>
#endif
#include "level.h"
#include "smx.h"
#ifdef HAS_MIXER
Mix_Chunk *intro_snd = 0, *collect_snd = 0, *fire_snd = 0, *kill_snd = 0;
Mix_Music *game_track;
#endif
struct SDL_Font *font = 0, *cfont = 0;
extern void render_start();
int cur_scr = ID_ENTER;
SDL_Surface *front = 0;
SDL_Surface *gfx[16], *hgfx[12], *particles[4], *bg, *collect[8], *lsd, *logo;
static const char *img_str[] = { "black.bmp", "grass.bmp", "bluebrick.bmp", "bluesky.bmp", "brick.bmp", "eblock.bmp", "red_brick.bmp", "sand1.bmp", "sand2.bmp",
    "snow.bmp", "stone.bmp", "stone2.bmp", "stone3.bmp", "stone4.bmp", "black.bmp",
    0 };
static const char *hstr[] = { "hero1.bmp", "hero2.bmp", "hero3.bmp", "hero4.bmp", "hero_jump1.bmp", "hero_shot1.bmp", "hero_shot2.bmp", "hero_shot3.bmp", "hero_shot4.bmp", 0 };
static const char *ev[] = { "img/grandma/", 0 };
SDL_Joystick *stick = 0;
Emiter emiter;
int custom_level = 0;
char custom_lvl[256];
int cur_level = 0;
int score = 0, lives = 0;
int active = 1;
int cur_levels = 0;
static const char *level_str[] = { "", "SuperMaster2/", 0 };
extern SDL_TimerID proc_game;

void reload_level() {
    char sbuf[256];
    if(cur_level >= 8)
        cur_level = 0;
#ifdef __EMSCRIPTEN__
    if(custom_level == 0)
        snprintf(sbuf,255,"/assets/%slevel/level%d.sml", level_str[cur_levels], ++cur_level);
#else
    if(custom_level == 0)
    snprintf(sbuf,255,"%slevel/level%d.sml", level_str[cur_levels], ++cur_level);

#endif
    else
        strcpy(sbuf, custom_lvl);
    if(level != 0) release_level(level);
    level = load_level(sbuf);
    hero.hpos = level->start_pos;
    srand((unsigned int) SDL_GetTicks() );
    {
        unsigned int i = 0;
        for(; i < 50; i++) {
            if(level->items[i].type != 0) do { level->items[i].type = rand()%COLLECT_NUM; } while( level->items[i].type == 0 );
        }
    }
    offset = 0;
    init_particles(&emiter);
    cur_scr = ID_ENTER;
}

SDL_TimerID check_in = 0;

void init_game() {
    score = 0, lives = 10;
    cur_level = 0;
    reload_level();
    cur_scr = ID_START;
    check_in = SDL_AddTimer(225, check_start_in, 0);
}
static void init() {
    Uint8 i = 0;
    font = SDL_InitFont(get_path("D:\\", "font/system.mxf"));
    cfont = SDL_InitFont(get_path("D:\\", "font/e.mxf"));
    init_game();
    SDL_AddTimer(1000, intro_wait, 0);
    particles[0] = SDL_LoadBMP(get_path("D:\\", "img/shot.bmp"));
    lsd = SDL_LoadBMP(get_path("D:\\", "img/lsd.bmp"));
    logo = SDL_LoadBMP(get_path("D:\\", "img/logo.bmp"));
    for(i = 0; img_str[i] != 0; i++) {
        static char sbuf[256];
        snprintf(sbuf,255, "img/%s", img_str[i]);
        gfx[i] = SDL_LoadBMP(get_path("D:\\", sbuf));
        if(!gfx[i])
            fprintf(stderr, "Error couldnt load graphic %s\n", sbuf);
    }
    for(i = 0; hstr[i] != 0; i++) {
        static char sbuf[256];
        snprintf(sbuf,255, "img/hero/%s", hstr[i]);
        hgfx[i] = SDL_LoadBMP(get_path("D:\\", sbuf));
        if(!hgfx[i])
            fprintf(stderr, "Error couldnt load graphic %s\n", sbuf);
    }
    for(i = 0; i < COLLECT_NUM; i++) {
        static char sbuf[256];
        snprintf(sbuf,255, "img/col%d.bmp", i+1);
        collect[i] = SDL_LoadBMP(get_path("D:\\", sbuf));
        if(!collect[i])
            fprintf(stderr, "Error couldnt load graphic %s\n", sbuf);
    }
    {
        for( i = 0; ev[i] != 0; i++ ) {
            unsigned int z;
            for( z = 0; z < 10; z++ ) {
                static char sbuf[256];
                memset(sbuf, 0, sizeof(sbuf));
                snprintf(sbuf,255,"%sevil%d.bmp", ev[i], z+1);
                evil_gfx[i].gfx[z] = SDL_LoadBMP(get_path("D:\\", sbuf));
                if(!evil_gfx[i].gfx[z])
                    fprintf(stderr, "Couldnt load %s ", sbuf);
            }
            evil_gfx[i].type = i;
        }
    }
    bg = SDL_LoadBMP(get_path("D:\\", "img/bg.bmp"));
#ifdef HAS_MIXER
    Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 4096);
    intro_snd = Mix_LoadWAV(get_path("D:\\", "snd/open.wav"));
    collect_snd = Mix_LoadWAV(get_path("D:\\", "snd/line.wav"));
    fire_snd = Mix_LoadWAV(get_path("D:\\", "snd/fire.wav"));
    kill_snd = Mix_LoadWAV(get_path("D:\\", "snd/scream.wav"));
#endif
#ifdef HAS_MIXER
    if(intro_snd != 0)
        Mix_PlayChannel( -1, intro_snd, 0);
#endif
}
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *tex;

extern void cleanup_all_timers();


static void rls() {

    cleanup_all_timers();

    Uint8 i = 0, z = 0;
    
    for( i = 0; img_str[i] != 0; i++)
        SDL_FreeSurface(gfx[i]);
    
        for( i = 0; hstr[i] != 0; i++)
        SDL_FreeSurface(hgfx[i]);
    
        for ( i = 0; ev[i] != 0; i++)
        for(z = 0; z < 10; z++)
            SDL_FreeSurface(evil_gfx[i].gfx[z]);
    
    for(i = 0; i < COLLECT_NUM; i++)
        SDL_FreeSurface(collect[i]);


    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(front);
    SDL_DestroyTexture(tex);
    SDL_DestroyWindow(window);
    SDL_FreeSurface(bg);
    SDL_FreeSurface(particles[0]);
    SDL_FreeSurface(lsd);
    SDL_FreeSurface(logo);
    SDL_FreeFont(cfont);
    SDL_FreeFont(font);
    release_level(level);
#ifdef HAS_MIXER
    Mix_FreeChunk(fire_snd);
    Mix_FreeChunk(collect_snd);
    Mix_FreeChunk(intro_snd);
    Mix_FreeChunk(kill_snd);
    Mix_HaltMusic();
    Mix_CloseAudio();
    Mix_Quit();
#endif
}
static void render() {
    switch(cur_scr) {
        case ID_GAME:
            render_map(front, level);
            break;
        case ID_ENTER:
            render_enter_level();
            break;
        case ID_START:
            render_start();
            break;
        case ID_CREDITS:
            render_credits();
            break;
        case ID_PAUSED:
            render_pause();
            break;
    }
}

extern void handleInput(SDL_Event *e);

void handleInputEvent(SDL_Event *e) {
    if(cur_scr == ID_START) {
        handleInput(e);
    }
}

static SDL_Event e;
int WIDTH=1440, HEIGHT=1080;

void eventPump() { 
    SDL_LockTexture(tex, 0, &front->pixels, &front->pitch);
    render();
    SDL_UnlockTexture(tex);
    if(SDL_PollEvent(&e)) {
        handleInputEvent(&e);
        switch(e.type) {
            case SDL_QUIT:
                active = 0;
                break;
            case SDL_KEYDOWN:
            {
                switch(e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        active = 0;
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    default:
                        break;
                }
            }
                break;
        case SDL_JOYDEVICEADDED:
            stick = SDL_JoystickOpen(e.cdevice.which);
            if(stick != NULL)
                printf("smx: Sucessfully initalied Joystick\n");
        break;

        case SDL_JOYDEVICEREMOVED:
            SDL_JoystickClose(stick);
            stick = NULL;
            printf("smx: Joystick closed..\n");
            break;
        }
   }
        
    SDL_Rect dst = { 0, 0, WIDTH, HEIGHT };
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tex, 0, &dst);
    SDL_RenderPresent(renderer);
    SDL_Delay(10);
}

#ifdef FOR_XBOX_OPENXDK
void XBoxStartup() {
    char **argv = 0;
    int argc = 0;
#else
    int main(int argc, char **argv) {
#endif
        Uint32 mode = 0;
        SDL_Surface *ico = 0;
        int full = 0;

        if(argc == 4 && strcmp(argv[1], "--size") == 0) {
            WIDTH = atoi(argv[2]);
            HEIGHT = atoi(argv[3]);
        }
        if(argc == 4  && strcmp(argv[1],"--full") == 0) {
            full = 1;
            WIDTH = atoi(argv[2]);
            HEIGHT = atoi(argv[3]);
        }
        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
            return -1;
#ifdef FOR_PSP
        scePowerSetClockFrequency(333, 333, 166); 
#endif
        SDL_DisplayMode current;
        if(SDL_GetCurrentDisplayMode(0, &current) != 0) {
            fprintf(stderr, "Error could not get display mode: %s", SDL_GetError());
            SDL_Quit();
            exit(-1);
        }


        current.w = 640;
        current.h = 480;
        SDL_ShowCursor(SDL_FALSE);
        ico = SDL_LoadBMP(get_path("D:\\", "img/col1.bmp"));
	if(ico == NULL) {
		fprintf(stderr, "Error loading icon, wrong path place this program in the directory with the resources.\n");
		SDL_Quit();
		return EXIT_FAILURE;
	}
        window = SDL_CreateWindow("Super Stoner 420", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
        if(!window) {
            fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
            SDL_Quit();
            exit(-1);
        }
	    SDL_SetWindowIcon(window, ico);
	    SDL_FreeSurface(ico);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if(!renderer) {
            fprintf(stderr, "Error creating Renderer: %s\n", SDL_GetError());
            SDL_Quit();
            exit(-1);
        }

        if(full == 1) {
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
        }

        SDL_JoystickEventState(SDL_ENABLE);

        if(SDL_NumJoysticks() > 0)
            printf("smx: %d Joysticks Available\n", SDL_NumJoysticks());
        else if(SDL_NumJoysticks() == 0)
            printf("smx: 0 joysticks avilable..\n");

        stick = SDL_JoystickOpen(0);

        if(stick != NULL)
            printf("smx: Joystick initalized.\n");

        fflush(stdout);
        tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
        if(!tex) {
            fprintf(stderr, "Error creating texture: %s", SDL_GetError());
            SDL_Quit();
            exit(-1);
        }
        front = SDL_CreateRGBSurfaceFrom(NULL, 640, 480, 32, 640*4, 0x00FF0000, 0x0000FF00,0x000000FF,0xFF000000);
        if(!front) {
            fprintf(stderr, "Error creating surface: %s", SDL_GetError());
            SDL_Quit();
            exit(-1);
        }



        if(argc == 3 && strcmp(argv[1], "--run") == 0)
        {
            memset(custom_lvl, 0, sizeof(custom_lvl));
            strcpy(custom_lvl, argv[2]);
            { 
                FILE *fptr = fopen(custom_lvl, "r");
                if(!fptr)
                {
                    custom_level = 0;
                    fprintf(stderr, "Error level map %s not found!", custom_lvl);
                } else {
                    custom_level = 1;
                    fclose(fptr);
                }
            }
        }
        init();
        printf("smx: initialized\n");
        {
#ifndef __EMSCRIPTEN__
            while(active == 1) {
                eventPump();
            }
#else
            emscripten_set_main_loop(eventPump, 0, 1);
#endif
        }
        rls();
        SDL_JoystickClose(stick);
        SDL_Quit();
        printf("smx: exit\n");
        return 0;
    }
    void SDL_ReverseBlt(SDL_Surface *surf, SDL_Rect *rc, SDL_Surface *front_surf, SDL_Rect *rc2, Uint32 transparent) {
        void *buf , *buf2;
        int i,z,i2,z2;
        buf = lock(surf, surf->format->BitsPerPixel);
        buf2 = lock(front_surf, front_surf->format->BitsPerPixel);
        i2 = rc2->x;
        z2 = rc2->y;
        for(i = rc->w-1; i > 0; i--) {
            for(z = 0; z < rc->h; z++) {
                SDL_Color col;
                Uint32 color = getpixel(surf, i, z, surf->format->BitsPerPixel, surf->pitch, &col);
                if(color != transparent)
                    setpixel(buf2, i2, z2, SDL_MapRGB(front_surf->format, col.r, col.g, col.b), front_surf->format->BitsPerPixel, front_surf->pitch);
                z2++;
            }
            z2 = rc2->y;
            i2++;
        }
        unlock(surf);
        unlock(front_surf);
    }
    int SDL_Colide(SDL_Rect *rc, SDL_Rect *rc2) {
        int i,z;
        if(!(rc->x > 0 && rc->x+rc->w < 640 && rc->y > 0 && rc->y+rc->h < 480))
            return 0;
        for( i = rc->x; i < rc->x+rc->w; i++) {
            for(z = rc->y; z < rc->y+rc->h; z++) {
                if(i >= rc2->x && i <= rc2->x+rc2->w && z >= rc2->y && z <= rc2->y+rc2->h) return 1;
            }
        }
        return 0;
    }
    char *get_path(const char *p, const char *s) {
        static char sbuf[4096];
#ifdef FOR_XBOX_XDK
        snprintf(sbuf, 4095, "%s%s", p, s);
        return sbuf;
#endif
#ifdef __EMSCRIPTEN__
        snprintf(sbuf, 4095, "/assets/%s", s);
        return sbuf;
#endif
        snprintf(sbuf,4095, "%s", s);
        return sbuf;
    }
