# https://cmake.org/examples/

# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.4)
project (VQEngine)

# Recurse into the "Hello" and "Demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
#add_subdirectory (Source/3rdParty)
add_subdirectory (Source/Application)
add_subdirectory (Source/Engine)
add_subdirectory (Source/Renderer)
add_subdirectory (Source/RenderPasses)
add_subdirectory (Source/Scenes)
add_subdirectory (Source/Utilities)