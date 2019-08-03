/*
 * Copyright (c) 2018-2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#include <stdio.h>
#include <time.h>

#if defined(__APPLE__)
// FIXME
#elif defined(_WIN32)
# include <windows.h>
#else
# include <X11/Xresource.h>
#endif

#include <pugl/pugl.h>

#include "core_internal.h"
#include <d2tk/frontend_pugl.h>

#include <d2tk/backend.h>

struct _d2tk_pugl_t {
	const d2tk_pugl_config_t *config;
	bool done;
	PuglView *view;
	d2tk_base_t *base;
	void *ctx;
};

static inline void
_d2tk_pugl_close(d2tk_pugl_t *dpugl)
{
	dpugl->done = true;
}

static inline void
_d2tk_pugl_expose(d2tk_pugl_t *dpugl)
{
	d2tk_base_t *base = dpugl->base;

	d2tk_coord_t w;
	d2tk_coord_t h;
	d2tk_base_get_dimensions(base, &w, &h);

	d2tk_base_pre(base);

	dpugl->config->expose(dpugl->config->data, w, h);

	d2tk_base_post(base);

	if(d2tk_base_get_again(base))
	{
		puglPostRedisplay(dpugl->view);
	}
}

static void
_d2tk_pugl_modifiers(d2tk_pugl_t *dpugl, unsigned state)
{
	d2tk_base_t *base = dpugl->base;

	d2tk_base_set_shift(base, state & PUGL_MOD_SHIFT ? true : false);
	d2tk_base_set_ctrl(base, state & PUGL_MOD_CTRL ? true : false);
	d2tk_base_set_alt(base, state & PUGL_MOD_ALT ? true : false);
}

static void
_d2tk_pugl_event_func(PuglView *view, const PuglEvent *e)
{
	d2tk_pugl_t *dpugl = puglGetHandle(view);
	d2tk_base_t *base = dpugl->base;

	switch(e->type)
	{
		case PUGL_CONFIGURE:
		{
			d2tk_base_set_dimensions(base, e->configure.width, e->configure.height);
			puglPostRedisplay(dpugl->view);
		}	break;
		case PUGL_EXPOSE:
		{
			_d2tk_pugl_expose(dpugl);
		}	break;
		case PUGL_CLOSE:
		{
			_d2tk_pugl_close(dpugl);
		}	break;

		case PUGL_FOCUS_IN:
			// fall-through
		case PUGL_FOCUS_OUT:
		{
			puglPostRedisplay(dpugl->view);
		} break;

		case PUGL_ENTER_NOTIFY:
		case PUGL_LEAVE_NOTIFY:
		{
			_d2tk_pugl_modifiers(dpugl, e->crossing.state);
			d2tk_base_set_mouse_pos(base, e->crossing.x, e->crossing.y);

			puglPostRedisplay(dpugl->view);
		} break;

		case PUGL_BUTTON_PRESS:
		case PUGL_BUTTON_RELEASE:
		{
			_d2tk_pugl_modifiers(dpugl, e->button.state);
			d2tk_base_set_mouse_pos(base, e->button.x, e->button.y);

			switch(e->button.button)
			{
				case 3:
				{
					d2tk_base_set_mouse_r(base, e->type == PUGL_BUTTON_PRESS);
				} break;
				case 2:
				{
					d2tk_base_set_mouse_m(base, e->type == PUGL_BUTTON_PRESS);
				} break;
				case 1:
					// fall-through
				default:
				{
					d2tk_base_set_mouse_l(base, e->type == PUGL_BUTTON_PRESS);
				} break;
			}

			puglPostRedisplay(dpugl->view);
		} break;

		case PUGL_MOTION_NOTIFY:
		{
			_d2tk_pugl_modifiers(dpugl, e->motion.state);
			d2tk_base_set_mouse_pos(base, e->motion.x, e->motion.y);

			puglPostRedisplay(dpugl->view);
		} break;

		case PUGL_SCROLL:
		{
			_d2tk_pugl_modifiers(dpugl, e->scroll.state);
			d2tk_base_set_mouse_pos(base, e->scroll.x, e->scroll.y);
			d2tk_base_add_mouse_scroll(base, e->scroll.dx, e->scroll.dy);

			puglPostRedisplay(dpugl->view);
		} break;

		case PUGL_KEY_PRESS:
		{
			_d2tk_pugl_modifiers(dpugl, e->key.state);

			if(e->key.special)
			{
				bool handled = false;

				switch(e->key.special)
				{
					case PUGL_KEY_LEFT:
					{
						d2tk_base_set_left(base, true);
						handled = true;
					} break;
					case PUGL_KEY_RIGHT:
					{
						d2tk_base_set_right(base, true);
						handled = true;
					} break;
					case PUGL_KEY_UP:
					{
						d2tk_base_set_up(base, true);
						handled = true;
					} break;
					case PUGL_KEY_DOWN:
					{
						d2tk_base_set_down(base, true);
						handled = true;
					} break;
					case PUGL_KEY_SHIFT:
					{
						d2tk_base_set_shift(base, true);
						handled = true;
					} break;
					case PUGL_KEY_CTRL:
					{
						d2tk_base_set_ctrl(base, true);
						handled = true;
					} break;
					case PUGL_KEY_ALT:
					{
						d2tk_base_set_alt(base, true);
						handled = true;
					} break;
					default:
					{
						// nothing
					} break;
				}

				if(handled)
				{
					puglPostRedisplay(dpugl->view);
				}
			}
			else
			{
				const char character = d2tk_base_get_ctrl(base)
					? e->key.character | 0x60
					: e->key.utf8[0];
				d2tk_base_append_char(base, character);
				puglPostRedisplay(dpugl->view);
			}
		} break;
		case PUGL_KEY_RELEASE:
		{
			_d2tk_pugl_modifiers(dpugl, e->key.state);

			if(e->key.special)
			{
				bool handled = false;

				switch(e->key.special)
				{
					case PUGL_KEY_LEFT:
					{
						d2tk_base_set_left(base, false);
						handled = true;
					} break;
					case PUGL_KEY_RIGHT:
					{
						d2tk_base_set_right(base, false);
						handled = true;
					} break;
					case PUGL_KEY_UP:
					{
						d2tk_base_set_up(base, false);
						handled = true;
					} break;
					case PUGL_KEY_DOWN:
					{
						d2tk_base_set_down(base, false);
						handled = true;
					} break;
					case PUGL_KEY_SHIFT:
					{
						d2tk_base_set_shift(base, false);
						handled = true;
					} break;
					case PUGL_KEY_CTRL:
					{
						d2tk_base_set_ctrl(base, false);
						handled = true;
					} break;
					case PUGL_KEY_ALT:
					{
						d2tk_base_set_alt(base, false);
						handled = true;
					} break;
					default:
					{
						// nothing
					} break;
				}

				if(handled)
				{
					puglPostRedisplay(dpugl->view);
				}
			}
			else
			{
				puglPostRedisplay(dpugl->view);
			}
		} break;
		case PUGL_NOTHING:
		{
			// nothing
		}	break;
	}
}

D2TK_API int
d2tk_pugl_step(d2tk_pugl_t *dpugl)
{
	const PuglStatus stat = puglProcessEvents(dpugl->view);
	(void)stat;

	return dpugl->done;
}

//#define D2TK_PUGL_POLLING

D2TK_API void
d2tk_pugl_run(d2tk_pugl_t *dpugl, const sig_atomic_t *done)
{
#if !defined(D2TK_PUGL_POLLING) && !defined(__APPLE__) && !defined(_WIN32)
	const unsigned step = 1000000000 / 24;
	struct timespec to;
	clock_gettime(CLOCK_MONOTONIC, &to);
#endif

	while(!*done)
	{
#if !defined(D2TK_PUGL_POLLING) && !defined(__APPLE__) && !defined(_WIN32)
		if(clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &to, NULL))
		{
			continue;
		}

		to.tv_nsec += step;
		while(to.tv_nsec >= 1000000000)
		{
			to.tv_sec += 1;
			to.tv_nsec -= 1000000000;
		}

#else
		puglWaitForEvent(dpugl->view);
#endif

		if(d2tk_pugl_step(dpugl))
		{
			break;
		}
	}
}

D2TK_API void
d2tk_pugl_free(d2tk_pugl_t *dpugl)
{
	if(dpugl->ctx)
	{
		puglEnterContext(dpugl->view);
		if(dpugl->base)
		{
			d2tk_base_free(dpugl->base);
		}
		d2tk_core_driver.free(dpugl->ctx);
		puglLeaveContext(dpugl->view, false);
	}

	if(dpugl->view)
	{
		if(puglGetVisible(dpugl->view))
		{
			puglHideWindow(dpugl->view);
		}
		puglDestroy(dpugl->view);
	}

	free(dpugl);
}

D2TK_API float
d2tk_pugl_get_scale()
{
	const char *D2TK_SCALE = getenv("D2TK_SCALE");
	const float scale = D2TK_SCALE ? atof(D2TK_SCALE) : 1.f;
	const float dpi0 = 96.f; // reference DPI we're designing for
	float dpi1 = dpi0;

#if defined(__APPLE__)
	// FIXME
#elif defined(_WIN32)
	// GetDpiForSystem/Monitor/Window is Win10 only
	HDC screen = GetDC(NULL);
	dpi1 = GetDeviceCaps(screen, LOGPIXELSX);
	ReleaseDC(NULL, screen);
#else
	Display *disp = XOpenDisplay(0);
	if(disp)
	{
		// modern X actually lies here, but proprietary nvidia
		dpi1 = XDisplayWidth(disp, 0) * 25.4f / XDisplayWidthMM(disp, 0);

		// read DPI from users's ~/.Xresources
		char *resource_string = XResourceManagerString(disp);
		XrmInitialize();
		if(resource_string)
		{
			XrmDatabase db = XrmGetStringDatabase(resource_string);
			if(db)
			{
				char *type = NULL;
				XrmValue value;

				XrmGetResource(db, "Xft.dpi", "String", &type, &value);
				if(value.addr)
				{
					dpi1 = atof(value.addr);
				}
			}
		}

		XCloseDisplay(disp);
	}
#endif

	return scale * dpi1 / dpi0;
}

D2TK_API d2tk_pugl_t *
d2tk_pugl_new(const d2tk_pugl_config_t *config, uintptr_t *widget)
{
	d2tk_pugl_t *dpugl = calloc(1, sizeof(d2tk_pugl_t));
	if(!dpugl)
	{
		goto fail;
	}

	dpugl->config = config;

	dpugl->view = puglInit(NULL, NULL);
	if(!dpugl->view)
	{
		fprintf(stderr, "puglInit failed\n");
		goto fail;
	}

	puglInitWindowClass(dpugl->view, "d2tk");
	puglInitWindowSize(dpugl->view, config->w, config->h);
	if(config->min_w && config->min_h)
	{
		puglInitWindowMinSize(dpugl->view, config->min_w, config->min_h);
	}
	if(config->parent)
	{
		puglInitWindowParent(dpugl->view, config->parent);
		puglInitTransientFor(dpugl->view, config->parent);
	}
	if(config->fixed_aspect)
	{
		puglInitWindowAspectRatio(dpugl->view, config->w, config->h,
			config->w, config->h);
	}
	puglInitResizable(dpugl->view, !config->fixed_size);
	puglSetHandle(dpugl->view, dpugl);
	puglSetEventFunc(dpugl->view, _d2tk_pugl_event_func);

#if defined(PUGL_HAVE_CAIRO)
	puglInitContextType(dpugl->view, PUGL_CAIRO_GL);
#else
	puglInitContextType(dpugl->view, PUGL_GL);
#endif
	const int stat = puglCreateWindow(dpugl->view, "d2tk");

	if(stat != 0)
	{
		fprintf(stderr, "puglCreateWindow failed\n");
		goto fail;
	}
	puglShowWindow(dpugl->view);

	if(widget)
	{
		*widget = puglGetNativeWindow(dpugl->view);
	}

#if defined(_WIN32)
	void *ctx = NULL;
#else
	puglEnterContext(dpugl->view);
	void *ctx = puglGetContext(dpugl->view);
#endif
	dpugl->ctx = d2tk_core_driver.new(config->bundle_path, ctx);
	puglLeaveContext(dpugl->view, false);

	if(!dpugl->ctx)
	{
		goto fail;
	}

	dpugl->base = d2tk_base_new(&d2tk_core_driver, dpugl->ctx);
	if(!dpugl->base)
	{
		goto fail;
	}

	d2tk_base_set_dimensions(dpugl->base, config->w, config->h);

	return dpugl;

fail:
	if(dpugl)
	{
		if(dpugl->view)
		{
			if(dpugl->ctx)
			{
				puglEnterContext(dpugl->view);
				d2tk_core_driver.free(dpugl->ctx);
				puglLeaveContext(dpugl->view, false);
			}

			puglDestroy(dpugl->view);
		}

		free(dpugl);
	}

	return NULL;
}

D2TK_API void
d2tk_pugl_redisplay(d2tk_pugl_t *dpugl)
{
	puglPostRedisplay(dpugl->view);
}

D2TK_API int
d2tk_pugl_resize(d2tk_pugl_t *dpugl, d2tk_coord_t w, d2tk_coord_t h)
{
	d2tk_base_set_dimensions(dpugl->base, w, h);
	puglPostRedisplay(dpugl->view);

	return 0;
}

D2TK_API d2tk_base_t *
d2tk_pugl_get_base(d2tk_pugl_t *dpugl)
{
	return dpugl->base;
}
