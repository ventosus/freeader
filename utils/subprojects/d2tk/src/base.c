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

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include <d2tk/base.h>
#include <d2tk/hash.h>
#include "core_internal.h"

#define _D2TK_MAX_ATOM 0x1000
#define _D2TK_MASK_ATOMS (_D2TK_MAX_ATOM - 1)

typedef struct _d2tk_flip_t d2tk_flip_t;
typedef struct _d2tk_atom_body_scroll_t d2tk_atom_body_scroll_t;
typedef struct _d2tk_atom_body_pane_t d2tk_atom_body_pane_t;
typedef struct _d2tk_atom_body_flow_t d2tk_atom_body_flow_t;
typedef union _d2tk_atom_body_t d2tk_atom_body_t;
typedef struct _d2tk_atom_t d2tk_atom_t;

typedef enum _d2tk_atom_type_t {
	D2TK_ATOM_NONE,
	D2TK_ATOM_SCROLL,
	D2TK_ATOM_PANE,
	D2TK_ATOM_FLOW,
	D2TK_ATOM_FLOW_NODE,
	D2TK_ATOM_FLOW_ARC
} d2tk_atom_type_t;

struct _d2tk_flip_t {
	d2tk_id_t old;
	d2tk_id_t cur;
};

struct _d2tk_atom_body_scroll_t {
	float offset [2];
};

struct _d2tk_atom_body_pane_t {
	float fraction;
};

struct _d2tk_atom_body_flow_t {
	d2tk_coord_t x;
	d2tk_coord_t y;
	d2tk_coord_t lx;
	d2tk_coord_t ly;
	float exponent;
	d2tk_id_t src_id;
	d2tk_id_t dst_id;
};

union _d2tk_atom_body_t {
	d2tk_atom_body_scroll_t scroll;
	d2tk_atom_body_pane_t pane;
	d2tk_atom_body_flow_t flow;
};

struct _d2tk_atom_t {
	d2tk_id_t id;
	d2tk_atom_type_t type;
	d2tk_atom_body_t body;
};

struct _d2tk_base_t {
	d2tk_flip_t hotitem;
	d2tk_flip_t activeitem;
	d2tk_flip_t focusitem;
	d2tk_id_t lastitem;

	bool not_first_time;

	struct {
		d2tk_coord_t x;
		d2tk_coord_t y;
		d2tk_coord_t ox;
		d2tk_coord_t oy;
		d2tk_coord_t dx;
		d2tk_coord_t dy;
		bool l;
		bool m;
		bool r;
	} mouse;

	struct {
		int32_t odx;
		int32_t ody;
		int32_t dx;
		int32_t dy;
	} scroll;

	struct {
		size_t nchars;
		char chars [32];
		unsigned keymod;

		bool shift;
		bool ctrl;
		bool alt;
		bool left;
		bool right;
		bool up;
		bool down;
	} keys;

	struct {
		char text_in [1024];
		char text_out [1024];
	} edit;

	const d2tk_style_t *style;

	bool again;
	bool clear_focus;
	bool focused;

	d2tk_core_t *core;

	d2tk_atom_t atoms [_D2TK_MAX_ATOM];
};

struct _d2tk_table_t {
	unsigned x;
	unsigned y;
	unsigned N;
	unsigned NM;
	unsigned k;
	unsigned x0;
	d2tk_rect_t rect;
};

struct _d2tk_frame_t {
	d2tk_rect_t rect;
};

struct _d2tk_layout_t {
	unsigned N;
	const d2tk_coord_t *frac;
	d2tk_flag_t flag;
	d2tk_coord_t dd;
	d2tk_coord_t rem;
	unsigned k;
	d2tk_rect_t rect;
};

struct _d2tk_scrollbar_t {
	d2tk_id_t id;
	d2tk_flag_t flags;
	int32_t max [2];
	int32_t num [2];
	d2tk_atom_body_t *atom_body;
	const d2tk_rect_t *rect;
	d2tk_rect_t sub;
};

struct _d2tk_flowmatrix_t {
	d2tk_base_t *base;
	d2tk_id_t id;
	const d2tk_rect_t *rect;
	d2tk_atom_body_t *atom_body;
	float scale;
	d2tk_coord_t cx;
	d2tk_coord_t cy;
	size_t ref;
	d2tk_coord_t w;
	d2tk_coord_t h;
	d2tk_coord_t dd;
	d2tk_coord_t r;
	d2tk_coord_t s;
	bool src_conn;
	bool dst_conn;
	d2tk_pos_t src_pos;
	d2tk_pos_t dst_pos;
};

struct _d2tk_flowmatrix_node_t {
	d2tk_flowmatrix_t *flowmatrix;
	d2tk_rect_t rect;
};

struct _d2tk_flowmatrix_arc_t {
	d2tk_flowmatrix_t *flowmatrix;
	unsigned x;
	unsigned y;
	unsigned N;
	unsigned M;
	unsigned NM;
	unsigned k;
	d2tk_coord_t c;
	d2tk_coord_t c_2;
	d2tk_coord_t c_4;
	d2tk_coord_t xo;
	d2tk_coord_t yo;
	d2tk_rect_t rect;
};

struct _d2tk_pane_t {
	d2tk_atom_body_t *atom_body;
	unsigned k;
	d2tk_rect_t rect [2];
};

const size_t d2tk_table_sz = sizeof(d2tk_table_t);
const size_t d2tk_frame_sz = sizeof(d2tk_frame_t);
const size_t d2tk_layout_sz = sizeof(d2tk_layout_t);
const size_t d2tk_scrollbar_sz = sizeof(d2tk_scrollbar_t);
const size_t d2tk_flowmatrix_sz = sizeof(d2tk_flowmatrix_t);
const size_t d2tk_flowmatrix_node_sz = sizeof(d2tk_flowmatrix_node_t);
const size_t d2tk_flowmatrix_arc_sz = sizeof(d2tk_flowmatrix_arc_t);
const size_t d2tk_pane_sz = sizeof(d2tk_pane_t);

static inline d2tk_id_t
_d2tk_flip_get_cur(d2tk_flip_t *flip)
{
	return flip->cur;
}

static inline d2tk_id_t
_d2tk_flip_get_old(d2tk_flip_t *flip)
{
	return flip->old;
}

static inline bool
_d2tk_flip_equal_cur(d2tk_flip_t *flip, d2tk_id_t id)
{
	return _d2tk_flip_get_cur(flip) == id;
}

static inline bool
_d2tk_flip_equal_old(d2tk_flip_t *flip, d2tk_id_t id)
{
	return _d2tk_flip_get_old(flip) == id;
}

static inline bool
_d2tk_flip_invalid(d2tk_flip_t *flip)
{
	return _d2tk_flip_equal_cur(flip, 0);
}

static inline bool
_d2tk_flip_invalid_old(d2tk_flip_t *flip)
{
	return _d2tk_flip_equal_old(flip, 0);
}

static inline void
_d2tk_flip_set_old(d2tk_flip_t *flip, d2tk_id_t old)
{
	flip->old = old;
}

static inline void
_d2tk_flip_set(d2tk_flip_t *flip, d2tk_id_t new)
{
	if(_d2tk_flip_invalid_old(flip))
	{
		_d2tk_flip_set_old(flip, flip->cur);
	}

	flip->cur = new;
}

static inline void
_d2tk_flip_clear(d2tk_flip_t *flip)
{
	_d2tk_flip_set(flip, 0);
}

static inline void
_d2tk_flip_clear_old(d2tk_flip_t *flip)
{
	_d2tk_flip_set_old(flip, 0);
}

static d2tk_atom_body_t *
_d2tk_base_get_atom(d2tk_base_t *base, d2tk_id_t id, d2tk_atom_type_t type)
{
	for(unsigned i = 0, idx = (id + i*i) & _D2TK_MASK_ATOMS;
		i < _D2TK_MAX_ATOM;
		i++, idx = (id + i*i) & _D2TK_MASK_ATOMS)
	{
		d2tk_atom_t *atom = &base->atoms[idx];

		if( (atom->id != 0) && (atom->id != id) )
		{
			continue;
		}

		d2tk_atom_body_t *body = &atom->body;

		if( (atom->id == 0) || (atom->type != type) ) // new atom or changed type
		{
			memset(atom, 0x0, sizeof(d2tk_atom_t)); // clear atom and its body

			atom->id = id;
			atom->type = type;
		}

		return body;
	}

	return NULL; // no space left
}

D2TK_API d2tk_table_t *
d2tk_table_begin(const d2tk_rect_t *rect, unsigned N, unsigned M,
	d2tk_table_t *tab)
{
	tab->x = 0;
	tab->y = 0;
	tab->N = N;
	tab->NM = N*M;
	tab->k = 0;
	tab->x0 = rect->x;

	tab->rect.x = rect->x;
	tab->rect.y = rect->y;
	tab->rect.w = rect->w / N;
	tab->rect.h = rect->h / M;

	return tab;
}

D2TK_API bool
d2tk_table_not_end(d2tk_table_t *tab)
{
	return tab->k < tab->NM;
}

D2TK_API d2tk_table_t *
d2tk_table_next(d2tk_table_t *tab)
{
	++tab->k;

	if(++tab->x % tab->N)
	{
		tab->rect.x += tab->rect.w;
	}
	else // overflow
	{
		tab->x = 0;
		++tab->y;

		tab->rect.x = tab->x0;
		tab->rect.y += tab->rect.h;
	}

	return tab;
}

D2TK_API unsigned
d2tk_table_get_index(d2tk_table_t *tab)
{
	return tab->k;
}

D2TK_API unsigned
d2tk_table_get_index_x(d2tk_table_t *tab)
{
	return tab->x;
}

D2TK_API unsigned
d2tk_table_get_index_y(d2tk_table_t *tab)
{
	return tab->y;
}

D2TK_API const d2tk_rect_t *
d2tk_table_get_rect(d2tk_table_t *tab)
{
	return &tab->rect;
}

D2TK_API d2tk_frame_t *
d2tk_frame_begin(d2tk_base_t *base, const d2tk_rect_t *rect,
	ssize_t lbl_len, const char *lbl, d2tk_frame_t *frm)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);
	d2tk_core_t *core = base->core;
	const d2tk_coord_t h = 17; //FIXME

	d2tk_rect_shrink(&frm->rect, rect, 2*style->padding);
	frm->rect.y += h;
	frm->rect.h -= h;

	const uint64_t hash = d2tk_hash_foreach(rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		lbl, lbl_len,
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_rect_t bnd_outer;
		d2tk_rect_shrink(&bnd_outer, rect, style->padding);
		d2tk_rect_t bnd_inner = bnd_outer;;

		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		if(lbl)
		{
			if(lbl_len == -1) // zero terminated string
			{
				lbl_len = strlen(lbl);
			}

			bnd_inner.h = h;

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, &bnd_inner, style->rounding);
			d2tk_core_color(core, style->fill_color[D2TK_TRIPLE_NONE]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			bnd_inner.x += style->rounding;
			bnd_inner.w -= style->rounding*2;

			d2tk_core_save(core);
			d2tk_core_scissor(core, &bnd_inner);
			d2tk_core_font_size(core, bnd_inner.h - 2*style->padding);
			d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
			d2tk_core_color(core, style->text_color[D2TK_TRIPLE_NONE]);
			d2tk_core_text(core, &bnd_inner, lbl_len, lbl,
				D2TK_ALIGN_LEFT | D2TK_ALIGN_MIDDLE);
			d2tk_core_restore(core);
		}

		d2tk_core_begin_path(core);
		d2tk_core_rounded_rect(core, &bnd_outer, style->rounding);
		d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);
	}

	return frm;
}

D2TK_API bool
d2tk_frame_not_end(d2tk_frame_t *frm)
{
	return frm ? true : false;
}

D2TK_API d2tk_frame_t *
d2tk_frame_next(d2tk_frame_t *frm __attribute__((unused)))
{
	return NULL;
}

D2TK_API const d2tk_rect_t *
d2tk_frame_get_rect(d2tk_frame_t *frm)
{
	return &frm->rect;
}

D2TK_API d2tk_layout_t *
d2tk_layout_begin(const d2tk_rect_t *rect, unsigned N, const d2tk_coord_t *frac,
	d2tk_flag_t flag, d2tk_layout_t *lay)
{
	lay->N = N;
	lay->frac = frac;
	lay->flag = flag;

	unsigned tot = 0;
	unsigned missing = 0;
	for(unsigned i = 0; i < N; i++)
	{
		tot += frac[i];

		if(frac[i] == 0)
		{
			missing += 1;
		}
	}

	lay->k = 0;
	lay->rect.x = rect->x;
	lay->rect.y = rect->y;

	if(lay->flag & D2TK_FLAG_LAYOUT_Y)
	{
		if(lay->flag & D2TK_FLAG_LAYOUT_REL)
		{
			lay->dd = tot ? (rect->h / tot) : 0;
		}
		else
		{
			lay->dd = 1;
		}

		lay->rect.h = lay->dd * frac[lay->k];
		lay->rect.w = rect->w;

		lay->rem = missing ? (rect->h - tot) / missing : 0;
	}
	else // D2TK_FLAG_LAYOUT_X
	{
		if(lay->flag & D2TK_FLAG_LAYOUT_REL)
		{
			lay->dd = tot ? (rect->w / tot) : 0;
		}
		else
		{
			lay->dd = 1;
		}

		lay->rect.w = lay->dd * frac[lay->k];
		lay->rect.h = rect->h;

		lay->rem = missing ? (rect->w - tot) / missing : 0;
	}

	return lay;
}

D2TK_API bool
d2tk_layout_not_end(d2tk_layout_t *lay)
{
	return lay;
}

