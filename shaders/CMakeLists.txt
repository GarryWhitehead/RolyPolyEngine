
# compile all shaders
set(GLSLANG_EXE ${glslang_INCLUDE_DIRS}/../bin/glslangValidator)
set(SHADER_SOURCE_DIR ${RPE_SHADER_DIRECTORY})
set(SHADER_BINARY_DIR ${RPE_SHADER_SIPRV_DIRECTORY})
file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

file(GLOB SHADERS
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
  ${SHADER_SOURCE_DIR}/*.tesc
  ${SHADER_SOURCE_DIR}/*.tese
  ${SHADER_SOURCE_DIR}/*.mesh
  ${SHADER_SOURCE_DIR}/*.task
  ${SHADER_SOURCE_DIR}/*.rgen
  ${SHADER_SOURCE_DIR}/*.rchit
  ${SHADER_SOURCE_DIR}/*.rmiss)

#add_custom_command(
#  COMMAND
#    ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
#  OUTPUT ${SHADER_BINARY_DIR}
#  COMMENT "Creating ${SHADER_BINARY_DIR}"
#)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${GLSLANG_EXE}
      -V
      -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source} ${SHADER_BINARY_DIR}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_SHADERS})