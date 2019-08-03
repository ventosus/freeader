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

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <d2tk/base.h>
#include <d2tk/hash.h>
#include "mock.h"

#define N 4
static void
_expose_hot(d2tk_base_t *base, const d2tk_rect_t *rect, unsigned n, unsigned p)
{
#define hot_over (D2TK_STATE_HOT | D2TK_STATE_OVER)
	static const d2tk_state_t states [N][N] = {
		[0] = {
			[0] = D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN | hot_over
		},
		[1] = {
			[0] = D2TK_STATE_FOCUS,
			[1] = hot_over
		},
		[2] = {
			[0] = D2TK_STATE_FOCUS,
			[2] = hot_over
		},
		[3] = {
			[0] = D2TK_STATE_FOCUS,
			[3] = hot_over
		}
	};
#undef hot_over

	D2TK_BASE_TABLE(rect, n, 1, tab)
	{
		const d2tk_rect_t *bnd = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const d2tk_id_t id = D2TK_ID_IDX(k);

		const d2tk_state_t state = d2tk_base_button(base, id, bnd);

		assert(state == states[p][k]);
	}
}

static void
_test_hot()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	d2tk_base_set_dimensions(base, DIM_W, DIM_H);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);

	for(unsigned p = 0; p < N; p++)
	{
		const float p5 = p + 0.5f;
		const d2tk_coord_t mx = DIM_W / N * p5;
		const d2tk_coord_t my = DIM_H / 2;

		d2tk_base_set_mouse_pos(base, mx, my);

		d2tk_base_pre(base);

		_expose_hot(base, &rect, N, p);

		d2tk_base_post(base);
		assert(!d2tk_base_get_again(base));
	}

	// trigger garbage collector
	for(unsigned i = 0; i < 0x400; i++)
	{
		d2tk_base_pre(base);
		d2tk_base_post(base);
		assert(!d2tk_base_get_again(base));
	}

	d2tk_base_free(base);
}
#undef N

#define N 4
static void
_expose_mouse_fwd_focus(d2tk_base_t *base, const d2tk_rect_t *rect, unsigned n,
	unsigned p, bool down)
{
#define focus_down ( \
		D2TK_STATE_DOWN \
	| D2TK_STATE_ACTIVE \
	| D2TK_STATE_HOT \
	| D2TK_STATE_FOCUS \
	| D2TK_STATE_FOCUS_IN \
	| D2TK_STATE_MOTION \
	| D2TK_STATE_CHANGED \
	| D2TK_STATE_OVER )
#define focus_up ( \
		D2TK_STATE_UP \
	| D2TK_STATE_HOT \
	| D2TK_STATE_FOCUS \
	| D2TK_STATE_OVER )
	static const d2tk_state_t states [N][2][N] = {
		[0] = {
			[true] = {
				[0] = focus_down
			},
			[false] = {
				[0] = focus_up
			}
		},
		[1] = {
			[true] = {
				[0] = D2TK_STATE_FOCUS,
				[1] = focus_down
			},
			[false] = {
				[0] = D2TK_STATE_FOCUS_OUT,
				[1] = focus_up
			}
		},
		[2] = {
			[true] = {
				[1] = D2TK_STATE_FOCUS,
				[2] = focus_down
			},
			[false] = {
				[1] = D2TK_STATE_FOCUS_OUT,
				[2] = focus_up
			}
		},
		[3] = {
			[true] = {
				[2] = D2TK_STATE_FOCUS,
				[3] = focus_down
			},
			[false] = {
				[2] = D2TK_STATE_FOCUS_OUT,
				[3] = focus_up
			}
		}
	};
#undef focus_up
#undef focus_down

	D2TK_BASE_TABLE(rect, n, 1, tab)
	{
		const d2tk_rect_t *bnd = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const d2tk_id_t id = D2TK_ID_IDX(k);

		const d2tk_state_t state = d2tk_base_button(base, id, bnd);

		assert(state == states[p][down][k]);
	}
}

static void
_test_mouse_fwd_focus()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	d2tk_base_set_dimensions(base, DIM_W, DIM_H);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);

	for(unsigned p = 0; p < N; p++)
	{
		const float p5 = p + 0.5f;
		const d2tk_coord_t mx = DIM_W / N * p5;
		const d2tk_coord_t my = DIM_H / 2;

		{
			d2tk_base_set_mouse_pos(base, mx, my);
			d2tk_base_set_mouse_l(base, true);

			d2tk_base_pre(base);

			_expose_mouse_fwd_focus(base, &rect, N, p, true);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}

		{
			d2tk_base_set_mouse_l(base, false);

			d2tk_base_pre(base);

			_expose_mouse_fwd_focus(base, &rect, N, p, false);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}
	}

	// trigger garbage collector
	for(unsigned i = 0; i < 0x400; i++)
	{
		d2tk_base_pre(base);
		d2tk_base_post(base);
		assert(!d2tk_base_get_again(base));
	}

	d2tk_base_free(base);
}
#undef N