D2TK_API d2tk_layout_t *
d2tk_layout_next(d2tk_layout_t *lay)
{
	if(++lay->k >= lay->N)
	{
		return NULL;
	}

	if(lay->flag & D2TK_FLAG_LAYOUT_Y)
	{
		lay->rect.y += lay->rect.h;
		lay->rect.h = lay->frac[lay->k]
			? lay->dd * lay->frac[lay->k]
			: lay->rem;
	}
	else // D2TK_FLAG_LAYOUT_X
	{
		lay->rect.x += lay->rect.w;
		lay->rect.w = lay->frac[lay->k]
			? lay->dd * lay->frac[lay->k]
			: lay->rem;
	}

	return lay;
}

D2TK_API unsigned
d2tk_layout_get_index(d2tk_layout_t *lay)
{
	return lay->k;
}

D2TK_API const d2tk_rect_t *
d2tk_layout_get_rect(d2tk_layout_t *lay)
{
	return &lay->rect;
}

D2TK_API void
d2tk_clip_int32(int32_t min, int32_t *value, int32_t max)
{
	if(*value < min)
	{
		*value = min;
	}
	else if(*value > max)
	{
		*value = max;
	}
}

D2TK_API void
d2tk_clip_int64(int64_t min, int64_t *value, int64_t max)
{
	if(*value < min)
	{
		*value = min;
	}
	else if(*value > max)
	{
		*value = max;
	}
}

D2TK_API void
d2tk_clip_float(float min, float *value, float max)
{
	if(*value < min)
	{
		*value = min;
	}
	else if(*value > max)
	{
		*value = max;
	}
}

D2TK_API void
d2tk_clip_double(double min, double *value, double max)
{
	if(*value < min)
	{
		*value = min;
	}
	else if(*value > max)
	{
		*value = max;
	}
}

D2TK_API bool
d2tk_base_get_shift(d2tk_base_t *base)
{
	return base->keys.shift;
}

D2TK_API bool
d2tk_base_get_ctrl(d2tk_base_t *base)
{
	return base->keys.ctrl;
}

D2TK_API bool
d2tk_base_get_alt(d2tk_base_t *base)
{
	return base->keys.alt;
}

D2TK_API bool
d2tk_base_get_mod(d2tk_base_t *base)
{
	return base->keys.shift || base->keys.ctrl || base->keys.alt;
}

D2TK_API const char *
d2tk_state_dump(d2tk_state_t state)
{
#define LEN 16
	static char buf [LEN + 1];

	for(unsigned i = 0; i < LEN; i++)
	{
		buf[LEN - 1 - i] = (1 << i) & state
			? '1'
			: '.';
	}

	buf[LEN] = '\0';

	return buf;
#undef LEN
}

D2TK_API bool
d2tk_state_is_down(d2tk_state_t state)
{
	return (state & D2TK_STATE_DOWN);
}

D2TK_API bool
d2tk_state_is_up(d2tk_state_t state)
{
	return (state & D2TK_STATE_UP);
}

D2TK_API bool
d2tk_state_is_scroll_up(d2tk_state_t state)
{
	return (state & D2TK_STATE_SCROLL_UP);
}

D2TK_API bool
d2tk_state_is_scroll_down(d2tk_state_t state)
{
	return (state & D2TK_STATE_SCROLL_DOWN);
}

D2TK_API bool
d2tk_state_is_motion(d2tk_state_t state)
{
	return (state & D2TK_STATE_MOTION);
}

D2TK_API bool
d2tk_state_is_scroll_left(d2tk_state_t state)
{
	return (state & D2TK_STATE_SCROLL_LEFT);
}

D2TK_API bool
d2tk_state_is_scroll_right(d2tk_state_t state)
{
	return (state & D2TK_STATE_SCROLL_RIGHT);
}

D2TK_API bool
d2tk_state_is_active(d2tk_state_t state)
{
	return (state & D2TK_STATE_ACTIVE);
}

D2TK_API bool
d2tk_state_is_hot(d2tk_state_t state)
{
	return (state & D2TK_STATE_HOT);
}

D2TK_API bool
d2tk_state_is_focused(d2tk_state_t state)
{
	return (state & D2TK_STATE_FOCUS);
}

D2TK_API bool
d2tk_state_is_focus_in(d2tk_state_t state)
{
	return (state & D2TK_STATE_FOCUS_IN);
}

D2TK_API bool
d2tk_state_is_focus_out(d2tk_state_t state)
{
	return (state & D2TK_STATE_FOCUS_OUT);
}

D2TK_API bool
d2tk_state_is_changed(d2tk_state_t state)
{
	return (state & D2TK_STATE_CHANGED);
}

D2TK_API bool
d2tk_state_is_enter(d2tk_state_t state)
{
	return (state & D2TK_STATE_ENTER);
}

D2TK_API bool
d2tk_state_is_over(d2tk_state_t state)
{
	return (state & D2TK_STATE_OVER);
}

D2TK_API bool
d2tk_base_is_hit(d2tk_base_t *base, const d2tk_rect_t *rect)
{
	if(  (base->mouse.x < rect->x)
		|| (base->mouse.y < rect->y)
		|| (base->mouse.x >= rect->x + rect->w)
		|| (base->mouse.y >= rect->y + rect->h) )
	{
		return false;
	}

	return true;
}

static inline bool
_d2tk_base_set_focus(d2tk_base_t *base, d2tk_id_t id)
{
	_d2tk_flip_set(&base->focusitem, id);

	return true;
}

static inline void
_d2tk_base_change_focus(d2tk_base_t *base)
{
	// copy edit.text_in to edit.text_out
	strncpy(base->edit.text_out, base->edit.text_in, sizeof(base->edit.text_out));
}

D2TK_API void
d2tk_base_append_char(d2tk_base_t *base, int ch)
{
	base->keys.chars[base->keys.nchars++] = ch;
	base->keys.chars[base->keys.nchars] = '\0';
}

static inline void
_d2tk_base_clear_chars(d2tk_base_t *base)
{
	base->keys.nchars = 0;
	base->keys.chars[0] = '\0';
}

static inline d2tk_state_t
_d2tk_base_is_active_hot_vertical_scroll(d2tk_base_t *base)
{
	d2tk_state_t state = D2TK_STATE_NONE;

	// test for vertical scrolling
	if(base->scroll.dy != 0.f)
	{
		if(base->scroll.dy > 0.f)
		{
			state |= D2TK_STATE_SCROLL_UP;
		}
		else
		{
			state |= D2TK_STATE_SCROLL_DOWN;
		}

		base->scroll.ody = base->scroll.dy;
		base->scroll.dy = 0; // eat scrolling
	}

	return state;
}

static inline d2tk_state_t
_d2tk_base_is_active_hot_horizontal_scroll(d2tk_base_t *base)
{
	d2tk_state_t state = D2TK_STATE_NONE;

	// test for horizontal scrolling
	if(base->scroll.dx != 0.f)
	{
		if(base->scroll.dx > 0.f)
		{
			state |= D2TK_STATE_SCROLL_RIGHT;
		}
		else
		{
			state |= D2TK_STATE_SCROLL_LEFT;
		}

		base->scroll.odx = base->scroll.dx;
		base->scroll.dx = 0; // eat scrolling
	}

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_is_active_hot(d2tk_base_t *base, d2tk_id_t id,
	const d2tk_rect_t *rect, d2tk_flag_t flags)
{
	d2tk_state_t state = D2TK_STATE_NONE;
	bool is_active = _d2tk_flip_equal_cur(&base->activeitem, id);
	bool is_hot = false;
	bool is_over = false;
	bool curfocus = _d2tk_flip_equal_cur(&base->focusitem, id);
	bool newfocus = curfocus;
	const bool lastfocus = _d2tk_flip_equal_old(&base->focusitem, id);

	// handle forward focus
	if(curfocus)
	{
		if(base->keys.ctrl)
		{
			if(base->keys.right)
			{
				newfocus = false; // do NOT change curfocus
				base->focused = false; // clear focused flag
				base->keys.right = false;
			}
		}
		else
		{
			if(base->keys.left)
			{
				state |= D2TK_STATE_SCROLL_LEFT;
				base->scroll.odx = -1;
				base->keys.left = false;
			}

			if(base->keys.right)
			{
				state |= D2TK_STATE_SCROLL_RIGHT;
				base->scroll.odx = 1;
				base->keys.right  = false;
			}

			if(base->keys.up)
			{
				state |= D2TK_STATE_SCROLL_UP;
				base->scroll.ody = 1;
				base->keys.up = false;
			}

			if(base->keys.down)
			{
				state |= D2TK_STATE_SCROLL_DOWN;
				base->scroll.ody = -1;
				base->keys.down = false;
			}
		}

		if(base->keys.nchars == 1)
		{
			switch(base->keys.chars[0])
			{
				case '\r': // test for enter
				{
					_d2tk_base_clear_chars(base); // eat key
					is_active = true;
					state |= D2TK_STATE_ENTER;
				} break;
			}
		}
	}
	else if(!base->focused)
	{
		curfocus = _d2tk_base_set_focus(base, id);
		newfocus = curfocus;
		base->focused = true; // set focused flag
	}

	// test for mouse up
	if(is_active && !base->mouse.l)
	{
		_d2tk_flip_clear(&base->activeitem);
		is_active = false;
		state |= D2TK_STATE_UP;
	}

	// test for mouse over
	if(d2tk_base_is_hit(base, rect))
	{
		// test for mouse down
		if(_d2tk_flip_invalid(&base->activeitem) && base->mouse.l)
		{
			_d2tk_flip_set(&base->activeitem, id);
			is_active = true;
			curfocus = _d2tk_base_set_focus(base, id);
			newfocus = curfocus;
			state |= D2TK_STATE_DOWN;
		}

		if(base->mouse.l && !is_active)
		{
			// another widget is active with mouse down, so don't be hot
		}
		else
		{
			_d2tk_flip_set(&base->hotitem, id);
			is_hot = true;
		}

		is_over = true;

		// test whether to handle scrolling
		if(flags & D2TK_FLAG_SCROLL_Y)
		{
			state |= _d2tk_base_is_active_hot_vertical_scroll(base);
		}

		if(flags & D2TK_FLAG_SCROLL_X)
		{
			state |= _d2tk_base_is_active_hot_horizontal_scroll(base);
		}
	}

	if(is_active)
	{
		if( (base->mouse.dx != 0) || (base->mouse.dy != 0) )
		{
			state |= D2TK_STATE_MOTION;
		}

		state |= D2TK_STATE_ACTIVE;
	}

	if(is_hot)
	{
		state |= D2TK_STATE_HOT;
	}

	if(is_over)
	{
		state |= D2TK_STATE_OVER;
	}

	if(newfocus)
	{
		state |= D2TK_STATE_FOCUS;
	}

	{
		if(lastfocus && !curfocus)
		{
			state |= D2TK_STATE_FOCUS_OUT;
			_d2tk_flip_clear_old(&base->focusitem); // clear previous focus
#ifdef D2TK_DEBUG
			fprintf(stderr, "\tfocus out 0x%016"PRIx64"\n", id);
#endif
		}
		else if(!lastfocus && curfocus)
		{
			if(_d2tk_flip_invalid_old(&base->focusitem) && base->not_first_time)
			{
				_d2tk_flip_set(&base->focusitem, _d2tk_flip_get_cur(&base->focusitem));
			}
			else
			{
				state |= D2TK_STATE_FOCUS_IN;
#ifdef D2TK_DEBUG
				fprintf(stderr, "\tfocus in 0x%016"PRIx64"\n", id);
#endif
				_d2tk_base_change_focus(base);
			}
		}
	}

	// handle backwards focus
	if(newfocus)
	{
		if(base->keys.ctrl)
		{
			if(base->keys.left)
			{
				_d2tk_base_set_focus(base, base->lastitem);
				base->keys.left = false;
			}
		}
	}

	base->lastitem = id;

	base->not_first_time = true;

	return state;
}

#define light_grey 0x7f7f7fff
#define dark_grey 0x3f3f3fff
#define darker_grey 0x222222ff
#define black 0x000000ff
#define white 0xffffffff
#define light_orange 0xffcf00ff
#define dark_orange 0xcf9f00ff

D2TK_API const d2tk_style_t *
d2tk_base_get_default_style()
{
	static const d2tk_style_t style = {
		.font_face                       = "Roboto-Bold.ttf",
		.border_width                    = 1,
		.padding                         = 1,
		.rounding                        = 4,
		.bg_color                        = darker_grey,
		.fill_color = {
			[D2TK_TRIPLE_NONE]             = dark_grey,
			[D2TK_TRIPLE_HOT]              = light_grey,
			[D2TK_TRIPLE_ACTIVE]           = dark_orange,
			[D2TK_TRIPLE_ACTIVE_HOT]       = light_orange,
			[D2TK_TRIPLE_FOCUS]            = dark_grey,
			[D2TK_TRIPLE_HOT_FOCUS]        = light_grey,
			[D2TK_TRIPLE_ACTIVE_FOCUS]     = dark_orange,
			[D2TK_TRIPLE_ACTIVE_HOT_FOCUS] = light_orange,
		},
		.stroke_color = {
			[D2TK_TRIPLE_NONE]             = black,
			[D2TK_TRIPLE_HOT]              = black,
			[D2TK_TRIPLE_ACTIVE]           = black,
			[D2TK_TRIPLE_ACTIVE_HOT]       = black,
			[D2TK_TRIPLE_FOCUS]            = white,
			[D2TK_TRIPLE_HOT_FOCUS]        = white,
			[D2TK_TRIPLE_ACTIVE_FOCUS]     = white,
			[D2TK_TRIPLE_ACTIVE_HOT_FOCUS] = white,
		},
		.text_color = {
			[D2TK_TRIPLE_NONE]             = white,
			[D2TK_TRIPLE_HOT]              = light_orange,
			[D2TK_TRIPLE_ACTIVE]           = white,
			[D2TK_TRIPLE_ACTIVE_HOT]       = dark_grey,
			[D2TK_TRIPLE_FOCUS]            = white,
			[D2TK_TRIPLE_HOT_FOCUS]        = light_orange,
			[D2TK_TRIPLE_ACTIVE_FOCUS]     = white,
			[D2TK_TRIPLE_ACTIVE_HOT_FOCUS] = dark_grey
		}
	};

	return &style;
}

D2TK_API const d2tk_style_t *
d2tk_base_get_style(d2tk_base_t *base)
{
	return base->style ? base->style : d2tk_base_get_default_style();
}

D2TK_API void
d2tk_base_set_style(d2tk_base_t *base, const d2tk_style_t *style)
{
	base->style = style;
}

D2TK_API void
d2tk_base_set_default_style(d2tk_base_t *base)
{
	d2tk_base_set_style(base, NULL);
}

D2TK_API d2tk_scrollbar_t *
d2tk_scrollbar_begin(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_id_t id,
	d2tk_flag_t flags, int32_t hmax, int32_t vmax, int32_t hnum, int32_t vnum,
	d2tk_scrollbar_t *scrollbar)
{
	scrollbar->id = id;
	scrollbar->flags = flags;
	scrollbar->max[0] = hmax;
	scrollbar->max[1] = vmax;
	scrollbar->num[0] = hnum;
	scrollbar->num[1] = vnum;
	scrollbar->rect = rect;
	scrollbar->sub = *rect;
	scrollbar->atom_body = _d2tk_base_get_atom(base, id, D2TK_ATOM_SCROLL);

	const d2tk_coord_t s = 10; //FIXME

	if(flags & D2TK_FLAG_SCROLL_X)
	{
		scrollbar->sub.h -= s;
	}

	if(flags & D2TK_FLAG_SCROLL_Y)
	{
		scrollbar->sub.w -= s;
	}

	return scrollbar;
}

D2TK_API bool
d2tk_scrollbar_not_end(d2tk_scrollbar_t *scrollbar)
{
	return scrollbar ? true : false;
}

static void
_d2tk_draw_scrollbar(d2tk_core_t *core, d2tk_state_t hstate, d2tk_state_t vstate,
	const d2tk_rect_t *hbar, const d2tk_rect_t *vbar, const d2tk_style_t *style,
	d2tk_flag_t flags)
{
	const uint64_t hash = d2tk_hash_foreach(&hstate, sizeof(d2tk_state_t),
		&vstate, sizeof(d2tk_state_t),
		hbar, sizeof(d2tk_rect_t),
		vbar, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		&flags, sizeof(d2tk_flag_t),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		if(flags & D2TK_FLAG_SCROLL_X)
		{
			d2tk_triple_t triple = D2TK_TRIPLE_NONE;

			if(d2tk_state_is_active(hstate))
			{
				triple |= D2TK_TRIPLE_ACTIVE;
			}

			if(d2tk_state_is_hot(hstate))
			{
				triple |= D2TK_TRIPLE_HOT;
			}

			if(d2tk_state_is_focused(hstate))
			{
				triple |= D2TK_TRIPLE_FOCUS;
			}

			const size_t ref = d2tk_core_bbox_push(core, true, hbar);

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, hbar, style->rounding);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, hbar, style->rounding);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);\
		}

		if(flags & D2TK_FLAG_SCROLL_Y)
		{
			d2tk_triple_t triple = D2TK_TRIPLE_NONE;

			if(d2tk_state_is_active(vstate))
			{
				triple |= D2TK_TRIPLE_ACTIVE;
			}

			if(d2tk_state_is_hot(vstate))
			{
				triple |= D2TK_TRIPLE_HOT;
			}

			if(d2tk_state_is_focused(vstate))
			{
				triple |= D2TK_TRIPLE_FOCUS;
			}

			const size_t ref = d2tk_core_bbox_push(core, true, vbar);

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, vbar, style->rounding);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, vbar, style->rounding);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}
	}
}

