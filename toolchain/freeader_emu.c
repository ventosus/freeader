#include <Elementary.h>

#include <freeader.h>
#include <jbig85.h>

#define DPI_SCREEN 120.f
#define DPI_DISPLAY 231.f

typedef struct _prog_t prog_t;

struct _prog_t {
	Evas_Object *win;
	Evas_Object *img;
	uint32_t *argb;
	int initialized;

	float scale;
	int page;
	int section;
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
};

static const uint32_t fg = 0xff000000;
static const uint32_t bg = 0xffffffff;

static void _next(prog_t *prog);

static void
_delete_request(void *data, Evas_Object *obj, void *event)
{
	elm_exit();
}

static const uint8_t bitmask [8] = {
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
};

static int
_out(const struct jbg85_dec_state *state, uint8_t *start, size_t len,
	unsigned long y, void *data)
{
	prog_t *prog = data;
	//printf("_out: %lu %lu %lu\n", y, prog->ymin, prog->ymax);

	if( (y >= prog->ymin) && (y < prog->ymax) )
	{
		const int y0 = y % prog->head->page_height;
		int offset = y0 * prog->head->page_width;

		for(int i=0; i<len; i++)
		{
			const uint8_t raw = start[i];

			for(int j=0; j<8; j++, offset++)
			{
				prog->argb[offset] = raw & bitmask[j]
					? fg
					: bg;
			}
		}

		if(y == prog->ymax - 1)
		{
			printf("Page: %i of %u\n", prog->page + 1, prog->head->page_number);
			return 1;
		}
	}

	return 0;
}

static void
_page_set(prog_t *prog, int page)
{
	if(page < 0)
		page = 0;

	if(page > prog->head->page_number - 1)
		page = prog->head->page_number - 1;
	
	uint32_t section = page / prog->head->pages_per_section;

	if( (section == prog->section) && (page > prog->page) )
	{
		prog->page = page;

		prog->ymin = (prog->page % prog->head->pages_per_section) * prog->head->page_height;
		prog->ymax = prog->ymin + prog->head->page_height;

		//printf("advance: %u %u %lu %lu\n", prog->page, prog->section, prog->ymin, prog->ymax);
	}
	else 
	{
		prog->page = page;
		prog->section = prog->page / prog->head->pages_per_section;

		prog->ymin = (prog->page % prog->head->pages_per_section) * prog->head->page_height;
		prog->ymax = prog->ymin + prog->head->page_height;
		
		//printf("seek: %u %u %lu %lu %u\n", prog->page, prog->section, prog->ymin, prog->ymax,
		//	prog->head->section_offset[prog->section]);

		prog->len = 0;
		prog->cnt = 0;
		jbg85_dec_init(&prog->state, prog->outbuf, prog->outbuflen, _out, prog);
		fseek(prog->fin, prog->head->section_offset[prog->section], SEEK_SET);
	}
}

static void
_ui_key(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
	prog_t *prog = data;
	Evas_Event_Key_Down *info = event_info;

	//printf("_ui_key: %s\n", info->keyname);

	if(  !strcmp(info->keyname, "Escape")
		|| !strcmp(info->keyname, "q") )
	{
		elm_exit();
	}
	else if(!strcmp(info->keyname, "Next")
		|| !strcmp(info->keyname, "Right")
		|| !strcmp(info->keyname, "Down")
		|| !strcmp(info->keyname, "space")
		|| !strcmp(info->keyname, "j") )
	{
		_page_set(prog, prog->page + 1);
		_next(prog);
	}
	else if(!strcmp(info->keyname, "Prior")
		|| !strcmp(info->keyname, "Left")
		|| !strcmp(info->keyname, "Up")
		|| !strcmp(info->keyname, "k") )
	{
		_page_set(prog, prog->page - 1);
		_next(prog);
	}
}

