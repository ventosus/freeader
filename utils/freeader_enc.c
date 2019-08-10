#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

#include <freeader.h>
#include <jbig85.h>

#define PNG_DEBUG 3
#include <png.h>

typedef enum _image_format_t {
	IMAGE_FORMAT_PBM,
	IMAGE_FORMAT_PNG
} image_format_t ;

typedef struct _app_t app_t;

struct _app_t {
	uint32_t width;
	uint32_t height;
	uint32_t page_number;

	head_t *head;

	uint8_t *lines [3];

	png_bytep *row_pointers;
	uint8_t **raw_pointers;

	encoder_t enc;
};

static void
_abort(const char *s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

static void
_read_png_file(app_t *app __attribute__((unused)), const char *file_name,
	png_bytep *row_pointers)
{
	png_structp png_ptr;
	png_infop info_ptr;

	uint8_t header [8];    // 8 is the maximum size that can be checked
	
	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if(!fp)
	{
		_abort("[read_png_file] File %s could not be opened for reading", file_name);
	}
	fread(header, 1, 8, fp);
	if(png_sig_cmp(header, 0, 8))
	{
		_abort("[read_png_file] File %s is not recognized as a PNG file", file_name);
	}
		
	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if(!png_ptr)
	{
		_abort("[read_png_file] png_create_read_struct failed");
	}
		
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
	{
		_abort("[read_png_file] png_create_info_struct failed");
	}
		
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		_abort("[read_png_file] Error during init_io");
	}
		
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);

	// convert TYPE_PALETTE -> TYPE_RGB
	png_set_expand(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	
	/* read file */
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		_abort("[read_png_file] Error during read_image");
	}
		
	png_read_image(png_ptr, row_pointers);
	
	fclose(fp);
}

static void
_read_pbm_file(app_t *app, const char *file_name, uint8_t **raw_pointers)
{
	FILE *fp = fopen(file_name, "rb");

	char fmt [128];
	fscanf(fp, "%s\n", fmt);
	if(strcmp(fmt, "P4"))
	{
		_abort("[read_pbm_file] File %s has not type 'P4'", file_name);
	}

	uint32_t width, height;
	fscanf(fp, "%"SCNu32" %"SCNu32"\n", &width, &height);
	if( (width != app->width) || (height != app->height) )
	{
		_abort("[read_pbm_file] File %s has not expected size", file_name);
	}

	for(uint32_t j = 0; j < height; j++)
	{
		fread(raw_pointers[j], 1, width/8, fp);
	}

	fclose(fp);
}

static void
_out(uint8_t *start, size_t len, void *data)
{
	app_t *app = data;

	fwrite(start, 1, len, app->enc.fout);
}

static void
_app_free(app_t *app)
{
	if(!app)
	{
		return;
	}

	freeader_encoder_deinit(&app->enc, app->head);

	for(uint32_t j = 0; j < app->height; j++)
	{
		free(app->row_pointers[j]);
		free(app->raw_pointers[j]);
	}
	free(app->row_pointers);
	free(app->raw_pointers);

	for(uint8_t l = 0; l < 3; l++)
	{
		free(app->lines[l]);
	}

	free(app->head);

	free(app);
}

static app_t *
_app_new(uint32_t width, uint32_t height, uint32_t page_number,
	const char *title, const char *author, const char *output_file)
{
	app_t *app = calloc(1, sizeof(app_t));
	if(!app)
	{
		return NULL;
	}

	app->width = width;
	app->height = height;
	app->page_number = page_number;

	const size_t head_size = sizeof(head_t) + page_number*sizeof(uint32_t);
	app->head = calloc(head_size, 1);
	if(!app->head)
	{
		goto fail;
	}

	memcpy(app->head->magic, FREEADER_MAGIC, FREEADER_MAGIC_LEN);
	strncpy(app->head->title, title, FREEADER_TITLE_LEN);
	strncpy(app->head->author, author, FREEADER_AUTHOR_LEN);
	app->head->page_width = width;
	app->head->page_height = height;
	app->head->page_number = page_number;

	const size_t buflen = (width >> 3) + !!(width & 7);
	for(uint8_t l = 0; l < 3; l++)
	{
		app->lines[l] = malloc(buflen);
		if(!app->lines[l])
		{
			goto fail;
		}
	}

	app->row_pointers = malloc(sizeof(png_bytep) * height);
	if(!app->row_pointers)
	{
		goto fail;
	}

	app->raw_pointers = malloc(sizeof(uint8_t *) * height);
	if(!app->raw_pointers)
	{
		goto fail;
	}

	for(uint32_t j = 0; j < height; j++)
	{
		app->row_pointers[j] = malloc(width * sizeof(uint32_t));
		if(!app->row_pointers[j])
		{
			goto fail;
		}

		app->raw_pointers[j] = malloc(width * sizeof(uint8_t) / 8);
		if(!app->raw_pointers[j])
		{
			goto fail;
		}
	}

	assert(freeader_encoder_init(&app->enc, output_file, page_number) == 0);

	return app;

fail:
	_app_free(app);

	return NULL;
}

