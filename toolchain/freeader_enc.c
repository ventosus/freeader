#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <freeader.h>
#include <jbig85.h>

#define PNG_DEBUG 3
#include <png.h>

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
	fscanf(fp, "%s\n", &fmt);
	if(strcmp(fmt, "P4"))
		_abort("[read_pbm_file] File %s has not type 'P4'", file_name);

	unsigned int width, height;
	fscanf(fp, "%u %u\n", &width, &height);
	if( (width != 800) || (height != 600) )
		_abort("[read_pbm_file] File %s has not expected size", file_name);

	for(int j=0; j<height; j++)
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

	uint8_t thresh = 0xff;

	uint32_t width = 800; //TODO
	uint32_t height = 600; //TODO
	uint32_t page_number = argc - 2;
	uint32_t pages_per_section = 15; //TODO
	if(pages_per_section == 0)
		pages_per_section = page_number;
	uint32_t section_number = (page_number % pages_per_section) == 0
		? page_number / pages_per_section
		: page_number / pages_per_section + 1;

	size_t head_size = sizeof(head_t) + section_number*sizeof(uint32_t);
	head_t *head = malloc(head_size);
	if(!head)
		return -1;

	strncpy(head->magic, MAGIC, 8);

	size_t buflen = (width >> 3) + !!(width & 7);
	uint8_t *lines [3] = {
		malloc(buflen),
		malloc(buflen),
		malloc(buflen)
	};
		
	png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
	uint8_t **raw_pointers = malloc(sizeof(uint8_t *) * height);
	for(int j=0; j<height; j++)
	{
		row_pointers[j] = malloc(width * sizeof(uint32_t));
		raw_pointers[j] = malloc(width * sizeof(uint8_t) / 8);
	}

	FILE *fout = fopen(argv[1], "wb");
	if(!fout)
		return -1;

	//fseek(fout, head_size, SEEK_SET);
	fwrite(head, 1, head_size, fout);

	uint32_t s = 0; // section number
	uint32_t y = 0; // line number

	for(int p=0; p<page_number; p++)
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

		if(strstr(argv[p+2], ".png"))
		{
			_read_png_file(argv[p+2], row_pointers);

			for(int j=0; j<height; j++, y++)
			{
				uint8_t *line = lines[y % 3];
				uint8_t *prevline = NULL;
				uint8_t *prevprevline = NULL;

				uint8_t *dst = line;
				png_byte *row = row_pointers[j];
				for(int x=0; x<width; x+=8, dst++)
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
		}
		else if(strstr(argv[p+2], ".pbm"))
		{
			_read_pbm_file(argv[p+2], raw_pointers);

			for(int j=0; j<height; j++, y++)
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
		}

		printf(" [%i]", p);
		fflush(stdout);
	}
	printf("\n");

	for(int j=0; j<height; j++)
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
