# 🧪 Wolf Tests - Windows Build Guide

Quick reference for building and running Wolf tests on Windows.

## TL;DR - Quick Start

```powershell
# 1. Run setup (one-time, interactive)
cd c:\source\wolf\tests
.\setup.ps1

# 2. Build tests
.\build.ps1

# 3. Run tests
cd build
.\router_test.exe
.\web_server_test.exe
```

## Prerequisites

- **Clang++ 16+** (LLVM)
- **Boost 1.89+**
- **Catch2 v3**

## Directory Structure

### Required Catch2 Structure

```
C:\libraries\Catch2\
├── include\
│   └── catch2\
│       ├── catch_test_macros.hpp
│       ├── catch_all.hpp
│       └── ...
└── lib\
    ├── libCatch2.a              # Core library (Clang build)
    ├── libCatch2Main.a          # Main function (Clang build)
    OR
    ├── libCatch2.lib            # Core library (MSVC build)
    └── libCatch2Main.lib        # Main function (MSVC build)
```

**Important:** You need BOTH libraries:
- `libCatch2` (or `Catch2`) - The core Catch2 framework
- `libCatch2Main` (or `Catch2Main`) - Provides the `main()` function

**Note:** When Catch2 is compiled with Clang on Windows, it produces `.a` files instead of `.lib` files. The build script handles both automatically!

### Required Boost Structure

```
C:\source\wolf\boost\  (or c:\libraries\boost\)
├── include\
│   └── boost\
│       ├── asio.hpp
│       ├── beast.hpp
│       ├── json.hpp
│       └── ...
└── lib\
    ├── libboost_json-*.lib
    ├── libboost_url-*.lib
    └── ...
```

## Environment Setup

### Temporary (Current Session)

```powershell
$env:BOOST_DIR = "c:\source\wolf\boost"
$env:CATCH2_DIR = "c:\libraries\Catch2"
```

### Permanent (Recommended)

```powershell
# Set permanently
[Environment]::SetEnvironmentVariable('BOOST_DIR', 'c:\source\wolf\boost', 'User')
[Environment]::SetEnvironmentVariable('CATCH2_DIR', 'c:\libraries\Catch2', 'User')

# Reload in current session
$env:BOOST_DIR = [Environment]::GetEnvironmentVariable('BOOST_DIR', 'User')
$env:CATCH2_DIR = [Environment]::GetEnvironmentVariable('CATCH2_DIR', 'User')

# Verify
echo $env:BOOST_DIR
echo $env:CATCH2_DIR
```

## Build Commands

### Using PowerShell Script (Recommended)

```powershell
cd c:\source\wolf\tests

# Standard build
.\build.ps1

# Clean build
.\build.ps1 -Clean

# Verbose output (shows compiler commands)
.\build.ps1 -Verbose

# Custom directories
.\build.ps1 -BoostDir "c:\custom\boost" -Catch2Dir "c:\custom\catch2"
```

### Using Batch File

```cmd
cd c:\source\wolf\tests
build.bat
```

**Note:** You may need to edit `build.bat` to set the correct Boost/Catch2 library names for your installation.

### Manual Build

```powershell
# Router tests
clang++ -std=c++23 `
    -I"$env:BOOST_DIR\include" `
    -I"$env:CATCH2_DIR\include" `
    -I.. `
    router_test.cpp -o build\router_test.exe `
    -L"$env:BOOST_DIR\lib" `
    -L"$env:CATCH2_DIR\lib" `
    -llibboost_json-clang21-mt-s-x64-1_89 `
    -llibboost_url-clang21-mt-s-x64-1_89 `
    -lCatch2Maind `
    -lws2_32 -lwsock32

# Web server tests
clang++ -std=c++23 `
    -I"$env:BOOST_DIR\include" `
    -I"$env:CATCH2_DIR\include" `
    -I.. `
    web_server_test.cpp -o build\web_server_test.exe `
    -L"$env:BOOST_DIR\lib" `
    -L"$env:CATCH2_DIR\lib" `
    -llibboost_json-clang21-mt-s-x64-1_89 `
    -llibboost_url-clang21-mt-s-x64-1_89 `
    -lCatch2Maind `
    -lws2_32 -lwsock32
```

Replace library names with your actual Boost library names (use `dir $env:BOOST_DIR\lib` to find them).

## Running Tests

### Run All Tests

```powershell
cd build

# Router tests (93 assertions, 6 test cases)
.\router_test.exe

# Web server tests
.\web_server_test.exe
```

