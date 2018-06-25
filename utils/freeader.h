#ifndef FREEADER_H
#define FREEADER_H

#include <stdint.h>

#define FREEADER_MAGIC "FreEader"
#define FREEADER_MAGIC_LEN 8
#define FREEADER_TITLE_LEN 128
#define FREEADER_AUTHOR_LEN 128

typedef struct _head_t head_t;

struct _head_t {
	char magic [FREEADER_MAGIC_LEN];
	char title [FREEADER_AUTHOR_LEN];
	char author [FREEADER_TITLE_LEN];
	uint32_t page_width;
	uint32_t page_height;
	uint32_t page_number; 
	uint32_t pages_per_section;
	uint32_t section_offset [];
} __attribute__((packed));

#endif
