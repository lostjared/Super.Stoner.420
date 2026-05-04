#ifdef __EMSCRIPTEN__
#include<emscripten/emscripten.h>
#include<GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif
#include <time.h>
#include <math.h>
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
SDL_GameController *controller = 0;
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

enum {
    GAME_WIDTH = 640,
    GAME_HEIGHT = 480
};

int controller_button(SDL_GameControllerButton button) {
    return controller && SDL_GameControllerGetButton(controller, button);
}

Sint16 controller_axis(SDL_GameControllerAxis axis) {
    if (!controller)
        return 0;
    return SDL_GameControllerGetAxis(controller, axis);
}

static void close_controller(void) {
    if (controller) {
        SDL_GameControllerClose(controller);
        controller = 0;
    }
    stick = 0;
}

static int open_controller(int device_index) {
    close_controller();

    if (!SDL_IsGameController(device_index)) {
        fprintf(stderr, "smx: device %d is not a mapped game controller\n", device_index);
        return -1;
    }

    controller = SDL_GameControllerOpen(device_index);
    if (!controller) {
        fprintf(stderr, "smx: failed to open game controller %d: %s\n", device_index, SDL_GetError());
        return -1;
    }

    stick = SDL_GameControllerGetJoystick(controller);
    printf("smx: initialized controller: %s\n", SDL_GameControllerName(controller));
    return 0;
}

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
    font = SDL_InitFont(get_path("D:\\", "font/DejaVuSans.ttf"), 12);
    cfont = SDL_InitFont(get_path("D:\\", "font/DejaVuSans.ttf"), 16);
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
SDL_GLContext gl_ctx;
GLuint gl_program = 0;
GLuint gl_fx_program = 0;
GLuint gl_fx_type5_program = 0;
GLuint gl_fx_type0_program = 0;
GLuint gl_fx_item5_program = 0;
GLuint gl_fx_mix_program = 0;
GLuint gl_vbo = 0;
GLuint gl_vao = 0;
GLuint gl_tex = 0;
GLuint gl_pingpong_tex[2] = { 0, 0 };
GLuint gl_pingpong_fbo[2] = { 0, 0 };
GLint gl_fx_time_loc = -1;
GLint gl_fx_resolution_loc = -1;
GLint gl_fx_samp_loc = -1;
GLint gl_fx_wobble_loc = -1;
GLint gl_program_flip_y_loc = -1;
GLint gl_flip_y_loc = -1;
GLint gl_fx_type5_time_loc = -1;
GLint gl_fx_type5_resolution_loc = -1;
GLint gl_fx_type5_samp_loc = -1;
GLint gl_fx_type5_flip_y_loc = -1;
GLint gl_fx_type0_time_loc = -1;
GLint gl_fx_type0_resolution_loc = -1;
GLint gl_fx_type0_samp_loc = -1;
GLint gl_fx_type0_flip_y_loc = -1;
GLint gl_fx_item5_samp_loc = -1;
GLint gl_fx_item5_time_loc = -1;
GLint gl_fx_item5_flip_y_loc = -1;
GLint gl_fx_mix_base_loc = -1;
GLint gl_fx_mix_effect_loc = -1;
GLint gl_fx_mix_amount_loc = -1;
GLint gl_fx_mix_flip_y_loc = -1;
int viewport_x = 0;
int viewport_y = 0;
int viewport_w = GAME_WIDTH;
int viewport_h = GAME_HEIGHT;

#define COLLECT_FX_DURATION_MS      8000U
#define COLLECT_FX_FADE_MS           900U
#define COLLECT_FX_WOBBLE_BASE      0.012f
#define COLLECT_FX_WOBBLE_INCREMENT 0.008f
#define COLLECT_FX_WOBBLE_MAX       0.072f
#define COLLECT_FX_MAX_STACK        8

enum {
    COLLECT_SHADER_FX_NONE = 0,
    COLLECT_SHADER_FX_WOBBLE = 1,
    COLLECT_SHADER_FX_KALEIDO = 2,
    COLLECT_SHADER_FX_POSTERIZE = 3,
    COLLECT_SHADER_FX_BLURSHIFT = 5
};

typedef struct CollectShaderEffect {
    int type;
    Uint32 timeout_ticks;
    float wobble_intensity;
} CollectShaderEffect;

CollectShaderEffect collect_fx_stack[COLLECT_FX_MAX_STACK];
int collect_fx_count = 0;

static int map_collect_effect_type(int item_type) {
    if (item_type == 4)
        return COLLECT_SHADER_FX_BLURSHIFT;
    if (item_type == 1)
        return COLLECT_SHADER_FX_KALEIDO;
    if (item_type == 5)
        return COLLECT_SHADER_FX_POSTERIZE;
    if (item_type >= 2 && item_type <= 3)
        return COLLECT_SHADER_FX_WOBBLE;
    return COLLECT_SHADER_FX_NONE;
}

static GLuint collect_effect_program(int effect_type) {
    if (effect_type == COLLECT_SHADER_FX_BLURSHIFT)
        return gl_fx_type5_program;
    if (effect_type == COLLECT_SHADER_FX_KALEIDO)
        return gl_fx_type0_program;
    if (effect_type == COLLECT_SHADER_FX_POSTERIZE)
        return gl_fx_item5_program;
    return gl_fx_program;
}

static void prune_collect_shader_effects(Uint32 now) {
    int read_index = 0;
    int write_index = 0;

    for (; read_index < collect_fx_count; read_index++) {
        if ((Sint32)(collect_fx_stack[read_index].timeout_ticks - now) > 0) {
            if (write_index != read_index)
                collect_fx_stack[write_index] = collect_fx_stack[read_index];
            write_index++;
        }
    }

    collect_fx_count = write_index;
}

