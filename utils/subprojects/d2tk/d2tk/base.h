/*
 * Copyright (c) 2018-2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _D2TK_BASE_H
#define _D2TK_BASE_H

#include <stdlib.h>

#include <d2tk/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t d2tk_id_t;
typedef struct _d2tk_pos_t d2tk_pos_t;
typedef struct _d2tk_style_t d2tk_style_t;
typedef struct _d2tk_table_t d2tk_table_t;
typedef struct _d2tk_frame_t d2tk_frame_t;
typedef struct _d2tk_layout_t d2tk_layout_t;
typedef struct _d2tk_scrollbar_t d2tk_scrollbar_t;
typedef struct _d2tk_flowmatrix_t d2tk_flowmatrix_t;
typedef struct _d2tk_flowmatrix_node_t d2tk_flowmatrix_node_t;
typedef struct _d2tk_flowmatrix_arc_t d2tk_flowmatrix_arc_t;
typedef struct _d2tk_pane_t d2tk_pane_t;
typedef struct _d2tk_base_t d2tk_base_t;

struct _d2tk_pos_t {
	d2tk_coord_t x;
	d2tk_coord_t y;
};

typedef enum _d2tk_triple_t  {
	D2TK_TRIPLE_NONE             = 0,

	D2TK_TRIPLE_ACTIVE           = (1 << 0),
	D2TK_TRIPLE_HOT              = (1 << 1),
	D2TK_TRIPLE_ACTIVE_HOT       = D2TK_TRIPLE_ACTIVE | D2TK_TRIPLE_HOT,

	D2TK_TRIPLE_FOCUS            = (1 << 2),
	D2TK_TRIPLE_ACTIVE_FOCUS     = D2TK_TRIPLE_ACTIVE | D2TK_TRIPLE_FOCUS,
	D2TK_TRIPLE_HOT_FOCUS        = D2TK_TRIPLE_HOT | D2TK_TRIPLE_FOCUS,
	D2TK_TRIPLE_ACTIVE_HOT_FOCUS = D2TK_TRIPLE_ACTIVE | D2TK_TRIPLE_HOT
	                             | D2TK_TRIPLE_FOCUS,

	D2TK_TRIPLE_MAX
} d2tk_triple_t;

struct _d2tk_style_t {
	const char *font_face;
	uint32_t border_width;
	uint32_t padding;
	uint32_t rounding;
	uint32_t bg_color;
	uint32_t fill_color [D2TK_TRIPLE_MAX];
	uint32_t stroke_color [D2TK_TRIPLE_MAX];
	uint32_t text_color [D2TK_TRIPLE_MAX];
};

typedef enum _d2tk_state_t {
	D2TK_STATE_NONE					= 0,
	D2TK_STATE_DOWN					= (1 << 0),
	D2TK_STATE_UP						= (1 << 1),
	D2TK_STATE_ACTIVE				= (1 << 2),
	D2TK_STATE_HOT					= (1 << 3),
	D2TK_STATE_FOCUS				= (1 << 4),
	D2TK_STATE_FOCUS_IN			= (1 << 5),
	D2TK_STATE_FOCUS_OUT		= (1 << 6),
	D2TK_STATE_SCROLL_DOWN	= (1 << 7),
	D2TK_STATE_SCROLL_UP		= (1 << 8),
	D2TK_STATE_SCROLL_LEFT	= (1 << 9),
	D2TK_STATE_SCROLL_RIGHT	= (1 << 10),
	D2TK_STATE_MOTION				= (1 << 11),
	D2TK_STATE_CHANGED			= (1 << 12),
	D2TK_STATE_ENTER				= (1 << 13),
	D2TK_STATE_OVER	  			= (1 << 14)
} d2tk_state_t;

typedef enum _d2tk_flag_t {
	D2TK_FLAG_NONE          = 0,
	D2TK_FLAG_SCROLL_Y      = (1 << 0),
	D2TK_FLAG_SCROLL_X      = (1 << 1),
	D2TK_FLAG_SCROLL        = D2TK_FLAG_SCROLL_Y | D2TK_FLAG_SCROLL_X,
	D2TK_FLAG_PANE_Y        = (1 << 2),
	D2TK_FLAG_PANE_X        = (1 << 3),
	D2TK_FLAG_LAYOUT_Y      = (1 << 4),
	D2TK_FLAG_LAYOUT_X      = (1 << 5),
	D2TK_FLAG_LAYOUT_REL    = (1 << 6),
	D2TK_FLAG_LAYOUT_ABS    = (1 << 7),
	D2TK_FLAG_LAYOUT_X_REL  = D2TK_FLAG_LAYOUT_X | D2TK_FLAG_LAYOUT_REL,
	D2TK_FLAG_LAYOUT_Y_REL  = D2TK_FLAG_LAYOUT_Y | D2TK_FLAG_LAYOUT_REL,
	D2TK_FLAG_LAYOUT_X_ABS  = D2TK_FLAG_LAYOUT_X | D2TK_FLAG_LAYOUT_ABS,
	D2TK_FLAG_LAYOUT_Y_ABS  = D2TK_FLAG_LAYOUT_Y | D2TK_FLAG_LAYOUT_ABS,
} d2tk_flag_t;

#define D2TK_ID_IDX(IDX) ((d2tk_id_t)__LINE__ << 16) | (IDX)
#define D2TK_ID_FILE_IDX(IDX) \
	((d2tk_id_t)d2tk_hash(__FILE__, -1) << 32) | D2TK_ID_IDX((IDX))
#define D2TK_ID D2TK_ID_IDX(0)
#define D2TK_ID_FILE D2TK_ID_FILE_IDX(0)

extern const size_t d2tk_table_sz;
extern const size_t d2tk_frame_sz;
extern const size_t d2tk_layout_sz;
extern const size_t d2tk_scrollbar_sz;
extern const size_t d2tk_flowmatrix_sz;
extern const size_t d2tk_flowmatrix_node_sz;
extern const size_t d2tk_flowmatrix_arc_sz;
extern const size_t d2tk_pane_sz;

D2TK_API d2tk_table_t *
d2tk_table_begin(const d2tk_rect_t *rect, unsigned N, unsigned M,
	d2tk_table_t *tab);

D2TK_API bool
d2tk_table_not_end(d2tk_table_t *tab);

D2TK_API d2tk_table_t *
d2tk_table_next(d2tk_table_t *tab);

D2TK_API unsigned
d2tk_table_get_index(d2tk_table_t *tab);

D2TK_API unsigned
d2tk_table_get_index_x(d2tk_table_t *tab);

D2TK_API unsigned
d2tk_table_get_index_y(d2tk_table_t *tab);

D2TK_API const d2tk_rect_t *
d2tk_table_get_rect(d2tk_table_t *tab);

#define D2TK_BASE_TABLE(RECT, N, M, TAB) \
	for(d2tk_table_t *(TAB) = d2tk_table_begin((RECT), (N), (M), \
			alloca(d2tk_table_sz)); \
		d2tk_table_not_end((TAB)); \
		(TAB) = d2tk_table_next((TAB)))

D2TK_API d2tk_frame_t *
d2tk_frame_begin(d2tk_base_t *base, const d2tk_rect_t *rect,
	ssize_t lbl_len, const char *lbl, d2tk_frame_t *frm);

D2TK_API bool
d2tk_frame_not_end(d2tk_frame_t *frm);

D2TK_API d2tk_frame_t *
d2tk_frame_next(d2tk_frame_t *frm);

D2TK_API const d2tk_rect_t *
d2tk_frame_get_rect(d2tk_frame_t *frm);

#define D2TK_BASE_FRAME(BASE, RECT, LBL_LEN, LBL, FRM) \
	for(d2tk_frame_t *(FRM) = d2tk_frame_begin((BASE), (RECT), (LBL_LEN), (LBL), \
			alloca(d2tk_frame_sz)); \
		d2tk_frame_not_end((FRM)); \
		(FRM) = d2tk_frame_next((FRM)))

D2TK_API d2tk_layout_t *
d2tk_layout_begin(const d2tk_rect_t *rect, unsigned N, const d2tk_coord_t *frac,
	d2tk_flag_t flag, d2tk_layout_t *lay);

D2TK_API bool
d2tk_layout_not_end(d2tk_layout_t *lay);

D2TK_API d2tk_layout_t *
d2tk_layout_next(d2tk_layout_t *lay);

D2TK_API unsigned
d2tk_layout_get_index(d2tk_layout_t *lay);

D2TK_API const d2tk_rect_t *
d2tk_layout_get_rect(d2tk_layout_t *lay);

#define D2TK_BASE_LAYOUT(RECT, N, FRAC, FLAG, LAY) \
	for(d2tk_layout_t *(LAY) = d2tk_layout_begin((RECT), (N), (FRAC), (FLAG), \
			alloca(d2tk_layout_sz)); \
		d2tk_layout_not_end((LAY)); \
		(LAY) = d2tk_layout_next((LAY)))

D2TK_API d2tk_scrollbar_t *
d2tk_scrollbar_begin(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_id_t id,
	d2tk_flag_t flags, int32_t hmax, int32_t vmax, int32_t hnum, int32_t vnum,
	d2tk_scrollbar_t *scrollbar);

D2TK_API bool
d2tk_scrollbar_not_end(d2tk_scrollbar_t *scrollbar);

D2TK_API d2tk_scrollbar_t *
d2tk_scrollbar_next(d2tk_base_t *base, d2tk_scrollbar_t *scrollbar);

D2TK_API float
d2tk_scrollbar_get_offset_y(d2tk_scrollbar_t *scrollbar);

D2TK_API float
d2tk_scrollbar_get_offset_x(d2tk_scrollbar_t *scrollbar);

D2TK_API const d2tk_rect_t *
d2tk_scrollbar_get_rect(d2tk_scrollbar_t *scrollbar);

#define D2TK_BASE_SCROLLBAR(BASE, RECT, ID, FLAGS, HMAX, VMAX, HNUM, VNUM, \
		SCROLLBAR) \
	for(d2tk_scrollbar_t *(SCROLLBAR) = d2tk_scrollbar_begin((BASE), (RECT), \
			(ID), (FLAGS), (HMAX), (VMAX), (HNUM), (VNUM), \
			alloca(d2tk_scrollbar_sz)); \
		d2tk_scrollbar_not_end((SCROLLBAR)); \
		(SCROLLBAR) = d2tk_scrollbar_next((BASE), (SCROLLBAR)))

D2TK_API d2tk_pane_t *
d2tk_pane_begin(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_id_t id,
	d2tk_flag_t flags, float fmin, float fmax, float fstep, d2tk_pane_t *pane);

D2TK_API bool
d2tk_pane_not_end(d2tk_pane_t *pane);

D2TK_API d2tk_pane_t *
d2tk_pane_next(d2tk_pane_t *pane);

D2TK_API float
d2tk_pane_get_fraction(d2tk_pane_t *pane);

D2TK_API unsigned
d2tk_pane_get_index(d2tk_pane_t *pane);

D2TK_API const d2tk_rect_t *
d2tk_pane_get_rect(d2tk_pane_t *pane);

#define D2TK_BASE_PANE(BASE, RECT, ID, FLAGS, FMIN, FMAX, FSTEP, PANE) \
	for(d2tk_pane_t *(PANE) = d2tk_pane_begin((BASE), (RECT), \
			(ID), (FLAGS), (FMIN), (FMAX), (FSTEP), alloca(d2tk_pane_sz)); \
		d2tk_pane_not_end((PANE)); \
		(PANE) = d2tk_pane_next((PANE)))

D2TK_API void
d2tk_clip_int32(int32_t min, int32_t *value, int32_t max);

D2TK_API void
d2tk_clip_int64(int64_t min, int64_t *value, int64_t max);

D2TK_API void
d2tk_clip_float(float min, float *value, float max);

D2TK_API void
d2tk_clip_double(double min, double *value, double max);

D2TK_API bool
d2tk_base_get_shift(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_ctrl(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_alt(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_mod(d2tk_base_t *base);

D2TK_API const char *
d2tk_state_dump(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_down(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_up(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_scroll_up(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_scroll_down(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_motion(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_scroll_left(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_scroll_right(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_active(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_hot(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_focused(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_focus_in(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_focus_out(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_changed(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_enter(d2tk_state_t state);

D2TK_API bool
d2tk_state_is_over(d2tk_state_t state);

D2TK_API bool
d2tk_base_is_hit(d2tk_base_t *base, const d2tk_rect_t *rect);

D2TK_API void
d2tk_base_append_char(d2tk_base_t *base, int ch);

D2TK_API d2tk_state_t
d2tk_base_is_active_hot(d2tk_base_t *base, d2tk_id_t id,
	const d2tk_rect_t *rect, d2tk_flag_t flags);

D2TK_API void
d2tk_base_cursor(d2tk_base_t *base, const d2tk_rect_t *rect);

D2TK_API d2tk_state_t
d2tk_base_button_label_image(d2tk_base_t *base, d2tk_id_t id, ssize_t lbl_len,
	const char *lbl, d2tk_align_t align, ssize_t path_len, const char *path, 
	const d2tk_rect_t *rect);

#define d2tk_base_button_label_image_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_button_label_image(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_button_label(d2tk_base_t *base, d2tk_id_t id, ssize_t lbl_len,
	const char *lbl, d2tk_align_t align, const d2tk_rect_t *rect);

#define d2tk_base_button_label_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_button_label(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_button_image(d2tk_base_t *base, d2tk_id_t id, ssize_t path_len,
	const char *path, const d2tk_rect_t *rect);

#define d2tk_base_button_image_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_button_image(__VA_ARGS__))

D2TK_API const d2tk_style_t *
d2tk_base_get_default_style();

D2TK_API const d2tk_style_t *
d2tk_base_get_style(d2tk_base_t *base);

D2TK_API void
d2tk_base_set_style(d2tk_base_t *base, const d2tk_style_t *style);

D2TK_API void
d2tk_base_set_default_style(d2tk_base_t *base);

D2TK_API d2tk_state_t
d2tk_base_button(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect);

#define d2tk_base_button_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_button(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_toggle_label(d2tk_base_t *base, d2tk_id_t id, ssize_t lbl_len,
	const char *lbl, d2tk_align_t align, const d2tk_rect_t *rect, bool *value);

#define d2tk_base_toggle_label_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_toggle_label(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_toggle(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	bool *value);

#define d2tk_base_toggle_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_toggle(__VA_ARGS__))

D2TK_API void
d2tk_base_image(d2tk_base_t *base, ssize_t path_len, const char *path,
	const d2tk_rect_t *rect, d2tk_align_t align);

D2TK_API void
d2tk_base_bitmap(d2tk_base_t *base, uint32_t w, uint32_t h, uint32_t stride,
	const uint32_t *argb, const d2tk_rect_t *rect, d2tk_align_t align);

D2TK_API void
d2tk_base_custom(d2tk_base_t *base, uint32_t size, const void *data,
	const d2tk_rect_t *rect, d2tk_core_custom_t custom);

D2TK_API d2tk_state_t
d2tk_base_meter(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	const int32_t *value);

#define d2tk_base_meter_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_meter(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_combo(d2tk_base_t *base, d2tk_id_t id, ssize_t nitms,
	const char **itms, const d2tk_rect_t *rect, int32_t *value);

#define d2tk_base_combo_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_combo(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_text_field(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	size_t maxlen, char *value, d2tk_align_t align, const char *accept);

#define d2tk_base_text_field_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_text_field(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_label(d2tk_base_t *base, ssize_t lbl_len, const char *lbl,
	float mul, const d2tk_rect_t *rect, d2tk_align_t align);

D2TK_API d2tk_state_t
d2tk_base_dial_bool(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	bool *value);

#define d2tk_base_dial_bool_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_dial_bool(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_dial_int32(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	int32_t min, int32_t *value, int32_t max);

#define d2tk_base_dial_int32_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_dial_int32(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_prop_int32(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	int32_t min, int32_t *value, int32_t max);

#define d2tk_base_prop_int32_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_prop_int32(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_dial_int64(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	int64_t min, int64_t *value, int64_t max);

#define d2tk_base_dial_int64_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_dial_int64(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_dial_float(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	float min, float *value, float max);

#define d2tk_base_dial_float_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_dial_float(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_prop_float(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	float min, float *value, float max);

#define d2tk_base_prop_float_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_prop_float(__VA_ARGS__))

D2TK_API d2tk_state_t
d2tk_base_dial_double(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	double min, double *value, double max);

#define d2tk_base_dial_double_is_changed(...) \
	d2tk_state_is_changed(d2tk_base_dial_double(__VA_ARGS__))

D2TK_API d2tk_flowmatrix_t *
d2tk_flowmatrix_begin(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_id_t id,
	d2tk_flowmatrix_t *flowmatrix);

D2TK_API bool
d2tk_flowmatrix_not_end(d2tk_flowmatrix_t *flowmatrix);

D2TK_API d2tk_flowmatrix_t *
d2tk_flowmatrix_next(d2tk_flowmatrix_t *flowmatrix);

#define D2TK_BASE_FLOWMATRIX(BASE, RECT, ID, FLOWMATRIX) \
	for(d2tk_flowmatrix_t *(FLOWMATRIX) = d2tk_flowmatrix_begin((BASE), (RECT), \
			(ID), alloca(d2tk_flowmatrix_sz)); \
		d2tk_flowmatrix_not_end((FLOWMATRIX)); \
		(FLOWMATRIX) = d2tk_flowmatrix_next((FLOWMATRIX)))

D2TK_API void
d2tk_flowmatrix_set_src(d2tk_flowmatrix_t *flowmatrix, d2tk_id_t id,
	const d2tk_pos_t *pos);

D2TK_API void
d2tk_flowmatrix_set_dst(d2tk_flowmatrix_t *flowmatrix, d2tk_id_t id,
	const d2tk_pos_t *pos);

D2TK_API d2tk_id_t
d2tk_flowmatrix_get_src(d2tk_flowmatrix_t *flowmatrix, d2tk_pos_t *pos);

D2TK_API d2tk_id_t
d2tk_flowmatrix_get_dst(d2tk_flowmatrix_t *flowmatrix, d2tk_pos_t *pos);

D2TK_API d2tk_flowmatrix_node_t *
d2tk_flowmatrix_node_begin(d2tk_base_t *base, d2tk_flowmatrix_t *flowmatrix,
	d2tk_pos_t *pos, d2tk_flowmatrix_node_t *node);

D2TK_API bool
d2tk_flowmatrix_node_not_end(d2tk_flowmatrix_node_t *node);

D2TK_API d2tk_flowmatrix_node_t *
d2tk_flowmatrix_node_next(d2tk_flowmatrix_node_t *node, d2tk_pos_t *pos,
	const d2tk_state_t *state);

D2TK_API const d2tk_rect_t *
d2tk_flowmatrix_node_get_rect(d2tk_flowmatrix_node_t *node);

#define D2TK_BASE_FLOWMATRIX_NODE(BASE, FLOWM, POS, NODE, STATE) \
	for(d2tk_flowmatrix_node_t *(NODE) = d2tk_flowmatrix_node_begin((BASE), (FLOWM), \
			(POS), alloca(d2tk_flowmatrix_node_sz)); \
		d2tk_flowmatrix_node_not_end((NODE)); \
		(NODE) = d2tk_flowmatrix_node_next((NODE), (POS), (STATE)))

D2TK_API d2tk_flowmatrix_arc_t *
d2tk_flowmatrix_arc_begin(d2tk_base_t *base, d2tk_flowmatrix_t *flowmatrix,
	unsigned N, unsigned M, const d2tk_pos_t *src, const d2tk_pos_t *dst,
	d2tk_pos_t *pos, d2tk_flowmatrix_arc_t *arc);

D2TK_API bool
d2tk_flowmatrix_arc_not_end(d2tk_flowmatrix_arc_t *arc);

D2TK_API d2tk_flowmatrix_arc_t *
d2tk_flowmatrix_arc_next(d2tk_flowmatrix_arc_t *arc, d2tk_pos_t *pos,
	const d2tk_state_t *state);

D2TK_API unsigned
d2tk_flowmatrix_arc_get_index(d2tk_flowmatrix_arc_t *arc);

D2TK_API unsigned
d2tk_flowmatrix_arc_get_index_x(d2tk_flowmatrix_arc_t *arc);

D2TK_API unsigned
d2tk_flowmatrix_arc_get_index_y(d2tk_flowmatrix_arc_t *arc);

D2TK_API const d2tk_rect_t *
d2tk_flowmatrix_arc_get_rect(d2tk_flowmatrix_arc_t *arc);

#define D2TK_BASE_FLOWMATRIX_ARC(BASE, FLOWM, N, M, SRC, DST, POS, ARC, STATE) \
	for(d2tk_flowmatrix_arc_t *(ARC) = d2tk_flowmatrix_arc_begin((BASE), (FLOWM), \
			(N), (M), (SRC), (DST), (POS), alloca(d2tk_flowmatrix_arc_sz)); \
		d2tk_flowmatrix_arc_not_end((ARC)); \
		(ARC) = d2tk_flowmatrix_arc_next((ARC), (POS), (STATE)))

D2TK_API d2tk_base_t *
d2tk_base_new(const d2tk_core_driver_t *driver, void *data);

D2TK_API void
d2tk_base_set_ttls(d2tk_base_t *base, uint32_t sprites, uint32_t memcaches);

D2TK_API void
d2tk_base_free(d2tk_base_t *base);

D2TK_API void
d2tk_base_pre(d2tk_base_t *base);

D2TK_API void
d2tk_base_post(d2tk_base_t *base);

D2TK_API void
d2tk_base_set_again(d2tk_base_t *base);

D2TK_API void
d2tk_base_clear_focus(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_again(d2tk_base_t *base);

D2TK_API void
d2tk_base_set_mouse_l(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_mouse_m(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_mouse_r(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_mouse_pos(d2tk_base_t *base, d2tk_coord_t x, d2tk_coord_t y);

D2TK_API void
d2tk_base_get_mouse_pos(d2tk_base_t *base, d2tk_coord_t *x, d2tk_coord_t *y);

D2TK_API void
d2tk_base_add_mouse_scroll(d2tk_base_t *base, int32_t dx, int32_t dy);

D2TK_API void
d2tk_base_set_shift(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_ctrl(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_alt(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_left(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_right(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_up(d2tk_base_t *base, bool down);

D2TK_API void
d2tk_base_set_down(d2tk_base_t *base, bool down);

D2TK_API bool
d2tk_base_get_left(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_right(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_up(d2tk_base_t *base);

D2TK_API bool
d2tk_base_get_down(d2tk_base_t *base);

D2TK_API void
d2tk_base_set_dimensions(d2tk_base_t *base, d2tk_coord_t w, d2tk_coord_t h);

D2TK_API void
d2tk_base_get_dimensions(d2tk_base_t *base, d2tk_coord_t *w, d2tk_coord_t *h);

#ifdef __cplusplus
}
#endif

#endif // _D2TK_BASE_H
