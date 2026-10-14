idf_component_register(SRCS "framework_memory.c" INCLUDE_DIRS "." "../hal/interface")
target_compile_features(${COMPONENT_LIB} PRIVATE c_std_11)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    FRAMEWORK_MEMORY_CAPACITY=${CONFIG_FRAMEWORK_MEMORY_CAPACITY})