#define N 4
static void
_expose_mouse_bwd_focus(d2tk_base_t *base, const d2tk_rect_t *rect, unsigned n,
	unsigned p, bool down)
{
#define focus_down ( \
		D2TK_STATE_DOWN \
	| D2TK_STATE_ACTIVE \
	| D2TK_STATE_HOT \
	| D2TK_STATE_FOCUS \
	| D2TK_STATE_FOCUS_IN \
	| D2TK_STATE_MOTION \
	| D2TK_STATE_CHANGED \
	| D2TK_STATE_OVER )
#define focus_up ( \
		D2TK_STATE_UP \
	| D2TK_STATE_HOT \
	| D2TK_STATE_FOCUS \
	| D2TK_STATE_OVER )
	static const d2tk_state_t states [N][2][N] = {
		[0] = {
			[true] = {
				[0] = D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN,
				[3] = focus_down
			},
			[false] = {
				[0] = D2TK_STATE_FOCUS_OUT,
				[3] = focus_up
			}
		},
		[1] = {
			[true] = {
				[2] = focus_down,
				[3] = D2TK_STATE_FOCUS_OUT
			},
			[false] = {
				[2] = focus_up
			}
		},
		[2] = {
			[true] = {
				[1] = focus_down,
				[2] = D2TK_STATE_FOCUS_OUT
			},
			[false] = {
				[1] = focus_up
			}
		},
		[3] = {
			[true] = {
				[0] = focus_down,
				[1] = D2TK_STATE_FOCUS_OUT
			},
			[false] = {
				[0] = focus_up
			}
		}
	};
#undef focus_up
#undef focus_down

	D2TK_BASE_TABLE(rect, n, 1, tab)
	{
		const d2tk_rect_t *bnd = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const d2tk_id_t id = D2TK_ID_IDX(k);

		const d2tk_state_t state = d2tk_base_button(base, id, bnd);

		assert(state == states[p][down][k]);
	}
}

static void
_test_mouse_bwd_focus()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	d2tk_base_set_dimensions(base, DIM_W, DIM_H);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);

	for(unsigned p = 0; p < N; p++)
	{
		const float p5 = p + 0.5f;
		const d2tk_coord_t mx = DIM_W - DIM_W / N * p5;
		const d2tk_coord_t my = DIM_H / 2;

		{
			d2tk_base_set_mouse_pos(base, mx, my);
			d2tk_base_set_mouse_l(base, true);

			d2tk_base_pre(base);

			_expose_mouse_bwd_focus(base, &rect, N, p, true);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}

		{
			d2tk_base_set_mouse_l(base, false);

			d2tk_base_pre(base);

			_expose_mouse_bwd_focus(base, &rect, N, p, false);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}
	}

	// trigger garbage collector
	for(unsigned i = 0; i < 0x400; i++)
	{
		d2tk_base_pre(base);
		d2tk_base_post(base);
		assert(!d2tk_base_get_again(base));
	}

	d2tk_base_free(base);
}
#undef N

#define N 4
static void
_expose_key_fwd_focus(d2tk_base_t *base, const d2tk_rect_t *rect, unsigned n,
	unsigned p, bool down)
{
#define invar D2TK_STATE_HOT | D2TK_STATE_OVER
	static const d2tk_state_t states [N][2][N] = {
		[0] = {
			[true] = {
				[0] = invar | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
			},
			[false] = {
				[0] = invar | D2TK_STATE_FOCUS
			}
		},
		[1] = {
			[true] = {
				[0] = invar,
				[1] = D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
			},
			[false] = {
				[0] = invar | D2TK_STATE_FOCUS_OUT,
				[1] = D2TK_STATE_FOCUS
			}
		},
		[2] = {
			[true] = {
				[0] = invar,
				[2] = D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
			},
			[false] = {
				[0] = invar,
				[1] = D2TK_STATE_FOCUS_OUT,
				[2] = D2TK_STATE_FOCUS
			}
		},
		[3] = {
			[true] = {
				[0] = invar,
				[3] = D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
			},
			[false] = {
				[0] = invar,
				[2] = D2TK_STATE_FOCUS_OUT,
				[3] = D2TK_STATE_FOCUS
			}
		}
	};
#undef invar

	D2TK_BASE_TABLE(rect, n, 1, tab)
	{
		const d2tk_rect_t *bnd = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const d2tk_id_t id = D2TK_ID_IDX(k);

		const d2tk_state_t state = d2tk_base_button(base, id, bnd);

		assert(state == states[p][down][k]);
	}
}

static void
_test_key_fwd_focus()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	d2tk_base_set_dimensions(base, DIM_W, DIM_H);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);

	for(unsigned p = 0; p < N; p++)
	{
		{
			d2tk_base_set_ctrl(base, true);
			d2tk_base_set_right(base, true);

			d2tk_base_pre(base);

			_expose_key_fwd_focus(base, &rect, N, p, true);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}

		{
			d2tk_base_set_right(base, false);
			d2tk_base_set_ctrl(base, false);

			d2tk_base_pre(base);

			_expose_key_fwd_focus(base, &rect, N, p, false);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}
	}

	// trigger garbage collector
	for(unsigned i = 0; i < 0x400; i++)
	{
		d2tk_base_pre(base);
		d2tk_base_post(base);
		assert(!d2tk_base_get_again(base));
	}

	d2tk_base_free(base);
}
#undef N

