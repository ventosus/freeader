#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <freeader.h>
#include <jbig85.h>

#include <d2tk/frontend_pugl.h>

#define WIDTH 800
#define HEIGHT 600
#define STRIDE (WIDTH * sizeof(uint32_t))
#define FOOTER 24
#define BUFSZ (WIDTH * HEIGHT / 8)

typedef struct _app_t app_t;

struct _app_t {
	d2tk_pugl_config_t config;
	d2tk_pugl_t *dpugl;

	uint32_t argb [800*600];

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

static int
_out(const struct jbg85_dec_state *state __attribute__((unused)),
	uint8_t *start, size_t len, unsigned long y, void *data)
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

		app->ymin = (app->page % app->head->pages_per_section)
			* app->head->page_height;
		app->ymax = app->ymin + app->head->page_height;

		//printf("advance: %u %u %lu %lu\n", app->page, app->section,
		//	app->ymin, app->ymax);
	}
	else 
	{
		app->page = page;
		app->section = app->page / app->head->pages_per_section;

		app->ymin = (app->page % app->head->pages_per_section)
			* app->head->page_height;
		app->ymax = app->ymin + app->head->page_height;
		
		//printf("seek: %u %u %lu %lu %u\n", app->page, app->section,
		//	app->ymin, app->ymax, app->head->section_offset[app->section]);

		app->len = 0;
		app->cnt = 0;
		jbg85_dec_init(&app->state, app->outbuf, app->outbuflen, _out, app);
		fseek(app->fin, app->head->section_offset[app->section], SEEK_SET);

		uint32_t len;
		fread(&len, sizeof(uint32_t), 1, app->fin);
		len = be32toh(len);
		if(len > 0)
		{
			char *link = alloca(len + 1);
			if(link)
			{
				fread(link, len, 1, app->fin);
				link[len] = '\0';
				fprintf(stdout, "link: %s\n", link);
			}
			else
			{
				fseek(app->fin, len, SEEK_CUR);
			}
		}
	}
}

static void
_next(app_t *app)
{
	// process remaining bytes from input buffer
	while(app->cnt != app->len)
	{
		size_t cnt2;
		const int result = jbg85_dec_in(&app->state, app->inbuf + app->cnt,
			app->len - app->cnt, &cnt2);
		app->cnt += cnt2;
		//printf("oldlen: %i %zu %zu\n", result, app->len, app->cnt);

		if(result == JBG_EOK_INTR)
		{
			d2tk_pugl_redisplay(app->dpugl);

			return;
		}

    if(result != JBG_EAGAIN)
		{
      break;
		}
	}

	// load new chunk to input buffer
  while( (app->len = fread(app->inbuf, 1, app->inbuflen, app->fin)) )
	{
		app->cnt = 0;
		while(app->cnt != app->len)
		{
			size_t cnt2;
			const int result = jbg85_dec_in(&app->state, app->inbuf + app->cnt,
				app->len - app->cnt, &cnt2);
			app->cnt += cnt2;
			//printf("newlen: %i %zu %zu\n", result, app->len, app->cnt);

			if(result == JBG_EOK_INTR)
			{
				d2tk_pugl_redisplay(app->dpugl);

				return;
			}

			if(result != JBG_EAGAIN)
			{
				break;
			}
		}
  }

	return;
}

static void
_expose_page(app_t *app, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_pugl_get_base(app->dpugl);

	d2tk_base_bitmap(base, WIDTH, HEIGHT, STRIDE, app->argb, app->page, rect,
		D2TK_ALIGN_CENTERED);
}

