cmake_minimum_required(VERSION 3.9.1)

project(ToyRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(PROJECT_OUTPUT_DIR "${CMAKE_BINARY_DIR}/../bin")
set(PROJECT_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUTPUT_DIR})
set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(SHADER_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADERS_BINARY_DIR "${PROJECT_BIN_DIR}/shaders")
set(PROJECT_LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/libs")
set(TEXTURES_DIR "${CMAKE_SOURCE_DIR}/assets/textures")
set(MODEL_DIR "${CMAKE_SOURCE_DIR}/assets/models")

# 递归查找文件夹下的 .h .hpp. ini 文件保存到 HEADER_FILES
file(GLOB_RECURSE HEADER_FILES  ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.ini) 

# 递归查找文件夹下的 *.cpp *.c 文件保存到 SOURCE_FILES
file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.c)

# 将 HEDADER_FILES 和 SOURCE_FILES  保存到 AllFile 变量
set(AllFile ${HEADER_FILES} ${SOURCE_FILES})

# 对 AllFile 变量里面的所有文件分类(保留资源管理器的目录结构)
source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${AllFile})

if (CMAKE_BUILD_TYPE MATCHES Debug)
   set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall")
   #else ()
   #   set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall")
endif ()

#####################################Libs######################################

# - Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)

# - GLFW
set(GLFW_DIR "${PROJECT_LIBRARIES_DIR}/glfw")
add_subdirectory(${GLFW_DIR} EXCLUDE_FROM_ALL)

# - GLM
set(GLM_DIR "${PROJECT_LIBRARIES_DIR}/glm")
add_subdirectory(${GLM_DIR} EXCLUDE_FROM_ALL)

# -STB(Image Loader) -> Header file that contains all the code.
set(STB_DIR "${PROJECT_LIBRARIES_DIR}/stb_image")
add_subdirectory(${STB_DIR} EXCLUDE_FROM_ALL)

# -TinyObjLoader
set(TINY_OBJ_LOADER_DIR "${PROJECT_LIBRARIES_DIR}/tiny_objloader")
add_subdirectory(${TINY_OBJ_LOADER_DIR})

# - PThreads
set(THREADS_PREFER_PTRHEAD_FLAG ON)
find_package(Threads REQUIRED)

##################################Shaders######################################

# - Compilation of shaders
add_subdirectory(${SHADER_DIR})
# To get the full path of the compiled shaders in the c++ code.
add_definitions(-DSHADERS_BINARY_DIR="${SHADERS_BINARY_DIR}")


################################Definitions####################################

add_definitions(-DTEXTURES_DIR="${TEXTURES_DIR}/")
add_definitions(-DMODEL_DIR="${MODEL_DIR}/")

#################################Executable####################################

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(
   ${PROJECT_NAME}
   PUBLIC
      "${Vulkan_INCLUDE_DIRS}"
      "${GLFW_INCLUDE_DIRS}"
   PRIVATE
      "${PROJECT_SOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME} PUBLIC
   glfw 
   ${Vulkan_LIBRARIES}
   Threads::Threads  # Needed by GLFW.
   glm
   stb_image
   tinyobjloader
   PRIVATE ${CMAKE_DL_LIBS}
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
# CMAKE_DL_LIBS -> is the library libdl which helps to link dynamic
# libraries. We need it in order to use Vulkan Loader.