#define N 4
static void
_expose_key_bwd_focus(d2tk_base_t *base, const d2tk_rect_t *rect, unsigned n,
	unsigned p, bool down)
{
#define invar D2TK_STATE_HOT | D2TK_STATE_OVER
	static const d2tk_state_t states [N][2][N] = {
		[0] = {
			[true] = {
				[0] = invar | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
			},
			[false] = {
				[0] = invar | D2TK_STATE_FOCUS_OUT
			}
		},
		[1] = { //FIXME something's very wrong here, focus seems lost
			[true] = {
				[0] = invar
			},
			[false] = {
				[0] = invar
			}
		},
		[2] = {
			[true] = {
				[0] = invar
			},
			[false] = {
				[0] = invar
			}
		},
		[3] = {
			[true] = {
				[0] = invar
			},
			[false] = {
				[0] = invar
			}
		}
	};
#undef invar

	D2TK_BASE_TABLE(rect, n, 1, tab)
	{
		const d2tk_rect_t *bnd = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const d2tk_id_t id = D2TK_ID_IDX(k);

		const d2tk_state_t state = d2tk_base_button(base, id, bnd);

		assert(state == states[p][down][k]);
	}
}

static void
_test_key_bwd_focus()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	d2tk_base_set_dimensions(base, DIM_W, DIM_H);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);

	for(unsigned p = 0; p < N; p++)
	{
		{
			d2tk_base_set_ctrl(base, true);
			d2tk_base_set_left(base, true);

			d2tk_base_pre(base);

			_expose_key_bwd_focus(base, &rect, N, p, true);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}

		{
			d2tk_base_set_left(base, false);
			d2tk_base_set_ctrl(base, false);

			d2tk_base_pre(base);

			_expose_key_bwd_focus(base, &rect, N, p, false);

			d2tk_base_post(base);
			assert(!d2tk_base_get_again(base));
		}
	}

	// trigger garbage collector
	for(unsigned i = 0; i < 0x400; i++)
	{
		d2tk_base_pre(base);
		d2tk_base_post(base);
		assert(!d2tk_base_get_again(base));
	}

	d2tk_base_free(base);
}
#undef N

static void
_test_get_set()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	{
		d2tk_coord_t w = 0;
		d2tk_coord_t h = 0;

		d2tk_base_set_dimensions(base, DIM_W, DIM_H);
		d2tk_base_get_dimensions(base, &w, &h);

		assert(w == DIM_W);
		assert(h == DIM_H);
	}

	{
		d2tk_base_set_ttls(base, 128, 128);
		//FIXME add getter
	}

	{
		d2tk_base_clear_focus(base);
		d2tk_base_set_again(base);
		//FIXME add getter
	}

	{
		d2tk_base_set_mouse_l(base, true);
		d2tk_base_set_mouse_m(base, true);
		d2tk_base_set_mouse_r(base, true);
		//FIXME add getter
	}

	{
		d2tk_coord_t x = 0;
		d2tk_coord_t y = 0;

		d2tk_base_set_mouse_pos(base, 30, 40);

		d2tk_base_get_mouse_pos(base, &x, &y);
		assert(x == 30);
		assert(y == 40);

		x = 0;
		y = 0;
		d2tk_base_get_mouse_pos(base, NULL, NULL);
		assert(x == 0);
		assert(y == 0);
	}

	{
		d2tk_base_add_mouse_scroll(base, 2, 3);
		//FIXME add getter
	}

	{
		d2tk_base_set_shift(base, false);
		d2tk_base_set_ctrl(base, false);
		d2tk_base_set_alt(base, false);

		assert(d2tk_base_get_shift(base) == false);
		assert(d2tk_base_get_ctrl(base) == false);
		assert(d2tk_base_get_alt(base) == false);

		d2tk_base_set_shift(base, true);
		d2tk_base_set_ctrl(base, true);
		d2tk_base_set_alt(base, true);

		assert(d2tk_base_get_shift(base) == true);
		assert(d2tk_base_get_ctrl(base) == true);
		assert(d2tk_base_get_alt(base) == true);
	}

	{
		d2tk_base_set_shift(base, false);
		d2tk_base_set_ctrl(base, false);
		d2tk_base_set_alt(base, false);

		assert(d2tk_base_get_mod(base) == false);

		d2tk_base_set_shift(base, true);
		d2tk_base_set_ctrl(base, false);
		d2tk_base_set_alt(base, false);

		assert(d2tk_base_get_mod(base) == true);

		d2tk_base_set_shift(base, false);
		d2tk_base_set_ctrl(base, true);
		d2tk_base_set_alt(base, false);

		assert(d2tk_base_get_mod(base) == true);

		d2tk_base_set_shift(base, false);
		d2tk_base_set_ctrl(base, false);
		d2tk_base_set_alt(base, true);

		assert(d2tk_base_get_mod(base) == true);
	}

	{
		d2tk_base_set_left(base, false);
		d2tk_base_set_right(base, false);
		d2tk_base_set_up(base, false);
		d2tk_base_set_down(base, false);

		assert(d2tk_base_get_left(base) == false);
		assert(d2tk_base_get_right(base) == false);
		assert(d2tk_base_get_up(base) == false);
		assert(d2tk_base_get_down(base) == false);

		d2tk_base_set_left(base, true);
		d2tk_base_set_right(base, true);
		d2tk_base_set_up(base, true);
		d2tk_base_set_down(base, true);

		assert(d2tk_base_get_left(base) == true);
		assert(d2tk_base_get_right(base) == true);
		assert(d2tk_base_get_up(base) == true);
		assert(d2tk_base_get_down(base) == true);
	}

	d2tk_base_free(base);
}

