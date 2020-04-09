# SDL Tossup simulator


## Building

This project uses CMake and builds SDL from source rather than picking up preinstall binaries. 

### 1. Download SDL source

The SDL source should we placed under `external\SDL2-2.0.12`.

#### Windows

If you have curl and unzip install you can do this from the command line.
```
md external
cd external
curl -O https://www.libsdl.org/release/SDL2-2.0.12.zip
unzip SDL2-2.0.12.zip
```

#### Linux/Raspberry/Mac

```
mkdir external
cd external
curl https://www.libsdl.org/release/SDL2-2.0.12.tar.gz | tar zxv
```

### 2. Build from Command line

**Windows** Download Visual Studio and make sure the CMake option is included.

```
md build
cd build
"c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\vsdevcmd"
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

This should create `SDLTossup.exe`.

_Note:_ the location of the `vsdevcmd.bat` batch file varies between installations but usually takes the form `c:\Program Files (x86)\Microsoft Visual Studio\[VS Version]\[VS Type]\Common7\Tools\vsdevcmd"`



