#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#ifdef __unix__
#	include <dirent.h>
#	include <unistd.h>
#endif

#include <freeader.h>

#include <cairo.h>

#define MAX_PATH_LEN 1024

typedef struct _item_t item_t;
typedef struct _app_t app_t;

struct _item_t {
	char title [FREEADER_TITLE_LEN];
	char author [FREEADER_AUTHOR_LEN];
	bool is_folder;
};

struct _app_t {
	cairo_surface_t *surf;
	cairo_t *ctx;
};

static int
_save_to_pbm(const char *path, unsigned width, unsigned height, unsigned stride,
	const uint8_t *data)
{
	FILE *fout = fopen(path, "wb");
	if(fout)
	{
		fprintf(fout, "P4\n%u %u\n", width, height);

		for(unsigned j = 0; j < height; j++)
		{
			const uint8_t *line = &data[j*stride];

			for(unsigned x = 0; x < width/8; x++)
			{
				const uint8_t byt = line[x];

				const uint8_t rer = ( (byt >> 7) & 0x1)
					| ( (byt >> 5) & 0x2)
					| ( (byt >> 3) & 0x4)
					| ( (byt >> 1) & 0x8)
					| ( (byt << 1) & 0x10)
					| ( (byt << 3) & 0x20)
					| ( (byt << 5) & 0x40)
					| ( (byt << 7) & 0x80);

				fwrite(&rer, sizeof(uint8_t), 1, fout);
			}
		}

		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}

static void
_render_item(cairo_t *ctx, const item_t *item, float Y, float DY, bool hi)
{
	//const char *font_face = "cairo:monospace";
	const char *font_face = "Gentium";
	cairo_text_extents_t extents;

	DY /= 7;
	cairo_set_font_size(ctx, 2*DY);
	const float R = DY*3;

	cairo_new_sub_path(ctx);
	cairo_arc(ctx, R + DY/2, Y + DY/2 + R, R, M_PI/2, 3*M_PI/2);
	cairo_arc(ctx, 1.0 - DY/2 - R, Y + DY/2 + R, R, 3*M_PI/2, M_PI/2);
	cairo_close_path(ctx);
	cairo_set_source_rgb(ctx, 0.0, 0.0, 0.0);
	if(hi)
	{
		cairo_fill(ctx);
		cairo_set_operator(ctx, CAIRO_OPERATOR_CLEAR);
	}
	else
	{
		cairo_stroke(ctx);
	}

	if(item->is_folder)
	{
		cairo_arc(ctx, 1.0 - DY/2 - R, Y + DY/2 + R, 2*R/3, 0, 2*M_PI);
		cairo_fill(ctx);
	}

	{
		cairo_text_extents(ctx, item->title, &extents);
		const float dx = extents.x_bearing;
		const float dy = extents.y_bearing;

		cairo_move_to(ctx, R + DY/2, Y + DY/2 + DY);
		cairo_rel_move_to(ctx, -dx, -dy);

		cairo_select_font_face(ctx, font_face,
			CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_show_text(ctx, item->title);
	}

	{
		cairo_text_extents(ctx, item->author, &extents);
		const float dx = extents.x_bearing;
		const float dy = extents.y_bearing;

		cairo_move_to(ctx, R + DY/2, Y + DY/2 + DY*3);
		cairo_rel_move_to(ctx, -dx, -dy);

		cairo_select_font_face(ctx, font_face,
			CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_NORMAL);
		cairo_show_text(ctx, item->author);
	}

	if(hi)
	{
		cairo_set_operator(ctx, CAIRO_OPERATOR_SOURCE);
	}
}

static void
_render(app_t *app, const char *fmt, const item_t *items, unsigned num,
	unsigned width, unsigned height, unsigned stride, const void *data)
{
	unsigned i = 0;

	for(const item_t *item1 = items;
		item1 - items < num;
		item1++, i++)
	{
		float Y = 0.0;
		const float DY = 1.0 / 8;

		// save state
		cairo_save(app->ctx);

		// clear surface
		cairo_set_operator(app->ctx, CAIRO_OPERATOR_CLEAR);
		cairo_paint(app->ctx);

		// default attributes
		cairo_set_operator(app->ctx, CAIRO_OPERATOR_SOURCE);
		cairo_set_line_width(app->ctx, 0.004);

		for(const item_t *item2 = items;
			item2 - items < num;
			item2++, Y += DY)
		{
			_render_item(app->ctx, item2, Y, DY, item1 == item2);
		}

		// save state
		cairo_restore(app->ctx);

		// flush
		cairo_surface_flush(app->surf);

		char path [256];
		snprintf(path, sizeof(path), fmt, i);
		_save_to_pbm(path, width, height, stride, data);
	}
}

static item_t *
_item_append(item_t *items, const char *title, const char *author,
	bool is_folder, unsigned *num)
{
	items = realloc(items, sizeof(item_t) * (*num + 1));
	if(items)
	{
		item_t *item = &items[*num];

		strncpy(item->title, title, FREEADER_TITLE_LEN);
		strncpy(item->author, author, FREEADER_TITLE_LEN);
		item->is_folder = is_folder;

		(*num)++;
	}

	return items;
}

static int
_item_cmp(const void *a, const void *b)
{
	const item_t *c = a;
	const item_t *d = b;

	if(c->is_folder != d->is_folder)
	{
		return c->is_folder ? -1 : 1;
	}

	return strcasecmp(c->title, d->title);
}

static item_t *
_item_sort(item_t *items, unsigned num)
{
	qsort(items, num, sizeof(item_t), _item_cmp);

	return items;
}

static int
_iterate(app_t *app, const char *fmt, const char *dir,
	unsigned width, unsigned height, unsigned stride, const void *data)
{
	char buffer[MAX_PATH_LEN];

	strncpy(buffer, dir, MAX_PATH_LEN);
	const size_t n = strlen(buffer);

	DIR *z = opendir(dir);
	if(!z)
	{
		return EXIT_FAILURE;
	}

	unsigned num = 0;
	item_t *items = NULL;

	for(struct dirent *data = readdir(z); data; data = readdir(z) )
	{
		if(  (data->d_name[0] == '.')
			&& ( ( (data->d_name[1] == '\0') || (data->d_name[1] == '.'))) )
		{
			continue;
		}

		fprintf(stderr, ":: %s (%i)\n", data->d_name, data->d_type);
		if(data->d_type == DT_DIR)
		{
			items = _item_append(items, data->d_name, "Folder", true, &num);

			fprintf(stderr, ":: is directory\n");
			//_iterate(app, fmt, buffer, width, height, stride, data)
		}
		else if(data->d_type == DT_REG)
		{
			const char *point = strrchr(data->d_name, '.');
			if(!point) // no suffix
			{
				continue;
			}

			if(strcmp(point, ".pig")) // no *.pig suffix
			{
				continue;
			}

			strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);

			FILE *f = fopen(buffer, "rb");
			if(f)
			{
				head_t head;
				memset(&head, 0x0, sizeof(head_t));

				if(fread(&head, sizeof(head), 1, f) == 1)
				{
					fprintf(stdout, "%s: %s (%s)\n", buffer, head.title, head.author);
					items = _item_append(items, head.title, head.author, false, &num);
				}

				fclose(f);
			}
		}
	}

	if(items)
	{
		_item_sort(items, num);
		_render(app, fmt, items, num, width, height, stride, data);

		free(items);
	}

	closedir(z);


	return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
	static app_t app;

	const char *fmt = (argc > 1)
		? argv[1]
		: "toc_%04d.pbm";

	const unsigned width = 800;
	const unsigned height = 600;
	const unsigned stride = cairo_format_stride_for_width(CAIRO_FORMAT_A1, width);

	void *data = malloc(stride * height);
	if(data)
	{
		app.surf= cairo_image_surface_create_for_data(
			data, CAIRO_FORMAT_A1, width, height, stride);

		if(app.surf)
		{
			cairo_surface_set_device_scale(app.surf, width, height);

			app.ctx = cairo_create(app.surf);
			if(app.ctx)
			{
				_iterate(&app, fmt, "./", width, height, stride, data);

				cairo_destroy(app.ctx);
			}

			cairo_surface_finish(app.surf);
			cairo_surface_destroy(app.surf);
		}

		free(data);
	}

	return EXIT_SUCCESS;
}