static void
_test_table()
{
#define N 12
#define M 8
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	D2TK_BASE_TABLE(&rect, N, M, tab)
	{
		const d2tk_rect_t *bnd = d2tk_table_get_rect(tab);
		const unsigned k = d2tk_table_get_index(tab);
		const unsigned x = d2tk_table_get_index_x(tab);
		const unsigned y = d2tk_table_get_index_y(tab);

		assert(bnd);
		assert(bnd->x == rect.w / N * (d2tk_coord_t)x);
		assert(bnd->y == rect.h / M * (d2tk_coord_t)y);
		assert(bnd->w == rect.w / N);
		assert(bnd->h == rect.h / M);
		assert(y*N + x == k);
	}

	d2tk_base_free(base);
#undef M
#undef N
}

static void
_test_frame()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	D2TK_BASE_FRAME(base, &rect, -1, "label", frm)
	{
		const d2tk_rect_t *bnd = d2tk_frame_get_rect(frm);

		assert(bnd);
		assert(bnd->x > rect.x);
		assert(bnd->y > rect.y);
		assert(bnd->w < rect.w);
		assert(bnd->h < rect.h);
	}

	d2tk_base_free(base);
}

static void
_test_layout_relative_x()
{
#define N 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	static const d2tk_coord_t frac [N] = {
		1, 2, 4, 0
	};
	unsigned i = 0;
	D2TK_BASE_LAYOUT(&rect, N, frac, D2TK_FLAG_LAYOUT_X | D2TK_FLAG_LAYOUT_REL, lay)
	{
		const d2tk_rect_t *bnd = d2tk_layout_get_rect(lay);
		const unsigned k = d2tk_layout_get_index(lay);

		assert(k == i++);
		assert(bnd);
		//FIXME bnd->x/y/w/h
	}

	d2tk_base_free(base);
#undef N
}

static void
_test_layout_relative_y()
{
#define N 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	static const d2tk_coord_t frac [N] = {
		1, 2, 4, 0
	};
	unsigned i = 0;
	D2TK_BASE_LAYOUT(&rect, N, frac, D2TK_FLAG_LAYOUT_Y | D2TK_FLAG_LAYOUT_REL, lay)
	{
		const d2tk_rect_t *bnd = d2tk_layout_get_rect(lay);
		const unsigned k = d2tk_layout_get_index(lay);

		assert(k == i++);
		assert(bnd);
		//FIXME bnd->x/y/w/h
	}

	d2tk_base_free(base);
#undef N
}

static void
_test_layout_absolute_x()
{
#define N 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const d2tk_coord_t frac [N] = {
		DIM_W/4, 0, 0, 0
	};
	unsigned i = 0;
	D2TK_BASE_LAYOUT(&rect, N, frac, D2TK_FLAG_LAYOUT_X | D2TK_FLAG_LAYOUT_ABS, lay)
	{
		const d2tk_rect_t *bnd = d2tk_layout_get_rect(lay);
		const unsigned k = d2tk_layout_get_index(lay);

		assert(k == i++);
		assert(bnd);
		//FIXME bnd->x/y/w/h
	}

	d2tk_base_free(base);
#undef N
}

static void
_test_layout_absolute_y()
{
#define N 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const d2tk_coord_t frac [N] = {
		DIM_W/4, 0, 0, 0
	};
	unsigned i = 0;
	D2TK_BASE_LAYOUT(&rect, N, frac, D2TK_FLAG_LAYOUT_Y | D2TK_FLAG_LAYOUT_ABS, lay)
	{
		const d2tk_rect_t *bnd = d2tk_layout_get_rect(lay);
		const unsigned k = d2tk_layout_get_index(lay);

		assert(k == i++);
		assert(bnd);
		//FIXME bnd->x/y/w/h
	}

	d2tk_base_free(base);
#undef N
}

