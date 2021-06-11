idf_component_register(
  SRCS
  "bt_gap.cpp"
  "bt_spp.cpp"
  "bt.cpp"
  "main.cpp"
  "uart.cpp"
  INCLUDE_DIRS
  "./include")

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
