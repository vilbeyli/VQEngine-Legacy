This folder contains the minimum required files for using assimp with VQEngine
 - include headers
 - static libs and dlls for runtime (x64 | Debug/Release)

 The License and Readme files for assimp 4.1.0 is left in the directory.

---

 The reason for all this is because the assimp library is built 32-bit by default if you install CMake on Windows and run the default config. 
 Following [this StackOverflow question](https://stackoverflow.com/questions/45928202/assimp-model-loading-library-install-linking-troubles), I've rebuilt
 the project for x64 and left only the necessary files here to save headache in the future.