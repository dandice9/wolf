# Quick Build Guide for Windows

## Prerequisites

1. **Set BOOST_ROOT environment variable:**
   ```powershell
   # Temporary (current session only)
   $env:BOOST_ROOT="C:\path\to\boost"
   
   # Permanent (requires admin)
   [System.Environment]::SetEnvironmentVariable('BOOST_ROOT', 'C:\path\to\boost', 'User')
   ```

2. **Install LLVM/Clang (recommended):**
   - Download from: https://releases.llvm.org/
   - Or use: `winget install LLVM.LLVM`
   - Ensure `clang++` is in your PATH

3. **Or install Visual Studio 2022+** (alternative to Clang)

## Quick Build (Direct Compilation with Clang)

```powershell
# Navigate to example directory
cd c:\source\wolf\example

# Run build script
.\build.bat
```

**Output:** Executables in `build\` directory
- `build\example_router.exe`
- `build\example_web.exe`  
- `build\test_client.exe`

## CMake Build (More Flexible)

```powershell
# Navigate to example directory
cd c:\source\wolf\example

# Run CMake build script
.\build_cmake.bat
```

**Output:** Executables in `build\` or `build\Release\` directory

## Run Examples

```powershell
# Start web server
cd build
.\example_web.exe

# In another terminal, test it
curl http://localhost:8080/ping
# Output: pong

curl http://localhost:8080/set-cookie
# Sets a cookie

curl -b "session_id=demo_session_12345" http://localhost:8080/get-cookie
# Returns cookie value
```

## Troubleshooting

### Error: BOOST_ROOT not set
```powershell
$env:BOOST_ROOT="C:\boost_1_89_0"  # Adjust path
```

### Error: clang++ not found
- Install LLVM: `winget install LLVM.LLVM`
- Or add to PATH: `$env:PATH += ";C:\Program Files\LLVM\bin"`

### Error: Boost libraries not found
- Check BOOST_ROOT points to correct directory
- Ensure Boost libraries are built in `%BOOST_ROOT%\stage\lib`
- Build Boost if needed: `.\b2 --build-type=complete stage`

### Port 8080 already in use
```powershell
# Find process using port 8080
netstat -ano | findstr :8080

# Kill process (replace PID)
taskkill /PID <PID> /F
```

## Clean Build

```powershell
# Remove build directory
Remove-Item -Recurse -Force build

# Rebuild
.\build.bat
```

## Next Steps

- See [README.md](README.md) for detailed examples
- Check [../README.md](../README.md) for full framework documentation
- Run tests: `cd ..\tests && .\build.bat` (if available)