static void
_test_clip()
{
	{
		const int32_t imin = 2;
		const int32_t imax = 4;

		int32_t ival = 1;
		d2tk_clip_int32(imin, &ival, imax);
		assert(ival == imin);

		ival = 2;
		d2tk_clip_int32(imin, &ival, imax);
		assert(ival == imin);

		ival = 3;
		d2tk_clip_int32(imin, &ival, imax);
		assert(ival == 3);

		ival = 4;
		d2tk_clip_int32(imin, &ival, imax);
		assert(ival == imax);

		ival = 5;
		d2tk_clip_int32(imin, &ival, imax);
		assert(ival == imax);
	}

	{
		const int64_t imin = 2;
		const int64_t imax = 4;

		int64_t ival = 1;
		d2tk_clip_int64(imin, &ival, imax);
		assert(ival == imin);

		ival = 2;
		d2tk_clip_int64(imin, &ival, imax);
		assert(ival == imin);

		ival = 3;
		d2tk_clip_int64(imin, &ival, imax);
		assert(ival == 3);

		ival = 4;
		d2tk_clip_int64(imin, &ival, imax);
		assert(ival == imax);

		ival = 5;
		d2tk_clip_int64(imin, &ival, imax);
		assert(ival == imax);
	}

	{
		const float imin = 2.f;
		const float imax = 4.f;

		float ival = 1.f;
		d2tk_clip_float(imin, &ival, imax);
		assert(ival == imin);

		ival = 2.f;
		d2tk_clip_float(imin, &ival, imax);
		assert(ival == imin);

		ival = 3.f;
		d2tk_clip_float(imin, &ival, imax);
		assert(ival == 3);

		ival = 4.f;
		d2tk_clip_float(imin, &ival, imax);
		assert(ival == imax);

		ival = 5.f;
		d2tk_clip_float(imin, &ival, imax);
		assert(ival == imax);
	}

	{
		const double imin = 2.0;
		const double imax = 4.0;

		double ival = 1.0;
		d2tk_clip_double(imin, &ival, imax);
		assert(ival == imin);

		ival = 2.0;
		d2tk_clip_double(imin, &ival, imax);
		assert(ival == imin);

		ival = 3.0;
		d2tk_clip_double(imin, &ival, imax);
		assert(ival == 3);

		ival = 4.0;
		d2tk_clip_double(imin, &ival, imax);
		assert(ival == imax);

		ival = 5.0;
		d2tk_clip_double(imin, &ival, imax);
		assert(ival == imax);
	}
}

static void
_test_state()
{
	assert(d2tk_state_is_down(D2TK_STATE_DOWN));
	assert(!d2tk_state_is_down(D2TK_STATE_NONE));

	assert(d2tk_state_is_up(D2TK_STATE_UP));
	assert(!d2tk_state_is_up(D2TK_STATE_NONE));

	assert(d2tk_state_is_motion(D2TK_STATE_MOTION));
	assert(!d2tk_state_is_motion(D2TK_STATE_NONE));

	assert(d2tk_state_is_scroll_up(D2TK_STATE_SCROLL_UP));
	assert(!d2tk_state_is_scroll_up(D2TK_STATE_NONE));

	assert(d2tk_state_is_scroll_down(D2TK_STATE_SCROLL_DOWN));
	assert(!d2tk_state_is_scroll_down(D2TK_STATE_NONE));

	assert(d2tk_state_is_scroll_left(D2TK_STATE_SCROLL_LEFT));
	assert(!d2tk_state_is_scroll_left(D2TK_STATE_NONE));

	assert(d2tk_state_is_scroll_right(D2TK_STATE_SCROLL_RIGHT));
	assert(!d2tk_state_is_scroll_right(D2TK_STATE_NONE));

	assert(d2tk_state_is_active(D2TK_STATE_ACTIVE));
	assert(!d2tk_state_is_active(D2TK_STATE_NONE));

	assert(d2tk_state_is_hot(D2TK_STATE_HOT));
	assert(!d2tk_state_is_hot(D2TK_STATE_NONE));

	assert(d2tk_state_is_focused(D2TK_STATE_FOCUS));
	assert(!d2tk_state_is_focused(D2TK_STATE_NONE));

	assert(d2tk_state_is_focus_in(D2TK_STATE_FOCUS_IN));
	assert(!d2tk_state_is_focus_in(D2TK_STATE_NONE));

	assert(d2tk_state_is_focus_out(D2TK_STATE_FOCUS_OUT));
	assert(!d2tk_state_is_focus_out(D2TK_STATE_NONE));

	assert(d2tk_state_is_changed(D2TK_STATE_CHANGED));
	assert(!d2tk_state_is_changed(D2TK_STATE_NONE));

	assert(d2tk_state_is_enter(D2TK_STATE_ENTER));
	assert(!d2tk_state_is_enter(D2TK_STATE_NONE));

	assert(d2tk_state_is_over(D2TK_STATE_OVER));
	assert(!d2tk_state_is_over(D2TK_STATE_NONE));
}