### Run Specific Test Case

```powershell
# By tag
.\router_test.exe "[http_router]"
.\web_server_test.exe "[http_request]"

# By name
.\router_test.exe "HTTP Router - Basic functionality"
```

### List Available Tests

```powershell
.\router_test.exe --list-tests
.\web_server_test.exe --list-tests

# List tags
.\router_test.exe --list-tags
```

### Verbose Output

```powershell
.\router_test.exe -s          # Show successful tests
.\router_test.exe -d yes      # Show durations
```

## Common Issues

### Issue: "CATCH2_DIR is not set"

**Solution:**

```powershell
$env:CATCH2_DIR = "c:\libraries\Catch2"

# Or permanently:
[Environment]::SetEnvironmentVariable('CATCH2_DIR', 'c:\libraries\Catch2', 'User')
```

### Issue: "Catch2 library not found"

**Solution:**

Check your Catch2 lib directory:

```powershell
dir $env:CATCH2_DIR\lib
```

Expected files:
- `libCatch2Main.a` / `libCatch2Maind.a` (Clang-compiled Catch2)
- `Catch2Main.lib` / `Catch2Maind.lib` (MSVC-compiled Catch2)

**If you see .a files:** The PowerShell build script (`build.ps1`) handles this automatically!

**If you see .lib files:** Also works automatically.

**If missing completely:** Build/install Catch2 (see "Installing Catch2" section below).

### Issue: "undefined reference to Catch::..." or linking errors

**This happens when the linker can't find Catch2 symbols.**

**Solution:**

```powershell
# Check what library files you have:
dir $env:CATCH2_DIR\lib

# For Clang-compiled Catch2 (produces .a files):
# The linker needs: -lCatch2Main (for libCatch2Main.a)
# Our build script handles this automatically

# Verify the build script found it:
.\build.ps1 -Verbose
```

### Issue: "clang++ not found"

**Solution:**

```powershell
winget install LLVM.LLVM

# Add to PATH
$env:PATH += ";C:\Program Files\LLVM\bin"

# Verify
clang++ --version
```

### Issue: Include path errors

**Solution:**

Verify directory structures:

```powershell
# Should exist:
dir "$env:CATCH2_DIR\include\catch2\catch_test_macros.hpp"
dir "$env:BOOST_DIR\include\boost\asio.hpp"
```

If missing, check your Catch2/Boost installation.

### Issue: Linking errors

**Solution:**

1. Make sure you're using the **static** library versions (e.g., `libboost_json-*-mt-s-*.lib`, note the `s` for static)
2. Verify library names match your build:

```powershell
dir "$env:BOOST_DIR\lib" | findstr boost_json
dir "$env:CATCH2_DIR\lib" | findstr Catch2
```

3. Update the library names in the build script to match

## Installing Catch2

If you don't have Catch2 installed:

### Option 1: Build from Source (Recommended)

```powershell
# Clone Catch2
git clone https://github.com/catchorg/Catch2.git
cd Catch2

# Build with CMake
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="c:\libraries\Catch2"
cmake --build . --config Release
cmake --install . --config Release

# Set environment variable
$env:CATCH2_DIR = "c:\libraries\Catch2"
```

### Option 2: Vcpkg

```powershell
# Install vcpkg (if not installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install Catch2
.\vcpkg install catch2:x64-windows

# Catch2 will be in:
# vcpkg\installed\x64-windows\include
# vcpkg\installed\x64-windows\lib
```

## Test Statistics

### Router Tests
- **6 test cases**
- **93 assertions**
- Tests: HTTP routing, Trie algorithm, parameter extraction, method mapping

### Web Server Tests
- **5 test cases**
- **60+ assertions**
- Tests: Request/response handling, cookies, status codes, RESTful patterns

## Quick Reference Commands

```powershell
# Setup (one-time)
$env:BOOST_DIR = "c:\source\wolf\boost"
$env:CATCH2_DIR = "c:\libraries\Catch2"

# Build
cd tests && .\build.ps1

# Run all tests
cd build && .\router_test.exe && .\web_server_test.exe

# Run with details
.\router_test.exe -s -d yes

# List tests
.\router_test.exe --list-tests
```

---

**For more details, see:**
- `tests/README.md` - Full test documentation
- `README.md` - Main Wolf documentation
- [Catch2 Documentation](https://github.com/catchorg/Catch2)
