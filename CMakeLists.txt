cmake_minimum_required(VERSION 3.9.1)

project(ToyRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../bin")
set(PROJECT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(PROJECT_LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/libs")


# 递归查找文件夹下的 .h .hpp. ini 文件保存到 HEADER_FILES
file(GLOB_RECURSE HEADER_FILES  ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp ${PROJECT_SOURCE_DIR}/*.ini) 

# 递归查找文件夹下的 *.cpp *.c 文件保存到 SOURCE_FILES
file(GLOB_RECURSE SOURCE_FILES ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/*.c)

# 递归查找文件夹下的 *.hlsl 文件保存到 SHADER_FILES(如果没有 shader 可以删除)
file(GLOB_RECURSE SHADER_FILES ${PROJECT_SOURCE_DIR}/*.hlsl)

# 将 HEDADER_FILES 和 SOURCE_FILES 和 SHADER_FILES 保存到 AllFile 变量
set(AllFile ${HEADER_FILES} ${SOURCE_FILES} ${SHADER_FILES})

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

# - PThreads
set(THREADS_PREFER_PTRHEAD_FLAG ON)
find_package(Threads REQUIRED)

#################################Executable####################################

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})

target_include_directories(
   ${PROJECT_NAME}
   PUBLIC
      "${PROJECT_INCLUDE_DIR}"
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
   PRIVATE ${CMAKE_DL_LIBS}
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# CMAKE_DL_LIBS -> is the library libdl which helps to link dynamic
# libraries. We need it in order to use Vulkan Loader.