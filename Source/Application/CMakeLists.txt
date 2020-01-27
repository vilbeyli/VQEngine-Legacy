# https://cmake.org/examples/

# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.4)
project (Application)

add_compile_options(/MP)
add_compile_options(/std:c++17)

set(Headers 
    "Application.h"
    "CMakeLists.txt"
    "HandleTypedefs.h"
    "Input.h"
    "resource.h"    
    "SystemDefs.h"  
    "ThreadPool.h"  
)   
    
set (Source 
    "Source/Application.cpp"
    "Source/Input.cpp"
    "Source/Main.cpp"
    "Source/ThreadPool.cpp"
)

set (Includes
    "../"
    
)

# Create a library with the project name that is build with the Headers and Source files
add_library(${PROJECT_NAME} ${Headers} ${Source} )

# Make sure the compile can find include files for the library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Includes})

target_link_libraries(${PROJECT_NAME} Engine Utilities)
