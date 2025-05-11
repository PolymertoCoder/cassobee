# CassoBee Project

## Overview
CassoBee is a high-performance C++ network application framework with built-in HTTP server capabilities. It provides core components for building scalable network services and web applications.

Key Features:
- Event-driven architecture based on Reactor pattern
- Full HTTP/1.1 protocol support
- WebSocket support
- Asynchronous I/O with thread pool
- Built-in logging system
- MySQL database connectivity
- Command line interface

## Module Structure

### Core Modules (bee)

#### Common
- Basic utilities and helper functions
- String operations (trim, split, etc.)
- System utilities (process/thread management)
- Time measurement macros

#### IO
- Reactor pattern implementation
- Event demultiplexer
- Session management
- Protocol abstraction layer
- SSL/TLS support

#### HTTP
- HTTP/1.1 protocol implementation
- Request/response processing
- Header and cookie management
- URI parsing
- WebSocket support
- Servlet container

#### Database
- MySQL client wrapper
- Connection pooling
- Query execution

#### Logging
- Log levels (DEBUG, INFO, WARN, ERROR)
- Multiple appenders (file, console)
- Formatted output
- Asynchronous logging

#### Security
- Cryptography utilities
- SSL/TLS support
- Based on Crypto++ library

#### CLI
- Interactive command line interface
- Command history
- Tab completion
- Based on readline library

### Application Modules

#### Casso (Main Application)
- Application entry point
- Service initialization
- Configuration management

#### LogServer
- Centralized logging service
- Log collection and storage
- Remote logging capabilities

#### LogClient
- Logging client library
- Remote logging support
- Log forwarding

## Dependencies
- Crypto++ (security)
- MySQL Connector/C++ (database)
- readline (CLI) 
- tinyxml2 (configuration)

## Build & Installation

### Prerequisites
- CMake 3.8+
- Clang/GCC with C++23 support
- Development packages for dependencies

### Build Steps
```bash
# Clone the repository
git clone https://gitee.com/qinchao11/cassobee
cd cassobee

# Configure build
cd tools
./install.sh
./build_thirdparty.sh
cd ..
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
mk

# Install (optional)
sudo make install
```

### Build Options
| Option | Description | Default |
|--------|-------------|---------|
| CASSOBEE_BENCHMARK_ENABLE | Enable benchmark tests | OFF |
| CASSOBEE_IWYU_ENABLE | Enable include-what-you-use | OFF | 
| CASSOBEE_CLANG_TIDY_ENABLE | Enable clang-tidy checks | OFF |

## Configuration
Main configuration files are located in `config/` directory:
- `cassobee.conf`: Main application config
- `logserver.conf`: Log server config
- `tabledef.xml`: Database table definitions

## Basic Usage

### Logging Example
```cpp
#include "bee/log/log.h"

DEBUGLOG("Application started, now:%lld.", systemtime::get_time());
ERRORLOGF("Error occurred: {}.", error_msg);
```

### Reactor (source/bee/io/reactor.h)
```cpp
// Initialize and run reactor
auto* looper = bee::reactor::get_instance();
looper->init();
looper->run()
```

### TimeWheel (source/bee/common/timewheel.h)
```cpp
// Add timer (single shot)
bee::add_timer(1000, [](){ 
    std::cout << "Timer fired after 1 second" << std::endl;
    return false; // return true to repeat
});

// Add periodic timer
bee::add_timer(true, 1000, -1, [](){
    std::cout << "Timer fired every 1 second" << std::endl;
    return true;
});
```

### HTTP Server (source/bee/http/)
```cpp
// Custom servlet example
class myservlet : public bee::servlet {
public:
    void handle(bee::httprequest& req, bee::httpresponse& resp) override {
        if (req.get_url() == "/hello") {
            resp.set_body("Hello World");
        }
    }
};

// Register and run server
auto* httpmanager = bee::httpsession_manager::get_instance();
httpmanager->init();
httpmanager->register_servlet("/hello", new myservlet());
server(httpmanager);
return 0;
```

### Database (source/bee/database/cmysql.h)
```cpp
// Transaction example
bee::cmysql mysql;
mysql.connect("localhost", "user", "pass", "db");

mysql.begin();
try {
    auto result = mysql.query("SELECT * FROM users");
    while (result.next()) {
        std::cout << result.get_string("username") << std::endl;
    }

    mysql.execute("INSERT INTO users VALUES(1, 'test')");
    mysql.execute("UPDATE stats SET count=count+1");
    mysql.commit();
} catch (...) {
    mysql.rollback();
}
```

### Logging (source/bee/log/)
```cpp
// Custom logger configuration
auto logger = bee::log::create_logger("mylogger");
logger->add_appender<bee::log::file_appender>("app.log");
logger->set_level(bee::log::level::debug);

// Usage
logger->debug("Debug message");
logger->error("Error: {}", errmsg);
```

## Advanced Topics

### Event Loop Architecture
![Reactor Pattern](docs/images/reactor.png)

### Performance Tuning
- Worker thread pool configuration
- IO buffer sizing
- Connection pooling settings

## Examples Directory
See [examples/](examples/) for:
- HTTP REST API server
- WebSocket chat server 
- Database ORM implementation
- Custom protocol implementation

## License
MIT License
