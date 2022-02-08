add_library(vma STATIC
        vma/vm_mem_alloc.cpp)

target_compile_definitions(vma PRIVATE
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1)
target_include_directories(vma PUBLIC
        vma)

add_library(stb STATIC
        stb/stb_image.c)
target_include_directories(stb PUBLIC
        stb)
