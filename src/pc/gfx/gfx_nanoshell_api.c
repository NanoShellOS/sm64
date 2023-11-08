#ifdef TARGET_NANOSHELL

// dithering and palette construction code taken from
// https://bisqwit.iki.fi/jutut/kuvat/programming_examples/walk.cc

#include "macros.h"
#include "gfx_nanoshell_api.h"
#include "gfx_opengl.h"
#include "../configfile.h"
#include "../common.h"

#include <nanoshell/nanoshell.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdarg.h>

#define REG_SELECT          0x3C4     /* VGA register select */
#define REG_VALUE           0x3C5     /* send value to selected register */
#define REG_MASK            0x02      /* map mask register */
#define PAL_LOAD            0x3C8     /* start loading palette */
#define PAL_COLOR           0x3C9     /* load next palette color */

#define SCREEN_WIDTH        320       /* width in pixels */
#define SCREEN_HEIGHT       240       /* height in mode 13h, in pixels */
#define SCREEN_HEIGHT_X     240       /* height in mode X, in pixels */

#define WINDOW_WIDTH        640
#define WINDOW_HEIGHT       480

// 7*9*4 regular palette (252 colors)
#define PAL_RBITS 7
#define PAL_GBITS 9
#define PAL_BBITS 4
#define PAL_GAMMA 1.5 // apply this gamma to palette

#define DIT_GAMMA (2.0 / PAL_GAMMA) // apply this gamma to dithering
#define DIT_BITS 6                  // dithering strength

#define VGA_BASE 0xA0000

#define umin(a, b) (((a) < (b)) ? (a) : (b))

typedef union {
	struct { uint8_t r, g, b, a; };
	uint32_t c;
}
__attribute__((packed))
RGBA;
static uint8_t rgbconv[3][256][256];
static uint8_t dit_kernel[8][8];

#ifdef ENABLE_OSMESA
#include <osmesa.h>
OSMesaContext ctx;
uint32_t *osmesa_buffer; // 320x240x4 bytes (RGBA)
#define GFX_BUFFER osmesa_buffer
#else
#include "gfx_soft.h"
#define GFX_BUFFER gfx_output
#endif

uint32_t hack_palette[256];
uint8_t  hack_palette_colors[3];
int hack_cur_palette_index = 0, hack_cur_palette_color_index = 0;

void hack_start_palette_load(int unused1, int unused2) 
{
	hack_cur_palette_index = 0;
	hack_cur_palette_color_index = 0;
}

void hack_set_palette_color(int unused, int color)
{
	hack_palette_colors[hack_cur_palette_color_index++] = color;
	
	if (hack_cur_palette_color_index == 3)
	{
		// set new palette color
		hack_palette[hack_cur_palette_index++] = 
			(hack_palette_colors[0] << 16)   | // red
			(hack_palette_colors[1] <<  8)   | // green
			(hack_palette_colors[2]);          // blue
	}
}

#ifdef VERSION_EU
# define FRAMERATE 25
# define FRAMETIME 40
#else
# define FRAMERATE 30
# define FRAMETIME 33
#endif

static bool init_done = false;
static bool do_render = true;
static volatile uint32_t tick = 0;
static uint32_t last = 0;

static Window* game_window;

static void(*game_run_one_game_iter)();

static void gfx_nanoshell_window_procedure(Window* window, int msg, int parm1, int parm2)
{
	switch (msg)
	{
		case EVENT_CREATE:
		{
			AddTimer(window, FRAMETIME, EVENT_USER);
			break;
		}
		case EVENT_USER:
		{
			game_run_one_game_iter();
			break;
		}
		default:
		{
			DefaultWindowProc(window, msg, parm1, parm2);
			break;
		}
	}
}

static void gfx_nanoshell_init_impl(void) {
    // create Bayer 8x8 dithering matrix
    for (unsigned y = 0; y < 8; ++y)
        for (unsigned x = 0; x < 8; ++x)
            dit_kernel[y][x] =
                ((x  ) & 4)/4u + ((x  ) & 2)*2u + ((x  ) & 1)*16u
              + ((x^y) & 4)/2u + ((x^y) & 2)*4u + ((x^y) & 1)*32u;

    // create gamma-corrected look-up tables for dithering
    double dtab[256], ptab[256];
    for(unsigned n = 0; n < 256; ++n) {
        dtab[n] = (255.0/256.0) - pow(n/256.0, 1.0 / DIT_GAMMA);
        ptab[n] = pow(n/255.0, 1.0 / PAL_GAMMA);
    }

    for(unsigned n = 0; n < 256; ++n) {
        for(unsigned d = 0; d < 256; ++d) {
            rgbconv[0][n][d] =                         umin(PAL_BBITS-1, (unsigned)(ptab[n]*(PAL_BBITS-1) + dtab[d]));
            rgbconv[1][n][d] =             PAL_BBITS * umin(PAL_GBITS-1, (unsigned)(ptab[n]*(PAL_GBITS-1) + dtab[d]));
            rgbconv[2][n][d] = PAL_GBITS * PAL_BBITS * umin(PAL_RBITS-1, (unsigned)(ptab[n]*(PAL_RBITS-1) + dtab[d]));
        }
    }
	
    // set up regular palette as configured above;
    // however, bias the colors towards darker ones in an exponential curve
    hack_start_palette_load(PAL_LOAD, 0);
    for(unsigned color = 0; color < PAL_RBITS * PAL_GBITS * PAL_BBITS; ++color) {
        hack_set_palette_color(PAL_COLOR, pow(((color / (PAL_BBITS * PAL_GBITS)) % PAL_RBITS) * 1.0 / (PAL_RBITS-1), PAL_GAMMA) * 63);
        hack_set_palette_color(PAL_COLOR, pow(((color / (PAL_BBITS            )) % PAL_GBITS) * 1.0 / (PAL_GBITS-1), PAL_GAMMA) * 63);
        hack_set_palette_color(PAL_COLOR, pow(((color                          ) % PAL_BBITS) * 1.0 / (PAL_BBITS-1), PAL_GAMMA) * 63);
    }
}