D2TK_API d2tk_scrollbar_t *
d2tk_scrollbar_next(d2tk_base_t *base, d2tk_scrollbar_t *scrollbar)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);
	const d2tk_coord_t s = 10; //FIXME
	const d2tk_coord_t s2 = s*2;

	const d2tk_id_t id = scrollbar->id;
	d2tk_flag_t flags = scrollbar->flags;
	const int32_t *max = scrollbar->max;
	const int32_t *num = scrollbar->num;
	const d2tk_rect_t *rect = scrollbar->rect;
	float *scroll = scrollbar->atom_body->scroll.offset;

	const int32_t rel_max [2] = {
		max[0] - num[0],
		max[1] - num[1]
	};
	d2tk_rect_t hbar = {
		.x = rect->x,
		.y = rect->y + rect->h - s,
		.w = rect->w,
		.h = s
	};
	d2tk_rect_t vbar = {
		.x = rect->x + rect->w - s,
		.y = rect->y,
		.w = s,
		.h = rect->h
	};
	d2tk_rect_t sub = *rect;
	d2tk_state_t hstate = D2TK_STATE_NONE;
	d2tk_state_t vstate = D2TK_STATE_NONE;

	const d2tk_id_t hid = (1 << 24) | id;
	const d2tk_id_t vid = (2 << 24) | id;

	if(max[0] < num[0])
	{
		flags &= ~D2TK_FLAG_SCROLL_X;
	}

	if(max[1] < num[1])
	{
		flags &= ~D2TK_FLAG_SCROLL_Y;
	}

	if(flags & D2TK_FLAG_SCROLL_X)
	{
		sub.h -= s;

		hstate |= d2tk_base_is_active_hot(base, hid, &hbar, D2TK_FLAG_SCROLL_X);
	}

	if(flags & D2TK_FLAG_SCROLL_Y)
	{
		sub.w -= s;

		vstate |= d2tk_base_is_active_hot(base, vid, &vbar, D2TK_FLAG_SCROLL_Y);
	}

	if(d2tk_base_is_hit(base, &sub))
	{
		if(flags & D2TK_FLAG_SCROLL_X)
		{
			hstate |= _d2tk_base_is_active_hot_horizontal_scroll(base);
		}

		if(flags & D2TK_FLAG_SCROLL_Y)
		{
			vstate |= _d2tk_base_is_active_hot_vertical_scroll(base);
		}
	}

	const float old_scroll [2] = {
		scroll[0],
		scroll[1]
	};

	if(flags & D2TK_FLAG_SCROLL_X)
	{
		int32_t dw = hbar.w * num[0] / max[0];
		d2tk_clip_int32(s2, &dw, dw);
		const float w = (float)(hbar.w - dw) / rel_max[0];

		if(d2tk_state_is_scroll_right(hstate))
		{
			scroll[0] += base->scroll.odx;
		}
		else if(d2tk_state_is_scroll_left(hstate))
		{
			scroll[0] += base->scroll.odx;
		}
		else if(d2tk_state_is_motion(hstate))
		{
			const float adx = base->mouse.dx;

			scroll[0] += adx / w;
		}

		// always do clipping, as max may have changed in due course
		d2tk_clip_float(0, &scroll[0], rel_max[0]);

		hbar.w = dw;
		hbar.x += scroll[0]*w;
	}

	if(flags & D2TK_FLAG_SCROLL_Y)
	{
		int32_t dh = vbar.h * num[1] / max[1];
		d2tk_clip_int32(s2, &dh, dh);
		const float h = (float)(vbar.h - dh) / rel_max[1];

		if(d2tk_state_is_scroll_down(vstate))
		{
			scroll[1] -= base->scroll.ody;
		}
		else if(d2tk_state_is_scroll_up(vstate))
		{
			scroll[1] -= base->scroll.ody;
		}
		else if(d2tk_state_is_motion(vstate))
		{
			const float ady = base->mouse.dy;

			scroll[1] += ady / h;
		}

		// always do clipping, as max may have changed in due course
		d2tk_clip_float(0, &scroll[1], rel_max[1]);

		vbar.h = dh;
		vbar.y += scroll[1]*h;
	}

	if( (old_scroll[0] != scroll[0]) || (old_scroll[1] != scroll[1]) )
	{
		d2tk_base_set_again(base);
	}

	d2tk_core_t *core = base->core;

	_d2tk_draw_scrollbar(core, hstate, vstate, &hbar, &vbar, style, flags);

	//return state; //FIXME
	return NULL;
}

D2TK_API float
d2tk_scrollbar_get_offset_y(d2tk_scrollbar_t *scrollbar)
{
	return scrollbar->atom_body->scroll.offset[1];
}

D2TK_API float
d2tk_scrollbar_get_offset_x(d2tk_scrollbar_t *scrollbar)
{
	return scrollbar->atom_body->scroll.offset[0];
}

D2TK_API const d2tk_rect_t *
d2tk_scrollbar_get_rect(d2tk_scrollbar_t *scrollbar)
{
	return &scrollbar->sub;
}

static void
_d2tk_draw_pane(d2tk_core_t *core, d2tk_state_t state, const d2tk_rect_t *sub,
	const d2tk_style_t *style, d2tk_flag_t flags)
{
	const uint64_t hash = d2tk_hash_foreach(&state, sizeof(d2tk_state_t),
		sub, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		&flags, sizeof(d2tk_flag_t),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const d2tk_coord_t s = 10; //FIXME
		const d2tk_coord_t r = (s - 2) / 2; //FIXME

		d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		if(d2tk_state_is_active(state))
		{
			triple |= D2TK_TRIPLE_ACTIVE;
		}

		if(d2tk_state_is_hot(state))
		{
			triple |= D2TK_TRIPLE_HOT;
		}

		if(d2tk_state_is_focused(state))
		{
			triple |= D2TK_TRIPLE_FOCUS;
		}

		d2tk_coord_t x0, x1, x2, y0, y1, y2;

		if(flags & D2TK_FLAG_PANE_X)
		{
			x0 = sub->x + sub->w/2;
			x1 = x0;
			x2 = x0;

			y0 = sub->y;
			y1 = y0 + sub->h/2;
			y2 = y0 + sub->h;
		}
		else // flags & D2TK_FLAG_PANE_Y
		{
			x0 = sub->x;
			x1 = x0 + sub->w/2;
			x2 = x0 + sub->w;

			y0 = sub->y + sub->h/2;
			y1 = y0;
			y2 = y0;
		}

		const size_t ref = d2tk_core_bbox_push(core, true, sub);

		d2tk_core_begin_path(core);
		d2tk_core_move_to(core, x0, y0);
		d2tk_core_line_to(core, x2, y2);
		d2tk_core_color(core, style->stroke_color[triple]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, x1, y1, r, 0, 360, true);
		d2tk_core_color(core, style->fill_color[triple]);
		d2tk_core_stroke_width(core, 0);
		d2tk_core_fill(core);

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, x1, y1, r, 0, 360, true);
		d2tk_core_color(core, style->stroke_color[triple]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);\
	}
}

D2TK_API d2tk_pane_t *
d2tk_pane_begin(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_id_t id,
	d2tk_flag_t flags, float fmin, float fmax, float fstep, d2tk_pane_t *pane)
{
	pane->k = 0;
	pane->rect[0] = *rect;
	pane->rect[1] = *rect;
	pane->atom_body = _d2tk_base_get_atom(base, id, D2TK_ATOM_PANE);

	float *fraction = &pane->atom_body->pane.fraction;
	d2tk_rect_t sub = *rect;

	const d2tk_coord_t s = 10; //FIXME

	d2tk_clip_float(fmin, &pane->atom_body->pane.fraction, fmax);

	if(flags & D2TK_FLAG_PANE_X)
	{
		pane->rect[0].w *= pane->atom_body->pane.fraction;

		sub.x += pane->rect[0].w;
		sub.w = s;

		const d2tk_coord_t rsvd = pane->rect[0].w + s;
		pane->rect[1].x += rsvd;
		pane->rect[1].w -= rsvd;
	}
	else if(flags & D2TK_FLAG_PANE_Y)
	{
		pane->rect[0].h *= pane->atom_body->pane.fraction;

		sub.y += pane->rect[0].h;
		sub.h = s;

		const d2tk_coord_t rsvd = pane->rect[0].h + s;
		pane->rect[1].y += rsvd;
		pane->rect[1].h -= rsvd;
	}

	d2tk_state_t state = D2TK_STATE_NONE;

	if(flags & D2TK_FLAG_PANE_X)
	{
		state |= d2tk_base_is_active_hot(base, id, &sub, D2TK_FLAG_NONE);
	}
	else if(flags & D2TK_FLAG_PANE_Y)
	{
		state |= d2tk_base_is_active_hot(base, id, &sub, D2TK_FLAG_NONE);
	}

	const float old_fraction = *fraction;

	if(flags & D2TK_FLAG_PANE_X)
	{
		if(d2tk_state_is_scroll_left(state))
		{
			*fraction -= fstep;
		}
		else if(d2tk_state_is_scroll_right(state))
		{
			*fraction += fstep;
		}
		else if(d2tk_state_is_motion(state))
		{
			*fraction = roundf((float)(base->mouse.x - rect->x) / rect->w / fstep) * fstep;
		}
	}
	else if(flags & D2TK_FLAG_PANE_Y)
	{
		if(d2tk_state_is_scroll_up(state))
		{
			*fraction -= fstep;
		}
		else if(d2tk_state_is_scroll_down(state))
		{
			*fraction += fstep;
		}
		else if(d2tk_state_is_motion(state))
		{
			*fraction = roundf((float)(base->mouse.y - rect->y) / rect->h / fstep) * fstep;
		}
	}

	if(old_fraction != *fraction)
	{
		state |= D2TK_STATE_CHANGED;
		d2tk_base_set_again(base);
	}

	const d2tk_style_t *style = d2tk_base_get_style(base);

	d2tk_core_t *core = base->core;

	_d2tk_draw_pane(core, state, &sub, style, flags);

	return pane;
}