static void
_ui_init(prog_t *prog)
{
	const unsigned long W = prog->head->page_width * prog->scale;
	const unsigned long H = prog->head->page_height * prog->scale;

	prog->win = elm_win_util_standard_add("freeader_emu", "freeader emu");
	if(prog->win)
	{
		evas_object_event_callback_add(prog->win, EVAS_CALLBACK_KEY_DOWN, _ui_key, prog);
		evas_object_smart_callback_add(prog->win, "delete,request", _delete_request, NULL);
		evas_object_resize(prog->win, W, H);
		evas_object_show(prog->win);

		prog->img = evas_object_image_add(evas_object_evas_get(prog->win));
		if(prog->img)
		{
			evas_object_show(prog->img);
			evas_object_focus_set(prog->img, EINA_TRUE);

			evas_object_image_fill_set(prog->img, 0, 0, W, H);
			evas_object_image_size_set(prog->img, prog->head->page_width, prog->head->page_height);
			evas_object_image_smooth_scale_set(prog->img, EINA_FALSE);
			evas_object_resize(prog->img, W, H);
			
			prog->argb = evas_object_image_data_get(prog->img, EINA_TRUE);
		}
	}
}

static void
_next(prog_t *prog)
{
	int result;
	size_t cnt2;

	// process remaining bytes from input buffer
	while(prog->cnt != prog->len)
	{
		result = jbg85_dec_in(&prog->state, prog->inbuf + prog->cnt,
			prog->len - prog->cnt, &cnt2);
		prog->cnt += cnt2;
		//printf("oldlen: %i %zu %zu\n", result, prog->len, prog->cnt);

		if(result == JBG_EOK_INTR)
		{
			evas_object_image_data_update_add(prog->img, 0, 0, prog->head->page_width, prog->head->page_height);
			return;
		}

    if(result != JBG_EAGAIN)
      break;
	}

	// load new chunk to input buffer
  while( (prog->len = fread(prog->inbuf, 1, prog->inbuflen, prog->fin)) )
	{
		prog->cnt = 0;
		while(prog->cnt != prog->len)
		{
			result = jbg85_dec_in(&prog->state, prog->inbuf + prog->cnt,
				prog->len - prog->cnt, &cnt2);
			prog->cnt += cnt2;
			//printf("newlen: %i %zu %zu\n", result, prog->len, prog->cnt);

			if(result == JBG_EOK_INTR)
			{
				evas_object_image_data_update_add(prog->img, 0, 0, prog->head->page_width, prog->head->page_height);
				return;
			}

			if(result != JBG_EAGAIN)
				break;
		}
  }

	return;
}

int
elm_main(int argc, char **argv)
{
	static prog_t prog;
	prog.page = UINT_MAX;
	prog.section = UINT_MAX;

	if(!argv[1])
		return -1;

	int page = 1;
	if(argv[2])
		page = atoi(argv[2]);

	prog.scale = 1.f; //DPI_SCREEN / DPI_DISPLAY;

	prog.fin = fopen(argv[1], "rb");
	if(!prog.fin)
		return -1;

	head_t head;
	fread(&head, 1, sizeof(head_t), prog.fin);
	if(strncmp(head.magic, MAGIC, 8))
		return -1;
	head.page_width = be32toh(head.page_width);
	head.page_height = be32toh(head.page_height);
	head.page_number = be32toh(head.page_number);
	head.pages_per_section = be32toh(head.pages_per_section);
	uint32_t section_number = (head.page_number % head.pages_per_section) == 0
		? head.page_number / head.pages_per_section
		: head.page_number / head.pages_per_section + 1;

	size_t offset_size = section_number*sizeof(uint32_t);
	prog.head = malloc(sizeof(head_t) + offset_size);
	memcpy(prog.head, &head, sizeof(head));
	fread(prog.head->section_offset, 1, offset_size, prog.fin);
	for(int s=0; s<section_number; s++)
		prog.head->section_offset[s] = be32toh(prog.head->section_offset[s]);

  prog.inbuflen = 256 * 1024; //TODO
  prog.outbuflen = ((head.page_width >> 3) + !!(head.page_width & 7)) * 3;
	
	prog.inbuf = malloc(prog.inbuflen);
	prog.outbuf = malloc(prog.outbuflen);

	_ui_init(&prog);

	_page_set(&prog, page - 1);
	_next(&prog);

	elm_run();

	if(prog.head)
		free(prog.head);
	if(prog.inbuf)
		free(prog.inbuf);
	if(prog.outbuf)
		free(prog.outbuf);

	return 0;
}

ELM_MAIN();
