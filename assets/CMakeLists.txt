set(shaders
        shaders/trig.frag
        shaders/trig.vert)

foreach(shader ${shaders})
    get_filename_component(file_name ${shader} NAME)
    set(output_path "${file_name}.spv")
    set(compiled_shaders ${compiled_shaders} ${output_path})
    set(compiled_shaders ${compiled_shaders} PARENT_SCOPE)
    add_custom_command(
            OUTPUT ${output_path}
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.2
                -MD -MF ${output_path}.d
                -o ${output_path} \"${CMAKE_CURRENT_SOURCE_DIR}/${shader}\"
            DEPFILE ${output_path}.d
    )
endforeach()

add_custom_target(shaders
        DEPENDS ${compiled_shaders}
        SOURCES ${shaders})