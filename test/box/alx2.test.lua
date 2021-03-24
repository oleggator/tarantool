--box.cfg{}
test_run = require('test_run')
inspector = test_run.new()

space = box.schema.space.create('test', {engine = 'memtx'})
space

index = space:create_index('primary', { type = 'tree', parts = {1, 'unsigned'} })
index

space:insert({1,10,2})
for key = 2,9 do space:insert({key,key-1,key+1}) end
space:insert({10,9,1})

t = {} for state, v in index:pairs({}, {iterator = 'ALL'}) do table.insert(t, v) end
t

space:select(1, {iterator = 'ALL'})
space:delete(4)
space:update(3, {{3,2,5}})
space:update(5, {{5,3,6}})
space:select(1, {iterator = 'ALL'})
space:insert({4,3,5})
space:update(3, {{3,2,4}})
space:update(5, {{5,4,6}})
space:select(1, {iterator = 'ALL'})

space:drop()
