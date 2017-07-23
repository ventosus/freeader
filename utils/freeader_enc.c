#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>

#include <freeader.h>
#include <jbig85.h>

#define PNG_DEBUG 3
#include <png.h>

typedef enum _image_format_t image_format_t;
typedef enum _input_format_t input_format_t;

enum _image_format_t {
	IMAGE_FORMAT_PBM,
	IMAGE_FORMAT_PNG
};

enum _input_format_t {
	INPUT_FORMAT_STREAM,
	INPUT_FORMAT_FILE
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
_read_png_file(const char *file_name, png_bytep *row_pointers)
{
	png_structp png_ptr;
	png_infop info_ptr;

	uint8_t header [8];    // 8 is the maximum size that can be checked
	
	/* open file and test for it being a png */
	FILE *fp = fopen(file_name, "rb");
	if(!fp)
		_abort("[read_png_file] File %s could not be opened for reading", file_name);
	fread(header, 1, 8, fp);
	if(png_sig_cmp(header, 0, 8))
		_abort("[read_png_file] File %s is not recognized as a PNG file", file_name);
		
	/* initialize stuff */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	
	if(!png_ptr)
		_abort("[read_png_file] png_create_read_struct failed");
		
	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr)
		_abort("[read_png_file] png_create_info_struct failed");
		
	if(setjmp(png_jmpbuf(png_ptr)))
		_abort("[read_png_file] Error during init_io");
		
	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);

	// convert TYPE_PALETTE -> TYPE_RGB
	png_set_expand(png_ptr);
	png_read_update_info(png_ptr, info_ptr);
	
	/* read file */
	if(setjmp(png_jmpbuf(png_ptr)))
		_abort("[read_png_file] Error during read_image");
		
	png_read_image(png_ptr, row_pointers);
	
	fclose(fp);
}

static void
_read_pbm_file(const char *file_name, uint8_t **raw_pointers)
{
	FILE *fp = fopen(file_name, "rb");

	char fmt [128];
	fscanf(fp, "%s\n", fmt);
	if(strcmp(fmt, "P4"))
		_abort("[read_pbm_file] File %s has not type 'P4'", file_name);

	unsigned int width, height;
	fscanf(fp, "%u %u\n", &width, &height);
	if( (width != 800) || (height != 600) )
		_abort("[read_pbm_file] File %s has not expected size", file_name);

	for(unsigned j=0; j<height; j++)
		fread(raw_pointers[j], 1, width/8, fp);

	fclose(fp);
}

static void
_read_pbm_stream(const char *file_name, uint8_t **raw_pointers)
{
	FILE *fp = fopen(file_name, "rb");

	char fmt [128];
	fscanf(fp, "%s\n", fmt);
	if(strcmp(fmt, "P4"))
		_abort("[read_pbm_file] File %s has not type 'P4'", file_name);

	unsigned int width, height;
	fscanf(fp, "%u %u\n", &width, &height);
	if( (width != 800) || (height != 600) )
		_abort("[read_pbm_file] File %s has not expected size", file_name);

	for(unsigned j=0; j<height; j++)
		fread(raw_pointers[j], 1, width/8, fp);

	fclose(fp);
}

static void
_out(uint8_t *start, size_t len, void *data)
{
	FILE *fout = data;

	fwrite(start, 1, len, fout);
}

