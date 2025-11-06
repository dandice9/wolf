# 🐺 Wolf Tests - Getting Started

## What We Created

✅ **build.ps1** - PowerShell build script with auto-detection of Boost/Catch2 libraries
✅ **build.bat** - Batch file alternative for building tests
✅ **setup.ps1** - Interactive setup script for environment variables
✅ **BUILD_WINDOWS.md** - Comprehensive Windows build guide
✅ **README.md** - Updated with quick start instructions
✅ **Fixed include paths** - web_server_test.cpp and router_test.cpp now use correct paths

## Quick Start Guide

### Step 1: Set Environment Variables (One-Time Setup)

**Option A: Interactive Setup (Recommended)**
```powershell
cd c:\source\wolf\tests
.\setup.ps1
```

**Option B: Manual Setup**
```powershell
# Set CATCH2_DIR to your Catch2 installation
$env:CATCH2_DIR = "c:\libraries\Catch2"

# Make it permanent
[Environment]::SetEnvironmentVariable('CATCH2_DIR', 'c:\libraries\Catch2', 'User')

# Verify
echo $env:CATCH2_DIR
echo $env:BOOST_DIR  # Should already be set from examples build
```

### Step 2: Build Tests

```powershell
cd c:\source\wolf\tests
.\build.ps1
```

Expected output:
```
========================================
Wolf Tests Build Script (Windows)
========================================
→ BOOST_DIR: c:\libraries\boost
→ Boost headers: c:\libraries\boost\include
→ Boost libraries: c:\libraries\boost\lib
→ CATCH2_DIR: c:\libraries\Catch2
→ Catch2 headers: c:\libraries\Catch2\include
→ Catch2 libraries: c:\libraries\Catch2\lib
✓ Compiler: clang++

Building tests...

✓ Found: libboost_json-clang21-mt-s-x64-1_89.lib
✓ Found: libboost_url-clang21-mt-s-x64-1_89.lib
✓ Found: Catch2Maind.lib

[1/2] Building router_test...
✓ [1/2] router_test.exe - OK

[2/2] Building web_server_test...
✓ [2/2] web_server_test.exe - OK

========================================
Build completed successfully!
========================================
```

### Step 3: Run Tests

```powershell
cd build
.\router_test.exe
.\web_server_test.exe
```

## Environment Variables Summary

Your system needs these environment variables set:

| Variable | Purpose | Example Value |
|----------|---------|---------------|
| `BOOST_DIR` | Boost installation | `c:\libraries\boost` or `c:\source\wolf\boost` |
| `CATCH2_DIR` | Catch2 installation | `c:\libraries\Catch2` |

## Directory Structure Requirements

### Catch2 Structure
```
c:\libraries\Catch2\
├── include\
│   └── catch2\
│       ├── catch_test_macros.hpp
│       └── ...
└── lib\
    ├── libCatch2.a / libCatch2.lib         # Core library
    └── libCatch2Main.a / libCatch2Main.lib # Main function
```

**Important:** You need BOTH Catch2 libraries (core + main)!

**Note:** Catch2 compiled with Clang produces `.a` files instead of `.lib` files. The build scripts handle both automatically!

### Boost Structure
```
c:\libraries\boost\  (or wherever BOOST_DIR points)
├── include\
│   └── boost\
│       ├── asio.hpp
│       ├── beast.hpp
│       ├── json.hpp
│       └── ...
└── lib\
    ├── libboost_json-*.lib
    └── libboost_url-*.lib
```

## Build Script Features

### build.ps1 (PowerShell)

**Features:**
- ✅ Auto-detects Boost library names
- ✅ Auto-detects Catch2 library names
- ✅ Colored output for better readability
- ✅ Validates directory structure
- ✅ Shows file sizes after build
- ✅ Provides helpful error messages

**Usage:**
```powershell
.\build.ps1                                      # Standard build
.\build.ps1 -Clean                               # Clean build directory first
.\build.ps1 -Verbose                             # Show compiler commands
.\build.ps1 -BoostDir "c:\custom\boost"          # Override BOOST_DIR
.\build.ps1 -Catch2Dir "c:\custom\catch2"        # Override CATCH2_DIR
```

### build.bat (Batch File)

**Note:** May require manual editing to set library names.

**Usage:**
```cmd
build.bat
```

## Test Executables

After building, you'll have:

### router_test.exe
- Tests HTTP routing functionality
- Tests trie-based parameter extraction
- Tests all HTTP methods (GET, POST, PUT, DELETE, etc.)
- **6 test cases, 93 assertions**

### web_server_test.exe
- Tests HTTP request/response handling
- Tests cookie parsing and setting
- Tests query parameters
- Tests RESTful patterns
- **5 test cases, 60+ assertions**

## Running Specific Tests

```powershell
cd build

# Run by tag
.\router_test.exe "[http_router]"
.\web_server_test.exe "[http_request]"

# Run by name
.\router_test.exe "HTTP Router - Basic functionality"

# List all available tests
.\router_test.exe --list-tests
.\web_server_test.exe --list-tests

# Verbose output
.\router_test.exe -s          # Show successful tests
.\router_test.exe -d yes      # Show durations
```

## Troubleshooting

### Error: "CATCH2_DIR is not set"

**Solution:**
```powershell
.\setup.ps1
# Or manually:
$env:CATCH2_DIR = "c:\libraries\Catch2"
[Environment]::SetEnvironmentVariable('CATCH2_DIR', 'c:\libraries\Catch2', 'User')
```

### Error: "Catch2 library not found"

**Check your lib directory:**
```powershell
dir $env:CATCH2_DIR\lib
```

Expected to see: `Catch2Maind.lib`, `Catch2Main.lib`, or similar.

If missing, you need to install/build Catch2. See BUILD_WINDOWS.md for instructions.

### Error: "clang++ not found"

**Solution:**
```powershell
winget install LLVM.LLVM
```

### Build succeeds but tests don't run

**Check for missing DLLs:**
- If you built Boost dynamically, copy DLLs from `%BOOST_DIR%\bin` to `build\`
- For static linking (recommended), use libraries with `-s-` in the name

## Next Steps

1. ✅ **Run tests** to verify your Wolf installation
2. ✅ **Add your own tests** to router_test.cpp or web_server_test.cpp
3. ✅ **Check test coverage** - see which features are tested
4. ✅ **Contribute** - help improve Wolf's test suite!

## Documentation

- **BUILD_WINDOWS.md** - Detailed Windows build instructions, troubleshooting
- **README.md** - Test suite overview, test case descriptions
- **Main README** - Wolf framework documentation (../README.md)

---

**Happy Testing! 🐺**

Need help? Check BUILD_WINDOWS.md for detailed troubleshooting.
