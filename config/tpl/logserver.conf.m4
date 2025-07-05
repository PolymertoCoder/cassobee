include(`general.m4')
[threadpool]
groups = (524288, 4)

[timer]
interval = TIMER_INTERVAL
poolsize = TIMER_POOL_SIZE

[reactor]
demultiplexer = REACTOR_DEMULTIPLEXER
use_timer_thread = true
timeout = 1000

[log]
loglevel = 0
dir = LOG_DIR
filename = trace
pattern = LOG_PATTERN
asynclog = false
interval = 5000
threshold = 4096

[influxlog]
dir = INFLUXLOG_DIR
filename = influxdb

[logclient]
socktype = tcp
version = 4
address = LOCAL_IP
port = 8880
read_buffer_size = READ_BUFFER_SIZE
write_buffer_size = WRITE_BUFFER_SIZE