#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <freeader.h>
#include <jbig85.h>

typedef struct _app_t app_t;

struct _app_t {
	unsigned page;

	FILE *fin;
	FILE *fout;

	size_t inbuflen;
	size_t outbuflen;
	uint8_t *inbuf;
	uint8_t *outbuf;

	struct jbg85_dec_state state;
	size_t len;
	size_t cnt;

	head_t *head;
};

static int
_out(const struct jbg85_dec_state *state __attribute__((unused)),
	uint8_t *start, size_t len, unsigned long y, void *data)
{
	app_t *app = data;

	if(fwrite(start, len, 1, app->fout) != 1)
	{
		fprintf(stderr, "fwrite\n");
	}

	if(y == 600 - 1) //FIXME
	{
		return 1;
	}

	return 0;
}

static void
_page_set(app_t *app, unsigned page)
{
	if(page > app->head->page_number - 1)
	{
		page = app->head->page_number - 1;
	}
	
	app->page = page;

	app->len = 0;
	app->cnt = 0;
	jbg85_dec_init(&app->state, app->outbuf, app->outbuflen, _out, app);
	fseek(app->fin, app->head->page_offset[app->page], SEEK_SET);

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

int
main(int argc, char **argv)
{
	static app_t app;

	app.page = UINT_MAX;

	if(argc < 3)
	{
		return -1;
	}

	app.fin = fopen(argv[1], "rb");
	if(!app.fin)
	{
		return -1;
	}

	app.fout = fopen(argv[2], "wb");
	if(!app.fout)
	{
		return -1;
	}

	int page = 1;
	if(argc >= 4)
	{
		page = atoi(argv[3]);
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

#if 0
	fprintf(stderr, "%u %u %u\n",
		head.page_width,
		head.page_height,
		head.page_numbe)
#endif
	fprintf(app.fout, "P4\n%10"PRIu32"\n%10"PRIu32"\n", head.page_width, head.page_height);

	const uint32_t page_number = head.page_number;

	size_t offset_size = page_number*sizeof(uint32_t);
	app.head = malloc(sizeof(head_t) + offset_size);
	memcpy(app.head, &head, sizeof(head));
	fread(app.head->page_offset, 1, offset_size, app.fin);
	for(unsigned s=0; s<page_number; s++)
	{
		app.head->page_offset[s] = be32toh(app.head->page_offset[s]);
	}

  app.inbuflen = 256 * 1024; //TODO
  app.outbuflen = ((head.page_width >> 3) + !!(head.page_width & 7)) * 3;
	
	app.inbuf = malloc(app.inbuflen);
	app.outbuf = malloc(app.outbuflen);

	_page_set(&app, page - 1);
	_next(&app);

	//FIXME

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

	if(app.fin)
	{
		fclose(app.fin);
	}

	if(app.fout)
	{
		fclose(app.fout);
	}

	return 0;
}
