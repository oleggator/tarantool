-- Test starts server with different count of iproto threads, calculates
-- performance and latency of simple iproto calls and saved result in the
-- table format in the file with a name perf.txt.
env = require('test_run')
clock = require('clock')
net_box = require('net.box')
fiber = require('fiber')
test_run = env.new()
test_run:cmd("create server perf with script='perf/iproto.lua'")

test_run:cmd("setopt delimiter ';'")
function latency(bench_count, server_addr, result, threads_count)
     local time_before = clock.monotonic64()
     for _ = 1, bench_count do
        local conn = net_box.new(server_addr)
        pcall(conn.call, conn, 'ping')
     end
     result[threads_count].latency =
         tonumber(clock.monotonic64() - time_before) / (1.0e6 * bench_count)
end;
function benchmark(bench_count, server_addr, result, threads_count)
    local fibers_count = 100
    local fibers = {}
    local time_before = clock.monotonic64()
    for i = 1, fibers_count do
        fibers[i] = fiber.new(function()
            local conn = net_box.new(server_addr)
            for _ = 1, bench_count do
                pcall(conn.call, conn, 'ping')
            end
        end)
        fibers[i]:set_joinable(true)
    end
    for _, f in ipairs(fibers) do
        f:join()
    end
    result[threads_count].benchmark =
        (1.0e9 * fibers_count * bench_count) /
        (1.0e6 * tonumber(clock.monotonic64() - time_before))
end;
function benchmark_save_result(result)
    local one_thread_perf = result[1].benchmark
    local file = io.open("perf.txt", "w+")
    if file == nil then
        return
    end
    io.output(file)
    io.write(" ________________________________________________________________\n")
    io.write("| iproto threads count |     latency, ms     | performance, Mrps |\n")
    io.write("|----------------------------------------------------------------|\n")
    for threads_count, val in pairs(result) do
        io.write(string.format("|          %2d          ", threads_count))
        io.write(string.format("|        %0.3f        ", val.latency))
        io.write(string.format("|      %6.4f       |\n", val.benchmark))
    end
    io.write("|----------------------------------------------------------------|\n")
    io.close(file)
end;
test_run:cmd("setopt delimiter ''");

threads_count = 1
threads_count_max = 16
bench_count = 1000
result = {}
test_run:cmd("setopt delimiter ';'")
while threads_count <= threads_count_max do
    test_run:cmd(string.format('start server perf with args=\"%d\"', threads_count))
    server_addr = test_run:cmd("eval perf 'return box.cfg.listen'")[1]
    result[threads_count] = {}
    latency(bench_count, server_addr, result, threads_count)
    benchmark(bench_count, server_addr, result, threads_count)
    threads_count = threads_count * 2
    test_run:cmd("stop server perf")
end;
test_run:cmd("setopt delimiter ''");

benchmark_save_result(result)

test_run:cmd("cleanup server perf")
test_run:cmd("delete server perf")
