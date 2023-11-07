#ifdef ENABLE_WM_NANOSHELL
#include <time.h>
#include <errno.h>
#include <nanoshell/nanoshell.h>

#ifndef _LANGUAGE_C
#define _LANGUAGE_C
#endif
#include <PR/gbi.h>

#include "gfx_window_manager_api.h"
#include "gfx_rendering_api.h"

#include <GL/gl.h>
#include <zbuffer.h>

#define WINDOW_WIDTH   640
#define WINDOW_HEIGHT  480

static Window   *gWindow;
static uint32_t *gFrameBuffer;
static ZBuffer  *gZBuffer;

static void(*gRunOneGameIter)(void);

static void gfx_nanoshell_window_procedure(Window* window, int msg, int parm1, int parm2) {
	switch (msg) {
		case EVENT_CREATE:
			DefaultWindowProc(window, msg, parm1, parm2);
			AddTimer(window, 1000/30, EVENT_USER);
			break;
		case EVENT_USER:
			gRunOneGameIter();
			break;
		default:
			DefaultWindowProc(window, msg, parm1, parm2);
			break;
	}
}

static void gfx_nanoshell_wm_init(const char *game_name, bool start_in_fullscreen) {
	gFrameBuffer = malloc (sizeof(uint32_t) * WINDOW_WIDTH * WINDOW_HEIGHT);
	
	if (!gFrameBuffer) {
		LogMsg("No framebuffer");
		abort();
	}
	
	for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
	{
		gFrameBuffer[i] = i * 0x865674;
	}
	
	gZBuffer = ZB_open(WINDOW_WIDTH, WINDOW_HEIGHT, ZB_MODE_RGBA, gFrameBuffer);
	if (!gZBuffer) {
		LogMsg("No z-buffer");
		abort();
	}
	
	glInit(gZBuffer);
	
	gWindow = CreateWindow(
		game_name,
		GetScreenSizeX() / 2 - WINDOW_WIDTH  / 2,
		GetScreenSizeY() / 2 - WINDOW_HEIGHT / 2,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		gfx_nanoshell_window_procedure,
		0);
	
	if (!gWindow) {
		LogMsg("No window");
		abort();
	}
}

static void gfx_nanoshell_wm_set_keyboard_callbacks(bool (*on_key_down)(int scancode), bool (*on_key_up)(int scancode), void (*on_all_keys_up)(void)) {
}

static void gfx_nanoshell_wm_set_fullscreen_changed_callback(void (*on_fullscreen_changed)(bool is_now_fullscreen)) {
}

static void gfx_nanoshell_wm_set_fullscreen(bool enable) {
}

static void gfx_nanoshell_wm_main_loop(void (*run_one_game_iter)(void)) {
	gRunOneGameIter = run_one_game_iter;
    while (1) {
		if (!HandleMessages(gWindow)) {
			// @TODO
			LogMsg("TODO: Implement exit cleanup");
			exit(0);
		}
    }
}

static void gfx_nanoshell_wm_get_dimensions(uint32_t *width, uint32_t *height) {
    *width = WINDOW_WIDTH;
    *height = WINDOW_HEIGHT;
}

static void gfx_nanoshell_wm_handle_events(void) {
}

static bool gfx_nanoshell_wm_start_frame(void) {
    return true;
}

static void gfx_nanoshell_wm_swap_buffers_begin(void) {
}

static struct timespec gfx_nanoshell_wm_timediff(struct timespec t1, struct timespec t2) {
    t1.tv_sec -= t2.tv_sec;
    t1.tv_nsec -= t2.tv_nsec;
    if (t1.tv_nsec < 0) {
        t1.tv_nsec += 1000000000;
        t1.tv_sec -= 1;
    }
    return t1;
}

static struct timespec gfx_nanoshell_wm_timeadd(struct timespec t1, struct timespec t2) {
    t1.tv_sec += t2.tv_sec;
    t1.tv_nsec += t2.tv_nsec;
    if (t1.tv_nsec > 1000000000) {
        t1.tv_nsec -= 1000000000;
        t1.tv_sec += 1;
    }
    return t1;
}

