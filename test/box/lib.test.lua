--
-- gh-4642: New box.lib module to be able to
-- run C stored functions on read only nodes
-- without requirement to register them with
-- box.schema.func help.
--
build_path = os.getenv("BUILDDIR")
package.cpath = build_path..'/test/box/?.so;'..build_path..'/test/box/?.dylib;'..package.cpath

boxlib = require('box.lib')
fio = require('fio')

ext = (jit.os == "OSX" and "dylib" or "so")

cfunc_path = fio.pathjoin(build_path, "test/box/cfunc.") .. ext
cfunc1_path = fio.pathjoin(build_path, "test/box/cfunc1.") .. ext
cfunc2_path = fio.pathjoin(build_path, "test/box/cfunc2.") .. ext
cfunc3_path = fio.pathjoin(build_path, "test/box/cfunc3.") .. ext
cfunc4_path = fio.pathjoin(build_path, "test/box/cfunc4.") .. ext

_ = pcall(fio.unlink(cfunc_path))
fio.symlink(cfunc1_path, cfunc_path)

_, err = pcall(boxlib.load, 'non-such-module')
assert(err ~= nil)

-- All functions are sitting in cfunc.so.
old_module = boxlib.load('cfunc')
assert(old_module['debug_refs'] == 1)
old_module_copy = boxlib.load('cfunc')
assert(old_module['debug_refs'] == 2)
assert(old_module_copy['debug_refs'] == 2)
old_module_copy:unload()
assert(old_module['debug_refs'] == 1)
old_cfunc_nop = old_module:load('cfunc_nop')
old_cfunc_fetch_seq_evens = old_module:load('cfunc_fetch_seq_evens')
old_cfunc_multireturn = old_module:load('cfunc_multireturn')
old_cfunc_args = old_module:load('cfunc_args')
old_cfunc_sum = old_module:load('cfunc_sum')
assert(old_module['debug_refs'] == 6)

-- Test for error on nonexisting function.
_, err = pcall(old_module.load, old_module, 'no-such-func')
assert(err ~= nil)

-- Make sure they all are callable.
old_cfunc_nop()
old_cfunc_fetch_seq_evens()
old_cfunc_multireturn()
old_cfunc_args()
old_cfunc_sum()

-- Unload the module but keep old functions alive, so
-- they keep reference to NOP module internally
-- and still callable.
old_module:unload()
-- Test refs via function name.
assert(old_cfunc_nop['debug_module_refs'] == 5)
old_cfunc_nop()
old_cfunc_fetch_seq_evens()
old_cfunc_multireturn()
old_cfunc_args()
old_cfunc_sum()

-- The module is unloaded I should not be able
-- to load new shared library.
old_module:load('cfunc')
-- Neither I should be able to unload module twise.
old_module:unload()

-- Clean old functions.
old_cfunc_nop:unload()
old_cfunc_fetch_seq_evens:unload()
old_cfunc_multireturn:unload()
old_cfunc_args:unload()
assert(old_cfunc_sum['debug_module_refs'] == 1)
old_cfunc_sum:unload()

-- And reload old module again.
old_module = boxlib.load('cfunc')
old_module_ptr = old_module['debug_ptr']
assert(old_module['debug_refs'] == 1)

-- Overwrite module with new contents.
_ = pcall(fio.unlink(cfunc_path))
fio.symlink(cfunc2_path, cfunc_path)

-- Load new module, cache should be updated.
new_module = boxlib.load('cfunc')
new_module_ptr = new_module['debug_ptr']

-- Old and new module keep one reference with
-- different IDs.
assert(old_module['debug_refs'] == 1)
assert(old_module['debug_refs'] == new_module['debug_refs'])
assert(old_module_ptr ~= new_module_ptr)

