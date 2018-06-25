#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

#include <freeader.h>
#include <jbig85.h>

#define NK_PUGL_IMPLEMENTATION
#include <nk_pugl/nk_pugl.h>

#define WIDTH 800
#define HEIGHT 600
#define BUFSZ (WIDTH * HEIGHT / 8)

typedef struct _app_t app_t;

struct _app_t {
	nk_pugl_window_t win;

	uint32_t argb [800*600];
	struct nk_image img;

	float scale;
	unsigned page;
	unsigned section;
	unsigned long ymin;
	unsigned long ymax;

	FILE *fin;

	size_t inbuflen;
	size_t outbuflen;
	uint8_t *inbuf;
	uint8_t *outbuf;

	struct jbg85_dec_state state;
	size_t len;
	size_t cnt;

	head_t *head;
	bool dirty;
};

#if 0
static const uint32_t fg = 0xffbbbbbb;
static const uint32_t bg = 0xff2d2d2d;
#else
static const uint32_t fg = 0xff000000;
static const uint32_t bg = 0xffeeeeee;
#endif

static const uint8_t bitmask [8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

static struct nk_image
_page_load(nk_pugl_window_t *win, int w, int h, uint32_t *data)
{
	GLuint tex = 0;

	if(!win->view)
		return nk_image_id(tex);

	puglEnterContext(win->view);
	{
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		if(!win->glGenerateMipmap) // for GL >= 1.4 && < 3.1
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		if(win->glGenerateMipmap) // for GL >= 3.1
			win->glGenerateMipmap(GL_TEXTURE_2D);
	}
	puglLeaveContext(win->view, false);

	return nk_image_id(tex);
}

static void
_page_unload(nk_pugl_window_t *win, struct nk_image img)
{
	if(!win->view)
		return;

	if(img.handle.id)
	{
		puglEnterContext(win->view);
		{
			glDeleteTextures(1, (const GLuint *)&img.handle.id);
		}
		puglLeaveContext(win->view, false);
	}
}

static int
_out(const struct jbg85_dec_state *state, uint8_t *start, size_t len,
	unsigned long y, void *data)
{
	app_t *app = data;
	//printf("_out: %lu %lu %lu\n", y, app->ymin, app->ymax);

	if( (y >= app->ymin) && (y < app->ymax) )
	{
		const int y0 = y % app->head->page_height;
		int offset = y0 * app->head->page_width;

		for(size_t i=0; i<len; i++)
		{
			const uint8_t raw = start[i];

			for(int j=0; j<8; j++, offset++)
			{
				app->argb[offset] = raw & bitmask[j]
					? fg
					: bg;
			}
		}

		if(y == app->ymax - 1)
		{
			//printf("Page: %i of %u\n", app->page + 1, app->head->page_number);
			return 1;
		}
	}

	return 0;
}

static void
_page_set(app_t *app, unsigned page)
{
	if(page > app->head->page_number - 1)
		page = app->head->page_number - 1;
	
	const unsigned section = page / app->head->pages_per_section;

	if( (section == app->section) && (page > app->page) )
	{
		app->page = page;

		app->ymin = (app->page % app->head->pages_per_section) * app->head->page_height;
		app->ymax = app->ymin + app->head->page_height;

		//printf("advance: %u %u %lu %lu\n", app->page, app->section, app->ymin, app->ymax);
	}
	else 
	{
		app->page = page;
		app->section = app->page / app->head->pages_per_section;

		app->ymin = (app->page % app->head->pages_per_section) * app->head->page_height;
		app->ymax = app->ymin + app->head->page_height;
		
		//printf("seek: %u %u %lu %lu %u\n", app->page, app->section, app->ymin, app->ymax,
		//	app->head->section_offset[app->section]);

		app->len = 0;
		app->cnt = 0;
		jbg85_dec_init(&app->state, app->outbuf, app->outbuflen, _out, app);
		fseek(app->fin, app->head->section_offset[app->section], SEEK_SET);
	}
}

static void
_next(app_t *app)
{
	int result;
	size_t cnt2;

	// process remaining bytes from input buffer
	while(app->cnt != app->len)
	{
		result = jbg85_dec_in(&app->state, app->inbuf + app->cnt,
			app->len - app->cnt, &cnt2);
		app->cnt += cnt2;
		//printf("oldlen: %i %zu %zu\n", result, app->len, app->cnt);

		if(result == JBG_EOK_INTR)
		{
			_page_unload(&app->win, app->img);
			app->img = _page_load(&app->win, WIDTH, HEIGHT, app->argb);

			return;
		}

    if(result != JBG_EAGAIN)
      break;
	}

	// load new chunk to input buffer
  while( (app->len = fread(app->inbuf, 1, app->inbuflen, app->fin)) )
	{
		app->cnt = 0;
		while(app->cnt != app->len)
		{
			result = jbg85_dec_in(&app->state, app->inbuf + app->cnt,
				app->len - app->cnt, &cnt2);
			app->cnt += cnt2;
			//printf("newlen: %i %zu %zu\n", result, app->len, app->cnt);

			if(result == JBG_EOK_INTR)
			{
				_page_unload(&app->win, app->img);
				app->img = _page_load(&app->win, WIDTH, HEIGHT, app->argb);

				return;
			}

			if(result != JBG_EAGAIN)
				break;
		}
  }

	return;
}

static void
_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	app_t *app = data;

	struct nk_input *in = &ctx->input;

	const bool has_home = nk_input_is_key_pressed(in, NK_KEY_TEXT_LINE_START);
	const bool has_end = nk_input_is_key_pressed(in, NK_KEY_TEXT_LINE_END);
	const bool has_page_down = nk_input_is_key_pressed(in, NK_KEY_SCROLL_DOWN);
	const bool has_page_up = nk_input_is_key_pressed(in, NK_KEY_SCROLL_UP);
	const int scroll_delta = in->mouse.scroll_delta;

	if(nk_begin(ctx, "FreEader", wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		nk_window_set_bounds(ctx, wbounds);

		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

		nk_layout_row_static(ctx, 600.f, 800.f, 1);
		const struct nk_rect bb = nk_widget_bounds(ctx);
		nk_image(ctx, app->img);
		nk_stroke_rect(canvas, bb, 0.f, 1.f, nk_rgb(0x88, 0x88, 0x88));

		nk_layout_row_dynamic(ctx, 20.f, 5);
		nk_spacing(ctx, 1);

		const int old_page = app->page + 1;
		int new_page = old_page;

		if(nk_button_label(ctx, "1"))
			new_page = 1;

		if(has_home)
			new_page = 1;
		else if(has_end)
			new_page = app->head->page_number;
		else if(has_page_down)
			new_page += 1;
		else if(has_page_up)
			new_page -= 1;
		else if(scroll_delta)
			new_page -= scroll_delta;

		// handle under/overflow
		if(new_page < 1)
			new_page = 1;
		else if(new_page > app->head->page_number)
			new_page = app->head->page_number;

		new_page = nk_propertyi(ctx, "#", 1, new_page, app->head->page_number, 1.f, 0.f);

		char maxpage [32];
		snprintf(maxpage, 32, "%i", app->head->page_number);
		if(nk_button_label(ctx, maxpage))
			new_page = app->head->page_number;

		if(new_page != old_page)
		{
			app->page = new_page - 1;
			_page_set(app, app->page);
			_next(app);
		}
	}
	nk_end(ctx);

	if(has_home || has_end || has_page_down || has_page_up || (scroll_delta != 0.f) )
	{
		nk_pugl_post_redisplay(&app->win);
		app->dirty = true;
	}
	else
	{
		app->dirty = false;
	}
}

int
main(int argc, char **argv)
{
	static app_t app;

	app.page = UINT_MAX;
	app.section = UINT_MAX;

	if(!argv[1])
		return -1;

	int page = 1;
	if(argv[2])
		page = atoi(argv[2]);

	app.scale = 1.f; //DPI_SCREEN / DPI_DISPLAY;

	app.fin = fopen(argv[1], "rb");
	if(!app.fin)
		return -1;

	head_t head;
	fread(&head, 1, sizeof(head_t), app.fin);
	if(strncmp(head.magic, FREEADER_MAGIC, FREEADER_MAGIC_LEN))
		return -1;
	head.page_width = be32toh(head.page_width);
	head.page_height = be32toh(head.page_height);
	head.page_number = be32toh(head.page_number);
	head.pages_per_section = be32toh(head.pages_per_section);
	uint32_t section_number = (head.page_number % head.pages_per_section) == 0
		? head.page_number / head.pages_per_section
		: head.page_number / head.pages_per_section + 1;

	size_t offset_size = section_number*sizeof(uint32_t);
	app.head = malloc(sizeof(head_t) + offset_size);
	memcpy(app.head, &head, sizeof(head));
	fread(app.head->section_offset, 1, offset_size, app.fin);
	for(unsigned s=0; s<section_number; s++)
		app.head->section_offset[s] = be32toh(app.head->section_offset[s]);

  app.inbuflen = 256 * 1024; //TODO
  app.outbuflen = ((head.page_width >> 3) + !!(head.page_width & 7)) * 3;
	
	app.inbuf = malloc(app.inbuflen);
	app.outbuf = malloc(app.outbuflen);

	nk_pugl_window_t *win = &app.win;
	nk_pugl_config_t *cfg = &win->cfg;

	cfg->width = WIDTH + 15;
	cfg->height = HEIGHT + 35;
	cfg->resizable = false;
	cfg->fixed_aspect = true;
	cfg->ignore = false;
	cfg->class = "freeader";
	cfg->title = "FreEader";
	cfg->parent = 0;
	cfg->data = &app;
	cfg->expose = _expose;
	cfg->font.face = NULL;
	cfg->font.size = 13;

	nk_pugl_init(win);
	nk_pugl_show(win);

	_page_set(&app, page - 1);
	_next(&app);

	bool done = false;
	while(!done)
	{
		nk_pugl_wait_for_event(win);

		do {
			done = !done ? nk_pugl_process_events(win) : done;
		} while(app.dirty);
	}

	nk_pugl_hide(win);
	nk_pugl_shutdown(win);

	if(app.head)
		free(app.head);
	if(app.inbuf)
		free(app.inbuf);
	if(app.outbuf)
		free(app.outbuf);

	return 0;
}
