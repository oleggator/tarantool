/*
 * Copyright 2010-2021, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "box/lua/sysalloc.h"
#include "lua/utils.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "box/engine.h"
#include "box/memtx_engine.h"

static int
lbox_sysalloc_info(struct lua_State *L)
{
	struct memtx_engine *memtx;
	memtx = (struct memtx_engine *)engine_by_name("memtx");

	struct sys_stats totals;

	lua_newtable(L);
	sys_stats(&memtx->sys_alloc, &totals);

	double ratio;
	char ratio_buf[32];

	lua_pushstring(L, "bytes_used");
	luaL_pushuint64(L, totals.used);
	lua_settable(L, -3);

	/*
	 * This is pretty much the same as
	 * box.cfg.slab_alloc_arena, but in bytes
	 */
	lua_pushstring(L, "quota_size");
	luaL_pushuint64(L, quota_total(&memtx->quota));
	lua_settable(L, -3);

	/*
	 * How much quota has been booked - reflects the total
	 * size of slabs in various slab caches.
	 */
	lua_pushstring(L, "quota_used");
	luaL_pushuint64(L, quota_used(&memtx->quota));
	lua_settable(L, -3);

	/**
	 * This should be the same as arena_size/arena_used, however,
	 * don't trust totals in the most important monitoring
	 * factor, it's the quota that give you OOM error in the
	 * end of the day.
	 */
	ratio = 100 * ((double) quota_used(&memtx->quota) /
		 ((double) quota_total(&memtx->quota) + 0.0001));
	snprintf(ratio_buf, sizeof(ratio_buf), "%0.2lf%%", ratio);

	lua_pushstring(L, "quota_used_ratio");
	lua_pushstring(L, ratio_buf);
	lua_settable(L, -3);

	return 1;
}

/** Initialize box.sysalloc package. */
void
box_lua_sysalloc_init(struct lua_State *L)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "box");
	lua_pushstring(L, "sysalloc");
	lua_newtable(L);

	lua_pushstring(L, "info");
	lua_pushcfunction(L, lbox_sysalloc_info);
	lua_settable(L, -3);

	lua_settable(L, -3); /* box.sysalloc */

	lua_pop(L, 1); /* box. */
}
