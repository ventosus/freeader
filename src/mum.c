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

#include <stdarg.h>
#include <stdio.h>

#include <d2tk/hash.h>

#include "mum.h"

#define SEED 12345

__attribute__((always_inline))
static inline size_t
_len(const void *key, ssize_t len)
{
	return (len == -1) ? strlen((const char *)key) : (size_t)len;
}

__attribute__((always_inline))
static inline uint64_t
_d2tk_hash(uint64_t hash, const void *key, size_t len)
{
	return _mum_hash_aligned(hash + len, key, len);
}

D2TK_API uint64_t
d2tk_hash(const void *key, ssize_t len)
{
	len = _len(key, len);

	return mum_hash(key, len, SEED);
}

D2TK_API uint64_t
d2tk_hash_foreach(const void *key, ssize_t len, ...)
{
	va_list args;
	uint64_t hash = mum_hash_init(SEED);

	len = _len(key, len); //FIXME remove
	hash = _d2tk_hash(hash, key, len);

	va_start(args, len);

	while( (key = va_arg(args, const void *)) )
	{
		len = _len(key, va_arg(args, int)); //FIXME remove
		hash = _d2tk_hash(hash, key, len);
	}

	va_end(args);

  return mum_hash_finish(hash);
}

D2TK_API uint64_t
d2tk_hash_dict(const d2tk_hash_dict_t *dict)
{
	uint64_t hash = mum_hash_init(SEED);

	for( ; dict->key; dict++)
	{
		hash = _d2tk_hash(hash, dict->key, dict->len);
	}

	return mum_hash_finish(hash);
}