D2TK_API bool
d2tk_pane_not_end(d2tk_pane_t *pane)
{
	return pane ? true : false;
}

D2TK_API d2tk_pane_t *
d2tk_pane_next(d2tk_pane_t *pane)
{
	return (pane->k++ == 0) ? pane : NULL;
}

D2TK_API float
d2tk_pane_get_fraction(d2tk_pane_t *pane)
{
	return pane->atom_body->pane.fraction;
}

D2TK_API unsigned
d2tk_pane_get_index(d2tk_pane_t *pane)
{
	return pane->k;
}

D2TK_API const d2tk_rect_t *
d2tk_pane_get_rect(d2tk_pane_t *pane)
{
	return &pane->rect[pane->k];
}

D2TK_API void
d2tk_base_cursor(d2tk_base_t *base, const d2tk_rect_t *rect)
{
	d2tk_core_t *core = base->core;
	const d2tk_style_t *style = d2tk_base_get_style(base);

	const uint64_t hash = d2tk_hash_foreach(rect, sizeof(rect),
		style, sizeof(d2tk_style_t),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const d2tk_coord_t x0 = rect->x;
		const d2tk_coord_t x1 = x0 + rect->w/2;
		const d2tk_coord_t x2 = x0 + rect->w;
		const d2tk_coord_t y0 = rect->y;
		const d2tk_coord_t y1 = y0 + rect->h/2;
		const d2tk_coord_t y2 = y0 + rect->h;

		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_core_begin_path(core);
		d2tk_core_move_to(core, x0, y0);
		d2tk_core_line_to(core, x1, y2);
		d2tk_core_line_to(core, x1, y1);
		d2tk_core_line_to(core, x2, y1);
		d2tk_core_close_path(core);
		d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_FOCUS]);
		d2tk_core_stroke_width(core, 0);
		d2tk_core_fill(core);

		d2tk_core_begin_path(core);
		d2tk_core_move_to(core, x0, y0);
		d2tk_core_line_to(core, x1, y2);
		d2tk_core_line_to(core, x1, y1);
		d2tk_core_line_to(core, x2, y1);
		d2tk_core_close_path(core);
		d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
		d2tk_core_stroke_width(core, 2*style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);
	}
}

static inline void
_d2tk_base_draw_button(d2tk_core_t *core, ssize_t lbl_len, const char *lbl,
	d2tk_align_t align, ssize_t path_len, const char *path,
	const d2tk_rect_t *rect, d2tk_triple_t triple, const d2tk_style_t *style)
{
	const bool has_lbl = lbl_len && lbl;
	const bool has_img = path_len && path;

	if(has_lbl && (lbl_len == -1) ) // zero-terminated string
	{
		lbl_len = strlen(lbl);
	}

	if(has_img && (path_len == -1) ) // zero-terminated string
	{
		path_len = strlen(path);
	}

	const d2tk_hash_dict_t dict [] = {
		{ &triple, sizeof(d2tk_triple_t) },
		{ rect, sizeof(d2tk_rect_t) },
		{ style, sizeof(d2tk_style_t) },
		{ &align, sizeof(d2tk_align_t) },
		{ (lbl ? lbl : path), (lbl ? lbl_len : path_len) },
		{ path, path_len },
		{ NULL, 0 }
	};
	const uint64_t hash = d2tk_hash_dict(dict);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_rect_t bnd_outer;
		d2tk_rect_t bnd_inner;
		d2tk_rect_shrink(&bnd_outer, rect, style->padding);
		d2tk_rect_shrink(&bnd_inner, &bnd_outer, 2*style->padding);

		{
			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, &bnd_outer, style->rounding);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			d2tk_core_begin_path(core);
			d2tk_core_rounded_rect(core, &bnd_outer, style->rounding);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}

		if(has_lbl)
		{
			const d2tk_coord_t h_2 = rect->h / 2;
			const d2tk_align_t lbl_align = align;

			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			d2tk_core_save(core);
			d2tk_core_scissor(core, &bnd_inner);
			d2tk_core_font_size(core, h_2);
			d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
			d2tk_core_color(core, style->text_color[triple]);
			d2tk_core_text(core, &bnd_inner, lbl_len, lbl, lbl_align);
			d2tk_core_restore(core);

			d2tk_core_bbox_pop(core, ref);
		}

		if(has_img)
		{
			const d2tk_align_t img_align = D2TK_ALIGN_MIDDLE
				| (has_lbl ? D2TK_ALIGN_RIGHT : D2TK_ALIGN_CENTER);

			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			d2tk_core_image(core, &bnd_inner, path_len, path, img_align);

			d2tk_core_bbox_pop(core, ref);
		}
	}
}

D2TK_API d2tk_state_t
d2tk_base_button_label_image(d2tk_base_t *base, d2tk_id_t id, ssize_t lbl_len,
	const char *lbl, d2tk_align_t align, ssize_t path_len, const char *path,
	const d2tk_rect_t *rect)
{
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect, D2TK_FLAG_NONE);

	if(d2tk_state_is_down(state) || d2tk_state_is_enter(state))
	{
		state |= D2TK_STATE_CHANGED;
	}

	d2tk_triple_t triple = D2TK_TRIPLE_NONE;

	if(d2tk_state_is_active(state))
	{
		triple |= D2TK_TRIPLE_ACTIVE;
	}

	if(d2tk_state_is_hot(state))
	{
		triple |= D2TK_TRIPLE_HOT;
	}

	if(d2tk_state_is_focused(state))
	{
		triple |= D2TK_TRIPLE_FOCUS;
	}

	_d2tk_base_draw_button(base->core, lbl_len, lbl, align, path_len, path, rect,
		triple, d2tk_base_get_style(base));

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_button_label(d2tk_base_t *base, d2tk_id_t id, ssize_t lbl_len,
	const char *lbl, d2tk_align_t align, const d2tk_rect_t *rect)
{
	return d2tk_base_button_label_image(base, id, lbl_len, lbl,
		align, 0, NULL, rect);
}

D2TK_API d2tk_state_t
d2tk_base_button_image(d2tk_base_t *base, d2tk_id_t id, ssize_t path_len,
	const char *path, const d2tk_rect_t *rect)
{
	return d2tk_base_button_label_image(base, id, 0, NULL,
		D2TK_ALIGN_NONE, path_len, path, rect);
}

D2TK_API d2tk_state_t
d2tk_base_button(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect)
{
	return d2tk_base_button_label_image(base, id, 0, NULL,
		D2TK_ALIGN_NONE, 0, NULL, rect);
}

D2TK_API d2tk_state_t
d2tk_base_toggle_label(d2tk_base_t *base, d2tk_id_t id, ssize_t lbl_len,
	const char *lbl, d2tk_align_t align, const d2tk_rect_t *rect, bool *value)
{
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect, D2TK_FLAG_NONE);

	if(d2tk_state_is_down(state) || d2tk_state_is_enter(state))
	{
		*value = !*value;
		state |= D2TK_STATE_CHANGED;
	}

	d2tk_triple_t triple = D2TK_TRIPLE_NONE;

	if(*value)
	{
		triple |= D2TK_TRIPLE_ACTIVE;
	}

	if(d2tk_state_is_hot(state))
	{
		triple |= D2TK_TRIPLE_HOT;
	}

	if(d2tk_state_is_focused(state))
	{
		triple |= D2TK_TRIPLE_FOCUS;
	}

	_d2tk_base_draw_button(base->core, lbl_len, lbl, align, 0, NULL, rect, triple,
		d2tk_base_get_style(base));

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_toggle(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	bool *value)
{
	return d2tk_base_toggle_label(base, id, 0, NULL,
		D2TK_ALIGN_NONE, rect, value);
}

D2TK_API void
d2tk_base_image(d2tk_base_t *base, ssize_t path_len, const char *path,
	const d2tk_rect_t *rect, d2tk_align_t align)
{
	const bool has_img = path_len && path;

	if(has_img && (path_len == -1) ) // zero-terminated string
	{
		path_len = strlen(path);
	}

	const uint64_t hash = d2tk_hash_foreach(rect, sizeof(d2tk_rect_t),
		(path ? path : NULL), (path ? path_len : 0),
		NULL);

	d2tk_core_t *core = base->core;;

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		if(has_img)
		{
			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			d2tk_core_image(core, rect, path_len, path, align);

			d2tk_core_bbox_pop(core, ref);
		}
	}
}

D2TK_API void
d2tk_base_bitmap(d2tk_base_t *base, uint32_t w, uint32_t h, uint32_t stride,
	const uint32_t *argb, uint64_t rev, const d2tk_rect_t *rect,
	d2tk_align_t align)
{
	const uint64_t hash = d2tk_hash_foreach(rect, sizeof(d2tk_rect_t),
		&w, sizeof(uint32_t),
		&h, sizeof(uint32_t),
		&stride, sizeof(uint32_t),
		&rev, sizeof(uint64_t),
		NULL);

	d2tk_core_t *core = base->core;;

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_core_bitmap(core, rect, w, h, stride, argb, rev, align);

		d2tk_core_bbox_pop(core, ref);
	}
}

D2TK_API void
d2tk_base_custom(d2tk_base_t *base, uint32_t size, const void *data,
	const d2tk_rect_t *rect, d2tk_core_custom_t custom)
{
	const uint64_t hash = d2tk_hash_foreach(rect, sizeof(d2tk_rect_t),
		data, size, //FIXME
		NULL);

	d2tk_core_t *core = base->core;;

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_core_custom(core, rect, size, data, custom);

		d2tk_core_bbox_pop(core, ref);
	}
}

static inline void
_d2tk_base_draw_meter(d2tk_core_t *core, const d2tk_rect_t *rect,
	d2tk_state_t state, int32_t value, const d2tk_style_t *style)
{
	const uint64_t hash = d2tk_hash_foreach(&state, sizeof(d2tk_state_t),
		rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		&value, sizeof(int32_t),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
#define N 4
#define N_1 (N - 1)
#define L 11

#define dBFS3_min -18 // -54 dBFS
#define dBFS3_max 2  // +6 dBFS
#define dBFS3_range (dBFS3_max - dBFS3_min)

		static const int32_t dBFS3_off [N] = {
			dBFS3_min,
			-2, // -6 dBFS
			0,  // +0 dBFS
			dBFS3_max
		};

		static const uint32_t rgba [N] = {
			0x00ffffff, // cyan
			0x00ff00ff, // green
			0xffff00ff, // yellow
			0xff0000ff  // red
		};

		d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		if(d2tk_state_is_active(state))
		{
			triple |= D2TK_TRIPLE_ACTIVE;
		}

		if(d2tk_state_is_hot(state))
		{
			triple |= D2TK_TRIPLE_HOT;
		}

		if(d2tk_state_is_focused(state))
		{
			triple |= D2TK_TRIPLE_FOCUS;
		}

		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, rect, style->padding);
		bnd.h /= 2;

		const d2tk_coord_t dx = bnd.w / dBFS3_range;
		const d2tk_coord_t y0 = bnd.y;
		const d2tk_coord_t y1 = y0 + bnd.h;
		const d2tk_coord_t ym = (y0 + y1)/2;

		const d2tk_point_t p [N] = {
			D2TK_POINT(bnd.x + (dBFS3_off[0] - dBFS3_min)*dx, ym),
			D2TK_POINT(bnd.x + (dBFS3_off[1] - dBFS3_min)*dx, ym),
			D2TK_POINT(bnd.x + (dBFS3_off[2] - dBFS3_min)*dx, ym),
			D2TK_POINT(bnd.x + (dBFS3_off[3] - dBFS3_min)*dx, ym)
		};

		// dependent on value, e.g. linear gradient
		{
			const d2tk_coord_t xv = bnd.x + (value - dBFS3_min*3)*dx/3;

			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			for(unsigned i = 0; i < N_1; i++)
			{
				const d2tk_coord_t x0 = p[i].x;
				d2tk_coord_t x1 = p[i+1].x;
				bool do_break = false;

				if(x1 > xv)
				{
					x1 = xv;
					do_break = true;
				}

				const d2tk_rect_t bnd2 = {
					.x = x0,
					.y = y0,
					.w = x1 - x0,
					.h = bnd.h
				};

				d2tk_core_begin_path(core);
				d2tk_core_rect(core, &bnd2);
				d2tk_core_linear_gradient(core, &p[i], &rgba[i]);
				d2tk_core_stroke_width(core, 0);
				d2tk_core_fill(core);

				if(do_break)
				{
					break;
				}
			}

			d2tk_core_bbox_pop(core, ref);
		}

		// independent on value, eg scale + border
		{
			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			for(int32_t dBFS3 = dBFS3_min + 1; dBFS3 < dBFS3_max; dBFS3++)
			{
				const d2tk_coord_t x = bnd.x + (dBFS3 - dBFS3_min)*dx;

				d2tk_core_begin_path(core);
				d2tk_core_move_to(core, x, y0);
				d2tk_core_line_to(core, x, y1);
				d2tk_core_color(core, style->stroke_color[triple]);
				d2tk_core_stroke_width(core, style->border_width);
				d2tk_core_stroke(core);
			}

			{
				const d2tk_rect_t bnd2 = {
					.x = bnd.x,
					.y = y0,
					.w = p[N_1].x - p[0].x,
					.h = bnd.h
				};

				d2tk_core_begin_path(core);
				d2tk_core_rect(core, &bnd2);
				d2tk_core_color(core, style->stroke_color[triple]);
				d2tk_core_stroke_width(core, style->border_width);
				d2tk_core_stroke(core);
			}

			static const unsigned lbls [L] = {
				+2,
				+1,
				+0,
				-1,
				-2,
				-4,
				-8,
				-12,
				-15, // unit
				-16
			};

			for(unsigned i = 0; i<L; i++)
			{
				const int32_t dBFS3 = lbls[i] - 1;
				const d2tk_coord_t x = bnd.x + (dBFS3 - dBFS3_min)*dx;
				const bool is_unit = (i == 8);

				const d2tk_rect_t bnd2 = {
					.x = x,
					.y = y0 + bnd.h,
					.w = is_unit ? 3*dx : dx,
					.h = bnd.h
				};

				char lbl [16];
				const ssize_t lbl_len = is_unit
					? snprintf(lbl, sizeof(lbl), " dBFS")
					: snprintf(lbl, sizeof(lbl), "%+"PRIi32, (dBFS3+1)*3);

				d2tk_core_save(core);
				d2tk_core_scissor(core, &bnd2);
				d2tk_core_font_size(core, bnd2.h);
				d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
				d2tk_core_color(core, style->text_color[triple]);
				d2tk_core_text(core, &bnd2, lbl_len, lbl,
					D2TK_ALIGN_BOTTOM | (is_unit ? D2TK_ALIGN_LEFT: D2TK_ALIGN_RIGHT));
				d2tk_core_restore(core);
			}

			d2tk_core_bbox_pop(core, ref);
		}

#undef dBFS3_range
#undef dBFS3_max
#undef dBFS3_min

#undef L
#undef N
#undef N_1
	}
}

