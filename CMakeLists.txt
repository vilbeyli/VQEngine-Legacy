# https://cmake.org/examples/

# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.4)
set(CMAKE_GENERATOR_PLATFORM x64)

project (VQEngine)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# ouput bin directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/Bin)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_HOME_DIRECTORY}/Bin )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# Recurse into the subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (Source/Application)
add_subdirectory (Source/Engine)
add_subdirectory (Source/Renderer)
add_subdirectory (Source/RenderPasses)
add_subdirectory (Source/Scenes)
add_subdirectory (Source/Utilities)

set (SourceVQ 
    "Source/Application/Source/Main.cpp"
)

add_link_options(/SUBSYSTEM:WINDOWS)

# Create a library with the project name that is build with the Headers and Source files
add_executable( ${PROJECT_NAME} ${SourceVQ} )

# Make sure the compile can find include files for the library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Includes})

target_link_libraries(${PROJECT_NAME} Application )

set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_HOME_DIRECTORY}/Bin")



set(OutputDirectory
    "${CMAKE_HOME_DIRECTORY}/Bin"
)
set(CopyDirectories
    "${CMAKE_HOME_DIRECTORY}/Data"
)
set(CopyDLLs
    "${CMAKE_HOME_DIRECTORY}/Source/3rdParty/freetype-windows-binaries/win64/freetype.dll"
    "${CMAKE_HOME_DIRECTORY}/Source/3rdParty/assimp/lib/$(Platform)/$(Configuration)/assimp-vc140-mt.dll"    
)
add_custom_command(TARGET ${PROJECT_NAME} 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${OutputDirectory}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_HOME_DIRECTORY}/Data" "${OutputDirectory}/Data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_HOME_DIRECTORY}/Source/Shaders" "${OutputDirectory}/Shaders"
    COMMENT "Copy Directories ${CopyDirectories} to ${OutputDirectory}" 
    COMMAND ${CMAKE_COMMAND} -E copy ${CopyDLLs} ${OutputDirectory}
    COMMENT "Copy ${CopyDLLs} to ${OutputDirectory}" 
)
