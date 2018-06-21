#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <freeader.h>
#include <jbig85.h>

#include <Ecore.h>
#include <Edje.h>
#include <Evas.h>
#include <Ecore_Evas.h>

//#define TOGGLE

#if defined(TOGGLE)
#	define ENTRIES 5
#else
#	define ENTRIES 7
#endif

typedef enum _entry_type_t entry_type_t;
typedef struct _entry_t entry_t;

enum _entry_type_t {
	ENTRY_TYPE_FOLDER,
	ENTRY_TYPE_BOOK
};

struct _entry_t {
	entry_type_t type;	
	const char *title;
	const char *author;
};

static const entry_t entries [ENTRIES] = {
#if defined(TOGGLE)
	{ .type = ENTRY_TYPE_FOLDER,
		.title = "Jules Vernes",
		.author = "" },
	{ .type = ENTRY_TYPE_FOLDER,
		.title = "Harry Potter Series",
		.author = "" },
	{ .type = ENTRY_TYPE_FOLDER,
		.title = "Magierdämmerung Trilogie",
		.author = "" },
	{ .type = ENTRY_TYPE_FOLDER,
		.title = "Black Magician Trilogy",
		.author = "" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Flatland",
		.author = "Edwin. A. Abbott" },
#else
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Philosopher's Stone",
		.author = "J. K. Rowling" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Chamber of Secrets",
		.author = "J. K. Rowling" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Prisoner of Azkaban",
		.author = "J. K. Rowling" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Goblet of Fire",
		.author = "J. K. Rowling" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Order of the Phoenix",
		.author = "J. K. Rowling" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Halfblood Prince",
		.author = "J. K. Rowling" },
	{ .type = ENTRY_TYPE_BOOK,
		.title = "Harry Potter and the Deathly Hallows",
		.author = "J. K. Rowling" }
#endif
};

int
main(int argc, char **argv)
{
	const char *fmt = argv[1];

	ecore_evas_init();
	evas_init();
	edje_init();

	Ecore_Evas *ee = ecore_evas_buffer_new(800, 600);
	if(ee)
	{
		Evas *e = ecore_evas_get(ee);
		ecore_evas_show(ee);

		Evas_Object *box = edje_object_add(e);
		Evas_Object *obj [ENTRIES];
		if(box)
		{
			edje_object_file_set(box, "theme.edj", "/freeader/theme");
#if defined(TOGGLE)
			edje_object_part_text_set(box, "path", "/Phantastik");
#else
			edje_object_part_text_set(box, "path", "/Phantastik/Harry Potter Series");
#endif
			evas_object_resize(box, 800, 600);
			evas_object_show(box);

			for(int i=0; i<ENTRIES; i++)
			{
				obj[i] = edje_object_add(e);
				if(obj[i])
				{
					edje_object_file_set(obj[i], "theme.edj", "/freeader/entry");
					char *lab;
					if(entries[i].type == ENTRY_TYPE_BOOK)
					{
						asprintf(&lab, "<title>%s</title>, <author>%s</author>",
							entries[i].title, entries[i].author);
						edje_object_part_text_set(obj[i], "lab", lab);
						edje_object_signal_emit(obj[i], "type,book", "freeader");
					}
					else
					{
						asprintf(&lab, "<title>%s</title>", entries[i].title);
						edje_object_part_text_set(obj[i], "lab", lab);
						edje_object_signal_emit(obj[i], "type,folder", "freeader");
					}
					evas_object_size_hint_weight_set(obj[i], EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
					evas_object_size_hint_align_set(obj[i], EVAS_HINT_FILL, EVAS_HINT_FILL);
					evas_object_resize(obj[i], 800, 40);
					evas_object_show(obj[i]);
					edje_object_part_box_append(box, "box", obj[i]);
				}
			}
		}

		for(int j=0; j<10; j++)
		{
			ecore_main_loop_iterate();
		}

		const uint8_t thresh = 0x80;

		for(int i=0; i<ENTRIES; i++)
		{
			for(int j=0; j<ENTRIES; j++)
			{
				if(i == j)
				{
					edje_object_signal_emit(obj[j], "highlight,on", "freeader");
				}
				else
				{
					edje_object_signal_emit(obj[j], "highlight,off", "freeader");
				}
			}

			for(int j=0; j<10; j++)
			{
				ecore_main_loop_iterate();
			}

			const uint32_t *pixels = ecore_evas_buffer_pixels_get(ee);

			char *path;
			asprintf(&path, fmt, i);
			FILE *f = fopen(path, "wb");
			free(path);

			fprintf(f, "P4\n800 600\n");
			for(int x=0; x<600*800; x+=8)
			{
				uint8_t byte = 0x00;
				byte |= ((pixels[x+0] & 0xff) < thresh ? 1 : 0) << 7; // a, b, g, r
				byte |= ((pixels[x+1] & 0xff) < thresh ? 1 : 0) << 6;
				byte |= ((pixels[x+2] & 0xff) < thresh ? 1 : 0) << 5;
				byte |= ((pixels[x+3] & 0xff) < thresh ? 1 : 0) << 4;
				byte |= ((pixels[x+4] & 0xff) < thresh ? 1 : 0) << 3;
				byte |= ((pixels[x+5] & 0xff) < thresh ? 1 : 0) << 2;
				byte |= ((pixels[x+6] & 0xff) < thresh ? 1 : 0) << 1;
				byte |= ((pixels[x+7] & 0xff) < thresh ? 1 : 0) << 0;
				fwrite(&byte, 1, 1, f);
			}
			fflush(f);
			fclose(f);
		}

		ecore_evas_hide(ee);
		ecore_evas_free(ee);
	}

	edje_shutdown();
	evas_shutdown();
	ecore_evas_shutdown();

	return 0;
}
