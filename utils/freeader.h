#ifndef FREEADER_H
#define FREEADER_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FREEADER_MAGIC "FreEader"
#define FREEADER_MAGIC_LEN 8
#define FREEADER_TITLE_LEN 128
#define FREEADER_AUTHOR_LEN 128

typedef struct _head_t head_t;
typedef struct _encoder_t encoder_t;
typedef struct _decoder_t decoder_t;

struct _head_t {
	char magic [FREEADER_MAGIC_LEN];
	char title [FREEADER_AUTHOR_LEN];
	char author [FREEADER_TITLE_LEN];
	uint32_t page_width;
	uint32_t page_height;
	uint32_t page_number; 
	uint32_t page_offset [];
} __attribute__((packed));

struct _encoder_t {
	FILE *fout;
};

struct _decoder {
	FILE *fin;
};

static int
freeader_encoder_init(encoder_t *enc, const char *path, uint32_t page_number)
{
	memset(enc, 0x0, sizeof(encoder_t));

	enc->fout = fopen(path, "wb");
	if(!enc->fout)
	{
		return -1;
	}

	const size_t header_sz = sizeof(head_t) + page_number*sizeof(uint32_t);
	fseek(enc->fout, header_sz, SEEK_SET);

	return 0;
}

static int
freeader_encoder_deinit(encoder_t *enc, head_t *head)
{
	head_t behead = *head;

	const uint32_t page_number = behead.page_number;

	behead.page_width = htobe32(behead.page_width);
	behead.page_height = htobe32(behead.page_height);
	behead.page_number = htobe32(behead.page_number);

	fseek(enc->fout, 0, SEEK_SET);
	fwrite(&behead, sizeof(head_t), 1, enc->fout);

	for(uint32_t p = 0; p < page_number; p++)
	{
		const uint32_t page_offset = htobe32(head->page_offset[p]);

		fwrite(&page_offset, sizeof(uint32_t), 1, enc->fout);
	}

	fclose(enc->fout);

	return 0;
}

#endif
