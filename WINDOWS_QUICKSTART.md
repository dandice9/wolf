# 🪟 Wolf Web Framework - Windows Quick Start Guide

Get up and running with Wolf in under 5 minutes on Windows!

## Prerequisites

- Windows 10/11
- PowerShell (already installed)

## Step-by-Step Tutorial

### 1. Install Clang Compiler

Open PowerShell as **Administrator** and run:

```powershell
winget install LLVM.LLVM
```

Verify installation:

```powershell
clang++ --version
# Should show: clang version 16.x or higher
```

If `clang++` command not found, close and reopen PowerShell, or add to PATH manually:

```powershell
$env:PATH += ";C:\Program Files\LLVM\bin"
```

### 2. Extract Boost Libraries

**Option A: Use included boost.zip (Recommended)**

```powershell
# Navigate to project directory
cd c:\source\wolf

# Extract boost.zip to project root (if not already extracted)
# You should see a "boost" folder with: include, lib, bin, cmake
```

**Option B: Use custom Boost location**

If you have Boost installed elsewhere (e.g., `c:\libraries\boost`), skip extraction and note the path for Step 3.

### 3. Set BOOST_DIR Environment Variable

**Temporary (Current Session Only):**

```powershell
# If using project's boost folder:
$env:BOOST_DIR = "c:\source\wolf\boost"

# OR if using custom location:
$env:BOOST_DIR = "c:\libraries\boost"
```

**Permanent (Recommended):**

```powershell
# If using project's boost folder:
[Environment]::SetEnvironmentVariable('BOOST_DIR', 'c:\source\wolf\boost', 'User')

# OR if using custom location:
[Environment]::SetEnvironmentVariable('BOOST_DIR', 'c:\libraries\boost', 'User')

# Reload variable in current session:
$env:BOOST_DIR = [Environment]::GetEnvironmentVariable('BOOST_DIR', 'User')
```

Verify:

```powershell
echo $env:BOOST_DIR
# Should show: c:\source\wolf\boost or your custom path
```

### 4. Build Examples

```powershell
cd c:\source\wolf\example
.\build.ps1
```

You should see output like:

```
========================================
Wolf Examples Build Script (Windows)
========================================

BOOST_DIR: c:\source\wolf\boost
Boost headers: c:\source\wolf\boost\include
Boost libraries: c:\source\wolf\boost\lib

Compiler: clang++
clang version 21.1.4

Building examples...

Found: libboost_json-clang21-mt-s-x64-1_89.lib
Found: libboost_url-clang21-mt-s-x64-1_89.lib

[1/3] Building example_router...
[1/3] example_router.exe - OK

[2/3] Building example_web...
[2/3] example_web.exe - OK

[3/3] Building test_client...
[3/3] test_client.exe - OK

========================================
Build completed successfully!
========================================
```

### 5. Run Examples

```powershell
cd build
.\example_web.exe
```

You should see:

```
Starting Wolf Web Server on http://localhost:8080
Press Ctrl+C to stop
==================================================
Server started successfully!
Try: curl http://localhost:8080
==================================================
```

### 6. Test It!

Open a **new PowerShell window** (keep the server running) and test:

```powershell
# Test ping endpoint
curl http://localhost:8080/ping
# Output: pong

# Test cookie setting
curl -v http://localhost:8080/set-cookie
# Check Set-Cookie header

# Test cookie reading
curl -b "session_id=demo_session_12345" http://localhost:8080/get-cookie
# Output: {"session_id": "demo_session_12345"}

# Test all users
curl http://localhost:8080/api/users
# Output: JSON array of users
```

### 7. Create Your Own Server

Create `my_server.cpp`:

```cpp
#include "wolf/web_server.hpp"
#include <iostream>

int main() {
    wolf::web_server server(8080);
    
    server->get("/", [](auto& req) {
        return "Hello from my Wolf server!";
    });
    
    server->get("/api/time", [](auto& req) {
        return wolf::make_response(
            R"({"time": "2025-11-06"})",
            http::status::ok,
            "application/json"
        );
    });
    
    std::cout << "My server running on http://localhost:8080\n";
    server.start();
}
```

Build it:

```powershell
clang++ -std=c++23 `
    -I"$env:BOOST_DIR\include" -I.. `
    my_server.cpp -o my_server.exe `
    -L"$env:BOOST_DIR\lib" -lws2_32 -lwsock32 `
    -llibboost_json-clang21-mt-s-x64-1_89 `
    -llibboost_url-clang21-mt-s-x64-1_89
```

Run it:

```powershell
.\my_server.exe
```

## Common Issues & Solutions

### Issue: "clang++ : The term 'clang++' is not recognized"

**Solution:**

```powershell
# Install LLVM
winget install LLVM.LLVM

# Restart PowerShell or add to PATH:
$env:PATH += ";C:\Program Files\LLVM\bin"

# Verify
clang++ --version
```

### Issue: "BOOST_DIR environment variable is not set"

**Solution:**

```powershell
# Set it temporarily:
$env:BOOST_DIR = "c:\source\wolf\boost"

# Or permanently:
[Environment]::SetEnvironmentVariable('BOOST_DIR', 'c:\source\wolf\boost', 'User')

# Reload:
$env:BOOST_DIR = [Environment]::GetEnvironmentVariable('BOOST_DIR', 'User')
```

### Issue: "Boost headers not found"

**Solution:**

Verify boost directory structure:

```powershell
dir $env:BOOST_DIR\include\boost
dir $env:BOOST_DIR\lib

# Should see boost headers and .lib files
```

If missing, re-extract boost.zip to the correct location.

### Issue: "Port 8080 already in use"

**Solution:**

```powershell
# Find process using port 8080
netstat -ano | findstr :8080

# Kill it (replace <PID> with actual number)
taskkill /PID <PID> /F

# Or change port in your code:
wolf::web_server server(9090);  // Use different port
```

### Issue: Build warnings about "anonymous types"

This is normal and can be ignored. The warnings are from Boost.Beast internals and don't affect functionality.

## Next Steps

- **Read the main [README.md](README.md)** for complete API reference
- **Check examples** in `example/` folder
- **Run tests** in `tests/` folder
- **Build your own REST API** using the JSON examples

## Quick Reference

### Directory Structure

```
c:\source\wolf\
├── boost/                  # Boost libraries (from boost.zip)
│   ├── include\boost\
│   └── lib\
├── example/                # Example applications
│   ├── build.ps1          # Build script
│   └── build\             # Compiled executables
├── wolf/                   # Framework headers
│   ├── http_router.hpp
│   └── web_server.hpp
└── README.md
```

### Essential Commands

```powershell
# Build examples
cd example && .\build.ps1

# Run web server
cd build && .\example_web.exe

# Build custom app
clang++ -std=c++23 -I"$env:BOOST_DIR\include" -I. my_app.cpp -o my_app.exe -L"$env:BOOST_DIR\lib" -lws2_32 -lwsock32

# Test endpoints
curl http://localhost:8080/endpoint
```

### Environment Variables

```powershell
# View current value
echo $env:BOOST_DIR

# Set temporarily
$env:BOOST_DIR = "path\to\boost"

# Set permanently
[Environment]::SetEnvironmentVariable('BOOST_DIR', 'path\to\boost', 'User')
```

---

**Happy coding with Wolf! 🐺**

Need help? Check the main [README.md](README.md) or open an issue on GitHub.
