file(GLOB_RECURSE GLSL_SOURCE_FILES
    "./*.frag"
    "./*.vert"
    "./*.geom"
    "./*.comp"
    "./*.glsl"
)

message(STATUS ${GLSL_SOURCE_FILES})

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND glslc --target-env=vulkan1.2 -std=460 ${GLSL} -o ${SPIRV} -I "../../shaders/include" # relative to build directory
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
  shaders
  DEPENDS ${SPIRV_BINARY_FILES}
  )

add_dependencies(path-tracer shaders)
