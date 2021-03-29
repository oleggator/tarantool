env = require('test_run')
net_box = require('net.box')
fiber = require('fiber')
test_run = env.new()
test_run:cmd("create server test with script='box/gh-5645-several-iproto-threads.lua'")

test_run:cmd("setopt delimiter ';'")
function ddos(server_addr, fibers_count)
    local fibers = {}
    for i = 1, fibers_count do
        fibers[i] = fiber.new(function()
            local conn = net_box.new(server_addr)
            for _ = 1, 100 do
                pcall(conn.call, conn, 'ping')
            end
        end)
        fibers[i]:set_joinable(true)
    end
    for _, f in ipairs(fibers) do
        f:join()
    end
end;
test_run:cmd("setopt delimiter ''");

time_max = 20
time_before = fiber.clock()
fibers_count = 256
test_run:cmd('start server test with args="1"')
server_addr = test_run:cmd("eval test 'return box.cfg.listen'")[1]
test_run:cmd("setopt delimiter ';'")
while fiber.clock() - time_before < time_max do
    ddos(server_addr, fibers_count)
    if test_run:grep_log("test", "stopping input on connection") ~= nil then
        break
    end
    fibers_count = fibers_count * 4
end;
test_run:cmd("setopt delimiter ''");
test_run:cmd("stop server test")

test_run:cmd('start server test with args="8"')
server_addr = test_run:cmd("eval test 'return box.cfg.listen'")[1]
fibers_count = fibers_count * 2
assert(test_run:grep_log("test", "stopping input on connection") == nil)


test_run:cmd("stop server test")

test_run:cmd("cleanup server test")
test_run:cmd("delete server test")