D2TK_API d2tk_state_t
d2tk_base_meter(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	const int32_t *value)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);

	const d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_NONE);

	d2tk_core_t *core = base->core;

	_d2tk_base_draw_meter(core, rect, state, *value, style);

	return state;
}

static inline void
_d2tk_base_draw_combo(d2tk_core_t *core, ssize_t nitms, const char **itms,
	const d2tk_rect_t *rect, d2tk_state_t state, int32_t value,
	const d2tk_style_t *style)
{
	const uint64_t hash = d2tk_hash_foreach(&state, sizeof(d2tk_state_t),
		rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		&value, sizeof(int32_t),
		&nitms, sizeof(ssize_t),
		itms, sizeof(const char **), //FIXME we should actually cache the labels
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, rect, style->padding);

		const d2tk_coord_t w_2 = bnd.w / 2;
		const d2tk_coord_t w_4 = bnd.w / 4;

		d2tk_rect_t left = bnd;
		left.x -= w_4;
		left.w = w_2;

		d2tk_rect_t midd = bnd;
		midd.x += w_4;
		midd.w = w_2;

		d2tk_rect_t right = bnd;
		right.x += w_2 + w_4;
		right.w = w_2;

		d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		if(d2tk_state_is_hot(state))
		{
			triple |= D2TK_TRIPLE_HOT;
		}

		if(d2tk_state_is_focused(state))
		{
			triple |= D2TK_TRIPLE_FOCUS;
		}

		{
			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			uint32_t fill_color_2 = style->fill_color[triple];
			fill_color_2 = (fill_color_2 & 0xffffff00) | (fill_color_2 & 0xff / 2);

			// left filling
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &left);
			d2tk_core_color(core, fill_color_2);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			// middle filling
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &midd);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			// right filling
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &right);
			d2tk_core_color(core, fill_color_2);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			// draw lines above and below text
			const d2tk_coord_t h_8 = bnd.h / 8;
			const d2tk_coord_t dx = bnd.w / nitms;
			const d2tk_coord_t x0 = bnd.x;
			const d2tk_coord_t x1 = bnd.x + value*dx;
			const d2tk_coord_t x2 = x1 + dx;
			const d2tk_coord_t x3 = bnd.x + bnd.w;
			const d2tk_coord_t y0 = bnd.y + h_8;
			const d2tk_coord_t y1 = bnd.y + bnd.h - h_8;

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y0);
			d2tk_core_line_to(core, x3, y0);
			d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y1);
			d2tk_core_line_to(core, x3, y1);
			d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
			d2tk_core_stroke_width(core, style->border_width*2);
			d2tk_core_stroke(core);

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x1, y0);
			d2tk_core_line_to(core, x2, y0);
			d2tk_core_color(core, style->fill_color[triple | D2TK_TRIPLE_ACTIVE]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x1, y1);
			d2tk_core_line_to(core, x2, y1);
			d2tk_core_color(core, style->fill_color[triple | D2TK_TRIPLE_ACTIVE]);
			d2tk_core_stroke_width(core, style->border_width*2);
			d2tk_core_stroke(core);

			// draw bounding box
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &bnd);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}

		{
			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			// left label
			if(value > 0)
			{
				const char *lbl = itms[value - 1];
				const size_t lbl_len = lbl ? strlen(lbl) : 0;

				d2tk_core_save(core);
				d2tk_core_scissor(core, &left);
				d2tk_core_font_size(core, left.h / 2);
				d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
				d2tk_core_color(core, style->text_color[D2TK_TRIPLE_NONE]);
				d2tk_core_text(core, &left, lbl_len, lbl, D2TK_ALIGN_CENTERED);
				d2tk_core_restore(core);
			}

			// middle label
			{
				const char *lbl = itms[value];
				const size_t lbl_len = lbl ? strlen(lbl) : 0;

				d2tk_core_save(core);
				d2tk_core_scissor(core, &midd);
				d2tk_core_font_size(core, midd.h / 2);
				d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
				d2tk_core_color(core, style->text_color[triple]);
				d2tk_core_text(core, &midd, lbl_len, lbl, D2TK_ALIGN_CENTERED);
				d2tk_core_restore(core);
			}

			// right label
			if(value < (nitms-1) )
			{
				const char *lbl = itms[value + 1];
				const size_t lbl_len = lbl ? strlen(lbl) : 0;

				d2tk_core_save(core);
				d2tk_core_scissor(core, &right);
				d2tk_core_font_size(core, right.h / 2);
				d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
				d2tk_core_color(core, style->text_color[D2TK_TRIPLE_NONE]);
				d2tk_core_text(core, &right, lbl_len, lbl, D2TK_ALIGN_CENTERED);
				d2tk_core_restore(core);
			}

			d2tk_core_bbox_pop(core, ref);
		}
	}
}

D2TK_API d2tk_state_t
d2tk_base_combo(d2tk_base_t *base, d2tk_id_t id, ssize_t nitms,
	const char **itms, const d2tk_rect_t *rect, int32_t *value)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);

	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_SCROLL_Y);

	const int32_t old_value = *value;

	if(  d2tk_state_is_scroll_up(state)
			|| d2tk_state_is_scroll_right(state)
			|| d2tk_state_is_enter(state) )
	{
		*value += 1;
	}

	if(  d2tk_state_is_scroll_down(state)
		|| d2tk_state_is_scroll_left(state))
	{
		*value -= 1;
	}

	if(d2tk_state_is_down(state))
	{
		const d2tk_coord_t w_2 = rect->w/2;
		const d2tk_coord_t w_4 = rect->w/4;

		const d2tk_coord_t x1 = rect->x + w_4;
		const d2tk_coord_t x2 = x1 + w_2;

		if(base->mouse.x < x1)
		{
			*value -= 1;
		}
		else if(base->mouse.x > x2)
		{
			*value += 1;
		}
	}

	d2tk_clip_int32(0, value, nitms-1);

	if(*value != old_value)
	{
		state |= D2TK_STATE_CHANGED;
	}

	d2tk_core_t *core = base->core;

	_d2tk_base_draw_combo(core, nitms, itms, rect, state, *value, style);

	return state;
}

static void
_d2tk_base_draw_text_field(d2tk_core_t *core, d2tk_state_t state,
	const d2tk_rect_t *rect, const d2tk_style_t *style, char *value,
	d2tk_align_t align)
{
	const uint64_t hash = d2tk_hash_foreach(&state, sizeof(d2tk_state_t),
		rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		&align, sizeof(d2tk_align_t),
		value, -1,
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		if(d2tk_state_is_hot(state))
		{
			triple |= D2TK_TRIPLE_HOT;
		}

		if(d2tk_state_is_focused(state))
		{
			triple |= D2TK_TRIPLE_FOCUS;
		}

		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, rect, style->padding);

		const d2tk_coord_t h_8 = bnd.h / 8;

		{
			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			// draw background
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &bnd);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			// draw lines above and below text
			const d2tk_coord_t x0 = bnd.x;
			const d2tk_coord_t x1 = bnd.x + bnd.w;
			const d2tk_coord_t y0 = bnd.y + h_8;
			const d2tk_coord_t y1 = bnd.y + bnd.h - h_8;

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y0);
			d2tk_core_line_to(core, x1, y0);
			d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y1);
			d2tk_core_line_to(core, x1, y1);
			d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			// draw bounding box
			d2tk_core_begin_path(core);
			d2tk_core_rect(core, &bnd);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}

		const size_t valuelen= strlen(value);
		if(valuelen)
		{
			const d2tk_coord_t h_2 = bnd.h / 2;

			d2tk_rect_t bnd2;
			d2tk_rect_shrink_x(&bnd2, &bnd, h_8);

			const size_t ref = d2tk_core_bbox_push(core, true, rect);

			d2tk_core_save(core);
			d2tk_core_scissor(core, &bnd2);
			d2tk_core_font_size(core, h_2);
			d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
			d2tk_core_color(core, style->text_color[D2TK_TRIPLE_NONE]);
			d2tk_core_text(core, &bnd2, valuelen, value, align);
			d2tk_core_restore(core);

			d2tk_core_bbox_pop(core, ref);
		}
	}
}

D2TK_API d2tk_state_t
d2tk_base_text_field(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	size_t maxlen, char *value, d2tk_align_t align, const char *accept)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);

	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect, D2TK_FLAG_NONE);

	if(d2tk_state_is_focus_in(state))
	{
		_d2tk_base_clear_chars(base); // eat key

		// copy text from value to edit.text_in
		strncpy(base->edit.text_in, value, maxlen);
	}

	if(d2tk_state_is_focused(state))
	{
		// use edit.text_in
		value = base->edit.text_in;

		if(base->keys.nchars)
		{
			const size_t len = strlen(value);

			if(base->keys.nchars == 1)
			{
				switch(base->keys.chars[0])
				{
					case 0x7f:
					{
						value[0] = '\0';
						_d2tk_base_clear_chars(base); // eat key
					} break;
					case 0x08:
					{
						value[len-1] = '\0';
						_d2tk_base_clear_chars(base); // eat key
					} break;
				}
			}

			if(base->keys.nchars)
			{
				if(len + base->keys.nchars < maxlen)
				{
					if(accept) // only add valid characters
					{
						char *dst = &value[len];
						const char *src = (const char *)base->keys.chars;
						for(src = strpbrk(src, accept);
							src;
							src = strpbrk(src+1, accept))
						{
							*dst++ = *src;
						}
						*dst = '\0';
					}
					else
					{
						sprintf(&value[len], "%s", (const char *)base->keys.chars);
					}
					_d2tk_base_clear_chars(base); // eat key
				}
			}
		}

		char *buf = alloca(maxlen + 1);
		if(buf)
		{
			snprintf(buf, maxlen, "%s|", value);
			value = buf;
		}
	}

	if(d2tk_state_is_focus_out(state))
	{
		// copy text from edit.text_out to value
		strncpy(value, base->edit.text_out, maxlen);

		state |= D2TK_STATE_CHANGED;
	}

	//FIXME handle d2tk_state_is_enter(state)

	d2tk_core_t *core = base->core;

	_d2tk_base_draw_text_field(core, state, rect, style, value, align);

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_label(d2tk_base_t *base, ssize_t lbl_len, const char *lbl,
	float mul, const d2tk_rect_t *rect, d2tk_align_t align)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);

	d2tk_core_t *core = base->core;

	const uint64_t hash = d2tk_hash_foreach(rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		lbl, lbl_len,
		&mul, sizeof(float),
		&align, sizeof(d2tk_align_t),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, rect, style->padding);

		if(lbl_len == -1) // zero terminated string
		{
			lbl_len = strlen(lbl);
		}

		const d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_core_save(core);
		d2tk_core_scissor(core, &bnd);
		d2tk_core_font_size(core, mul*bnd.h);
		d2tk_core_font_face(core, strlen(style->font_face), style->font_face);
		d2tk_core_color(core, style->text_color[triple]);
		d2tk_core_text(core, &bnd, lbl_len, lbl, align);
		d2tk_core_restore(core);

		d2tk_core_bbox_pop(core, ref);
	}

	return D2TK_STATE_NONE;
}

D2TK_API d2tk_state_t
d2tk_base_dial_bool(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	bool *value)
{
	const bool oldvalue = *value;

	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_SCROLL);

	if(d2tk_state_is_down(state) || d2tk_state_is_enter(state))
	{
		*value = !*value;
	}
	else if(d2tk_state_is_scroll_up(state))
	{
		*value = true;
	}
	else if(d2tk_state_is_scroll_down(state))
	{
		*value = false;
	}

	if(oldvalue != *value)
	{
		state |= D2TK_STATE_CHANGED;
	}

	const d2tk_style_t *style = d2tk_base_get_style(base);
	d2tk_core_t *core = base->core;

	const uint64_t hash = d2tk_hash_foreach(&state, sizeof(d2tk_state_t),
		rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		value, sizeof(bool),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_triple_t triple = D2TK_TRIPLE_NONE;

		if(*value)
		{
			triple |= D2TK_TRIPLE_ACTIVE;
		}

		if(d2tk_state_is_hot(state))
		{
			triple |= D2TK_TRIPLE_HOT;
		}

		if(d2tk_state_is_focused(state))
		{
			triple |= D2TK_TRIPLE_FOCUS;
		}

		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, rect, style->padding);

		const d2tk_coord_t d = bnd.h < bnd.w ? bnd.h : bnd.w;
		const d2tk_coord_t r1 = d / 2;
		const d2tk_coord_t r0 = d / 3;
		bnd.x += bnd.w / 2;
		bnd.y += bnd.h / 2;

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, bnd.x, bnd.y, r1, 0, 360, true);
		d2tk_core_color(core, style->fill_color[D2TK_TRIPLE_NONE]);
		d2tk_core_stroke_width(core, 0);
		d2tk_core_fill(core);

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, bnd.x, bnd.y, r1, 0, 360, true);
		d2tk_core_color(core, style->stroke_color[triple]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, bnd.x, bnd.y, r0, 0, 360, true);
		d2tk_core_color(core, style->fill_color[triple]);
		d2tk_core_stroke_width(core, 0);
		d2tk_core_fill(core);

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, bnd.x, bnd.y, r0, 0, 360, true);
		d2tk_core_color(core, style->stroke_color[D2TK_TRIPLE_NONE]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);
	}

	return state;
}