static void
_test_hit()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	const d2tk_rect_t rect = D2TK_RECT(10, 10, 10, 10);

	d2tk_base_set_mouse_pos(base, 0, 0);
	assert(!d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 0, 10);
	assert(!d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 10, 0);
	assert(!d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 10, 10);
	assert(d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 15, 15);
	assert(d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 10, 20);
	assert(!d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 20, 10);
	assert(!d2tk_base_is_hit(base, &rect));

	d2tk_base_set_mouse_pos(base, 20, 20);
	assert(!d2tk_base_is_hit(base, &rect));

	d2tk_base_free(base);
}

static void
_test_default_style()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	assert(base);

	const d2tk_style_t *default_style = d2tk_base_get_default_style();
	const d2tk_style_t *style = d2tk_base_get_style(base);
	const d2tk_style_t custom_style = *default_style;

	assert(default_style);
	assert(style);
	assert(default_style == style);

	d2tk_base_set_style(base, &custom_style);
	style = d2tk_base_get_style(base);

	assert(style);
	assert(style == &custom_style);

	d2tk_base_set_default_style(base);
	style = d2tk_base_get_style(base);

	assert(style);
	assert(style == default_style);

	d2tk_base_free(base);
}

static void
_test_scrollbar_x()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

#define H 128
#define h 32
	D2TK_BASE_SCROLLBAR(base, &rect, D2TK_ID, D2TK_FLAG_SCROLL_X, H, 0, h, 0, scroll)
	{
		const d2tk_rect_t *bnd = d2tk_scrollbar_get_rect(scroll);
		const float xo = d2tk_scrollbar_get_offset_x(scroll);
		const float yo = d2tk_scrollbar_get_offset_y(scroll);

		assert(bnd);
		assert(xo == 0);
		assert(yo == 0);
	}
	//FIXME test scrolling
#undef h
#undef H

	d2tk_base_free(base);
}

static void
_test_scrollbar_y()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

#define V 128
#define v 32
	D2TK_BASE_SCROLLBAR(base, &rect, D2TK_ID, D2TK_FLAG_SCROLL_Y, 0, V, 0, v, scroll)
	{
		const d2tk_rect_t *bnd = d2tk_scrollbar_get_rect(scroll);
		const float xo = d2tk_scrollbar_get_offset_x(scroll);
		const float yo = d2tk_scrollbar_get_offset_y(scroll);

		assert(bnd);
		assert(xo == 0);
		assert(yo == 0);
	}
	//FIXME test scrolling
#undef v
#undef V

	d2tk_base_free(base);
}

static void
_test_pane_x()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

#define fmin 0.25f
#define fmax 0.75f
#define fstep 0.25f
	D2TK_BASE_PANE(base, &rect, D2TK_ID, D2TK_FLAG_PANE_X, fmin, fmax, fstep, pane)
	{
		const d2tk_rect_t *bnd = d2tk_pane_get_rect(pane);
		const unsigned k = d2tk_pane_get_index(pane);
		const float f = d2tk_pane_get_fraction(pane);

		assert(bnd);
		switch(k)
		{
			case 0:
			{
				assert(f == fmin);
			} break;
			case 1:
			{
				assert(f == fmin);
			} break;
		}
	}
	//FIXME test panning
#undef fmin
#undef fmax
#undef fstep

	d2tk_base_free(base);
}

static void
_test_pane_y()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

#define fmin 0.25f
#define fmax 0.75f
#define fstep 0.25f
	D2TK_BASE_PANE(base, &rect, D2TK_ID, D2TK_FLAG_PANE_Y, fmin, fmax, fstep, pane)
	{
		const d2tk_rect_t *bnd = d2tk_pane_get_rect(pane);
		const unsigned k = d2tk_pane_get_index(pane);
		const float f = d2tk_pane_get_fraction(pane);

		assert(bnd);
		switch(k)
		{
			case 0:
			{
				assert(f == fmin);
			} break;
			case 1:
			{
				assert(f == fmin);
			} break;
		}
	}
	//FIXME test panning
#undef fmin
#undef fmax
#undef fstep

	d2tk_base_free(base);
}

static void
_test_cursor()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	d2tk_base_cursor(base, &rect);

	d2tk_base_free(base);
}

static void
_test_button_label_image()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const d2tk_state_t state = d2tk_base_button_label_image(base, D2TK_ID,
		-1, "label", D2TK_ALIGN_CENTERED, -1, "image.png", &rect);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		 |D2TK_STATE_OVER) );

	d2tk_base_free(base);
}

static void
_test_button_label()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const d2tk_state_t state = d2tk_base_button_label(base, D2TK_ID,
		-1, "label", D2TK_ALIGN_CENTERED, &rect);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );

	d2tk_base_free(base);
}

static void
_test_button_image()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const d2tk_state_t state = d2tk_base_button_image(base, D2TK_ID,
		-1, "image.png", &rect);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );

	d2tk_base_free(base);
}

