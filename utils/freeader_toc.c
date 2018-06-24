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

#include <cairo.h>

#define MAX_PATH_LEN 1024

typedef struct _app_t app_t;

struct _app_t {
	cairo_surface_t *surf;
	cairo_t *ctx;
};

static int
_iterate(const char *dir)
{
	char buffer[MAX_PATH_LEN];

	strncpy(buffer, dir, MAX_PATH_LEN);
	const size_t n = strlen(buffer);

	DIR *z = opendir(dir);
	if(!z)
	{
		return EXIT_FAILURE;
	}

	for(struct dirent *data = readdir(z); data; data = readdir(z) )
	{
		if(  (data->d_name[0] == '.')
			&& ( ( (data->d_name[1] == '\0') || (data->d_name[1] == '.'))) )
		{
			continue;
		}

#if 0
		char *point = strrchr(data->d_name, '.');
		if(!point) // no suffix
		{
			continue;
		}

		if(strcmp(point, ".pig")) // no *.pig suffix
		{
			continue;
		}
#endif

		strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);

		fprintf(stdout, "%s\n", buffer);

		DIR *y = opendir(buffer);
		const bool is_subdir = (y != NULL);
		if (y != NULL)
		{
			closedir(y);
		}

		if(is_subdir)
		{
			_iterate(buffer);
		}
	}

	closedir(z);


	return EXIT_SUCCESS;
}

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

typedef struct _item_t item_t;

struct _item_t {
	const char *title;
	const char *author;
};

#define J_K_ROWLING "J. K. Rowling"

static const item_t items [] = {
	{
		.title = "Harry Potter and the Philosopher's Stone",
		.author = J_K_ROWLING
	},
	{
		.title = "Harry Potter and the Chamber of Secrets",
		.author = J_K_ROWLING
	},
	{
		.title = "Harry Potter and the Prisoner of Azkaban",
		.author = J_K_ROWLING
	},
	{
		.title = "Harry Potter and the Goblet of Fire",
		.author = J_K_ROWLING
	},
	{
		.title = "Harry Potter and the Order of the Phoenix",
		.author = J_K_ROWLING
	},
	{
		.title = "Harry Potter and the Half Blood Prince",
		.author = J_K_ROWLING
	},
	{
		.title = "Harry Potter and the Deathly Hallows",
		.author = J_K_ROWLING
	},

	{ // sentinel
		.title = NULL,
		.author = NULL
	}
};

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
				for(const item_t *item1 = items; item1->title; item1++)
				{
					float Y = 0.0;
					const float DY = 1.0 / 8;
					const unsigned num = item1 - items;

					// save state
					cairo_save(app.ctx);

					// clear surface
					cairo_set_operator(app.ctx, CAIRO_OPERATOR_CLEAR);
					cairo_paint(app.ctx);

					// default attributes
					cairo_set_operator(app.ctx, CAIRO_OPERATOR_SOURCE);
					cairo_set_line_width(app.ctx, 0.004);

					for(const item_t *item2 = items; item2->title; item2++, Y += DY)
					{
						_render_item(app.ctx, item2, Y, DY, item1 == item2);
					}

					// save state
					cairo_restore(app.ctx);

					// flush
					cairo_surface_flush(app.surf);

					char path [256];
					snprintf(path, sizeof(path), fmt, num);
					_save_to_pbm(path, width, height, stride, data);
				}

				cairo_destroy(app.ctx);
			}

			cairo_surface_finish(app.surf);
			cairo_surface_destroy(app.surf);
		}

		free(data);
	}

	return EXIT_SUCCESS;
	//return _iterate(argv[1]);
}