-- All functions from old module should be loadable.
old_cfunc_nop = old_module:load('cfunc_nop')
old_cfunc_fetch_seq_evens = old_module:load('cfunc_fetch_seq_evens')
old_cfunc_multireturn = old_module:load('cfunc_multireturn')
old_cfunc_args = old_module:load('cfunc_args')
old_cfunc_sum = old_module:load('cfunc_sum')
assert(old_cfunc_nop['debug_module_ptr'] == old_module_ptr)
assert(old_cfunc_fetch_seq_evens['debug_module_ptr'] == old_module_ptr)
assert(old_cfunc_multireturn['debug_module_ptr'] == old_module_ptr)
assert(old_cfunc_args['debug_module_ptr'] == old_module_ptr)
assert(old_cfunc_sum['debug_module_ptr'] == old_module_ptr)
assert(old_module['debug_refs'] == 6)

-- Lookup for updated symbols.
new_cfunc_nop = new_module:load('cfunc_nop')
new_cfunc_fetch_seq_evens = new_module:load('cfunc_fetch_seq_evens')
new_cfunc_multireturn = new_module:load('cfunc_multireturn')
new_cfunc_args = new_module:load('cfunc_args')
new_cfunc_sum = new_module:load('cfunc_sum')
assert(new_cfunc_nop['debug_module_ptr'] == new_module_ptr)
assert(new_cfunc_fetch_seq_evens['debug_module_ptr'] == new_module_ptr)
assert(new_cfunc_multireturn['debug_module_ptr'] == new_module_ptr)
assert(new_cfunc_args['debug_module_ptr'] == new_module_ptr)
assert(new_cfunc_sum['debug_module_ptr'] == new_module_ptr)
assert(new_module['debug_refs'] == 6)

-- Call old functions.
old_cfunc_nop()
old_cfunc_fetch_seq_evens()
old_cfunc_multireturn()
old_cfunc_args()
old_cfunc_sum()

-- Call new functions.
new_cfunc_nop()
new_cfunc_multireturn()
new_cfunc_fetch_seq_evens({2,4,6})
new_cfunc_fetch_seq_evens({1,2,3})  -- error, odd numbers sequence
new_cfunc_args(1, "hello")
new_cfunc_sum(1) -- error, one arg passed
new_cfunc_sum(1,2)

-- Cleanup old module's functions.
old_cfunc_nop:unload()
old_cfunc_fetch_seq_evens:unload()
old_cfunc_multireturn:unload()
old_cfunc_args:unload()
old_cfunc_sum:unload()
old_module:unload()

-- Cleanup new module data.
new_cfunc_nop:unload()
new_cfunc_multireturn:unload()
new_cfunc_fetch_seq_evens:unload()
new_cfunc_args:unload()
new_cfunc_sum:unload()
new_module:unload()

-- Cleanup the generated symlink.
_ = pcall(fio.unlink(cfunc_path))

-- Test double hashing: create function
-- in box.schema.fun so that it should
-- appear in box.lib hash.
fio.symlink(cfunc3_path, cfunc_path)
box.schema.func.create('cfunc.cfunc_add', {language = "C"})
box.schema.user.grant('guest', 'execute', 'function', 'cfunc.cfunc_add')
box.func['cfunc.cfunc_add']:call({1,2})

old_module = boxlib.load('cfunc')
assert(old_module['debug_refs'] == 3) -- box.lib + 2 box.schema.func
old_func = old_module:load('cfunc_add')
assert(old_module['debug_refs'] == 4) -- plus function instance
old_func(1,2)

-- Now update on disk and reload the module.
_ = pcall(fio.unlink(cfunc_path))
fio.symlink(cfunc4_path, cfunc_path)

box.schema.func.reload("cfunc")
box.func['cfunc.cfunc_add']:call({1,2})

-- The box.lib instance should carry own
-- references to the module and old
-- function. And reloading must not
-- affect old functions. Thus one for
-- box.lib and one for box.lib function.
assert(old_module['debug_refs'] == 2)
old_func(1,2)
old_func:unload()
old_module:unload()

-- Same time the reload should update
-- low level module cache, thus two
-- for box and box function plus one
-- new box.lib.
new_module = boxlib.load('cfunc')
assert(new_module['debug_refs'] == 3)

-- Box function should carry own module.
box.func['cfunc.cfunc_add']:call({1,2})

-- Cleanup.
_ = pcall(fio.unlink(cfunc_path))
new_module:unload()
box.schema.func.drop('cfunc.cfunc_add')
