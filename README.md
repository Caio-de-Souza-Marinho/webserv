*This project has been created as part of the 42 curriculum by caide-so, marcudos, ancarol9.*
 
# webserv
 
## Description
 
webserv is a fully functional HTTP/1.1 server written in C++98, built as part of the 42 School curriculum. The goal is to implement a real-world web server from scratch — handling multiple simultaneous connections, serving static files, processing CGI scripts, and managing file uploads — without relying on any external libraries.
 
The server is driven by a single `epoll`-based event loop, making all I/O operations non-blocking. It supports virtual hosting via a configuration file inspired by NGINX's `server` block syntax, and implements the `GET`, `POST`, and `DELETE` HTTP methods.
 
Key features:
- Non-blocking I/O with a single `epoll` instance for all sockets and CGI pipes
- Configuration file parser (NGINX-inspired `server`/`location` blocks)
- Static file serving with MIME-type detection
- Directory listing (autoindex)
- File uploads via `POST`
- CGI execution (Python, PHP) with proper environment variable setup
- Chunked transfer encoding (decoding on input)
- Keep-Alive connections
- Configurable error pages, body size limits, and redirects
- Timeout handling for idle clients and runaway CGI processes
## Instructions
 
### Requirements
 
- Linux (uses `epoll`)
- `c++` compiler with C++98 support
- Optional: `python3` and/or `php-cgi` for CGI support
### Compilation
 
```bash
make
```
 
This produces the `webserv` binary. The Makefile supports the standard rules: `all`, `clean`, `fclean`, `re`.
 
### Running
 
```bash
./webserv [configuration file]
```
 
If no configuration file is provided, the server defaults to `config/default.conf`.
 
Example:
 
```bash
./webserv config/default.conf
```
 
The default configuration listens on ports **8080** and **8081**.
 
### Running the test suite
 
```bash
make test
```
 
This compiles and runs the unit tests covering the request parser, router, config parser, and response builder.
 
### Configuration file overview
 
The config format is inspired by NGINX. A minimal example:
 
```nginx
server {
    listen 8080;
    root ./www;
    index index.html;
    client_max_body_size 10M;
 
    error_page 404 ./www/error_pages/404.html;
 
    location / {
        allow_methods GET POST DELETE;
        autoindex off;
    }
 
    location /uploads {
        allow_methods GET POST DELETE;
        autoindex on;
        upload_path ./www/uploads;
    }
 
    location /cgi-bin {
        allow_methods GET POST;
        root ./www/cgi-bin;
        cgi .py /usr/bin/python3;
        cgi .php /usr/bin/php-cgi;
    }
 
    location /old {
        allow_methods GET;
        return 301 /;
    }
}
```
 
Supported directives: `listen`, `root`, `index`, `client_max_body_size`, `error_page`, `location`, `allow_methods`, `autoindex`, `upload_path`, `return`, `cgi`.
 
## Architecture

### Core classes and relationships
```mermaid
classDiagram
    class WebServer {
        -int epfd
        -vector~Server~ servers
        -map~int, Client~ clients
        -map~int, Server*~ fdToServer
        -Router* router
        -ResponseBuilder* responseBuilder
        -CGIHandler* cgiHandler
        +run()
        -acceptClient(int)
        -readClient(int)
        -writeClient(int)
        -handleRequest(Client&)
        -handleCGI(Client&)
        -closeClient(int)
        -checkTimeouts()
    }

    class Server {
        +int fd
        +ServerConfig config
    }

    class Client {
        +int fd
        +string readBuffer
        +string writeBuffer
        +Request request
        +Response response
        +ClientState state
        +time_t lastActivity
        +Server* server
        +RequestParser* parser
        +pid_t cgiPid
        +int cgiInputFd
        +int cgiOutputFd
    }

    class ServerConfig {
        +string host
        +int port
        +size_t maxBodySize
        +map~int,string~ errorPages
        +vector~Route~ routes
    }

    class Route {
        +string path
        +string root
        +vector~string~ index
        +set~string~ methods
        +string redirectUrl
        +int redirectCode
        +bool autoindex
        +string uploadPath
        +map~string,string~ cgiHandlers
    }

    class RequestParser {
        +State parse(Request&, string&)
        -parseRequestLine()
        -parseHeaders()
        -parseBody()
    }

    class Request {
        +string method
        +string uri
        +string path
        +string query
        +map~string,string~ headers
        +string body
        +bool isChunked
        +bool isComplete
    }

    class ResponseBuilder {
        +buildResponse(Request&, Route*, ServerConfig&)
        +buildErrorResponse(int, ServerConfig&)
        -handleGET()
        -handlePOST()
        -handleDELETE()
    }

    class Response {
        +int statusCode
        +map~string,string~ headers
        +string body
        +build()
    }

    class CGIHandler {
        +execute(Client&, string scriptPath, string interpreter)
        -buildEnv()
        -buildEnvp()
        -buildArgv()
    }

    class Router {
        +matchRoute(Request&, ServerConfig&) const
        +isMethodAllowed(Route&, string) const
        +resolvePath(Route&, Request&) const
    }

    class ConfigParser {
        +parse() vector~ServerConfig~
        -parseServer()
        -parseLocation()
    }

    WebServer "1" *-- "1..*" Server
    WebServer "1" *-- "1" Router
    WebServer "1" *-- "1" ResponseBuilder
    WebServer "1" *-- "1" CGIHandler
    WebServer "1" --> "0..*" Client : manages
    Client --> Server : points to
    Server --> ServerConfig : owns
    ServerConfig --> Route : contains
    Client --> RequestParser : owns
    Client --> Request : contains
    Client --> Response : contains
    WebServer --> ConfigParser : uses
    ResponseBuilder --> Response : creates
    Router --> Route : uses
    CGIHandler --> Client : modifies
```