static void
_test_button()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const d2tk_state_t state = d2tk_base_button(base, D2TK_ID, &rect);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );

	d2tk_base_free(base);
}

static void
_test_toggle_label()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	bool val = false;
	const d2tk_state_t state = d2tk_base_toggle_label(base, D2TK_ID,
		-1, "label", D2TK_ALIGN_CENTERED, &rect, &val);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == false);

	d2tk_base_free(base);
}

static void
_test_toggle()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	bool val = false;
	d2tk_base_set_mouse_l(base, true);
	const d2tk_state_t state = d2tk_base_toggle(base, D2TK_ID, &rect, &val);
	assert(state == (D2TK_STATE_DOWN | D2TK_STATE_ACTIVE | D2TK_STATE_HOT
		| D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN | D2TK_STATE_CHANGED
		| D2TK_STATE_OVER) );
	assert(val == true);

	d2tk_base_free(base);
}

static void
_test_image()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	d2tk_base_image(base, -1, "image.png", &rect, D2TK_ALIGN_CENTERED);

	d2tk_base_free(base);
}

static void
_test_bitmap()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const uint32_t bmp [4] = {
		0xff0000ff, 0x00ff001f,
		0x0000ff7f, 0xffff003f
	};
	const uint64_t rev = 0;

	d2tk_base_bitmap(base, 2, 2, 2*sizeof(uint32_t), bmp, rev, &rect,
		D2TK_ALIGN_CENTERED);

	d2tk_base_free(base);
}

static const uint32_t custom_data;

static void
_custom(void *ctx, uint32_t size, const void *data)
{
	assert(ctx == NULL);
	assert(size == sizeof(custom_data));
	assert(data == &custom_data);
}

static void
_test_custom()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	d2tk_base_custom(base, sizeof(custom_data), &custom_data, &rect, _custom);

	d2tk_base_free(base);
}

static void
_test_meter()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const int32_t val = -32;
	const d2tk_state_t state = d2tk_base_meter(base, D2TK_ID, &rect, &val);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );

	d2tk_base_free(base);
}

static void
_test_combo()
{
#define nitms 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const char *itms [nitms] = {
		"1", "2", "3", "4"
	};
	int32_t val = 0;
	const d2tk_state_t state = d2tk_base_combo(base, D2TK_ID, nitms, itms, &rect, &val);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 0);
	//FIXME test toggling

	d2tk_base_free(base);
#undef ntims
}

static void
_test_combo_scroll_up()
{
#define nitms 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	d2tk_base_add_mouse_scroll(base, 0.f, 1.f);

	const char *itms [nitms] = {
		"1", "2", "3", "4"
	};
	int32_t val = 0;
	const d2tk_state_t state = d2tk_base_combo(base, D2TK_ID, nitms, itms, &rect, &val);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		|D2TK_STATE_SCROLL_UP | D2TK_STATE_CHANGED | D2TK_STATE_OVER) );
	assert(val == 1);
	//FIXME test toggling

	d2tk_base_free(base);
#undef ntims
}

static void
_test_combo_scroll_down()
{
#define nitms 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	d2tk_base_add_mouse_scroll(base, 0.f, -1.f);

	const char *itms [nitms] = {
		"1", "2", "3", "4"
	};
	int32_t val = 0;
	const d2tk_state_t state = d2tk_base_combo(base, D2TK_ID, nitms, itms, &rect, &val);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_SCROLL_DOWN | D2TK_STATE_OVER) );
	assert(val == 0);
	//FIXME test toggling

	d2tk_base_free(base);
#undef ntims
}

static void
_test_text_field()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	d2tk_base_append_char(base, 'h');
	d2tk_base_append_char(base, 'e');
	d2tk_base_append_char(base, 'l');
	d2tk_base_append_char(base, 'l');
	d2tk_base_append_char(base, 'o');

	char txt [32] = "foo bar";
	const d2tk_state_t state = d2tk_base_text_field(base, D2TK_ID, &rect,
		sizeof(txt), txt, D2TK_ALIGN_LEFT, "hello");
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(!strcmp(txt, "foo bar"));

	d2tk_base_free(base);
}

static void
_test_label()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	const char *lbl= "label";
	const d2tk_state_t state = d2tk_base_label(base, -1, lbl, 0.8f, &rect,
		D2TK_ALIGN_LEFT);
	assert(state == D2TK_STATE_NONE);

	d2tk_base_free(base);
}

