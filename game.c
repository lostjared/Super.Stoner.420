#ifdef __EMSCRIPTEN__
#include<emscripten/emscripten.h>
#endif
#include <GLES3/gl3.h>
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
GLuint gl_vbo = 0;
GLuint gl_vao = 0;
GLuint gl_tex = 0;
GLint gl_fx_time_loc = -1;
GLint gl_fx_resolution_loc = -1;
GLint gl_fx_samp_loc = -1;
GLint gl_fx_wobble_loc = -1;
GLint gl_fx_type5_time_loc = -1;
GLint gl_fx_type5_resolution_loc = -1;
GLint gl_fx_type5_samp_loc = -1;
GLint gl_fx_type0_time_loc = -1;
GLint gl_fx_type0_resolution_loc = -1;
GLint gl_fx_type0_samp_loc = -1;
GLint gl_fx_item5_samp_loc = -1;
int viewport_x = 0;
int viewport_y = 0;
int viewport_w = GAME_WIDTH;
int viewport_h = GAME_HEIGHT;
Uint32 collect_fx_until_ticks = 0;
Uint32 collect_fx_decay_end_ticks = 0;
int active_collect_fx_type = 0;

#define COLLECT_FX_DURATION_MS      8000U
#define COLLECT_FX_DECAY_MS         3000U
#define COLLECT_FX_WOBBLE_BASE      0.012f
#define COLLECT_FX_WOBBLE_INCREMENT 0.008f
#define COLLECT_FX_WOBBLE_MAX       0.072f

float collect_fx_wobble_intensity = COLLECT_FX_WOBBLE_BASE;

void activate_collect_shader_effect(int item_type) {
    int new_type = 0;
    if (item_type == 4) {
        new_type = 5;
    } else if (item_type == 1) {
        new_type = 2;
    } else if (item_type == 5) {
        new_type = 3;
    } else if (item_type >= 2 && item_type <= 3) {
        new_type = 1;
    }
    if (new_type == 0)
        return;
    /* accumulate wobble intensity when collecting the same wobble-type item */
    if (new_type == 1 && active_collect_fx_type == 1) {
        collect_fx_wobble_intensity += COLLECT_FX_WOBBLE_INCREMENT;
        if (collect_fx_wobble_intensity > COLLECT_FX_WOBBLE_MAX)
            collect_fx_wobble_intensity = COLLECT_FX_WOBBLE_MAX;
    } else {
        collect_fx_wobble_intensity = COLLECT_FX_WOBBLE_BASE;
    }
    active_collect_fx_type = new_type;
    collect_fx_until_ticks = SDL_GetTicks() + COLLECT_FX_DURATION_MS;
    collect_fx_decay_end_ticks = collect_fx_until_ticks + COLLECT_FX_DECAY_MS;
}

