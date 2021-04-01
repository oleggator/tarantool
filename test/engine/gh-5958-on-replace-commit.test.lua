-- gh-5958 Introduce on_replace_commit trigger

test_run = require('test_run')
inspector = test_run.new()
engine = inspector:get_cfg('engine')

s = box.schema.space.create('test', {engine = engine})
i = s:create_index('pk',  {type='tree', parts={{1, 'uint'}}})

result = ''
function r(t) result = result .. tostring(t) .. ' ' end

not not s:on_replace(function(old, new) r('on_replace') r(old) r(new) end)
not not s:on_replace_commit(function(old, new) r('on_replace_commit') r(old) r(new) end)

function test(f) result = '' f() return result end

test(function() s:replace{1, 1} end)
test(function() s:replace{1, 2} end)
test(function() box.begin() s:replace{1, 3} box.commit() end)
test(function() box.begin() s:replace{1, 4} box.rollback() end)
test(function() box.begin() s:replace{2, 1} box.commit() end)
test(function() box.begin() s:replace{3, 1} box.rollback() end)

s:drop()
