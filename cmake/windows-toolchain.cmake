# CMake Toolchain file for cross-compiling to Windows from macOS using MinGW-w64

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Find MinGW compilers
set(TOOLCHAIN_PREFIX x86_64-w64-mingw32)

# Use Homebrew's MinGW
set(MINGW_PATH /opt/homebrew/bin)

set(CMAKE_C_COMPILER ${MINGW_PATH}/${TOOLCHAIN_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PATH}/${TOOLCHAIN_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PATH}/${TOOLCHAIN_PREFIX}-windres)

# Target environment
set(CMAKE_FIND_ROOT_PATH /opt/homebrew/Cellar/mingw-w64/13.0.0_2/toolchain-x86_64/${TOOLCHAIN_PREFIX})

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Windows-specific flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -lpthread")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
