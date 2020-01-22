# https://cmake.org/examples/

# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 3.4)
project (Engine)

add_compile_options(/MP)
add_compile_options(/std:c++17)

set(Headers 
    "Animation.h"
    "Camera.h"
    "CMakeLists.txt"
    "DataStructures.h"
    "Engine.h"
    "GameObject.h"
    "GameObjectPool.h"
    "Light.h"
    "Material.h"
    "Mesh.h"
    "Model.h"
    "ObjectCullingSystem.h"
    "PerfTree.h"
    "Scene.h"
    "SceneLODManager.h"
    "SceneResourceView.h"
    "SceneView.h"
    "Settings.h"
    "Skybox.h"
    "Transform.h"
    "UI.h"
)

set (Source
    "Source/Camera.cpp"
    "Source/Component.cpp"
    "Source/Engine.cpp"
    "Source/GameObject.cpp"
    "Source/GameObjectPool.cpp"
    "Source/Light.cpp"
    "Source/Material.cpp"
    "Source/Mesh.cpp"
    "Source/Model.cpp"
    "Source/ObjectCullingSystem.cpp"
    "Source/Scene.cpp"
    "Source/SceneLODManager.cpp"
    "Source/SceneResourceView.cpp"
    "Source/Skybox.cpp"
    "Source/Transform.cpp"
    "Source/UI.cpp"
)

set (Includes
    "../"
    "../3rdParty/assimp/include/"
)

set (Libraries
    "Renderer"
    "RenderPasses"
    "Scenes"
    "assimp-vc140-mt"
)


set (LibraryDirs
    "../3rdParty/assimp/lib/x64/Release"
)

# Create a library with the project name that is build with the Headers and Source files
add_library(${PROJECT_NAME} ${Headers} ${Source} )

# Make sure the compile can find include files for the library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Includes})

target_link_directories(${PROJECT_NAME} PUBLIC ${LibraryDirs})
target_link_libraries(${PROJECT_NAME} ${Libraries})