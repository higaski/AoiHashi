file(GLOB_RECURSE SRC *.cpp)
idf_component_register(
  SRCS
  ${SRC}
  INCLUDE_DIRS
  .
  REQUIRES
  bt
  driver
  nvs_flash)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
