cmake_minimum_required (VERSION 3.30)

set(SHADERS shader.vert shader.frag)

foreach(SHADER ${SHADERS})
    get_filename_component(NAME ${SHADER} NAME)
    set(SPV ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.spv)

    add_custom_command(
        OUTPUT ${SPV}
        COMMAND glslc ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} -o ${SPV}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}
        COMMENT "Compiling ${SHADER}"
    )

    list(APPEND SPV_OUTPUTS ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPV_OUTPUTS})
