# https://cmake.org/examples/

# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.4)
project (Renderer)

add_compile_options(/MP)
add_compile_options(/std:c++17)

set(Headers 
    "BufferObject.h"
    "CMakeLists.txt"
    "D3DManager.h"
    "GeometryGenerator.h"
    "RenderCommands.h"
    "Renderer.h"
    "RenderingEnums.h"
    "RenderingStructs.h"
    "Shader.h"
    "TextRenderer.h"
    "Texture.h"
    "Profiler.h"
)

set (Source
    "Source/Buffer.cpp"
    "Source/D3DManager.cpp"
    "Source/GeometryGenerator.cpp"
    "Source/RenderCommands.cpp"
    "Source/Renderer.cpp"
    "Source/Shader.cpp"
    "Source/TextRenderer.cpp"
    "Source/Texture.cpp"
    "Source/Profiler.cpp"
)


set (Includes
    "../3rdParty/freetype-windows-binaries/include/"
    "../"
)


set (Libraries
    DirectXTex
    freetype
    dxguid
)


set (LibraryDirs
    "../3rdParty/freetype-windows-binaries/win64/"
    "../3rdParty/DirectXTex/DirectXTex/Bin/Desktop_2017/x64/${Configuration}"
)

# Create a library with the project name that is build with the Headers and Source files
add_library(${PROJECT_NAME} ${Headers} ${Source} )

# Make sure the compile can find include files for the library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Includes})

target_link_directories(${PROJECT_NAME} PUBLIC ${LibraryDirs})
target_link_libraries(${PROJECT_NAME} ${Libraries})

add_custom_command(TARGET ${PROJECT_NAME} 
    PRE_BUILD
    COMMAND cmd /C "${CMAKE_CURRENT_SOURCE_DIR}/../../Scripts/BUILD_DEPENDENCIES.bat -Dir:${CMAKE_CURRENT_SOURCE_DIR} -$(Configuration)"
)