include(`general.m4')
[threadpool]
groups = THREADPOOL_GROUPS
issteal = true

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

[mysql]
user = root
password = 123456
host = LOCAL_IP
port = 3306
db = cassobee
charset = utf8
minsize = 10
maxsize = 100
timeout = 1000
max_idle_time = 300

[client]
socktype = tcp
version = 4
address = LOCAL_IP
port = 8888
read_buffer_size = READ_BUFFER_SIZE
write_buffer_size = WRITE_BUFFER_SIZE

[httpserver]
socktype = tcp
version = 4
address = LOCAL_IP
port = 443
read_buffer_size = READ_BUFFER_SIZE
write_buffer_size = WRITE_BUFFER_SIZE
ssl_enabled = false
ssl_server = true
cert_file = config/ssl/cert.pem
key_file = config/ssl/key.pem
keepalive_timeout = 30000

[monitor]
exporter = influx
collectors = cpu, memory, disk, process, network, system
interval = 1000