int
main(int argc, char **argv)
{
	struct jbg85_enc_state state;

	uint32_t width = 800;
	uint32_t height = 600;
	uint32_t pages_per_section = 15;
	uint8_t thresh = 0xff;
	image_format_t image_format = IMAGE_FORMAT_PBM;
	uint32_t page_number = 0;
	const char *output_file = "out.pig";

	fprintf(stderr,
		"%s 0.1.0\n" //FIXME
		"Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
		"Released under Artistic License 2.0 by Open Music Kontrollers\n", argv[0]);
	
	int c;
	while((c = getopt(argc, argv, "vhW:H:P:S:F:T:O:")) != -1)
	{
		switch(c)
		{
			case 'v':
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
				return 0;
			case 'h':
				fprintf(stderr,
					"--------------------------------------------------------------------\n"
					"USAGE\n"
					"   %s [OPTIONS] { - | files }\n"
					"\n"
					"OPTIONS\n"
					"   [-v]                   print version and full license information\n"
					"   [-h]                   print usage information\n"
					"   [-W] width             width in pixels (800)\n"
					"   [-H] height            height in pixels (600)\n"
					"   [-P] page-number       page number\n"
					"   [-S] pages-per-section pages per section (15)\n"
					"   [-F] image-format      image format (pbm)\n"
					"   [-T] threshold         greyscale threshold (0xff)\n"
					"   [-O] output-file       output file (out.pig)\n\n"
					, argv[0]);
				return 0;
			case 'W':
				width = atoi(optarg);
				break;
			case 'H':
				height = atoi(optarg);
				break;
			case 'P':
				page_number = atoi(optarg);
				break;
			case 'S':
				pages_per_section = atoi(optarg);
				break;
			case 'T':
				thresh = atoi(optarg);
				break;
			case 'F':
				if(!strcasecmp(optarg, "png"))
					image_format = IMAGE_FORMAT_PNG;
				else if(!strcasecmp(optarg, "pbm"))
					image_format = IMAGE_FORMAT_PBM;
				break;
			case 'O':
				output_file = optarg;
				break;
			case '?':
				if(  (optopt == 'W') || (optopt == 'H') || (optopt == 'P')
					|| (optopt == 'S') || (optopt == 'F') || (optopt == 'T')
					|| (optopt == 'O') )
					fprintf(stderr, "Option `-%c' requires an argument.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				return -1;
			default:
				return -1;
		}
	}

	const input_format_t input_format = argc > optind
		? INPUT_FORMAT_FILE
		: INPUT_FORMAT_STREAM;

	if(input_format == INPUT_FORMAT_FILE)
		page_number = argc - optind;

	if(pages_per_section == 0)
		pages_per_section = page_number;

	/*
	printf(
		"Width: %u\n"
		"Height: %u\n"
		"PageNumber#: %u\n"
		"PagesPerSection#: %u\n"
		"Thresh#: %u\n"
		"Format#: %u\n"
		"OutputFile#: %s\n",
		width, height, page_number, pages_per_section, thresh, image_format, output_file);
	*/

	const uint32_t section_number = (page_number % pages_per_section) == 0
		? page_number / pages_per_section
		: page_number / pages_per_section + 1;

	const size_t head_size = sizeof(head_t) + section_number*sizeof(uint32_t);
	head_t *head = malloc(head_size);
	if(!head)
		return -1;

	strncpy(head->magic, MAGIC, 8);

	const size_t buflen = (width >> 3) + !!(width & 7);
	uint8_t *lines [3] = {
		malloc(buflen),
		malloc(buflen),
		malloc(buflen)
	};
		
	png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
	uint8_t **raw_pointers = malloc(sizeof(uint8_t *) * height);
	for(unsigned j=0; j<height; j++)
	{
		row_pointers[j] = malloc(width * sizeof(uint32_t));
		raw_pointers[j] = malloc(width * sizeof(uint8_t) / 8);
	}

	FILE *fout = fopen(output_file, "wb");
	if(!fout)
		return -1;

	//fseek(fout, head_size, SEEK_SET);
	fwrite(head, 1, head_size, fout);

	uint32_t s = 0; // section number
	uint32_t y = 0; // line number

	switch(input_format)
	{
		case INPUT_FORMAT_FILE:
		{
			//FIXME
			break;
		}
		case INPUT_FORMAT_STREAM:
		{
			//FIXME
			break;
		}
	}

	for(unsigned p=0; p<page_number; p++)
	{
		if(p % pages_per_section == 0)
		{
			uint32_t pages = p + pages_per_section > page_number
				? page_number - p
				: pages_per_section;

			if(p > 0)
				jbg85_enc_newlen(&state, y);
			jbg85_enc_init(&state, width, height * pages, _out, fout);

			int options = JBG_TPBON;
			unsigned long l0 = height; // use default
			int mx = 8; // use default
			jbg85_enc_options(&state, options, l0, mx);

			head->section_offset[s++] = htobe32(ftell(fout));
			y = 0;

			printf("\n%i:", s-1);
		}

		switch(image_format)
		{
			case IMAGE_FORMAT_PNG:
			{
				_read_png_file(argv[optind + p], row_pointers);

				for(unsigned j=0; j<height; j++, y++)
				{
					uint8_t *line = lines[y % 3];
					uint8_t *prevline = NULL;
					uint8_t *prevprevline = NULL;

					uint8_t *dst = line;
					png_byte *row = row_pointers[j];
					for(unsigned x=0; x<width; x+=8, dst++)
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

					if(y>0)
						prevline = lines[(y-1) % 3];
					if(y>1)
						prevprevline = lines[(y-2) % 3];
					jbg85_enc_lineout(&state, line, prevline, prevprevline);
				}

				break;
			}
			case IMAGE_FORMAT_PBM:
			{
				_read_pbm_file(argv[optind + p], raw_pointers);

				for(unsigned j=0; j<height; j++, y++)
				{
					uint8_t *line = lines[y % 3];
					uint8_t *prevline = NULL;
					uint8_t *prevprevline = NULL;

					uint8_t *dst = line;
					uint8_t *row = raw_pointers[j];
					memcpy(dst, row, width/8);

					if(y>0)
						prevline = lines[(y-1) % 3];
					if(y>1)
						prevprevline = lines[(y-2) % 3];
					jbg85_enc_lineout(&state, line, prevline, prevprevline);
				}

				break;
			}
		}

		printf(" [%i]", p);
		fflush(stdout);
	}
	printf("\n");

	for(unsigned j=0; j<height; j++)
	{
		free(row_pointers[j]);
		free(raw_pointers[j]);
	}
	free(row_pointers);
	free(raw_pointers);

	free(lines[0]);
	free(lines[1]);
	free(lines[2]);
	
	head->page_width = htobe32(width);
	head->page_height = htobe32(height);
	head->page_number = htobe32(page_number);
	head->pages_per_section = htobe32(pages_per_section);


	fseek(fout, 0, SEEK_SET);
	fwrite(head, 1, head_size, fout);

	fclose(fout);

	free(head);

	return 0;
}