static inline void
_d2tk_base_draw_dial(d2tk_core_t *core, const d2tk_rect_t *rect,
	d2tk_state_t state, float rel, const d2tk_style_t *style)
{
	const uint64_t hash = d2tk_hash_foreach(&state, sizeof(d2tk_state_t),
		rect, sizeof(d2tk_rect_t),
		style, sizeof(d2tk_style_t),
		&rel, sizeof(float),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		d2tk_triple_t triple = D2TK_TRIPLE_NONE;
		d2tk_triple_t triple_active = D2TK_TRIPLE_ACTIVE;
		d2tk_triple_t triple_inactive = D2TK_TRIPLE_NONE;

		if(d2tk_state_is_active(state))
		{
			triple |= D2TK_TRIPLE_ACTIVE;
		}

		if(d2tk_state_is_hot(state))
		{
			triple |= D2TK_TRIPLE_HOT;
			triple_active |= D2TK_TRIPLE_HOT;
			triple_inactive |= D2TK_TRIPLE_HOT;
		}

		if(d2tk_state_is_focused(state))
		{
			triple |= D2TK_TRIPLE_FOCUS;
			triple_active |= D2TK_TRIPLE_FOCUS;
			triple_inactive |= D2TK_TRIPLE_FOCUS;
		}

		const size_t ref = d2tk_core_bbox_push(core, true, rect);

		d2tk_rect_t bnd;
		d2tk_rect_shrink(&bnd, rect, style->padding);

		const d2tk_coord_t d = bnd.h < bnd.w ? bnd.h : bnd.w;
		const d2tk_coord_t r1 = d / 2;
		const d2tk_coord_t r0 = d / 4;
		bnd.x += bnd.w / 2;
		bnd.y += bnd.h / 2;

		static const d2tk_coord_t a = 90 + 22; //FIXME
		static const d2tk_coord_t c = 90 - 22; //FIXME
		const d2tk_coord_t b = a + (360 - 44)*rel; //FIXME

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, bnd.x, bnd.y, (r1 + r0)/2, a, c, true);
		d2tk_core_color(core, style->fill_color[triple_inactive]);
		d2tk_core_stroke_width(core, r1 - r0);
		d2tk_core_stroke(core);

		if(rel > 0.f)
		{
			d2tk_core_begin_path(core);
			d2tk_core_arc(core, bnd.x, bnd.y, (r1 + r0)/2, a, b, true);
			d2tk_core_color(core, style->fill_color[triple_active]);
			d2tk_core_stroke_width(core, (r1 - r0) * 3/4);
			d2tk_core_stroke(core);
		}

		const float phi = (b + 90) / 180.f * M_PI;
		const d2tk_coord_t rx1 = bnd.x + r0 * sinf(phi);
		const d2tk_coord_t ry1 = bnd.y - r0 * cosf(phi);

		d2tk_core_begin_path(core);
		d2tk_core_move_to(core, bnd.x, bnd.y);
		d2tk_core_line_to(core, rx1, ry1);
		d2tk_core_close_path(core);
		d2tk_core_color(core, style->fill_color[triple_active]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_begin_path(core);
		d2tk_core_arc(core, bnd.x, bnd.y, r1, a, c, true);
		d2tk_core_arc(core, bnd.x, bnd.y, r0, c, a, false);
		d2tk_core_close_path(core);
		d2tk_core_color(core, style->stroke_color[triple]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);
	}
}

D2TK_API d2tk_state_t
d2tk_base_dial_int32(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	int32_t min, int32_t *value, int32_t max)
{
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_SCROLL);

	const int32_t oldvalue = *value;

	if(d2tk_state_is_scroll_up(state))
	{
		*value += base->scroll.ody;
		d2tk_clip_int32(min, value, max);
	}
	else if(d2tk_state_is_scroll_down(state))
	{
		*value += base->scroll.ody;
		d2tk_clip_int32(min, value, max);
	}
	else if(d2tk_state_is_motion(state))
	{
		const int32_t adx = abs(base->mouse.dx);
		const int32_t ady = abs(base->mouse.dy);
		const int32_t adz = adx > ady ? base->mouse.dx : -base->mouse.dy;

		*value += adz;
		d2tk_clip_int32(min, value, max);
	}

	if(oldvalue != *value)
	{
		state |= D2TK_STATE_CHANGED;
	}

	float rel = (float)(*value - min) / (max - min);
	d2tk_clip_float(0.f, &rel, 1.f);

	d2tk_core_t *core = base->core;
	_d2tk_base_draw_dial(core, rect, state, rel, d2tk_base_get_style(base));

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_prop_int32(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	int32_t min, int32_t *value, int32_t max)
{
	const d2tk_coord_t dw = rect->w / 3;

	const d2tk_rect_t left = {
		.x = rect->x,
		.y = rect->y,
		.w = dw,
		.h = rect->h
	};
	const d2tk_rect_t right = {
		.x = rect->x + dw,
		.y = rect->y,
		.w = rect->w - dw,
		.h = rect->h
	};

	const d2tk_id_t id_left = (1 << 24) | id;
	const d2tk_id_t id_right = (2 << 24) | id;

	const d2tk_state_t state_dial = d2tk_base_dial_int32(base, id_left, &left,
		min, value, max);

	char text [32];
	snprintf(text, sizeof(text), "%+"PRIi32, *value);

	const d2tk_state_t state_field = d2tk_base_text_field(base, id_right, &right,
		sizeof(text), text, D2TK_ALIGN_RIGHT | D2TK_ALIGN_MIDDLE, "1234567890+-");

	if(d2tk_state_is_changed(state_field))
	{
		d2tk_base_set_again(base);
	}

	const d2tk_state_t state = state_dial | state_field;

	if(d2tk_state_is_focus_out(state))
	{
		int32_t val;
		if(sscanf(text, "%"SCNi32, &val) == 1)
		{
			*value = val;
			d2tk_clip_int32(min, value, max);
		}
	}

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_dial_int64(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	int64_t min, int64_t *value, int64_t max)
{
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_SCROLL);

	const int64_t oldvalue = *value;

	if(d2tk_state_is_scroll_up(state))
	{
		*value += base->scroll.ody;
		d2tk_clip_int64(min, value, max);
	}
	else if(d2tk_state_is_scroll_down(state))
	{
		*value += base->scroll.ody;
		d2tk_clip_int64(min, value, max);
	}
	else if(d2tk_state_is_motion(state))
	{
		const int64_t adx = abs(base->mouse.dx);
		const int64_t ady = abs(base->mouse.dy);
		const int64_t adz = adx > ady ? base->mouse.dx : -base->mouse.dy;

		*value += adz;
		d2tk_clip_int64(min, value, max);
	}

	if(oldvalue != *value)
	{
		state |= D2TK_STATE_CHANGED;
	}

	float rel = (float)(*value - min) / (max - min);
	d2tk_clip_float(0.f, &rel, 1.f);

	d2tk_core_t *core = base->core;
	_d2tk_base_draw_dial(core, rect, state, rel, d2tk_base_get_style(base));

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_dial_float(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	float min, float *value, float max)
{
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_SCROLL);

	const float oldvalue = *value;

	if(d2tk_state_is_scroll_up(state))
	{
		const float dv = (max - min);
		const float mul = d2tk_base_get_mod(base) ? 0.01f : 0.1f;
		*value += dv * mul * base->scroll.ody;
		d2tk_clip_float(min, value, max);
	}
	else if(d2tk_state_is_scroll_down(state))
	{
		const float dv = (max - min);
		const float mul = d2tk_base_get_mod(base) ? 0.01f : 0.1f;
		*value += dv * mul * base->scroll.ody;
		d2tk_clip_float(min, value, max);
	}
	else if(d2tk_state_is_motion(state))
	{
		const float adx = abs(base->mouse.dx);
		const float ady = abs(base->mouse.dy);
		const float adz = adx > ady ? base->mouse.dx : -base->mouse.dy;

		const float dv = (max - min);
		const float mul = d2tk_base_get_mod(base) ? 0.001f : 0.01f;
		*value += dv * adz * mul;
		d2tk_clip_float(min, value, max);
	}

	if(oldvalue != *value)
	{
		state |= D2TK_STATE_CHANGED;
	}

	float rel = (*value - min) / (max - min);
	d2tk_clip_float(0.f, &rel, 1.f);

	d2tk_core_t *core = base->core;
	_d2tk_base_draw_dial(core, rect, state, rel, d2tk_base_get_style(base));

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_prop_float(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	float min, float *value, float max)
{
	const d2tk_coord_t dw = rect->w / 3;

	const d2tk_rect_t left = {
		.x = rect->x,
		.y = rect->y,
		.w = dw,
		.h = rect->h
	};
	const d2tk_rect_t right = {
		.x = rect->x + dw,
		.y = rect->y,
		.w = rect->w - dw,
		.h = rect->h
	};

	const d2tk_id_t id_left = (1 << 24) | id;
	const d2tk_id_t id_right = (2 << 24) | id;

	const d2tk_state_t state_dial = d2tk_base_dial_float(base, id_left, &left,
		min, value, max);

	char text [32];
	snprintf(text, sizeof(text), "%+.4f", *value);

	const d2tk_state_t state_field = d2tk_base_text_field(base, id_right, &right,
		sizeof(text), text, D2TK_ALIGN_RIGHT | D2TK_ALIGN_MIDDLE, "1234567890.+-");

	if(d2tk_state_is_changed(state_field))
	{
		d2tk_base_set_again(base);
	}

	const d2tk_state_t state = state_dial | state_field;

	if(d2tk_state_is_focus_out(state))
	{
		float val;
		if(sscanf(text, "%f", &val) == 1)
		{
			*value = val;
			d2tk_clip_float(min, value, max);
		}
	}

	return state;
}

D2TK_API d2tk_state_t
d2tk_base_dial_double(d2tk_base_t *base, d2tk_id_t id, const d2tk_rect_t *rect,
	double min, double *value, double max)
{
	d2tk_state_t state = d2tk_base_is_active_hot(base, id, rect,
		D2TK_FLAG_SCROLL);

	const double oldvalue = *value;

	if(d2tk_state_is_scroll_up(state))
	{
		const double dv = (max - min);
		const double mul = d2tk_base_get_mod(base) ? 0.01 : 0.1;
		*value += dv * mul * base->scroll.ody;
		d2tk_clip_double(min, value, max);
	}
	else if(d2tk_state_is_scroll_down(state))
	{
		const double dv = (max - min);
		const double mul = d2tk_base_get_mod(base) ? 0.01 : 0.1;
		*value += dv * mul * base->scroll.ody;
		d2tk_clip_double(min, value, max);
	}
	else if(d2tk_state_is_motion(state))
	{
		const double adx = abs(base->mouse.dx);
		const double ady = abs(base->mouse.dy);
		const double adz = adx > ady ? base->mouse.dx : -base->mouse.dy;

		const double dv = (max - min);
		const double mul = d2tk_base_get_mod(base) ? 0.001 : 0.01;
		*value += dv * adz * mul;
		d2tk_clip_double(min, value, max);
	}

	if(oldvalue != *value)
	{
		state |= D2TK_STATE_CHANGED;
	}

	float rel = (*value - min) / (max - min);
	d2tk_clip_float(0.f, &rel, 1.f);

	d2tk_core_t *core = base->core;
	_d2tk_base_draw_dial(core, rect, state, rel, d2tk_base_get_style(base));

	return state;
}

static d2tk_coord_t
_d2tk_flowmatrix_abs_x(d2tk_flowmatrix_t *flowmatrix, d2tk_coord_t rel_x)
{
	return flowmatrix->cx + rel_x * flowmatrix->scale;
}

static d2tk_coord_t
_d2tk_flowmatrix_abs_y(d2tk_flowmatrix_t *flowmatrix, d2tk_coord_t rel_y)
{
	return flowmatrix->cy + rel_y * flowmatrix->scale;
}

static void
_d2tk_flowmatrix_connect(d2tk_base_t *base, d2tk_flowmatrix_t *flowmatrix,
	const d2tk_pos_t *src_pos, const d2tk_pos_t *dst_pos)
{
	const d2tk_style_t *style = d2tk_base_get_style(base);

	d2tk_pos_t dst;
	if(!dst_pos) // connect to mouse pointer
	{
		d2tk_base_get_mouse_pos(base, &dst.x, &dst.y);
	}

	const uint64_t hash = d2tk_hash_foreach(flowmatrix, sizeof(d2tk_flowmatrix_t),
		src_pos, sizeof(d2tk_pos_t),
		dst_pos ? dst_pos : &dst, sizeof(d2tk_pos_t),
		style, sizeof(d2tk_style_t),
		NULL);

	d2tk_core_t *core = base->core;
	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const d2tk_coord_t w = flowmatrix->w;
		const d2tk_coord_t r = flowmatrix->r;
		const d2tk_coord_t x = _d2tk_flowmatrix_abs_x(flowmatrix, src_pos->x) + w/2 + r;
		const d2tk_coord_t y = _d2tk_flowmatrix_abs_y(flowmatrix, src_pos->y);

		if(dst_pos)
		{
			dst.x = _d2tk_flowmatrix_abs_x(flowmatrix, dst_pos->x) - w/2 - r;
			dst.y = _d2tk_flowmatrix_abs_y(flowmatrix, dst_pos->y);
		}

		const d2tk_coord_t x0 = (x < dst.x) ? x : dst.x;
		const d2tk_coord_t y0 = (y < dst.y) ? y : dst.y;
		const d2tk_coord_t x1 = (x > dst.x) ? x : dst.x;
		const d2tk_coord_t y1 = (y > dst.y) ? y : dst.y;

		const d2tk_rect_t bnd = {
			.x = x0 - 1,
			.y = y0 - 1,
			.w = x1 - x0 + 2,
			.h = y1 - y0 + 2
		};

		const d2tk_triple_t triple = D2TK_TRIPLE_FOCUS;

		const size_t ref = d2tk_core_bbox_push(core, false, &bnd);

		d2tk_core_begin_path(core);
		d2tk_core_move_to(core, x, y);
		d2tk_core_line_to(core, dst.x, dst.y);
		d2tk_core_color(core, style->stroke_color[triple]);
		d2tk_core_stroke_width(core, style->border_width);
		d2tk_core_stroke(core);

		d2tk_core_bbox_pop(core, ref);
	}
}