#define CLOCK_MONOTONIC 0
#define TIMER_ABSTIME 0

static int get_time()
{
	static int old_time = -1;
	if (old_time == -1)
		old_time = GetTickCount();
	
	return GetTickCount() - old_time;
}

static void clock_gettime(int u, struct timespec * t)
{
	t->tv_sec  = (unsigned long)get_time() / 1000;
	t->tv_nsec = (unsigned long)get_time() % 1000 * 1000000;
}

static void sleep_for(unsigned long nsec)
{
	sleep(nsec / 1000000);
}

static void gfx_nanoshell_wm_swap_buffers_end(void) {
	/*
    static struct timespec prev;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    struct timespec diff = gfx_nanoshell_wm_timediff(t, prev);
    if (diff.tv_sec == 0 && diff.tv_nsec < 1000000000 / 30) {
        struct timespec add = {0, 1000000000 / 30};
        struct timespec next = gfx_nanoshell_wm_timeadd(prev, add);
        //while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR) {
        //}
		struct timespec diff2 = gfx_nanoshell_wm_timediff(next, t);
		sleep_for(diff2.tv_sec * 1000000000 + diff2.tv_nsec);
		
        prev = next;
    } else {
        prev = t;
    }*/
	
	// NOTE: Run in the window procedure
	Image img;
	img.width  = WINDOW_WIDTH;
	img.height = WINDOW_HEIGHT;
	img.framebuffer = gFrameBuffer;
	
	VidBlitImage(&img, 0, 0);
}

static double gfx_nanoshell_wm_get_time(void) {
    return (double)get_time() / 1000;
}

struct GfxWindowManagerAPI gfx_nanoshell_wm_api = {
    gfx_nanoshell_wm_init,
    gfx_nanoshell_wm_set_keyboard_callbacks,
    gfx_nanoshell_wm_set_fullscreen_changed_callback,
    gfx_nanoshell_wm_set_fullscreen,
    gfx_nanoshell_wm_main_loop,
    gfx_nanoshell_wm_get_dimensions,
    gfx_nanoshell_wm_handle_events,
    gfx_nanoshell_wm_start_frame,
    gfx_nanoshell_wm_swap_buffers_begin,
    gfx_nanoshell_wm_swap_buffers_end,
    gfx_nanoshell_wm_get_time
};

// Rendering API
static GLuint opengl_vbo;

static uint32_t frame_count;
static uint32_t current_height;

static bool gfx_nanoshell_renderer_z_is_from_0_to_1(void) {
    return false;
}

static void gfx_nanoshell_renderer_unload_shader(struct ShaderProgram *old_prg) {
}

static void gfx_nanoshell_renderer_load_shader(struct ShaderProgram *new_prg) {
}

static struct ShaderProgram *gfx_nanoshell_renderer_create_and_load_new_shader(uint32_t shader_id) {
    return NULL;
}

static struct ShaderProgram *gfx_nanoshell_renderer_lookup_shader(uint32_t shader_id) {
    return NULL;
}

static void gfx_nanoshell_renderer_shader_get_info(struct ShaderProgram *prg, uint8_t *num_inputs, bool used_textures[2]) {
    *num_inputs = 0;
    used_textures[0] = false;
    used_textures[1] = false;
}

static uint32_t gfx_nanoshell_renderer_new_texture(void) {
    return 0;
}

static GLuint gfx_opengl_new_texture(void) {
	GLuint ret;
	glGenTextures(1, &ret);
	return ret;
}

static void gfx_nanoshell_renderer_select_texture(int tile, uint32_t texture_id) {
	glBindTexture(GL_TEXTURE_2D, texture_id);
}

static void gfx_nanoshell_renderer_upload_texture(const uint8_t *rgba32_buf, int width, int height) {
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba32_buf);
}

static uint32_t gfx_cm_to_opengl(uint32_t val) {
    if (val & G_TX_CLAMP) {
        //return GL_CLAMP_TO_EDGE;
        return GL_CLAMP;
    }
    //return (val & G_TX_MIRROR) ? GL_MIRRORED_REPEAT : GL_REPEAT;
    return GL_REPEAT;
}


