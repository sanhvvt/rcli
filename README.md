# rcli
a Lightweight Redis client

# Requirement
rcli requires hiredis.

# Build
```
# linux
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make
```
```
# win32 + mingw64 + ninja
mkdir build && cd build
cmake .. -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build .
```