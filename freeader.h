#ifndef FREEADER_H
#define FREEADER_H

#include <stdint.h>

#define MAGIC "FreEader"

typedef struct _head_t head_t;

struct _head_t {
	char magic [8];
	uint32_t page_width;
	uint32_t page_height;
	uint32_t page_number; 
	uint32_t pages_per_section;
	uint32_t section_offset [0];
} __attribute__((packed));

#endif
