# C++ Backend Server Framework

A lightweight, high-performance backend server framework written in modern C++ (C++17/20), designed for building scalable, reliable, and easy-to-maintain backend services.

## Features

- **Lightweight HTTP Server**: Implemented with native sockets, no external dependencies
- **Routing System**: Supports registration of routes for HTTP methods like GET, POST, PUT, DELETE
- **Thread Pool**: Efficient concurrency mechanism to improve server performance
- **JSON Support**: Built-in JSON parsing and generation functionality with nested data structure support
- **JWT Authentication**: Complete JWT token generation and validation functionality
- **Logging System**: Simple and easy-to-use logging functionality
- **Cross-Platform**: Supports both Windows and Linux platforms
- **Modern C++**: Makes full use of C++17/20 features for clean and readable code

## System Requirements

- C++17 or higher compiler
- CMake 3.15 or higher
- Windows: Visual Studio 2019+ or MinGW
- Linux: GCC 8+ or Clang 7+

## Installation and Building

### Clone the Repository

```bash
git clone <repository-url>
cd C-backend-server-framework
```

### Build with CMake

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

After building, the executable file will be located in the `build/bin` directory.

## Quick Start

Here is a simple example showing how to create a basic server:

```cpp
#include "include/Server.h"
#include "include/route.h"

int main() {
    // Create a server instance, listening on port 8080
    Server app(8080);
    
    // Register a route
    app.get("/", [](const Request& req, Response& res) {
        res.json(R"({"message":"Welcome to C++ Server"})"; 
    });
    
    // Start the server
    app.run();
    
    return 0;
}
```

## API Documentation

### Server Class

`Server` is the core class of the framework, responsible for creating and managing HTTP servers.

#### Constructor

```cpp
Server(int port = 8080)
```
- **Parameter**: `port` - Port number the server listens on, defaults to 8080

#### Route Registration Methods

```cpp
void get(const std::string& path, Handler handler);
void post(const std::string& path, Handler handler);
void put(const std::string& path, Handler handler);
void del(const std::string& path, Handler handler);
```
- **Parameters**:
  - `path` - Route path
  - `handler` - Callback function to handle requests, of type `std::function<void(const Request&, Response&)>`

#### Server Control Methods

```cpp
void run();        // Start the server
void stop();       // Stop the server
```

### Request Class

`Request` represents a client's HTTP request.

#### Member Variables

```cpp
std::string method;           // HTTP method (GET, POST, etc.)
std::string path;             // Request path
std::map<std::string, std::string> headers;  // Request headers
std::string body;             // Request body
std::map<std::string, std::string> queryParams;  // Query parameters
std::map<std::string, std::string> bodyParams;   // Form parameters
```

#### Member Functions

```cpp
std::string query_param(const std::string& key) const;  // Get query parameter
void parseBody();          // Parse request body
void show() const;         // Display request details
```

### Response Class

`Response` is used to build HTTP responses from the server.

#### Member Variables

```cpp
int statusCode = 200;       // HTTP status code
std::map<std::string, std::string> headers;  // Response headers
std::string body;           // Response body
```

#### Member Functions

```cpp
void json(const std::string& jsonStr);  // Set JSON response
void text(const std::string& textStr);  // Set text response
void status(int code);       // Set status code
void success();              // Set success response
void success(const std::map<std::string, std::string>& resMap);  // Set success response with data
void success(const std::map<std::string, JsonValue>& resMap);    // Set success response with nested JSON
void error(int code, const std::string& message);  // Set error response
```

### JsonValue Class

`JsonValue` provides complete JSON value representation with support for nested JSON data structures.

#### Supported Types

- NULL_TYPE: null value
- STRING: string value
- NUMBER: numeric value
- BOOLEAN: boolean value
- OBJECT: object (key-value pair mapping)
- ARRAY: array

#### Main Methods

```cpp
// Constructors
explicit JsonValue(const std::string& value);
explicit JsonValue(int value);
explicit JsonValue(double value);
explicit JsonValue(bool value);
JsonValue(const std::map<std::string, JsonValue>& value);
JsonValue(const std::vector<JsonValue>& value);

// Get type
Type getType() const;
```

### JWT Class

`JWT` provides JWT token generation and validation functionality.

#### Main Methods