static void bind_effect_uniforms(GLuint program, float wallclock_seconds, float wobble_intensity, float resolution_w, float resolution_h, int flip_y) {
    glUseProgram(program);

    if (program == gl_fx_program) {
        if (gl_fx_samp_loc >= 0)
            glUniform1i(gl_fx_samp_loc, 0);
        if (gl_fx_time_loc >= 0)
            glUniform1f(gl_fx_time_loc, wallclock_seconds);
        if (gl_fx_resolution_loc >= 0)
            glUniform2f(gl_fx_resolution_loc, resolution_w, resolution_h);
        if (gl_fx_wobble_loc >= 0)
            glUniform1f(gl_fx_wobble_loc, wobble_intensity);
        if (gl_flip_y_loc >= 0)
            glUniform1f(gl_flip_y_loc, flip_y ? 1.0f : 0.0f);
    } else if (program == gl_fx_type5_program) {
        if (gl_fx_type5_samp_loc >= 0)
            glUniform1i(gl_fx_type5_samp_loc, 0);
        if (gl_fx_type5_time_loc >= 0)
            glUniform1f(gl_fx_type5_time_loc, wallclock_seconds);
        if (gl_fx_type5_resolution_loc >= 0)
            glUniform2f(gl_fx_type5_resolution_loc, resolution_w, resolution_h);
        if (gl_fx_type5_flip_y_loc >= 0)
            glUniform1f(gl_fx_type5_flip_y_loc, flip_y ? 1.0f : 0.0f);
    } else if (program == gl_fx_type0_program) {
        if (gl_fx_type0_samp_loc >= 0)
            glUniform1i(gl_fx_type0_samp_loc, 0);
        if (gl_fx_type0_time_loc >= 0)
            glUniform1f(gl_fx_type0_time_loc, wallclock_seconds);
        if (gl_fx_type0_resolution_loc >= 0)
            glUniform2f(gl_fx_type0_resolution_loc, resolution_w, resolution_h);
        if (gl_fx_type0_flip_y_loc >= 0)
            glUniform1f(gl_fx_type0_flip_y_loc, flip_y ? 1.0f : 0.0f);
    } else if (program == gl_fx_item5_program) {
        if (gl_fx_item5_samp_loc >= 0)
            glUniform1i(gl_fx_item5_samp_loc, 0);
        if (gl_fx_item5_time_loc >= 0)
            glUniform1f(gl_fx_item5_time_loc, wallclock_seconds);
        if (gl_fx_item5_flip_y_loc >= 0)
            glUniform1f(gl_fx_item5_flip_y_loc, flip_y ? 1.0f : 0.0f);
    } else if (program == gl_program) {
        if (gl_program_flip_y_loc >= 0)
            glUniform1f(gl_program_flip_y_loc, flip_y ? 1.0f : 0.0f);
    }
}