static void gfx_nanoshell_shutdown_impl(void) {
	if (!game_window)
		return;
	
	DestroyWindow(game_window);
	while (HandleMessages(game_window));
	game_window = NULL;
}

static inline void gfx_nanoshell_swap_buffers_mode13(void) {
    RGBA *inp = (RGBA *)GFX_BUFFER;
	
	Image img;
	img.width  = SCREEN_WIDTH;
	img.height = SCREEN_HEIGHT;
	img.framebuffer = (const uint32_t*) inp;
	
	int n_pixels = SCREEN_WIDTH * SCREEN_HEIGHT;
	for (int i = 0; i < n_pixels; i++) {
		// hack to swap the red and blue channels of a pixel to convert them to VBE format
		uint8_t tmp = inp[i].r;
		inp[i].r = inp[i].b;
		inp[i].b = tmp;
	}
	
	//VidBlitImage(&img, 0, 0);
	VidBlitImageResize(&img, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

static void gfx_nanoshell_init(UNUSED const char *game_name, UNUSED bool start_in_fullscreen) {
	game_window = CreateWindow (
		game_name,
		GetScreenSizeX() / 2 - WINDOW_WIDTH / 2,
		GetScreenSizeY() / 2 - WINDOW_HEIGHT / 2,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		gfx_nanoshell_window_procedure,
		WF_NOMAXIMZ | WF_NOMINIMZ
	);
	
	if (!game_window) {
		LogMsg("This application can only run in windowed mode.");
		abort();
	}
	
    gfx_nanoshell_init_impl();
	
    init_done = true;
}

static void gfx_nanoshell_set_keyboard_callbacks(UNUSED bool (*on_key_down)(int scancode), UNUSED bool (*on_key_up)(int scancode), UNUSED void (*on_all_keys_up)(void)) {
	
}

static void gfx_nanoshell_set_fullscreen_changed_callback(UNUSED void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
	
}

static void gfx_nanoshell_set_fullscreen(UNUSED bool enable) {
	
}

static int gfx_nanoshell_get_tick_count() {
	static int old = -1;
	if (old == -1)
		old = GetTickCount();
	
	return (GetTickCount() - old) / FRAMETIME;
}

static void gfx_nanoshell_update_tick_count() {
	tick = gfx_nanoshell_get_tick_count ();
}

static void gfx_nanoshell_main_loop(void (*run_one_game_iter)(void)) {
	if (!game_window) {
		LogMsg("left running without a window??");
		return;
	}
	
	game_run_one_game_iter = run_one_game_iter;
	
	while (HandleMessages(game_window));
	
	game_window = NULL;
}

static void gfx_nanoshell_get_dimensions(uint32_t *width, uint32_t *height) {
    *width  = SCREEN_WIDTH;
    *height = SCREEN_HEIGHT;
}

static void gfx_nanoshell_handle_events(void) {
	
}

static bool gfx_nanoshell_start_frame(void) {
    return do_render;
}

static void gfx_nanoshell_swap_buffers_begin(void) {
    if (GFX_BUFFER != NULL) {
        gfx_nanoshell_swap_buffers_mode13();
    }
}

static void gfx_nanoshell_swap_buffers_end(void) { }

static double gfx_nanoshell_get_time(void) {
    return 0.0;
}

static void gfx_nanoshell_shutdown(void) {
    if (!init_done) return;

    gfx_nanoshell_shutdown_impl();

    init_done = false;
}

struct GfxWindowManagerAPI gfx_nanoshell_api =
{
    gfx_nanoshell_init,
    gfx_nanoshell_set_keyboard_callbacks,
    gfx_nanoshell_set_fullscreen_changed_callback,
    gfx_nanoshell_set_fullscreen,
    gfx_nanoshell_main_loop,
    gfx_nanoshell_get_dimensions,
    gfx_nanoshell_handle_events,
    gfx_nanoshell_start_frame,
    gfx_nanoshell_swap_buffers_begin,
    gfx_nanoshell_swap_buffers_end,
    gfx_nanoshell_get_time,
    gfx_nanoshell_shutdown
};

#endif
