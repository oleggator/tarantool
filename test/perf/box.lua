#!/usr/bin/env tarantool
local os = require('os')

box.cfg{
    listen              = os.getenv("LISTEN"),
    wal_mode            = 'none',
    pid_file            = "tarantool.pid",
    memtx_allocator     = os.getenv("MEMTX_ALLOCATOR")
}

require('console').listen(os.getenv('ADMIN'))
