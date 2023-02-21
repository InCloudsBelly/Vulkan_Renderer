find_package(Vulkan COMPONENTS glslc)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

set(SHADERS_BINARY_DIR "${PROJECT_BIN_DIR}/shaders")

file (GLOB SHADERS
   ${SHADER_DIR}/*.vert
   ${SHADER_DIR}/*.frag
)
 set(SPV_SHADERS "")

foreach(SHADER IN LISTS SHADERS)
   get_filename_component(FILENAME ${SHADER} NAME)

   get_filename_component(TMP_EXTENSION ${SHADER} EXT)
   string(REPLACE "." "" EXTENSION ${TMP_EXTENSION})

   set(SPV_FILE "${SHADERS_BINARY_DIR}/${EXTENSION}.spv")
   #add_custom_command(
   #   OUTPUT ${SPV_FILE}
   #   COMMAND ${glslc_executable} -o ${SPV_FILE} ${SHADER}
   #   DEPENDS ${SHADER}
   #)

   execute_process(COMMAND ${glslc_executable} -o ${SPV_FILE} ${SHADER})

   message(${glslc_executable} -o ${SPV_FILE} ${SHADER})
   list(APPEND SPV_SHADERS ${SPV_FILE})
endforeach()

add_custom_target(shaders 
        DEPENDS ${SPV_SHADERS} SOURCES ${SHADERS})