D2TK_API d2tk_flowmatrix_t *
d2tk_flowmatrix_begin(d2tk_base_t *base, const d2tk_rect_t *rect, d2tk_id_t id,
	d2tk_flowmatrix_t *flowmatrix)
{
	memset(flowmatrix, 0x0, sizeof(d2tk_flowmatrix_t));

	flowmatrix->base = base;
	flowmatrix->id = id;
	flowmatrix->rect = rect;
	flowmatrix->atom_body = _d2tk_base_get_atom(base, id, D2TK_ATOM_FLOW);
	flowmatrix->scale = exp2f(flowmatrix->atom_body->flow.exponent); //FIXME cache this instead
	flowmatrix->cx = flowmatrix->rect->x + flowmatrix->rect->w / 2
		- flowmatrix->atom_body->flow.x * flowmatrix->scale;
	flowmatrix->cy = flowmatrix->rect->y + flowmatrix->rect->h / 2
		- flowmatrix->atom_body->flow.y * flowmatrix->scale;

	flowmatrix->w = flowmatrix->scale * 150; //FIXME
	flowmatrix->h = flowmatrix->scale * 25; //FIXME
	flowmatrix->dd = flowmatrix->scale * 40; //FIXME
	flowmatrix->r = flowmatrix->scale * 4; //FIXME
	flowmatrix->s = flowmatrix->scale * 20; //FIXME

	d2tk_core_t *core = base->core;
	flowmatrix->ref = d2tk_core_bbox_container_push(core, false, flowmatrix->rect);

	return flowmatrix;
}

D2TK_API bool
d2tk_flowmatrix_not_end(d2tk_flowmatrix_t *flowmatrix)
{
	return flowmatrix ? true : false;
}

D2TK_API d2tk_flowmatrix_t *
d2tk_flowmatrix_next(d2tk_flowmatrix_t *flowmatrix)
{
	d2tk_base_t *base = flowmatrix->base;
	float *exponent = &flowmatrix->atom_body->flow.exponent;
	const float old_exponent = *exponent;

	const d2tk_state_t state = d2tk_base_is_active_hot(base, flowmatrix->id,
		flowmatrix->rect, D2TK_FLAG_SCROLL_Y);

	if(d2tk_state_is_scroll_down(state))
	{
		*exponent -= 0.125f; //FIXME
	}
	else if(d2tk_state_is_scroll_up(state))
	{
		*exponent += 0.125f; //FIXME
	}
	else if(d2tk_state_is_motion(state))
	{
		const d2tk_coord_t adx = base->mouse.dx / flowmatrix->scale;
		const d2tk_coord_t ady = base->mouse.dy / flowmatrix->scale;

		flowmatrix->atom_body->flow.x -= adx;
		flowmatrix->atom_body->flow.y -= ady;
	}

	d2tk_clip_float(-2.f, exponent, 1.f); //FIXME

	if(*exponent != old_exponent)
	{
		const d2tk_coord_t ox = (base->mouse.x - flowmatrix->cx) / flowmatrix->scale;
		const d2tk_coord_t oy = (base->mouse.y - flowmatrix->cy) / flowmatrix->scale;

		const float scale = exp2f(*exponent);

		const d2tk_coord_t fx = base->mouse.x - (ox * scale);
		const d2tk_coord_t fy = base->mouse.y - (oy * scale);

		flowmatrix->atom_body->flow.x = (flowmatrix->rect->x + flowmatrix->rect->w / 2 - fx) / scale;
		flowmatrix->atom_body->flow.y = (flowmatrix->rect->y + flowmatrix->rect->h / 2 - fy) / scale;

		d2tk_base_set_again(base);
	}

	if(flowmatrix->src_conn)
	{
		if(flowmatrix->dst_conn)
		{
			_d2tk_flowmatrix_connect(base, flowmatrix, &flowmatrix->src_pos,
				&flowmatrix->dst_pos);
		}
		else
		{
			_d2tk_flowmatrix_connect(base, flowmatrix, &flowmatrix->src_pos, NULL);

			// invalidate dst_id
			flowmatrix->atom_body->flow.dst_id = 0;
		}
	}

	d2tk_core_t *core = base->core;
	d2tk_core_bbox_pop(core, flowmatrix->ref);

	return NULL;
}

static void
_d2tk_flowmatrix_next_pos(d2tk_flowmatrix_t *flowmatrix, d2tk_pos_t *pos)
{
	flowmatrix->atom_body->flow.lx += 150; //FIXME
	flowmatrix->atom_body->flow.ly += 25; //FIXME

	pos->x = flowmatrix->atom_body->flow.lx;
	pos->y = flowmatrix->atom_body->flow.ly;
}

D2TK_API void
d2tk_flowmatrix_set_src(d2tk_flowmatrix_t *flowmatrix, d2tk_id_t id,
	const d2tk_pos_t *pos)
{
	flowmatrix->src_conn = true;
	flowmatrix->atom_body->flow.src_id = id;

	if(pos)
	{
		flowmatrix->src_pos = *pos;
	}
}

D2TK_API void
d2tk_flowmatrix_set_dst(d2tk_flowmatrix_t *flowmatrix, d2tk_id_t id,
	const d2tk_pos_t *pos)
{
	flowmatrix->dst_conn = true;
	flowmatrix->atom_body->flow.dst_id = id;

	if(pos)
	{
		flowmatrix->dst_pos = *pos;
	}
}

D2TK_API d2tk_id_t
d2tk_flowmatrix_get_src(d2tk_flowmatrix_t *flowmatrix, d2tk_pos_t *pos)
{
	if(pos)
	{
		*pos = flowmatrix->src_pos;
	}

	return flowmatrix->atom_body->flow.src_id;
}

D2TK_API d2tk_id_t
d2tk_flowmatrix_get_dst(d2tk_flowmatrix_t *flowmatrix, d2tk_pos_t *pos)
{
	if(pos)
	{
		*pos = flowmatrix->dst_pos;
	}

	return flowmatrix->atom_body->flow.dst_id;
}

D2TK_API d2tk_flowmatrix_node_t *
d2tk_flowmatrix_node_begin(d2tk_base_t *base, d2tk_flowmatrix_t *flowmatrix,
	d2tk_pos_t *pos, d2tk_flowmatrix_node_t *node)
{
	node->flowmatrix = flowmatrix;

	// derive initial position
	if( (pos->x == 0) && (pos->y == 0) )
	{
		_d2tk_flowmatrix_next_pos(flowmatrix, pos);
	}

	const d2tk_coord_t x = _d2tk_flowmatrix_abs_x(flowmatrix, pos->x);
	const d2tk_coord_t y = _d2tk_flowmatrix_abs_y(flowmatrix, pos->y);
	const d2tk_coord_t w = flowmatrix->w;
	const d2tk_coord_t h = flowmatrix->h;

	node->rect.x = x - w/2;
	node->rect.y = y - h/2;
	node->rect.w = w;
	node->rect.h = h;

	d2tk_core_t *core = base->core;
	d2tk_coord_t cw;
	d2tk_coord_t ch;

	d2tk_core_get_dimensions(core, &cw, &ch);
	if(  (node->rect.x >= cw)
			|| (node->rect.y >= ch)
			|| (node->rect.x <= -node->rect.w)
			|| (node->rect.y <= -node->rect.h) )
	{
		return NULL;
	}

	const d2tk_style_t *style = d2tk_base_get_style(base);

	const uint64_t hash = d2tk_hash_foreach(flowmatrix, sizeof(d2tk_flowmatrix_t),
		pos, sizeof(d2tk_pos_t),
		node, sizeof(d2tk_flowmatrix_node_t),
		style, sizeof(d2tk_style_t),
		NULL);

	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const d2tk_coord_t r = flowmatrix->r;
		const d2tk_coord_t r2 = r*2;
		const d2tk_triple_t triple = D2TK_TRIPLE_NONE; //FIXME

		// sink connection point
		{
			const d2tk_coord_t x0 = node->rect.x - r;

			const d2tk_rect_t bnd = {
				.x = x0 - r,
				.y = y - r,
				.w = r2,
				.h = r2
			};

			const size_t ref = d2tk_core_bbox_push(core, true, &bnd);

			d2tk_core_begin_path(core);
			d2tk_core_arc(core, x0, y, r, 0, 360, true);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			d2tk_core_begin_path(core);
			d2tk_core_arc(core, x0, y, r, 0, 360, true);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}

		// source connection point
		{
			const d2tk_coord_t x0 = node->rect.x + node->rect.w + r;

			const d2tk_rect_t bnd = {
				.x = x0 - r,
				.y = y - r,
				.w = r2,
				.h = r2
			};

			const size_t ref = d2tk_core_bbox_push(core, true, &bnd);

			d2tk_core_begin_path(core);
			d2tk_core_arc(core, x0, y, r, 0, 360, true);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			d2tk_core_begin_path(core);
			d2tk_core_arc(core, x0, y, r, 0, 360, true);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}
	}

	return node;
}

D2TK_API bool
d2tk_flowmatrix_node_not_end(d2tk_flowmatrix_node_t *node)
{
	return node ? true : false;
}

D2TK_API d2tk_flowmatrix_node_t *
d2tk_flowmatrix_node_next(d2tk_flowmatrix_node_t *node, d2tk_pos_t *pos,
	const d2tk_state_t *state)
{
	d2tk_flowmatrix_t *flowmatrix = node->flowmatrix;
	d2tk_base_t *base = flowmatrix->base;

	if(d2tk_state_is_motion(*state))
	{
		const d2tk_coord_t adx = base->mouse.dx / flowmatrix->scale;
		const d2tk_coord_t ady = base->mouse.dy / flowmatrix->scale;

		pos->x += adx;
		pos->y += ady;

		d2tk_base_set_again(base);
	}

	return NULL;
}

D2TK_API const d2tk_rect_t *
d2tk_flowmatrix_node_get_rect(d2tk_flowmatrix_node_t *node)
{
	return &node->rect;
}