static void gfx_nanoshell_renderer_set_sampler_parameters(int tile, bool linear_filter, uint32_t cms, uint32_t cmt) {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear_filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, gfx_cm_to_opengl(cms));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, gfx_cm_to_opengl(cmt));
}

static void gfx_nanoshell_renderer_set_depth_test(bool depth_test) {
    if (depth_test) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

static void gfx_nanoshell_renderer_set_depth_mask(bool z_upd) {
    glDepthMask(z_upd ? GL_TRUE : GL_FALSE);
}

static void gfx_nanoshell_renderer_set_zmode_decal(bool zmode_decal) {
    if (zmode_decal) {
        glPolygonOffset(-2, -2);
        glEnable(GL_POLYGON_OFFSET_FILL);
    } else {
        glPolygonOffset(0, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
}

static void gfx_nanoshell_renderer_set_viewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
    current_height = height;
}

static void gfx_nanoshell_renderer_set_scissor(int x, int y, int width, int height) {
    //glScissor(x, y, width, height);
}

static void gfx_nanoshell_renderer_set_use_alpha(bool use_alpha) {
    if (use_alpha) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
}

static void gfx_nanoshell_renderer_draw_triangles(float buf_vbo[], size_t buf_vbo_len, size_t buf_vbo_num_tris) {
    //glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buf_vbo_len, buf_vbo, GL_STATIC_DRAW);
    //glDrawArrays(GL_TRIANGLES, 0, 3 * buf_vbo_num_tris);
	
	glBegin(GL_TRIANGLES);
	
	for (size_t i = 0; i < buf_vbo_len; i++) {
		glVertex3f(buf_vbo[0], buf_vbo[1], buf_vbo[2]);
		buf_vbo += 3;
	}
	
	glEnd();
}

static void gfx_nanoshell_renderer_init(void) {
    glGenBuffers(1, &opengl_vbo);
    
    glBindBuffer(GL_ARRAY_BUFFER, opengl_vbo);
    
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void gfx_nanoshell_renderer_on_resize(void) {
}

static void gfx_nanoshell_renderer_start_frame(void) {
    frame_count++;

    //glDisable(GL_SCISSOR_TEST);
    glDepthMask(GL_TRUE); // Must be set to clear Z-buffer
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glEnable(GL_SCISSOR_TEST);
	
	
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	
}

static void gfx_nanoshell_renderer_end_frame(void) {
	glDrawText("test", 1, 1, 0xffffff);
}

static void gfx_nanoshell_renderer_finish_render(void) {
}

struct GfxRenderingAPI gfx_nanoshell_renderer_api = {
    gfx_nanoshell_renderer_z_is_from_0_to_1,
    gfx_nanoshell_renderer_unload_shader,
    gfx_nanoshell_renderer_load_shader,
    gfx_nanoshell_renderer_create_and_load_new_shader,
    gfx_nanoshell_renderer_lookup_shader,
    gfx_nanoshell_renderer_shader_get_info,
    gfx_nanoshell_renderer_new_texture,
    gfx_nanoshell_renderer_select_texture,
    gfx_nanoshell_renderer_upload_texture,
    gfx_nanoshell_renderer_set_sampler_parameters,
    gfx_nanoshell_renderer_set_depth_test,
    gfx_nanoshell_renderer_set_depth_mask,
    gfx_nanoshell_renderer_set_zmode_decal,
    gfx_nanoshell_renderer_set_viewport,
    gfx_nanoshell_renderer_set_scissor,
    gfx_nanoshell_renderer_set_use_alpha,
    gfx_nanoshell_renderer_draw_triangles,
    gfx_nanoshell_renderer_init,
    gfx_nanoshell_renderer_on_resize,
    gfx_nanoshell_renderer_start_frame,
    gfx_nanoshell_renderer_end_frame,
    gfx_nanoshell_renderer_finish_render
};


#endif
