#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
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

int
main(int argc, char **argv)
{
	static app_t app;

	const unsigned width = 800;
	const unsigned height = 600;
	const unsigned stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);

	void *data = malloc(stride * height);
	if(data)
	{
		app.surf= cairo_image_surface_create_for_data(
			data, CAIRO_FORMAT_ARGB32, width, height, stride);

		if(app.surf)
		{
			cairo_surface_set_device_scale(app.surf, width, height);

			app.ctx = cairo_create(app.surf);
			if(app.ctx)
			{
				cairo_select_font_face(app.ctx, "cairo:monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

				//FIXME

				cairo_destroy(app.ctx);
			}

			cairo_surface_destroy(app.surf);
		}

		free(data);
	}

	return _iterate(argv[1]);
}