### Request lifecycle
```mermaid
flowchart TD
    subgraph "Connection Setup"
        A[Client connects] --> B[WebServer::acceptClient]
        B --> C[Set socket non-blocking<br/>Add to epoll with EPOLLIN]
    end

    subgraph "Read & Parse Loop"
        C --> D{epoll event}
        D -->|EPOLLIN| E[readClient]
        E --> F[Append data to readBuffer]
        F --> G[RequestParser::parse]
        G -->|"Parse error"| ERR1[Build 400 Bad Request] --> WRITE1[State = WRITING]
        G -->|Incomplete| D
        G -->|Complete| H[handleRequest]
    end

    subgraph "Routing"
        H --> I[Router::matchRoute]
        I -->|"No matching route"| ERR2[Build 404 Not Found] --> WRITE2[State = WRITING]
        I -->|"Route found"| J{Method allowed?}
        J -->|No| ERR3[Build 405 Method Not Allowed] --> WRITE3[State = WRITING]
        J -->|Yes| K[Resolve physical path]
    end

    subgraph "CGI Branch"
        K -->|"CGI extension matched"| L[CGIHandler::execute]
        L --> M[Fork child process<br/>Setup pipes]
        M --> N[Add cgiOutputFd to epoll<br/>State = CGI_RUNNING]
        N --> O{epoll event on CGI pipe}
        O -->|"EPOLLIN stdout"| P[Read from cgiOutputFd<br/>Append to cgiBuffer]
        P -->|"Still running"| O
        P -->|EOF| Q[waitpid, collect exit status]
        O -->|"EPOLLOUT stdin"| R[Write request body to cgiInputFd]
        R -->|"More data"| O
        R -->|Done| S[Close cgiInputFd]
        Q --> T{CGI exit status}
        T -->|0| U[Build 200 Response from cgiBuffer]
        T -->|"Non-zero"| V[Build 502 Bad Gateway]
        U --> WRITE4[State = WRITING]
        V --> WRITE5[State = WRITING]
    end

    subgraph "Static File Branch"
        K -->|"Not CGI"| W[ResponseBuilder::buildResponse]
        W --> X{File/directory exists?}
        X -->|No| ERR4[Build 404 Not Found] --> WRITE6[State = WRITING]
        X -->|"Directory & autoindex off"| ERR5[Build 403 Forbidden] --> WRITE7[State = WRITING]
        X -->|"Directory & autoindex on"| Y[Generate directory listing]
        X -->|File| Z[Read file content]
        Y --> RESP[Build 200 Response]
        Z --> RESP
        RESP --> WRITE8[State = WRITING]
    end

    subgraph "Write & Cleanup"
        WRITE1 & WRITE2 & WRITE3 & WRITE4 & WRITE5 & WRITE6 & WRITE7 & WRITE8 --> AA[modifyEpoll to EPOLLOUT]
        AA --> AB[writeClient]
        AB --> AC[Send writeBuffer<br/>Advance writeOffset]
        AC --> AD{Fully sent?}
        AD -->|No| AB
        AD -->|Yes| AE{Keep-Alive?}
        AE -->|Yes| AF[Reset client<br/>State = READING]
        AF --> D
        AE -->|No| AG[closeClient]
    end

    subgraph "Timeout Handling"
        D -->|Timeout| TO[checkTimeouts]
        TO -->|Exceeded| ERR6[Build 408 Request Timeout] --> WRITE9[State = WRITING]
        TO -->|"Still alive"| D
        O -->|Timeout| TO2[CGI timeout?]
        TO2 -->|Yes| KILL[Kill CGI process<br/>Build 504 Gateway Timeout] --> WRITE10[State = WRITING]
    end
```

## Resources
 
### HTTP & Web Server
 
- [RFC 7230 — HTTP/1.1: Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 — HTTP/1.1: Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 — The Common Gateway Interface (CGI/1.1)](https://datatracker.ietf.org/doc/html/rfc3875)
- [MDN Web Docs — HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [NGINX Documentation](https://nginx.org/en/docs/)
### Linux I/O & Systems
 
- [`epoll` man page](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [`socket` man page](https://man7.org/linux/man-pages/man2/socket.2.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [The Linux Programming Interface — Michael Kerrisk](https://man7.org/tlpi/)
### C++98
 
- [cppreference.com — C++98 standard library](https://en.cppreference.com/w/)
### Use of AI in this project
 
AI tools (primarily Claude) were used throughout the project in the following ways:
 
- **Code review assistance** — reviewing specific functions for correctness (e.g. chunked body parsing, CGI pipe handling) and catching edge cases before they became runtime bugs.
- **Documentation** — helping draft inline comments and this README.
- **Architecture discussion** — talking through the epoll event loop design and CGI lifecycle to validate the approach before implementation.
All AI-generated suggestions were reviewed, understood, and adapted by the team before being incorporated into the codebase.