static void
_expose_footer(app_t *app, const d2tk_rect_t *rect)
{
	d2tk_base_t *base = d2tk_pugl_get_base(app->dpugl);

	const int32_t old_page = app->page + 1;
	int32_t new_page = old_page;

	const d2tk_coord_t hfrac [6] = { 1, 1, 1, 1, 1, 1 };
	D2TK_BASE_LAYOUT(rect, 6, hfrac, D2TK_FLAG_LAYOUT_X_REL, lay)
	{
		const d2tk_rect_t *hrect = d2tk_layout_get_rect(lay);
		const unsigned k = d2tk_layout_get_index(lay);

		switch(k)
		{
			case 0:
			{
				if(d2tk_base_button_label_is_changed(base, D2TK_ID, -1, "Home",
					D2TK_ALIGN_CENTERED, hrect))
				{
					new_page = 1;
				}
			} break;
			case 1:
			{
				if(d2tk_base_button_label_is_changed(base, D2TK_ID, -1, "Prev",
					D2TK_ALIGN_CENTERED, hrect)
					|| d2tk_base_get_left(base)
					|| d2tk_base_get_up(base) )
				{
					new_page -= 1;
				}
			} break;
			case 2:
			{
				if(d2tk_base_button_label_is_changed(base, D2TK_ID, -1, "Next",
					D2TK_ALIGN_CENTERED, hrect)
					|| d2tk_base_get_right(base)
					|| d2tk_base_get_down(base) )
				{
					new_page += 1;
				}
			} break;
			case 3:
			{
				if(d2tk_base_button_label_is_changed(base, D2TK_ID, -1, "End",
					D2TK_ALIGN_CENTERED, hrect))
				{
					new_page = app->head->page_number;
				}
			} break;
			case 4:
			{
				d2tk_base_prop_int32(base, D2TK_ID, hrect,
					1, &new_page, app->head->page_number);
			} break;
			case 5:
			{
				d2tk_base_label(base, -1, "Freeader", 1.f, hrect,
					D2TK_ALIGN_MIDDLE | D2TK_ALIGN_RIGHT);
			} break;
		}
	}

	// handle under/overflow
	if(new_page < 1)
	{
		new_page = 1;
	}
	else if(new_page > (int)app->head->page_number)
	{
		new_page = app->head->page_number;
	}

	if(new_page != old_page)
	{
		app->page = new_page - 1;

		_page_set(app, app->page);
		_next(app);
	}
}

static int
_expose(void *data, d2tk_coord_t w, d2tk_coord_t h)
{
	app_t *app = data;

	const d2tk_rect_t rect = D2TK_RECT(0, 0, w, h);

	const d2tk_coord_t vfrac [2] = { h - FOOTER, FOOTER };
	D2TK_BASE_LAYOUT(&rect, 2, vfrac, D2TK_FLAG_LAYOUT_Y_ABS, lay)
	{
		const d2tk_rect_t *vrect = d2tk_layout_get_rect(lay);
		const unsigned k = d2tk_layout_get_index(lay);

		switch(k)
		{
			case 0:
			{
				_expose_page(app, vrect);
			} break;
			case 1:
			{
				_expose_footer(app, vrect);
			} break;
		}
	}

	return 0;
}

int
main(int argc __attribute__((unused)), char **argv)
{
	static app_t app;

	app.page = UINT_MAX;
	app.section = UINT_MAX;

	if(!argv[1])
	{
		return -1;
	}

	int page = 1;
	if(argv[2])
	{
		page = atoi(argv[2]);
	}

	app.scale = 1.f; //DPI_SCREEN / DPI_DISPLAY;

	app.fin = fopen(argv[1], "rb");
	if(!app.fin)
	{
		return -1;
	}

	head_t head;
	fread(&head, 1, sizeof(head_t), app.fin);
	if(strncmp(head.magic, FREEADER_MAGIC, FREEADER_MAGIC_LEN))
	{
		return -1;
	}

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
	{
		app.head->section_offset[s] = be32toh(app.head->section_offset[s]);
	}

  app.inbuflen = 256 * 1024; //TODO
  app.outbuflen = ((head.page_width >> 3) + !!(head.page_width & 7)) * 3;
	
	app.inbuf = malloc(app.inbuflen);
	app.outbuf = malloc(app.outbuflen);

	const d2tk_coord_t w = WIDTH;
	const d2tk_coord_t h = HEIGHT + FOOTER;

	d2tk_pugl_config_t *config = &app.config;
	config->parent = 0;
	config->bundle_path = "/usr/local/share/freeader/"; //FIXME
	config->min_w = w/2;
	config->min_h = h/2;
	config->w = w;
	config->h = h;
	config->fixed_size = false;
	config->fixed_aspect = false;
	config->expose = _expose;
	config->data = &app;

	uintptr_t widget;
	app.dpugl = d2tk_pugl_new(config, &widget);
	if(!app.dpugl)
	{
		return -1;
	}

	_page_set(&app, page - 1);
	_next(&app);

	sig_atomic_t done = 0; //FIXME
	d2tk_pugl_run(app.dpugl, &done);

	d2tk_pugl_free(app.dpugl);

	if(app.head)
	{
		free(app.head);
	}

	if(app.inbuf)
	{
		free(app.inbuf);
	}

	if(app.outbuf)
	{
		free(app.outbuf);
	}

	return 0;
}