D2TK_API d2tk_flowmatrix_arc_t *
d2tk_flowmatrix_arc_begin(d2tk_base_t *base, d2tk_flowmatrix_t *flowmatrix,
	unsigned N, unsigned M, const d2tk_pos_t *src, const d2tk_pos_t *dst,
	d2tk_pos_t *pos, d2tk_flowmatrix_arc_t *arc)
{
	memset(arc, 0x0, sizeof(d2tk_flowmatrix_arc_t));

	// derive initial position
	if( (pos->x == 0) && (pos->y == 0) )
	{
		pos->x = (src->x + dst->x) / 2;
		pos->y = (src->y + dst->y) / 2;
	}

	arc->flowmatrix = flowmatrix;
	arc->x = 0;
	arc->y = 0;
	arc->N = N+1;
	arc->M = M+1;
	arc->NM = (N+1)*(M+1);
	arc->k = 0;

	const d2tk_coord_t x = _d2tk_flowmatrix_abs_x(flowmatrix, pos->x);
	const d2tk_coord_t y = _d2tk_flowmatrix_abs_y(flowmatrix, pos->y);
	const d2tk_coord_t s = flowmatrix->s;
	arc->c = M_SQRT2 * s;
	arc->c_2 = arc->c / 2;
	arc->c_4 = arc->c / 4;

	const d2tk_coord_t x0 = x - M*arc->c_2;
	const d2tk_coord_t x1 = x;
	const d2tk_coord_t x2 = x + (N - M)*arc->c_2;
	const d2tk_coord_t x3 = x + N*arc->c_2;

	const d2tk_coord_t y0 = y;
	const d2tk_coord_t y1 = y + M*arc->c_2;
	const d2tk_coord_t y2 = y + N*arc->c_2;
	const d2tk_coord_t y3 = y + (N+M)*arc->c_2;

	arc->xo = x1 - arc->c_2;
	arc->yo = y0 + arc->c_4;
	arc->rect.x = arc->xo;
	arc->rect.y = arc->yo;
	arc->rect.w = arc->c;
	arc->rect.h = arc->c_2;

	const d2tk_style_t *style = d2tk_base_get_style(base);

	const uint64_t hash = d2tk_hash_foreach(flowmatrix, sizeof(d2tk_flowmatrix_t),
		&N, sizeof(unsigned),
		&M, sizeof(unsigned),
		src, sizeof(d2tk_pos_t),
		dst, sizeof(d2tk_pos_t),
		pos, sizeof(d2tk_pos_t),
		arc, sizeof(d2tk_flowmatrix_arc_t),
		style, sizeof(d2tk_style_t),
		NULL);

	d2tk_core_t *core = base->core;
	D2TK_CORE_WIDGET(core, hash, widget)
	{
		const d2tk_coord_t r = flowmatrix->r;
		const d2tk_coord_t ox = flowmatrix->w/2;
		const d2tk_coord_t dd = flowmatrix->dd;
		const d2tk_triple_t triple = D2TK_TRIPLE_NONE; //FIXME
		const d2tk_coord_t b = style->border_width;
		const d2tk_coord_t b2 = b*2;

		const d2tk_coord_t xs = _d2tk_flowmatrix_abs_x(flowmatrix, src->x) + ox + r;
		const d2tk_coord_t ys = _d2tk_flowmatrix_abs_y(flowmatrix, src->y);
		const d2tk_coord_t xp = x;
		const d2tk_coord_t yp = y - r;
		const d2tk_coord_t xd = _d2tk_flowmatrix_abs_x(flowmatrix, dst->x) - ox - r;
		const d2tk_coord_t yd = _d2tk_flowmatrix_abs_y(flowmatrix, dst->y);

		// sink arc
		{
			const d2tk_coord_t x0 = xs;
			const d2tk_coord_t x1 = xs + dd;
			const d2tk_coord_t x2 = xp - dd;
			const d2tk_coord_t x3 = xp;

			const d2tk_coord_t y0 = ys;
			const d2tk_coord_t y1 = ys;
			const d2tk_coord_t y2 = yp;
			const d2tk_coord_t y3 = yp;

			d2tk_coord_t xa = INT32_MAX;
			d2tk_coord_t xb = INT32_MIN;
			if(x0 < xa) xa = x0;
			if(x1 < xa) xa = x1;
			if(x2 < xa) xa = x2;
			if(x3 < xa) xa = x3;
			if(x0 > xb) xb = x0;
			if(x1 > xb) xb = x1;
			if(x2 > xb) xb = x2;
			if(x3 > xb) xb = x3;

			d2tk_coord_t ya = INT32_MAX;
			d2tk_coord_t yb = INT32_MIN;
			if(y0 < ya) ya = y0;
			if(y1 < ya) ya = y1;
			if(y2 < ya) ya = y2;
			if(y3 < ya) ya = y3;
			if(y0 > yb) yb = y0;
			if(y1 > yb) yb = y1;
			if(y2 > yb) yb = y2;
			if(y3 > yb) yb = y3;

			const d2tk_rect_t bnd = {
				.x = xa - b,
				.y = ya - b,
				.w = xb - xa + b2,
				.h = yb - ya + b2
			};

			const size_t ref = d2tk_core_bbox_push(core, false, &bnd);

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y0);
			d2tk_core_curve_to(core, x1, y1, x2, y2, x3, y3);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, b);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}

		// source arc
		{
			const d2tk_coord_t x0 = xp;
			const d2tk_coord_t x1 = xp + dd;
			const d2tk_coord_t x2 = xd - dd;
			const d2tk_coord_t x3 = xd;

			const d2tk_coord_t y0 = yp;
			const d2tk_coord_t y1 = yp;
			const d2tk_coord_t y2 = yd;
			const d2tk_coord_t y3 = yd;

			d2tk_coord_t xa = INT32_MAX;
			d2tk_coord_t xb = INT32_MIN;
			if(x0 < xa) xa = x0;
			if(x1 < xa) xa = x1;
			if(x2 < xa) xa = x2;
			if(x3 < xa) xa = x3;
			if(x0 > xb) xb = x0;
			if(x1 > xb) xb = x1;
			if(x2 > xb) xb = x2;
			if(x3 > xb) xb = x3;

			d2tk_coord_t ya = INT32_MAX;
			d2tk_coord_t yb = INT32_MIN;
			if(y0 < ya) ya = y0;
			if(y1 < ya) ya = y1;
			if(y2 < ya) ya = y2;
			if(y3 < ya) ya = y3;
			if(y0 > yb) yb = y0;
			if(y1 > yb) yb = y1;
			if(y2 > yb) yb = y2;
			if(y3 > yb) yb = y3;

			const d2tk_rect_t bnd = {
				.x = xa - b,
				.y = ya - b,
				.w = xb - xa + b2,
				.h = yb - ya + b2
			};

			const size_t ref = d2tk_core_bbox_push(core, false, &bnd);

			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y0);
			d2tk_core_curve_to(core, x1, y1, x2, y2, x3, y3);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, b);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}

		// matrix
		{
			const d2tk_rect_t bnd = {
				.x = x0,
				.y = y0,
				.w = x3 - x0,
				.h = y3 - y0
			};

			const size_t ref = d2tk_core_bbox_push(core, true, &bnd);

			// matrix bounding box
			d2tk_core_begin_path(core);
			d2tk_core_move_to(core, x0, y1);
			d2tk_core_line_to(core, x1, y0);
			d2tk_core_line_to(core, x3, y2);
			d2tk_core_line_to(core, x2, y3);
			d2tk_core_close_path(core);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			// grid lines
			for(unsigned j = 0, o = 0;
				j < M + 1;
				j++, o += arc->c_2)
			{
				d2tk_core_begin_path(core);
				d2tk_core_move_to(core, x0 + o, y1 - o);
				d2tk_core_line_to(core, x2 + o, y3 - o);
				d2tk_core_color(core, style->stroke_color[triple]);
				d2tk_core_stroke_width(core, style->border_width);
				d2tk_core_stroke(core);
			}

			// grid lines
			for(unsigned i = 0, o = 0;
				i < N + 1;
				i++, o += arc->c_2)
			{
				d2tk_core_begin_path(core);
				d2tk_core_move_to(core, x0 + o, y1 + o);
				d2tk_core_line_to(core, x1 + o, y0 + o);
				d2tk_core_color(core, style->stroke_color[triple]);
				d2tk_core_stroke_width(core, style->border_width);
				d2tk_core_stroke(core);
			}

			d2tk_core_bbox_pop(core, ref);
		}

		// connection point
		{
			const d2tk_coord_t r2 = r*2;
			const d2tk_rect_t bnd = {
				.x = xp - r,
				.y = yp - r,
				.w = r2,
				.h = r2
			};

			const size_t ref = d2tk_core_bbox_push(core, true, &bnd);

			d2tk_core_begin_path(core);
			d2tk_core_arc(core, xp, yp, r, 0, 360, true);
			d2tk_core_color(core, style->fill_color[triple]);
			d2tk_core_stroke_width(core, 0);
			d2tk_core_fill(core);

			d2tk_core_begin_path(core);
			d2tk_core_arc(core, xp, yp, r, 0, 360, true);
			d2tk_core_color(core, style->stroke_color[triple]);
			d2tk_core_stroke_width(core, style->border_width);
			d2tk_core_stroke(core);

			d2tk_core_bbox_pop(core, ref);
		}
	}

	return arc;
}

D2TK_API bool
d2tk_flowmatrix_arc_not_end(d2tk_flowmatrix_arc_t *arc)
{
	return arc->k < arc->NM - 1;
}

D2TK_API d2tk_flowmatrix_arc_t *
d2tk_flowmatrix_arc_next(d2tk_flowmatrix_arc_t *arc, d2tk_pos_t *pos,
	const d2tk_state_t *state)
{
	d2tk_flowmatrix_t *flowmatrix = arc->flowmatrix;
	d2tk_base_t *base = flowmatrix->base;

	if(d2tk_state_is_motion(*state))
	{
		const d2tk_coord_t adx = base->mouse.dx / flowmatrix->scale;
		const d2tk_coord_t ady = base->mouse.dy / flowmatrix->scale;

		pos->x += adx;
		pos->y += ady;

		d2tk_base_set_again(base);
	}

	{
		++arc->k;

		if(++arc->x % arc->N)
		{
			// nothing to do
		}
		else // overflow
		{
			arc->x = 0;
			++arc->y;
		}

		arc->rect.x = arc->xo + (arc->x - arc->y)*arc->c_2;
		arc->rect.y = arc->yo + (arc->x + arc->y)*arc->c_2;
		arc->rect.w = arc->c;
		arc->rect.h = arc->c_2;

		if(arc->y == (arc->M - 1)) // source label
		{
			arc->rect.x -= arc->rect.w*1 + arc->c_2;
			arc->rect.y -= arc->c_4;
			arc->rect.w *= 2;
		}
		else if(arc->x == (arc->N - 1)) // sink label
		{
			arc->rect.x += arc->c_2;
			arc->rect.y -= arc->c_4;
			arc->rect.w *= 2;
		}
	}

	return arc;
}

D2TK_API unsigned
d2tk_flowmatrix_arc_get_index(d2tk_flowmatrix_arc_t *arc)
{
	return arc->k;
}

D2TK_API unsigned
d2tk_flowmatrix_arc_get_index_x(d2tk_flowmatrix_arc_t *arc)
{
	return arc->x;
}

D2TK_API unsigned
d2tk_flowmatrix_arc_get_index_y(d2tk_flowmatrix_arc_t *arc)
{
	return arc->y;
}

D2TK_API const d2tk_rect_t *
d2tk_flowmatrix_arc_get_rect(d2tk_flowmatrix_arc_t *arc __attribute__((unused)))
{
	return &arc->rect;
}

D2TK_API d2tk_base_t *
d2tk_base_new(const d2tk_core_driver_t *driver, void *data)
{
	d2tk_base_t *base = calloc(1, sizeof(d2tk_base_t));
	if(!base)
	{
		return NULL;
	}

	base->core = d2tk_core_new(driver, data);

	return base;
}

D2TK_API void
d2tk_base_set_ttls(d2tk_base_t *base, uint32_t sprites, uint32_t memcaches)
{
	d2tk_core_set_ttls(base->core, sprites, memcaches);
}

D2TK_API void
d2tk_base_free(d2tk_base_t *base)
{
	d2tk_core_free(base->core);
	free(base);
}

D2TK_API void
d2tk_base_pre(d2tk_base_t *base)
{
	// reset hot item
	_d2tk_flip_clear(&base->hotitem);

	// calculate mouse motion
	base->mouse.dx = (int32_t)base->mouse.x - base->mouse.ox;
	base->mouse.dy = (int32_t)base->mouse.y - base->mouse.oy;

	// reset again flag
	base->again = false;

	// reset clear-focus flag
	base->clear_focus = false;

	const d2tk_style_t *style = d2tk_base_get_style(base);
	d2tk_core_set_bg_color(base->core, style->bg_color);

	d2tk_core_pre(base->core);
}

D2TK_API void
d2tk_base_post(d2tk_base_t *base)
{
	// clear scroll
	base->scroll.dx = 0.f;
	base->scroll.dy = 0.f;

	// store old mouse position
	base->mouse.ox = base->mouse.x;
	base->mouse.oy = base->mouse.y;

	if(base->clear_focus)
	{
		_d2tk_flip_clear(&base->activeitem);

		base->focused = false;
	}

	d2tk_core_post(base->core);
}

D2TK_API void
d2tk_base_clear_focus(d2tk_base_t *base)
{
	base->clear_focus = true;
}

D2TK_API void
d2tk_base_set_again(d2tk_base_t *base)
{
	base->again = true;
}

D2TK_API bool
d2tk_base_get_again(d2tk_base_t *base)
{
	return base->again;
}

D2TK_API void
d2tk_base_set_mouse_l(d2tk_base_t *base, bool down)
{
	base->mouse.l = down;
}

D2TK_API void
d2tk_base_set_mouse_m(d2tk_base_t *base, bool down)
{
	base->mouse.m = down;
}

D2TK_API void
d2tk_base_set_mouse_r(d2tk_base_t *base, bool down)
{
	base->mouse.r = down;
}

D2TK_API void
d2tk_base_set_mouse_pos(d2tk_base_t *base, d2tk_coord_t x, d2tk_coord_t y)
{
	base->mouse.x = x;
	base->mouse.y = y;
}

D2TK_API void
d2tk_base_get_mouse_pos(d2tk_base_t *base, d2tk_coord_t *x, d2tk_coord_t *y)
{
	if(x)
	{
		*x = base->mouse.x;
	}

	if(y)
	{
		*y = base->mouse.y;
	}
}

D2TK_API void
d2tk_base_add_mouse_scroll(d2tk_base_t *base, int32_t dx, int32_t dy)
{
	base->scroll.dx += dx;
	base->scroll.dy += dy;
}

D2TK_API void
d2tk_base_set_shift(d2tk_base_t *base, bool down)
{
	base->keys.shift = down;
}

D2TK_API void
d2tk_base_set_ctrl(d2tk_base_t *base, bool down)
{
	base->keys.ctrl = down;
}

D2TK_API void
d2tk_base_set_alt(d2tk_base_t *base, bool down)
{
	base->keys.alt = down;
}

D2TK_API void
d2tk_base_set_left(d2tk_base_t *base, bool down)
{
	base->keys.left = down;
}

D2TK_API void
d2tk_base_set_right(d2tk_base_t *base, bool down)
{
	base->keys.right = down;
}

D2TK_API void
d2tk_base_set_up(d2tk_base_t *base, bool down)
{
	base->keys.up = down;
}

D2TK_API void
d2tk_base_set_down(d2tk_base_t *base, bool down)
{
	base->keys.down = down;
}

D2TK_API bool
d2tk_base_get_left(d2tk_base_t *base)
{
	return base->keys.left;
}

D2TK_API bool
d2tk_base_get_right(d2tk_base_t *base)
{
	return base->keys.right;
}

D2TK_API bool
d2tk_base_get_up(d2tk_base_t *base)
{
	return base->keys.up;
}

D2TK_API bool
d2tk_base_get_down(d2tk_base_t *base)
{
	return base->keys.down;
}

D2TK_API void
d2tk_base_set_dimensions(d2tk_base_t *base, d2tk_coord_t w, d2tk_coord_t h)
{
	d2tk_core_set_dimensions(base->core, w, h);
}

D2TK_API void
d2tk_base_get_dimensions(d2tk_base_t *base, d2tk_coord_t *w, d2tk_coord_t *h)
{
	d2tk_core_get_dimensions(base->core, w, h);
}