```cpp
// Set secret
void setSecret(const std::string& secret);

// Generate token
std::string generateToken(const std::map<std::string, std::string>& customClaims = {}, 
                          long long ttlSeconds = -1) const;

// Verify token
bool verifyToken(const std::string& token, std::string* payloadJson = nullptr) const;

// Parse claims
std::optional<std::map<std::string, std::string>> parseClaims(const std::string& token) const;

// Password encryption and verification
static std::string encrypt_password(const std::string& password);
static bool verify_password(const std::string& password, const std::string& stored_hash);
```

### ThreadPool Class

`ThreadPool` provides thread pool implementation for concurrent processing of client requests.

#### Constructor

```cpp
explicit ThreadPool(size_t numThreads);
```
- **Parameter**: `numThreads` - Number of threads in the thread pool

#### Add Task

```cpp
template<typename F, typename... Args>
bool addTask(F&& f, Args&&... args);
```
- **Parameters**:
  - `f` - Function to execute
  - `args` - Function arguments
- **Return Value**: Returns true if task was added successfully, false if queue is full

### Log Class

`Log` provides logging functionality.

#### Main Methods

```cpp
// Get singleton instance
static Log* getInstance();

// Write log
void write(const std::string& msg);
```

## Project Structure

```
├── .gitignore          # Git ignore file
├── CMakeLists.txt      # CMake build configuration
├── LICENSE             # License file
├── README.md           # Project description document
├── include/            # Header files directory
│   ├── Headler.h       # Request handler header
│   ├── JsonValue.h     # JSON processing header
│   ├── Log.h           # Log system header
│   ├── Server.h        # Server core header
│   ├── Threadpool.h    # Thread pool header
│   ├── jwt.h           # JWT authentication header
│   └── route.h         # Routing system header
├── main.cpp            # Main program entry
└── src/                # Source code directory
    ├── JsonValue.cpp   # JSON processing implementation
    ├── Log.cpp         # Log system implementation
    ├── Server.cpp      # Server core implementation
    ├── Threadpool.cpp  # Thread pool implementation
    ├── jwt.cpp         # JWT authentication implementation
    └── route.cpp       # Routing system implementation
```

## Routing Examples

Here are some common routing registration examples:

### Basic Routes

```cpp
// GET route
app.get("/api/users", [](const Request& req, Response& res) {
    res.json(R"({"users": ["user1", "user2"]})"; 
});

// POST route
app.post("/api/users", [](const Request& req, Response& res) {
    // Handle user creation logic
    res.status(201);  // Created
    res.json(R"({"status": "success", "message": "User created"})"; 
});
```

### Getting Query Parameters

```cpp
app.get("/api/search", [](const Request& req, Response& res) {
    std::string query = req.query_param("q");
    res.json(R"({"query": "" + query + "", "results": []})"; 
});
```

### Authentication with JWT

```cpp
// Create JWT instance
JWT jwt("your-secret-key", 3600);  // Secret key and expiration time (seconds)

// Login route
app.post("/api/login", [&jwt](const Request& req, Response& res) {
    // Verify user credentials (simplified in example)
    bool validCredentials = true;  // In real application, verify username and password
    
    if (validCredentials) {
        // Generate JWT token
        std::map<std::string, std::string> claims = {{"username", "user123"}};
        std::string token = jwt.generateToken(claims);
        
        res.json(R"({"status": "success", "token": "" + token + ""})"; 
    } else {
        res.error(401, "Invalid credentials");
    }
});

// Protected route (requires token verification)
app.get("/api/protected", [&jwt](const Request& req, Response& res) {
    // Get token from request header
    auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) {
        res.error(401, "Authorization header required");
        return;
    }
    
    std::string authHeader = it->second;
    // Assume format is "Bearer <token>"
    size_t bearerPos = authHeader.find("Bearer ");
    if (bearerPos == std::string::npos || authHeader.substr(bearerPos + 7).empty()) {
        res.error(401, "Invalid authorization header format");
        return;
    }
    
    std::string token = authHeader.substr(bearerPos + 7);
    
    // Verify token
    if (jwt.verifyToken(token)) {
        res.json(R"({"status": "success", "data": "Protected resource accessed"})"; 
    } else {
        res.error(401, "Invalid or expired token");
    }
});
```

## Cross-Platform Notes

- **Windows**: Requires linking with `ws2_32` library, which CMake handles automatically
- **Linux**: Requires linking with `pthread` library, which CMake handles automatically
- Signal handling mechanisms differ between platforms, but the framework has encapsulated these differences so users don't need to worry

## License

[MIT License](LICENSE)

## Contributions

Contributions are welcome! Please submit issues and pull requests. If you have any suggestions or improvements, feel free to contact us.