static void
_test_dial_bool()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	bool val = false;
	const d2tk_state_t state = d2tk_base_dial_bool(base, D2TK_ID, &rect, &val);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == false);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_dial_int32()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	int32_t val = 12;
	const d2tk_state_t state = d2tk_base_dial_int32(base, D2TK_ID, &rect,
		0, &val, 20);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 12);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_dial_int64()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	int64_t val = 12;
	const d2tk_state_t state = d2tk_base_dial_int64(base, D2TK_ID, &rect,
		0, &val, 20);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 12);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_dial_float()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	float val = 12.f;
	const d2tk_state_t state = d2tk_base_dial_float(base, D2TK_ID, &rect,
		0.f, &val, 20.f);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 12.f);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_dial_double()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	double val = 12.0;
	const d2tk_state_t state = d2tk_base_dial_double(base, D2TK_ID, &rect,
		0.0, &val, 20.0);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 12.0);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_prop_int32()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	int32_t val = 12;
	const d2tk_state_t state = d2tk_base_prop_int32(base, D2TK_ID, &rect,
		0, &val, 20);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 12);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_prop_float()
{
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	float val = 12.f;
	const d2tk_state_t state = d2tk_base_prop_float(base, D2TK_ID, &rect,
		0.f, &val, 20.f);
	assert(state == (D2TK_STATE_HOT | D2TK_STATE_FOCUS | D2TK_STATE_FOCUS_IN
		| D2TK_STATE_OVER) );
	assert(val == 12.f);
	//FIXME toggling

	d2tk_base_free(base);
}

static void
_test_flowmatrix()
{
#define N 4
	d2tk_mock_ctx_t ctx = {
		.check = NULL
	};
	static d2tk_pos_t pos_nodes [N];
	static d2tk_pos_t pos_arcs [N][N];

	d2tk_base_t *base = d2tk_base_new(&d2tk_mock_driver_lazy, &ctx);
	const d2tk_rect_t rect = D2TK_RECT(0, 0, DIM_W, DIM_H);
	assert(base);

	D2TK_BASE_FLOWMATRIX(base, &rect, D2TK_ID, flowm)
	{
		// draw arcs
		for(unsigned i = 0; i < N; i++)
		{
			const unsigned nin = i + 1;

			d2tk_flowmatrix_set_src(flowm, D2TK_ID, &pos_nodes[i]);

			for(unsigned j = 0; j < N; j++)
			{
				const unsigned nout = j + 1;
				d2tk_state_t state = D2TK_STATE_NONE;

				d2tk_flowmatrix_set_dst(flowm, D2TK_ID, &pos_nodes[j]);

				D2TK_BASE_FLOWMATRIX_ARC(base, flowm, nin, nout, &pos_nodes[i],
					&pos_nodes[j], &pos_arcs[i][j], arc, &state)
				{
					const d2tk_rect_t *bnd = d2tk_flowmatrix_arc_get_rect(arc);
					const unsigned k = d2tk_flowmatrix_arc_get_index(arc);
					const unsigned x = d2tk_flowmatrix_arc_get_index_x(arc);
					const unsigned y = d2tk_flowmatrix_arc_get_index_y(arc);

					assert(pos_nodes[i].x == 0);
					assert(pos_nodes[i].y == 0);
					assert(pos_nodes[j].x == 0);
					assert(pos_nodes[j].y == 0);
					assert(pos_arcs[i][j].x == 0);
					assert(pos_arcs[i][j].y == 0);
					assert(bnd); //FIXME check x,y,w,h
					(void)k; //FIXME
					(void)x; //FIXME
					(void)y; //FIXME
				}

				assert(state == D2TK_STATE_NONE);
			}
		}

		// draw nodes
		for(int i = 0; i < N; i++)
		{
			d2tk_state_t state = D2TK_STATE_NONE;

			D2TK_BASE_FLOWMATRIX_NODE(base, flowm, &pos_nodes[i], node, &state)
			{
				const d2tk_rect_t *bnd = d2tk_flowmatrix_node_get_rect(node);

				assert(pos_nodes[i].x == 150 + 150*i);
				assert(pos_nodes[i].y == 25 + 25*i);
				assert(bnd); //FIXME check x,y,w,h
			}

			assert(state == D2TK_STATE_NONE);
		}
	}

	d2tk_base_free(base);
#undef N
}

int
main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	_test_hot();
	_test_mouse_fwd_focus();
	_test_mouse_bwd_focus();
	_test_key_fwd_focus();
	_test_key_bwd_focus();
	_test_get_set();
	_test_table();
	_test_frame();
	_test_layout_relative_x();
	_test_layout_relative_y();
	_test_layout_absolute_x();
	_test_layout_absolute_y();
	_test_clip();
	_test_state();
	_test_hit();
	_test_default_style();
	_test_scrollbar_x();
	_test_scrollbar_y();
	_test_pane_x();
	_test_pane_y();
	_test_cursor();
	_test_button_label_image();
	_test_button_label();
	_test_button_image();
	_test_button();
	_test_toggle_label();
	_test_toggle();
	_test_image();
	_test_bitmap();
	_test_custom();
	_test_meter();
	_test_combo();
	_test_combo_scroll_up();
	_test_combo_scroll_down();
	_test_text_field();
	_test_label();
	_test_dial_bool();
	_test_dial_int32();
	_test_dial_int64();
	_test_dial_float();
	_test_dial_double();
	_test_prop_int32();
	_test_prop_float();
	_test_flowmatrix();

	return EXIT_SUCCESS;
}