void reset_collect_shader_effect() {
    collect_fx_until_ticks = 0;
    collect_fx_decay_end_ticks = 0;
    active_collect_fx_type = 0;
    collect_fx_wobble_intensity = COLLECT_FX_WOBBLE_BASE;
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
        "out vec2 v_uv;\n"
        "void main() {\n"
        "  v_uv = a_uv;\n"
        "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
        "}\n";

    static const char *fs_src =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D u_tex;\n"
        "out vec4 frag_color;\n"
        "void main() {\n"
        "  frag_color = texture(u_tex, v_uv);\n"
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
        "float pingPong(float x, float length) {\n"
        "  float modVal = mod(x, length * 2.0);\n"
        "  return modVal <= length ? modVal : length * 2.0 - modVal;\n"
        "}\n"
        "vec4 blur(sampler2D image, vec2 uv, vec2 resolution) {\n"
        "  vec2 texelSize = 1.0 / resolution;\n"
        "  vec4 result = vec4(0.0);\n"
        "  const float kernelVals[100] = float[](0.5, 1.0, 1.5, 2.0, 2.5, 2.5, 2.0, 1.5, 1.0, 0.5,\n"
        "                                      1.0, 2.0, 2.5, 3.0, 3.5, 3.5, 3.0, 2.5, 2.0, 1.0,\n"
        "                                      1.5, 2.5, 3.0, 3.5, 4.0, 4.0, 3.5, 3.0, 2.5, 1.5,\n"
        "                                      2.0, 3.0, 3.5, 4.0, 4.5, 4.5, 4.0, 3.5, 3.0, 2.0,\n"
        "                                      2.5, 3.5, 4.0, 4.5, 5.0, 5.0, 4.5, 4.0, 3.5, 2.5,\n"
        "                                      2.5, 3.5, 4.0, 4.5, 5.0, 5.0, 4.5, 4.0, 3.5, 2.5,\n"
        "                                      2.0, 3.0, 3.5, 4.0, 4.5, 4.5, 4.0, 3.5, 3.0, 2.0,\n"
        "                                      1.5, 2.5, 3.0, 3.5, 4.0, 4.0, 3.5, 3.0, 2.5, 1.5,\n"
        "                                      1.0, 2.0, 2.5, 3.0, 3.5, 3.5, 3.0, 2.5, 2.0, 1.0,\n"
        "                                      0.5, 1.0, 1.5, 2.0, 2.5, 2.5, 2.0, 1.5, 1.0, 0.5);\n"
        "  float kernelSum = 0.0;\n"
        "  for (int i = 0; i < 100; i++) {\n"
        "    kernelSum += kernelVals[i];\n"
        "  }\n"
        "  for (int x = -5; x <= 4; ++x) {\n"
        "    for (int y = -5; y <= 4; ++y) {\n"
        "      vec2 offset = vec2(float(x), float(y)) * texelSize;\n"
        "      result += texture(image, uv + offset) * kernelVals[(y + 5) * 10 + (x + 5)];\n"
        "    }\n"
        "  }\n"
        "  return result / kernelSum;\n"
        "}\n"
        "vec4 colorShift(vec4 col) {\n"
        "  return vec4(\n"
        "    0.5 + 0.5 * cos(col.r * 3.14159265 * 0.5),\n"
        "    0.5 + 0.5 * cos(col.g * 3.14159265 * 0.5),\n"
        "    0.5 + 0.5 * cos(col.b * 3.14159265 * 0.5),\n"
        "    col.a\n"
        "  );\n"
        "}\n"
        "void main(void) {\n"
        "  float time_t = pingPong(time_f, 10.0) + 1.0;\n"
        "  vec4 pix = blur(samp, v_uv, iResolution);\n"
        "  pix = pix * time_t;\n"
        "  pix = colorShift(pix);\n"
        "  pix.rgb = mix(vec3(1.0), pix.rgb, 0.8);\n"
        "  color = pix;\n"
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
        "  color = vec4(finalRGB, baseTex.a);\n"
        "}\n";

    static const char *fs_fx_item5_src =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 color;\n"
        "in vec2 v_uv;\n"
        "uniform sampler2D samp;\n"
        "vec3 NormalizePalette(vec3 col, float levels) {\n"
        "  return floor(col * levels + 0.5) / levels;\n"
        "}\n"
        "void main(void) {\n"
        "  vec2 pixelSize = vec2(256.0, 240.0);\n"
        "  vec2 coord = floor(v_uv * pixelSize) / pixelSize;\n"
        "  vec4 texColor = texture(samp, coord);\n"
        "  vec3 quantizedColor = NormalizePalette(texColor.rgb, 6.0);\n"
        "  color = vec4(quantizedColor, texColor.a);\n"
        "}\n";

    static const float quad[] = {
        -1.0f,  1.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 1.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 1.0f
    };

    gl_program = create_program(vs_src, fs_src);
    gl_fx_program = create_program(vs_src, fs_fx_src);
    gl_fx_type5_program = create_program(vs_src, fs_fx_type5_src);
    gl_fx_type0_program = create_program(vs_src, fs_fx_type0_src);
    gl_fx_item5_program = create_program(vs_src, fs_fx_item5_src);

    if (!gl_program || !gl_fx_program || !gl_fx_type5_program || !gl_fx_type0_program || !gl_fx_item5_program)
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

    glUseProgram(gl_program);
    glUniform1i(glGetUniformLocation(gl_program, "u_tex"), 0);
    glUseProgram(gl_fx_program);
    gl_fx_samp_loc = glGetUniformLocation(gl_fx_program, "samp");
    if (gl_fx_samp_loc >= 0) {
        glUniform1i(gl_fx_samp_loc, 0);
    }
    gl_fx_time_loc = glGetUniformLocation(gl_fx_program, "time_f");
    gl_fx_resolution_loc = glGetUniformLocation(gl_fx_program, "iResolution");
    gl_fx_wobble_loc = glGetUniformLocation(gl_fx_program, "wobble_intensity");

    glUseProgram(gl_fx_type5_program);
    gl_fx_type5_samp_loc = glGetUniformLocation(gl_fx_type5_program, "samp");
    if (gl_fx_type5_samp_loc >= 0) {
        glUniform1i(gl_fx_type5_samp_loc, 0);
    }
    gl_fx_type5_time_loc = glGetUniformLocation(gl_fx_type5_program, "time_f");
    gl_fx_type5_resolution_loc = glGetUniformLocation(gl_fx_type5_program, "iResolution");

    glUseProgram(gl_fx_type0_program);
    gl_fx_type0_samp_loc = glGetUniformLocation(gl_fx_type0_program, "samp");
    if (gl_fx_type0_samp_loc >= 0) {
        glUniform1i(gl_fx_type0_samp_loc, 0);
    }
    gl_fx_type0_time_loc = glGetUniformLocation(gl_fx_type0_program, "time_f");
    gl_fx_type0_resolution_loc = glGetUniformLocation(gl_fx_type0_program, "iResolution");

    glUseProgram(gl_fx_item5_program);
    gl_fx_item5_samp_loc = glGetUniformLocation(gl_fx_item5_program, "samp");
    if (gl_fx_item5_samp_loc >= 0) {
        glUniform1i(gl_fx_item5_samp_loc, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
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
    GLuint active_program = gl_program;
    float wobble_to_upload = collect_fx_wobble_intensity;
    struct timespec ts;
    float wallclock_seconds = 0.0f;

    timespec_get(&ts, TIME_UTC);
    wallclock_seconds = (float)(ts.tv_sec % 86400) + (float)ts.tv_nsec * 1.0e-9f;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gl_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, front->w, front->h, GL_RGBA, GL_UNSIGNED_BYTE, front->pixels);

    {
        Uint32 now = SDL_GetTicks();
        /* compute effective wobble for the decay phase */
        if (active_collect_fx_type == 1 &&
            (Sint32)(collect_fx_until_ticks - now) <= 0 &&
            (Sint32)(collect_fx_decay_end_ticks - now) > 0) {
            float t = (float)(Sint32)(collect_fx_decay_end_ticks - now)
                      / (float)COLLECT_FX_DECAY_MS;
            collect_fx_wobble_intensity = COLLECT_FX_WOBBLE_BASE
                + t * (collect_fx_wobble_intensity - COLLECT_FX_WOBBLE_BASE);
            if (collect_fx_wobble_intensity <= COLLECT_FX_WOBBLE_BASE + 0.0001f) {
                reset_collect_shader_effect();
            }
        }
        if (cur_scr == ID_CREDITS) {
            /* smooth ping-pong: base → 8× base → base, 6-second period */
            float ct = (float)fmod(wallclock_seconds, 6.0) / 6.0f;
            float cycle = 0.5f - 0.5f * cosf(ct * 6.28318530f);
            wobble_to_upload = COLLECT_FX_WOBBLE_BASE * (1.0f + 7.0f * cycle);
            active_program = gl_fx_program;
        } else if (active_collect_fx_type != 0 &&
                   ((Sint32)(collect_fx_until_ticks - now) > 0 ||
                    (Sint32)(collect_fx_decay_end_ticks - now) > 0)) {
            if (active_collect_fx_type == 5) {
                active_program = gl_fx_type5_program;
            } else if (active_collect_fx_type == 2) {
                active_program = gl_fx_type0_program;
            } else if (active_collect_fx_type == 3) {
                active_program = gl_fx_item5_program;
            } else {
                active_program = gl_fx_program;
            }
        } else if (active_collect_fx_type != 0) {
            reset_collect_shader_effect();
        }
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(viewport_x, viewport_y, viewport_w, viewport_h);

    glUseProgram(active_program);
    if (active_program == gl_fx_program) {
        if (gl_fx_samp_loc >= 0) {
            glUniform1i(gl_fx_samp_loc, 0);
        }
        if (gl_fx_time_loc >= 0) {
            glUniform1f(gl_fx_time_loc, wallclock_seconds);
        }
        if (gl_fx_resolution_loc >= 0) {
            glUniform2f(gl_fx_resolution_loc, (float)viewport_w, (float)viewport_h);
        }
        if (gl_fx_wobble_loc >= 0) {
            glUniform1f(gl_fx_wobble_loc, wobble_to_upload);
        }
    } else if (active_program == gl_fx_type5_program) {
        if (gl_fx_type5_samp_loc >= 0) {
            glUniform1i(gl_fx_type5_samp_loc, 0);
        }
        if (gl_fx_type5_time_loc >= 0) {
            glUniform1f(gl_fx_type5_time_loc, wallclock_seconds);
        }
        if (gl_fx_type5_resolution_loc >= 0) {
            glUniform2f(gl_fx_type5_resolution_loc, (float)viewport_w, (float)viewport_h);
        }
    } else if (active_program == gl_fx_type0_program) {
        if (gl_fx_type0_samp_loc >= 0) {
            glUniform1i(gl_fx_type0_samp_loc, 0);
        }
        if (gl_fx_type0_time_loc >= 0) {
            glUniform1f(gl_fx_type0_time_loc, wallclock_seconds);
        }
        if (gl_fx_type0_resolution_loc >= 0) {
            glUniform2f(gl_fx_type0_resolution_loc, (float)viewport_w, (float)viewport_h);
        }
    } else if (active_program == gl_fx_item5_program) {
        if (gl_fx_item5_samp_loc >= 0) {
            glUniform1i(gl_fx_item5_samp_loc, 0);
        }
    }
    glBindVertexArray(gl_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

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

    if (gl_tex) {
        glDeleteTextures(1, &gl_tex);
        gl_tex = 0;
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
                            active = 0;
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
        case SDL_JOYBUTTONDOWN:
            /* Treat common back/cancel buttons like Escape. */
            if (e.jbutton.button == 6 || e.jbutton.button == 11) {
                reset_collect_shader_effect();
                if (cur_scr != ID_START) {
                    cleanup_all_timers();
                    cur_scr = ID_START;
                } else {
                    active = 0;
                }
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
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0)
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

    SDL_JoystickEventState(SDL_ENABLE);

    if(SDL_NumJoysticks() > 0)
        printf("smx: %d Joysticks Available\n", SDL_NumJoysticks());
    else if(SDL_NumJoysticks() == 0)
        printf("smx: 0 joysticks avilable..\n");

    stick = SDL_JoystickOpen(0);

    if(stick != NULL)
        printf("smx: Joystick initalized.\n");

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
    SDL_JoystickClose(stick);
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
