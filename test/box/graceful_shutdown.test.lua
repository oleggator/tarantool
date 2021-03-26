net_box = require('net.box')
test_run = require('test_run').new()

s = box.schema.space.create('test')
box.schema.user.create('test1', { password = 'test1' })
box.schema.user.create('test2', { password = 'test2' })

box.schema.user.grant('test1', 'read,write,alter', 'space', '_session_settings')
box.schema.user.grant('test2', 'read,write,alter', 'space', '_session_settings')
c1 = net_box.connect('test1:test1@' .. box.cfg.listen)
c2 = net_box.connect('test2:test2@' .. box.cfg.listen)

ss1 = c1.space._session_settings
ss2 = c2.space._session_settings

ss1:get({'graceful_shutdown'})
ss2:get({'graceful_shutdown'})

ss1:update('graceful_shutdown', {{'=', 'value', true}})

ss1:get({'graceful_shutdown'})
ss2:get({'graceful_shutdown'})