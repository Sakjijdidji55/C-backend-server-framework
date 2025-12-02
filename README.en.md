# C++ Backend Server Framework

A lightweight, high-performance HTTP server framework developed with modern C++ (C++17), designed for rapid development of reliable and scalable backend services.

## Key Features

- **Lightweight HTTP Server**: Built on native sockets with zero external dependencies
- **Complete Routing System**: Supports route registration and handling for HTTP methods including GET, POST, PUT, DELETE
- **Efficient Thread Pool**: Provides concurrent task processing to optimize server performance
- **Powerful JSON Processing**: Built-in JSON parsing and generation with support for complex nested data structures
- **JWT Authentication**: Complete JWT token generation and verification functionality
- **Logging System**: Singleton-based logging implementation for efficient message recording
- **Cross-platform Compatibility**: Supports both Windows and Linux platforms

## System Requirements

- C++17 or higher compiler
- CMake 3.15 or higher
- **Windows Platform**: Visual Studio 2019+ or MinGW
- **Linux Platform**: GCC 8+ or Clang 7+

## Installation and Building

### Clone the Repository

```bash
git clone <repository-url>
cd C-backend-server-framework
```

### Building with CMake

#### Windows (Visual Studio)

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

#### Linux (GCC/Clang)

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

## Quick Start

```cpp
#include "include/Server.h"
#include "include/route.h"

int main() {
    // Create server instance, listening on port 8080
    Server app(8080);
    
    // Register route
    app.get("/", [](const Request& req, Response& res) {
        res.json(R"({"message":"Welcome to C++ Backend Server"})"; 
    });
    
    // Start server
    app.run();
    
    return 0;
}
```

## API Documentation

### Server Class

**Constructor**:
```cpp
Server(int port = 8080, bool printParams = false)
```

**Route Registration**:
```cpp
void get(const std::string& path, Handler handler);
void post(const std::string& path, Handler handler);
void put(const std::string& path, Handler handler);
void del(const std::string& path, Handler handler);
```

**Server Control**:
```cpp
void run();        // Start the server
void stop();       // Stop the server
static Server* getInstance();  // Get server instance
```

### Request Class

**Main Properties**:
- `method`: HTTP method (GET, POST, etc.)
- `path`: Request path
- `headers`: Request headers
- `body`: Request body
- `queryParams`: URL query parameters
- `jsonBody`: Parsed JSON request body

**Main Methods**:
- `query_param(const std::string& key)`: Get query parameter value
- `json_param(const std::string& key)`: Get JSON parameter value
- `isJson()`: Check if request body is JSON format
- `getJsonBody()`: Get parsed JSON object

### Response Class

**Main Methods**:
- `json(const std::string& jsonStr)`: Set JSON response
- `text(const std::string& textStr)`: Set plain text response
- `status(int code)`: Set HTTP status code
- `success()`: Set success response
- `error(int code, const std::string& message)`: Set error response

## Project Structure

```
├── include/             # Header files directory
│   ├── Handler.h        # Request handler header
│   ├── JsonValue.h      # JSON handling header
│   ├── Log.h            # Logging system header
│   ├── Server.h         # Server core header
│   ├── Threadpool.h     # Thread pool header
│   ├── jwt.h            # JWT authentication header
│   └── route.h          # Routing system header
├── src/                 # Source code directory
│   ├── JsonValue.cpp    # JSON handling implementation
│   ├── Log.cpp          # Logging system implementation
│   ├── Server.cpp       # Server core implementation
│   ├── Threadpool.cpp   # Thread pool implementation
│   ├── jwt.cpp          # JWT authentication implementation
│   └── route.cpp        # Routing system implementation
├── main.cpp             # Main program entry
└── CMakeLists.txt       # CMake build configuration
```

## License

[MIT License](LICENSE)

## Version History

- Initial Version: Basic HTTP server functionality, routing system, and JSON handling
- Latest Version: JWT authentication, improved logging, and optimized JSON handling