int
main(int argc, char **argv)
{
	uint32_t width = 800;
	uint32_t height = 600;
	uint8_t thresh = 0xff;
	image_format_t image_format = IMAGE_FORMAT_PBM;
	const char *output_file = "out.pig";
	const char *title = "Unknown";
	const char *author = "Unknown";

	fprintf(stderr,
		"%s 0.1.0\n" //FIXME
		"Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
		"Released under Artistic License 2.0 by Open Music Kontrollers\n", argv[0]);
	
	int c;
	while((c = getopt(argc, argv, "vhW:H:F:T:O:t:a:")) != -1)
	{
		switch(c)
		{
			case 'v':
			{
				fprintf(stderr,
					"--------------------------------------------------------------------\n"
					"This is free software: you can redistribute it and/or modify\n"
					"it under the terms of the Artistic License 2.0 as published by\n"
					"The Perl Foundation.\n"
					"\n"
					"This source is distributed in the hope that it will be useful,\n"
					"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
					"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
					"Artistic License 2.0 for more details.\n"
					"\n"
					"You should have received a copy of the Artistic License 2.0\n"
					"along the source as a COPYING file. If not, obtain it from\n"
					"http://www.perlfoundation.org/artistic_license_2_0.\n\n");
			}	return 0;
			case 'h':
			{
				fprintf(stderr,
					"--------------------------------------------------------------------\n"
					"USAGE\n"
					"   %s [OPTIONS] { - | files }\n"
					"\n"
					"OPTIONS\n"
					"   [-v]                   print version and full license information\n"
					"   [-h]                   print usage information\n"
					"   [-W] width             width in pixels (%"PRIu32")\n"
					"   [-H] height            height in pixels (%"PRIu32")\n"
					"   [-F] image-format      image format (pbm|png)\n"
					"   [-T] threshold         greyscale threshold (0x%02"PRIx8")\n"
					"   [-O] output-file       output file (%s)\n"
					"   [-t] title             set book title (%s)\n"
					"   [-a] author            set book author (%s)\n\n"
					, argv[0], width, height, thresh, output_file, title, author);
			}	return 0;
			case 'W':
			{
				width = atoi(optarg);
			}	break;
			case 'H':
			{
				height = atoi(optarg);
			}	break;
			case 'T':
			{
				thresh = atoi(optarg);
			}	break;
			case 'F':
			{
				if(!strcasecmp(optarg, "png"))
				{
					image_format = IMAGE_FORMAT_PNG;
				}
				else if(!strcasecmp(optarg, "pbm"))
				{
					image_format = IMAGE_FORMAT_PBM;
				}
			}	break;
			case 'O':
			{
				output_file = optarg;
			}	break;
			case 't':
			{
				title = optarg;
			}	break;
			case 'a':
			{
				author = optarg;
			}	break;
			case '?':
			{
				if(  (optopt == 'W') || (optopt == 'H')
					|| (optopt == 'F') || (optopt == 'T')
					|| (optopt == 'O') || (optopt == 't') || (optopt == 'a') )
					fprintf(stderr, "Option `-%c' requires an argument.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
			}	return -1;
			default:
			{
			}	return -1;
		}
	}

	const uint32_t page_number = argc - optind;

	app_t *app = _app_new(width, height, page_number, title, author,
		output_file);
	if(!app)
	{
		goto fail;
	}

	struct jbg85_enc_state state;

	for(uint32_t p = 0; p < page_number; p++)
	{
		jbg85_enc_init(&state, width, height, _out, app);
		jbg85_enc_options(&state, JBG_TPBON, height, 8); // defaults

		app->head->page_offset[p] = ftell(app->enc.fout);

		{
			char link [32];
			snprintf(link, sizeof(link), "/link/%"PRIu32, p + 1); //FIXME
			const uint32_t len = htobe32(strlen(link));
			fwrite(&len, sizeof(uint32_t), 1, app->enc.fout);
			fwrite(link, strlen(link), 1, app->enc.fout);
		}

		switch(image_format)
		{
			case IMAGE_FORMAT_PNG:
			{
				_read_png_file(app, argv[optind + p], app->row_pointers);
			} break;
			case IMAGE_FORMAT_PBM:
			{
				_read_pbm_file(app, argv[optind + p], app->raw_pointers);
			} break;
		}

		for(uint32_t j = 0; j < height; j++)
		{
			uint8_t *line = app->lines[j % 3];
			uint8_t *prevline = NULL;
			uint8_t *prevprevline = NULL;

			uint8_t *dst = line;

			switch(image_format)
			{
				case IMAGE_FORMAT_PNG:
				{
					png_byte *row = app->row_pointers[j];

					for(uint32_t x = 0; x < width; x+=8, dst++)
					{
						png_byte *ptr = &(row[x*3]);
						*dst = 0;
						*dst |= (ptr[0*3] < thresh ? 1 : 0) << 7;
						*dst |= (ptr[1*3] < thresh ? 1 : 0) << 6;
						*dst |= (ptr[2*3] < thresh ? 1 : 0) << 5;
						*dst |= (ptr[3*3] < thresh ? 1 : 0) << 4;
						*dst |= (ptr[4*3] < thresh ? 1 : 0) << 3;
						*dst |= (ptr[5*3] < thresh ? 1 : 0) << 2;
						*dst |= (ptr[6*3] < thresh ? 1 : 0) << 1;
						*dst |= (ptr[7*3] < thresh ? 1 : 0) << 0;
					}
				} break;
				case IMAGE_FORMAT_PBM:
				{
					uint8_t *row = app->raw_pointers[j];

					memcpy(dst, row, width/8);
				} break;
			}

			if(j > 0)
			{
				prevline = app->lines[ (j - 1) % 3];
			}
			if(j > 1)
			{
				prevprevline = app->lines[ (j - 2) % 3];
			}
			jbg85_enc_lineout(&state, line, prevline, prevprevline);
		}

		printf("\r[%4i/%i]", p+1, page_number);
		fflush(stdout);
	}
	printf("\n");

	_app_free(app);

	return 0;

fail:
	return 1;
}
