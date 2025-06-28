include(`general.m4')
[threadpool]
groups = THREADPOOL_GROUPS

[timer]
interval = TIMER_INTERVAL
poolsize = TIMER_POOL_SIZE

[reactor]
demultiplexer = REACTOR_DEMULTIPLEXER
use_timer_thread = true
timeout = 1000

[log]
loglevel = 0
asynclog = false
pattern = LOG_PATTERN

[logserver]
socktype = tcp
version = 4
address = LOCAL_IP
port = 8880
read_buffer_size = READ_BUFFER_SIZE
write_buffer_size = WRITE_BUFFER_SIZE

[server]
socktype = tcp
version = 4
address = LOCAL_IP
port = 8888
read_buffer_size = READ_BUFFER_SIZE
write_buffer_size = WRITE_BUFFER_SIZE

[httpclient]
socktype = tcp
version = 4
uri = http://LOCAL_IP:443
read_buffer_size = READ_BUFFER_SIZE
write_buffer_size = WRITE_BUFFER_SIZE
ssl_enabled = false