static void render_texture_pass(GLuint program, GLuint source_texture, GLuint target_fbo, int target_viewport_x, int target_viewport_y, int target_viewport_w, int target_viewport_h, float wallclock_seconds, float wobble_intensity, float resolution_w, float resolution_h) {
    int flip_y = (source_texture == gl_tex) ? 0 : 1;

    glBindFramebuffer(GL_FRAMEBUFFER, target_fbo);
    glViewport(target_viewport_x, target_viewport_y, target_viewport_w, target_viewport_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, source_texture);
    bind_effect_uniforms(program, wallclock_seconds, wobble_intensity, resolution_w, resolution_h, flip_y);

    glBindVertexArray(gl_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

static void render_mix_pass(GLuint base_texture, GLuint effect_texture, GLuint target_fbo, int target_viewport_x, int target_viewport_y, int target_viewport_w, int target_viewport_h, float mix_amount) {
    int flip_y = (base_texture == gl_tex) ? 0 : 1;

    glUseProgram(gl_fx_mix_program);
    glBindFramebuffer(GL_FRAMEBUFFER, target_fbo);
    glViewport(target_viewport_x, target_viewport_y, target_viewport_w, target_viewport_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, base_texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, effect_texture);

    if (gl_fx_mix_base_loc >= 0)
        glUniform1i(gl_fx_mix_base_loc, 0);
    if (gl_fx_mix_effect_loc >= 0)
        glUniform1i(gl_fx_mix_effect_loc, 1);
    if (gl_fx_mix_amount_loc >= 0)
        glUniform1f(gl_fx_mix_amount_loc, mix_amount);
    if (gl_fx_mix_flip_y_loc >= 0)
        glUniform1f(gl_fx_mix_flip_y_loc, flip_y ? 1.0f : 0.0f);

    glBindVertexArray(gl_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void activate_collect_shader_effect(int item_type) {
    CollectShaderEffect effect;
    int new_type = map_collect_effect_type(item_type);
    int wobble_count = 0;
    int index = 0;

    if (new_type == COLLECT_SHADER_FX_NONE)
        return;

    for (; index < collect_fx_count; index++) {
        if (collect_fx_stack[index].type == COLLECT_SHADER_FX_WOBBLE)
            wobble_count++;
    }

    effect.type = new_type;
    effect.timeout_ticks = SDL_GetTicks() + COLLECT_FX_DURATION_MS;
    effect.wobble_intensity = COLLECT_FX_WOBBLE_BASE;

    if (new_type == COLLECT_SHADER_FX_WOBBLE) {
        effect.wobble_intensity += (float)wobble_count * COLLECT_FX_WOBBLE_INCREMENT;
        if (effect.wobble_intensity > COLLECT_FX_WOBBLE_MAX)
            effect.wobble_intensity = COLLECT_FX_WOBBLE_MAX;
    }

    if (collect_fx_count >= COLLECT_FX_MAX_STACK) {
        memmove(collect_fx_stack, collect_fx_stack + 1, sizeof(collect_fx_stack[0]) * (COLLECT_FX_MAX_STACK - 1));
        collect_fx_count = COLLECT_FX_MAX_STACK - 1;
    }

    collect_fx_stack[collect_fx_count++] = effect;
}

void reset_collect_shader_effect() {
    collect_fx_count = 0;
}

static GLuint compile_shader(GLenum shader_type, const char *src) {
    GLuint shader = glCreateShader(shader_type);
    GLint compiled = GL_FALSE;
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        char logbuf[1024];
        GLsizei len = 0;
        glGetShaderInfoLog(shader, (GLsizei)sizeof(logbuf), &len, logbuf);
        fprintf(stderr, "Shader compile error: %s\n", logbuf);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint create_program(const char *vs_src, const char *fs_src) {
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    GLuint program = 0;
    GLint linked = GL_FALSE;

    if (!vs || !fs)
        return 0;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    glDeleteShader(vs);
    glDeleteShader(fs);

    if (linked != GL_TRUE) {
        char logbuf[1024];
        GLsizei len = 0;
        glGetProgramInfoLog(program, (GLsizei)sizeof(logbuf), &len, logbuf);
        fprintf(stderr, "Program link error: %s\n", logbuf);
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static int init_gl_renderer() {
    static const char *vs_src =
        "#version 300 es\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "layout(location = 1) in vec2 a_uv;\n"
        "uniform float flip_y;\n"
        "out vec2 v_uv;\n"
        "void main() {\n"
        "  v_uv = vec2(a_uv.x, mix(a_uv.y, 1.0 - a_uv.y, flip_y));\n"
        "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
        "}\n";

    static const char *fs_src =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D u_tex;\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "  frag_color = vec4(texture(u_tex, v_uv).rgb, 1.0);\n"
        "}\n";

    static const char *fs_mix_src =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 color;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D base_samp;\n"
        "uniform sampler2D effect_samp;\n"
        "uniform float mix_amount;\n"
        "void main(void) {\n"
        "  vec3 base_col = texture(base_samp, v_uv).rgb;\n"
        "  vec3 effect_col = texture(effect_samp, v_uv).rgb;\n"
        "  color = vec4(mix(base_col, effect_col, clamp(mix_amount, 0.0, 1.0)), 1.0);\n"
        "}\n";

    static const char *fs_fx_src =
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 color;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D samp;\n"
        "uniform float time_f;\n"
        "uniform vec2 iResolution;\n"
        "uniform float wobble_intensity;\n"
        "vec3 hueRotate(vec3 c, float a) {\n"
        "  const mat3 toYIQ = mat3(\n"
        "    0.299, 0.596, 0.211,\n"
        "    0.587, -0.274, -0.523,\n"
        "    0.114, -0.322, 0.312\n"
        "  );\n"
        "  const mat3 toRGB = mat3(\n"
        "    1.0, 1.0, 1.0,\n"
        "    0.956, -0.272, -1.106,\n"
        "    0.621, -0.647, 1.703\n"
        "  );\n"
        "  vec3 yiq = toYIQ * c;\n"
        "  float ca = cos(a), sa = sin(a);\n"
        "  yiq.yz = mat2(ca, -sa, sa, ca) * yiq.yz;\n"
        "  return toRGB * yiq;\n"
        "}\n"
        "void main(void) {\n"
        "  vec2 uv = v_uv;\n"
        "  uv.x += sin(uv.y * 14.0 + time_f * 1.7) * wobble_intensity;\n"
        "  uv.y += sin(uv.x * 11.0 + time_f * 1.3) * wobble_intensity;\n"
        "  vec3 c = texture(samp, uv).rgb;\n"
        "  c = hueRotate(c, time_f * 0.8);\n"
        "  float lum = dot(c, vec3(0.299, 0.587, 0.114));\n"
        "  c = mix(vec3(lum), c, 1.7);\n"
        "  color = vec4(clamp(c, 0.0, 1.5), 1.0);\n"
        "}\n";

    static const char *fs_fx_type5_src =
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 color;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D samp;\n"
        "uniform float time_f;\n"
        "uniform vec2 iResolution;\n"
        "const float PI = 3.14159265;\n"
        "vec3 hypno(float t) {\n"
        "  return 0.5 + 0.5 * cos(6.28318 * (t * 1.5 + vec3(0.0, 0.25, 0.5)));\n"
        "}\n"
        "void main(void) {\n"
        "  float t = time_f;\n"
        "  float bass = 0.5 + 0.5 * sin(t * 0.90);\n"
        "  float mid = 0.5 + 0.5 * sin(t * 1.15 + 1.1);\n"
        "  float hiMid = 0.5 + 0.5 * sin(t * 1.55 + 2.4);\n"
        "  float treble = 0.5 + 0.5 * sin(t * 2.20 + 0.6);\n"
        "  float ampPeak = 0.5 + 0.5 * sin(t * 0.75 + 0.8);\n"
        "  float ampSmooth = 0.5 + 0.5 * sin(t * 0.45);\n"
        "  float aspect = iResolution.x / iResolution.y;\n"
        "  vec2 p = (v_uv - 0.5) * 2.0;\n"
        "  p.x *= aspect;\n"
        "  float d = length(p);\n"
        "  float sphereRadius = 1.0 + bass * 0.3;\n"
        "  float z = sqrt(max(0.0, sphereRadius * sphereRadius - d * d));\n"
        "  float fisheye = atan(d, z) / (PI * 0.5);\n"
        "  vec2 sphereUV = (d > 0.0) ? (p / d) * fisheye : vec2(0.0);\n"
        "  sphereUV.x /= aspect;\n"
        "  sphereUV = sphereUV * 0.5 + 0.5;\n"
        "  float ringFreq = 15.0 + hiMid * 12.0;\n"
        "  float rings = sin(d * ringFreq - t * 3.0 - bass * 8.0);\n"
        "  sphereUV += (p / (d + 0.01)) * rings * 0.015 * (1.0 + mid);\n"
        "  sphereUV = abs(fract(sphereUV * 0.5 + 0.5) * 2.0 - 1.0);\n"
        "  float chroma = treble * 0.05;\n"
        "  float bandAngle = atan(p.y, p.x);\n"
        "  vec2 chromaDir = vec2(cos(bandAngle), sin(bandAngle)) * chroma;\n"
        "  vec3 col;\n"
        "  col.r = texture(samp, sphereUV + chromaDir).r;\n"
        "  col.g = texture(samp, sphereUV).g;\n"
        "  col.b = texture(samp, sphereUV - chromaDir).b;\n"
        "  vec3 bandColor = hypno(d * 2.0 - t * 0.3 + bass);\n"
        "  float bandMask = rings * 0.5 + 0.5;\n"
        "  col = mix(col, col * bandColor, bandMask * (0.3 + mid * 0.25));\n"
        "  vec3 normal = normalize(vec3(p, z));\n"
        "  float lightAngle = t * 0.5 + mid * 3.0;\n"
        "  vec3 lightDir = normalize(vec3(sin(lightAngle), cos(lightAngle), 1.0));\n"
        "  float diff = max(dot(normal, lightDir), 0.0);\n"
        "  float spec = pow(max(dot(reflect(-lightDir, normal), vec3(0.0, 0.0, 1.0)), 0.0), 16.0);\n"
        "  col *= 0.6 + diff * 0.4;\n"
        "  col += spec * 0.3 * (1.0 + ampPeak * 2.0);\n"
        "  float coreGlow = exp(-d * 4.0) * (1.0 + bass * 1.2);\n"
        "  col += vec3(1.0, 0.95, 0.85) * coreGlow * 0.2;\n"
        "  col *= 0.85 + ampSmooth * 0.35;\n"
        "  col = mix(col, vec3(1.0) - col, smoothstep(0.93, 1.0, ampPeak));\n"
        "  color = vec4(col, 1.0);\n"
        "}\n";

    static const char *fs_fx_type0_src =
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 color;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D samp;\n"
        "uniform vec2 iResolution;\n"
        "uniform float time_f;\n"
        "const float PI = 3.1415926535897932384626433832795;\n"
        "float pingPong(float x, float len) {\n"
        "  float m = mod(x, len * 2.0);\n"
        "  return m <= len ? m : len * 2.0 - m;\n"
        "}\n"
        "vec2 rotateUV(vec2 uv, float angle, vec2 c, float aspect) {\n"
        "  float s = sin(angle), cc = cos(angle);\n"
        "  vec2 p = uv - c;\n"
        "  p.x *= aspect;\n"
        "  p = mat2(cc, -s, s, cc) * p;\n"
        "  p.x /= aspect;\n"
        "  return p + c;\n"
        "}\n"
        "vec2 reflectUV(vec2 uv, float segments, vec2 c, float aspect) {\n"
        "  vec2 p = uv - c;\n"
        "  p.x *= aspect;\n"
        "  float ang = atan(p.y, p.x);\n"
        "  float rad = length(p);\n"
        "  float stepA = 6.28318530718 / segments;\n"
        "  ang = mod(ang, stepA);\n"
        "  ang = abs(ang - stepA * 0.5);\n"
        "  vec2 r = vec2(cos(ang), sin(ang)) * rad;\n"
        "  r.x /= aspect;\n"
        "  return r + c;\n"
        "}\n"
        "vec2 fractalFold(vec2 uv, float zoom, float t, vec2 c, float aspect) {\n"
        "  vec2 p = uv;\n"
        "  for (int i = 0; i < 6; i++) {\n"
        "    p = abs((p - c) * (zoom + 0.15 * sin(t * 0.35 + float(i)))) - 0.5 + c;\n"
        "    p = rotateUV(p, t * 0.12 + float(i) * 0.07, c, aspect);\n"
        "  }\n"
        "  return p;\n"
        "}\n"
        "vec3 neonPalette(float t) {\n"
        "  vec3 pink  = vec3(1.0, 0.15, 0.75);\n"
        "  vec3 blue  = vec3(0.10, 0.55, 1.0);\n"
        "  vec3 green = vec3(0.10, 1.00, 0.45);\n"
        "  float ph = fract(t * 0.08);\n"
        "  vec3 k1 = mix(pink,  blue,  smoothstep(0.00, 0.33, ph));\n"
        "  vec3 k2 = mix(blue,  green, smoothstep(0.33, 0.66, ph));\n"
        "  vec3 k3 = mix(green, pink,  smoothstep(0.66, 1.00, ph));\n"
        "  float a = step(ph, 0.33);\n"
        "  float b = step(0.33, ph) * step(ph, 0.66);\n"
        "  float cv = step(0.66, ph);\n"
        "  return normalize(a * k1 + b * k2 + cv * k3) * 1.05;\n"
        "}\n"
        "vec3 softTone(vec3 c) {\n"
        "  c = pow(max(c, vec3(0.0)), vec3(0.95));\n"
        "  float l = dot(c, vec3(0.299, 0.587, 0.114));\n"
        "  c = mix(vec3(l), c, 0.9);\n"
        "  return clamp(c, 0.0, 1.0);\n"
        "}\n"
        "vec3 tentBlur3(sampler2D img, vec2 uv, vec2 res) {\n"
        "  vec2 ts = 1.0 / res;\n"
        "  vec3 s00 = textureGrad(img, uv + ts * vec2(-1.0,-1.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s10 = textureGrad(img, uv + ts * vec2( 0.0,-1.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s20 = textureGrad(img, uv + ts * vec2( 1.0,-1.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s01 = textureGrad(img, uv + ts * vec2(-1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s11 = textureGrad(img, uv,                        dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s21 = textureGrad(img, uv + ts * vec2( 1.0, 0.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s02 = textureGrad(img, uv + ts * vec2(-1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s12 = textureGrad(img, uv + ts * vec2( 0.0, 1.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  vec3 s22 = textureGrad(img, uv + ts * vec2( 1.0, 1.0), dFdx(uv), dFdy(uv)).rgb;\n"
        "  return (s00 + 2.0*s10 + s20 + 2.0*s01 + 4.0*s11 + 2.0*s21 + s02 + 2.0*s12 + s22) / 16.0;\n"
        "}\n"
        "vec3 preBlendColor(vec2 uv) {\n"
        "  vec3 tex = tentBlur3(samp, uv, iResolution);\n"
        "  float aspect = iResolution.x / iResolution.y;\n"
        "  vec2 p = (uv - 0.5) * vec2(aspect, 1.0);\n"
        "  float r = length(p);\n"
        "  float t = time_f;\n"
        "  vec3 neon = neonPalette(t + r * 1.3);\n"
        "  float neonAmt = smoothstep(0.1, 0.8, r);\n"
        "  neonAmt = 0.3 + 0.4 * (1.0 - neonAmt);\n"
        "  vec3 grad = mix(tex, neon, neonAmt);\n"
        "  grad = mix(grad, tex, 0.2);\n"
        "  grad = softTone(grad);\n"
        "  return grad;\n"
        "}\n"
        "float diamondRadius(vec2 p) {\n"
        "  p = sin(abs(p));\n"
        "  return max(p.x, p.y);\n"
        "}\n"
        "vec2 diamondFold(vec2 uv, vec2 c, float aspect) {\n"
        "  vec2 p = (uv - c) * vec2(aspect, 1.0);\n"
        "  p = abs(p);\n"
        "  if (p.y > p.x) p = p.yx;\n"
        "  p.x /= aspect;\n"
        "  return p + c;\n"
        "}\n"
        "void main(void) {\n"
        "  vec4 baseTex = texture(samp, v_uv);\n"
        "  vec2 uv = v_uv * 2.0 - 1.0;\n"
        "  float aspect = iResolution.x / iResolution.y;\n"
        "  uv.x *= aspect;\n"
        "  float r = pingPong(sin(length(uv) * time_f), 5.0);\n"
        "  float radius = sqrt(aspect * aspect + 1.0) + 0.5;\n"
        "  float glow = smoothstep(radius, radius - 0.25, r);\n"
        "  vec2 m = vec2(0.5);\n"
        "  vec2 ar = vec2(aspect, 1.0);\n"
        "  vec3 baseCol = preBlendColor(v_uv);\n"
        "  float seg = 4.0 + 2.0 * sin(time_f * 0.33);\n"
        "  vec2 kUV = reflectUV(v_uv, seg, m, aspect);\n"
        "  kUV = diamondFold(kUV, m, aspect);\n"
        "  float foldZoom = 1.45 + 0.55 * sin(time_f * 0.42);\n"
        "  kUV = fractalFold(kUV, foldZoom, time_f, m, aspect);\n"
        "  kUV = rotateUV(kUV, time_f * 0.23, m, aspect);\n"
        "  kUV = diamondFold(kUV, m, aspect);\n"
        "  vec2 pv = (kUV - m) * ar;\n"
        "  vec2 q = abs(pv);\n"
        "  if (q.y > q.x) q = q.yx;\n"
        "  float base = 1.82 + 0.18 * pingPong(sin(time_f * 0.2) * (PI * time_f), 5.0);\n"
        "  float period = log(base) * pingPong(time_f * PI, 5.0);\n"
        "  float tz = time_f * 0.65;\n"
        "  float rD = diamondRadius(pv) + 1e-6;\n"
        "  float ang = atan(q.y, q.x) + tz * 0.35 + 0.35 * sin(rD * 18.0 + time_f * 0.6);\n"
        "  float k = (period != 0.0) ? fract((log(rD) - tz) / period) : 0.0;\n"
        "  float rw = exp(k * period);\n"
        "  vec2 pwrap = vec2(cos(ang), sin(ang)) * rw;\n"
        "  vec2 u0 = fract(pwrap / ar + m);\n"
        "  vec2 u1 = fract((pwrap * 1.045) / ar + m);\n"
        "  vec2 u2 = fract((pwrap * 0.955) / ar + m);\n"
        "  vec2 dir = normalize(pwrap + vec2(1e-6));\n"
        "  vec2 off = dir * (0.0015 + 0.001 * sin(time_f * 1.3)) * vec2(1.0, 1.0 / aspect);\n"
        "  float vign = 1.0 - smoothstep(0.75, 1.2, length((v_uv - m) * ar));\n"
        "  vign = mix(0.9, 1.15, vign);\n"
        "  vec3 rC = preBlendColor(u0 + off);\n"
        "  vec3 gC = preBlendColor(u1);\n"
        "  vec3 bC = preBlendColor(u2 - off);\n"
        "  vec3 kaleidoRGB = vec3(rC.r, gC.g, bC.b);\n"
        "  float ring = smoothstep(0.0, 0.7, sin(log(rD + 1e-3) * 9.5 + time_f * 1.2));\n"
        "  ring = ring * pingPong(time_f * PI, 5.0);\n"
        "  float pulse = 0.5 + 0.5 * sin(time_f * 2.0 + rD * 28.0 + k * 12.0);\n"
        "  vec3 outCol = kaleidoRGB;\n"
        "  outCol *= (0.75 + 0.25 * ring) * (0.85 + 0.15 * pulse) * vign;\n"
        "  vec3 bloom = outCol * outCol * 0.18 + pow(max(outCol - 0.6, vec3(0.0)), vec3(2.0)) * 0.12;\n"
        "  outCol += bloom;\n"
        "  outCol = mix(outCol, baseCol, pingPong(pulse * PI, 5.0) * 0.18);\n"
        "  outCol = clamp(outCol, vec3(0.05), vec3(0.97));\n"
        "  vec3 finalRGB = mix(baseTex.rgb, outCol, pingPong(glow * PI, 5.0) * 0.8);\n"
        "  color = vec4(finalRGB, 1.0);\n"
        "}\n";

    static const char *fs_fx_item5_src =
        "#version 300 es\n"
        "precision highp float;\n"
        "out vec4 color;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D samp;\n"
        "uniform float time_f;\n"
        "void main(void) {\n"
        "  float rippleSpeed = 5.0;\n"
        "  float rippleAmplitude = 0.03;\n"
        "  float rippleWavelength = 10.0;\n"
        "  float twistStrength = 1.0;\n"
        "  float radius = length(v_uv - vec2(0.5, 0.5));\n"
        "  float ripple = sin(v_uv.x * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;\n"
        "  ripple += sin(v_uv.y * rippleWavelength + time_f * rippleSpeed) * rippleAmplitude;\n"
        "  vec2 rippleUV = v_uv + vec2(ripple, ripple);\n"
        "  float angle = twistStrength * (radius - 1.0) + time_f;\n"
        "  float cosA = cos(angle);\n"
        "  float sinA = sin(angle);\n"
        "  mat2 rotationMatrix = mat2(cosA, -sinA, sinA, cosA);\n"
        "  vec2 twistedUV = (rotationMatrix * (v_uv - vec2(0.5, 0.5))) + vec2(0.5, 0.5);\n"
        "  vec4 originalColor = texture(samp, v_uv);\n"
        "  vec4 twistedRippleColor = texture(samp, mix(rippleUV, twistedUV, 0.5));\n"
        "  color = mix(originalColor, twistedRippleColor, 0.5);\n"
        "}\n";

    static const float quad[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f
    };

    gl_program = create_program(vs_src, fs_src);
    gl_fx_mix_program = create_program(vs_src, fs_mix_src);
    gl_fx_program = create_program(vs_src, fs_fx_src);
    gl_fx_type5_program = create_program(vs_src, fs_fx_type5_src);
    gl_fx_type0_program = create_program(vs_src, fs_fx_type0_src);
    gl_fx_item5_program = create_program(vs_src, fs_fx_item5_src);

    if (!gl_program || !gl_fx_mix_program || !gl_fx_program || !gl_fx_type5_program || !gl_fx_type0_program || !gl_fx_item5_program)
        return -1;

    glGenVertexArrays(1, &gl_vao);
    glBindVertexArray(gl_vao);

    glGenBuffers(1, &gl_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, gl_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

    glGenTextures(1, &gl_tex);
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glGenTextures(2, gl_pingpong_tex);
    glGenFramebuffers(2, gl_pingpong_fbo);

    {
        int ping_index = 0;
        for (; ping_index < 2; ping_index++) {
            glBindTexture(GL_TEXTURE_2D, gl_pingpong_tex[ping_index]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, gl_pingpong_fbo[ping_index]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl_pingpong_tex[ping_index], 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                fprintf(stderr, "Ping-pong framebuffer %d is incomplete\n", ping_index);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                return -1;
            }
        }
    }

    glUseProgram(gl_program);
    glUniform1i(glGetUniformLocation(gl_program, "u_tex"), 0);
    gl_program_flip_y_loc = glGetUniformLocation(gl_program, "flip_y");

    glUseProgram(gl_fx_mix_program);
    gl_fx_mix_base_loc = glGetUniformLocation(gl_fx_mix_program, "base_samp");
    if (gl_fx_mix_base_loc >= 0)
        glUniform1i(gl_fx_mix_base_loc, 0);
    gl_fx_mix_effect_loc = glGetUniformLocation(gl_fx_mix_program, "effect_samp");
    if (gl_fx_mix_effect_loc >= 0)
        glUniform1i(gl_fx_mix_effect_loc, 1);
    gl_fx_mix_amount_loc = glGetUniformLocation(gl_fx_mix_program, "mix_amount");
    gl_fx_mix_flip_y_loc = glGetUniformLocation(gl_fx_mix_program, "flip_y");

    glUseProgram(gl_fx_program);
    gl_fx_samp_loc = glGetUniformLocation(gl_fx_program, "samp");
    if (gl_fx_samp_loc >= 0) {
        glUniform1i(gl_fx_samp_loc, 0);
    }
    gl_fx_time_loc = glGetUniformLocation(gl_fx_program, "time_f");
    gl_fx_resolution_loc = glGetUniformLocation(gl_fx_program, "iResolution");
    gl_fx_wobble_loc = glGetUniformLocation(gl_fx_program, "wobble_intensity");
    gl_flip_y_loc = glGetUniformLocation(gl_fx_program, "flip_y");

    glUseProgram(gl_fx_type5_program);
    gl_fx_type5_samp_loc = glGetUniformLocation(gl_fx_type5_program, "samp");
    if (gl_fx_type5_samp_loc >= 0) {
        glUniform1i(gl_fx_type5_samp_loc, 0);
    }
    gl_fx_type5_time_loc = glGetUniformLocation(gl_fx_type5_program, "time_f");
    gl_fx_type5_resolution_loc = glGetUniformLocation(gl_fx_type5_program, "iResolution");
    gl_fx_type5_flip_y_loc = glGetUniformLocation(gl_fx_type5_program, "flip_y");

    glUseProgram(gl_fx_type0_program);
    gl_fx_type0_samp_loc = glGetUniformLocation(gl_fx_type0_program, "samp");
    if (gl_fx_type0_samp_loc >= 0) {
        glUniform1i(gl_fx_type0_samp_loc, 0);
    }
    gl_fx_type0_time_loc = glGetUniformLocation(gl_fx_type0_program, "time_f");
    gl_fx_type0_resolution_loc = glGetUniformLocation(gl_fx_type0_program, "iResolution");
    gl_fx_type0_flip_y_loc = glGetUniformLocation(gl_fx_type0_program, "flip_y");

    glUseProgram(gl_fx_item5_program);
    gl_fx_item5_samp_loc = glGetUniformLocation(gl_fx_item5_program, "samp");
    if (gl_fx_item5_samp_loc >= 0) {
        glUniform1i(gl_fx_item5_samp_loc, 0);
    }
    gl_fx_item5_time_loc = glGetUniformLocation(gl_fx_item5_program, "time_f");
    gl_fx_item5_flip_y_loc = glGetUniformLocation(gl_fx_item5_program, "flip_y");

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    return 0;
}

static void update_viewport() {
    const float target_aspect = (float)GAME_WIDTH / (float)GAME_HEIGHT;
    int drawable_w = GAME_WIDTH;
    int drawable_h = GAME_HEIGHT;
    float drawable_aspect;

    SDL_GL_GetDrawableSize(window, &drawable_w, &drawable_h);
    drawable_aspect = (float)drawable_w / (float)drawable_h;

    if (drawable_aspect > target_aspect) {
        viewport_h = drawable_h;
        viewport_w = (int)(drawable_h * target_aspect);
        viewport_x = (drawable_w - viewport_w) / 2;
        viewport_y = 0;
    } else {
        viewport_w = drawable_w;
        viewport_h = (int)(drawable_w / target_aspect);
        viewport_x = 0;
        viewport_y = (drawable_h - viewport_h) / 2;
    }
}

static void present_frame() {
    GLuint source_texture = gl_tex;
    struct timespec ts;
    float wallclock_seconds = 0.0f;
    Uint32 now = SDL_GetTicks();

    timespec_get(&ts, TIME_UTC);
    wallclock_seconds = (float)(ts.tv_sec % 86400) + (float)ts.tv_nsec * 1.0e-9f;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, front->w, front->h, GL_RGBA, GL_UNSIGNED_BYTE, front->pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (cur_scr == ID_CREDITS) {
        float ct = (float)fmod(wallclock_seconds, 6.0) / 6.0f;
        float cycle = 0.5f - 0.5f * cosf(ct * 6.28318530f);
        float wobble_to_upload = COLLECT_FX_WOBBLE_BASE * (1.0f + 7.0f * cycle);
        render_texture_pass(gl_fx_program, source_texture, 0, viewport_x, viewport_y, viewport_w, viewport_h, wallclock_seconds, wobble_to_upload, (float)GAME_WIDTH, (float)GAME_HEIGHT);
    } else {
        int effect_index = 0;
        int ping_index = 0;
        float final_mix_amount = 1.0f;

        prune_collect_shader_effects(now);

        if (collect_fx_count == 0) {
            render_texture_pass(gl_program, source_texture, 0, viewport_x, viewport_y, viewport_w, viewport_h, wallclock_seconds, 0.0f, (float)GAME_WIDTH, (float)GAME_HEIGHT);
        } else {
            if (collect_fx_count == 1) {
                Sint32 remaining_ms = (Sint32)(collect_fx_stack[0].timeout_ticks - now);
                if (remaining_ms < (Sint32)COLLECT_FX_FADE_MS) {
                    if (remaining_ms < 0)
                        remaining_ms = 0;
                    final_mix_amount = (float)remaining_ms / (float)COLLECT_FX_FADE_MS;
                }
            }

            for (; effect_index < collect_fx_count; effect_index++) {
                CollectShaderEffect *effect = &collect_fx_stack[effect_index];
                GLuint target_fbo = gl_pingpong_fbo[ping_index];

                render_texture_pass(
                    collect_effect_program(effect->type),
                    source_texture,
                    target_fbo,
                    0,
                    0,
                    GAME_WIDTH,
                    GAME_HEIGHT,
                    wallclock_seconds,
                    effect->wobble_intensity,
                    (float)GAME_WIDTH,
                    (float)GAME_HEIGHT
                );

                source_texture = gl_pingpong_tex[ping_index];
                ping_index = 1 - ping_index;
            }

            if (final_mix_amount < 0.999f) {
                GLuint base_texture = gl_pingpong_tex[ping_index];
                render_texture_pass(gl_program, gl_tex, gl_pingpong_fbo[ping_index], 0, 0, GAME_WIDTH, GAME_HEIGHT, wallclock_seconds, 0.0f, (float)GAME_WIDTH, (float)GAME_HEIGHT);
                render_mix_pass(base_texture, source_texture, 0, viewport_x, viewport_y, viewport_w, viewport_h, final_mix_amount);
            } else {
                render_texture_pass(gl_program, source_texture, 0, viewport_x, viewport_y, viewport_w, viewport_h, wallclock_seconds, 0.0f, (float)GAME_WIDTH, (float)GAME_HEIGHT);
            }
        }
    }

    SDL_GL_SwapWindow(window);
}

extern void cleanup_all_timers();

static void rls() {

    cleanup_all_timers();

    Uint8 i = 0, z = 0;

    for( i = 0; img_str[i] != 0; i++) {
        if (gfx[i]) {
            SDL_FreeSurface(gfx[i]);
            gfx[i] = NULL;
        }
    }

    for( i = 0; hstr[i] != 0; i++) {
        if (hgfx[i]) {
            SDL_FreeSurface(hgfx[i]);
            hgfx[i] = NULL;
        }
    }

    for (i = 0; ev[i] != 0; i++) {
        for (z = 0; z < 10; z++) {
            if (evil_gfx[i].gfx[z]) {
                SDL_FreeSurface(evil_gfx[i].gfx[z]);
                evil_gfx[i].gfx[z] = NULL;
            }
        }
        evil_gfx[i].type = 0;
    }

    for(i = 0; i < COLLECT_NUM; i++) {
        if (collect[i]) {
            SDL_FreeSurface(collect[i]);
            collect[i] = NULL;
        }
    }

    for (i = 0; i < (sizeof(particles)/sizeof(particles[0])); i++) {
        if (particles[i]) {
            SDL_FreeSurface(particles[i]);
            particles[i] = NULL;
        }
    }

    if (bg) { SDL_FreeSurface(bg); bg = NULL; }
    if (lsd) { SDL_FreeSurface(lsd); lsd = NULL; }
    if (logo) { SDL_FreeSurface(logo); logo = NULL; }

#ifdef HAS_MIXER
    if (fire_snd) { Mix_FreeChunk(fire_snd); fire_snd = NULL; }
    if (collect_snd) { Mix_FreeChunk(collect_snd); collect_snd = NULL; }
    if (intro_snd) { Mix_FreeChunk(intro_snd); intro_snd = NULL; }
    if (kill_snd) { Mix_FreeChunk(kill_snd); kill_snd = NULL; }
    if (game_track) { Mix_FreeMusic(game_track); game_track = NULL; }

    Mix_HaltMusic();
    Mix_CloseAudio();
    Mix_Quit();
#endif
    if (cfont) { SDL_FreeFont(cfont); cfont = NULL; }
    if (font)  { SDL_FreeFont(font);  font  = NULL; }
    if (TTF_WasInit()) {
        TTF_Quit();
    }

    if (level) {
        release_level(level);
        level = NULL;
    }

    if (gl_pingpong_fbo[0] || gl_pingpong_fbo[1]) {
        glDeleteFramebuffers(2, gl_pingpong_fbo);
        gl_pingpong_fbo[0] = 0;
        gl_pingpong_fbo[1] = 0;
    }
    if (gl_tex || gl_pingpong_tex[0] || gl_pingpong_tex[1]) {
        GLuint textures_to_delete[3] = { gl_tex, gl_pingpong_tex[0], gl_pingpong_tex[1] };
        glDeleteTextures(3, textures_to_delete);
        gl_tex = 0;
        gl_pingpong_tex[0] = 0;
        gl_pingpong_tex[1] = 0;
    }
    if (gl_vbo) {
        glDeleteBuffers(1, &gl_vbo);
        gl_vbo = 0;
    }
    if (gl_vao) {
        glDeleteVertexArrays(1, &gl_vao);
        gl_vao = 0;
    }
    if (gl_program) {
        glDeleteProgram(gl_program);
        gl_program = 0;
    }
    if (gl_fx_program) {
        glDeleteProgram(gl_fx_program);
        gl_fx_program = 0;
    }
    if (gl_fx_type5_program) {
        glDeleteProgram(gl_fx_type5_program);
        gl_fx_type5_program = 0;
    }
    if (gl_fx_type0_program) {
        glDeleteProgram(gl_fx_type0_program);
        gl_fx_type0_program = 0;
    }
    if (gl_fx_item5_program) {
        glDeleteProgram(gl_fx_item5_program);
        gl_fx_item5_program = 0;
    }
    if (gl_fx_mix_program) {
        glDeleteProgram(gl_fx_mix_program);
        gl_fx_mix_program = 0;
    }
    if (gl_ctx) {
        SDL_GL_DeleteContext(gl_ctx);
        gl_ctx = 0;
    }
    if (front) {
        SDL_FreeSurface(front);
        front = NULL;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
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
    render();

    while(SDL_PollEvent(&e)) {
        handleInputEvent(&e);
        switch(e.type) {
            case SDL_QUIT:
                active = 0;
                break;
            case SDL_KEYDOWN:
            {
                switch(e.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        reset_collect_shader_effect();
                        if (cur_scr != ID_START) {
                            cleanup_all_timers();
                            cur_scr = ID_START;
                        } else {
#ifndef __EMSCRIPTEN__
                            active = 0;
#endif
                        }
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
        case SDL_CONTROLLERDEVICEADDED:
            if (controller == 0) {
                open_controller(e.cdevice.which);
            }
        break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if (stick && SDL_JoystickInstanceID(stick) == e.cdevice.which) {
                close_controller();
                printf("smx: controller closed\n");
            }
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            if (e.cbutton.button == SDL_CONTROLLER_BUTTON_BACK) {
                reset_collect_shader_effect();
                if (cur_scr != ID_START) {
                    cleanup_all_timers();
                    cur_scr = ID_START;
                    check_in = SDL_AddTimer(225, check_start_in, 0);
                } else {
#ifndef __EMSCRIPTEN__
                    active = 0;
#endif
                }
            } else if (e.cbutton.button == SDL_CONTROLLER_BUTTON_START && cur_scr == ID_GAME) {
                cur_scr = ID_PAUSED;
            }
            break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                WIDTH = e.window.data1;
                HEIGHT = e.window.data2;
                update_viewport();
            }
            break;
        }
   }
   present_frame();
#ifndef __EMSCRIPTEN__
   SDL_Delay(10);
#endif
}

int main(int argc, char **argv) {
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

#ifdef DEFAULT_FULL
    full = 1;
#endif

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
        return -1;

#ifdef __EMSCRIPTEN__
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
#endif
        
    SDL_ShowCursor(SDL_FALSE);
    ico = SDL_LoadBMP(get_path("D:\\", "img/col1.bmp"));
    if(ico == NULL) {
        fprintf(stderr, "Error loading icon, wrong path place this program in the directory with the resources.\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }
    if(full == 1)
        mode = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP;
    else
        mode = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) < 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) < 0 ||
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0) < 0 ||
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0) {
        fprintf(stderr, "Error setting GL attributes: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    window = SDL_CreateWindow("Super Stoner 420", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, mode);
    if(!window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        SDL_Quit();
        exit(-1);
    }
    SDL_SetWindowIcon(window, ico);
    SDL_FreeSurface(ico);

    gl_ctx = SDL_GL_CreateContext(window);
    if(!gl_ctx) {
        fprintf(stderr, "Error creating OpenGL ES context: %s\n", SDL_GetError());
        SDL_Quit();
        exit(-1);
    }

#ifndef __EMSCRIPTEN__
    if (!gladLoadGLES2Loader((GLADloadproc)SDL_GL_GetProcAddress)) {
        fprintf(stderr, "Error initializing GLAD\n");
        SDL_GL_DeleteContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }
#endif

    if (SDL_GL_SetSwapInterval(1) < 0) {
        fprintf(stderr, "Warning: could not enable vsync: %s\n", SDL_GetError());
    }

    if (init_gl_renderer() < 0) {
        SDL_Quit();
        exit(-1);
    }

    update_viewport();

    if(full == 1) {
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
    }

    SDL_GameControllerEventState(SDL_ENABLE);

    if(SDL_NumJoysticks() > 0)
        printf("smx: %d Joysticks Available\n", SDL_NumJoysticks());
    else if(SDL_NumJoysticks() == 0)
        printf("smx: 0 joysticks avilable..\n");

    {
        int controller_index;
        for (controller_index = 0; controller_index < SDL_NumJoysticks(); controller_index++) {
            if (SDL_IsGameController(controller_index)) {
                open_controller(controller_index);
                break;
            }
        }
    }

    fflush(stdout);

    front = SDL_CreateRGBSurfaceWithFormat(0, GAME_WIDTH, GAME_HEIGHT, 32, SDL_PIXELFORMAT_RGBA32);
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
    close_controller();
    SDL_Quit();
    printf("smx: exit\n");
    return 0;
}

void SDL_ReverseBlt(SDL_Surface *surf, SDL_Rect *rc, SDL_Surface *front_surf, SDL_Rect *rc2, Uint32 transparent) {
    void *buf , *buf2;
    int i,z,i2,z2;
    buf = lock(surf);
    buf2 = lock(front_surf);
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
#ifdef __EMSCRIPTEN__
    snprintf(sbuf, 4095, "/assets/%s", s);
    return sbuf;
#endif
    snprintf(sbuf,4095, "%s", s);
    return sbuf;
}
