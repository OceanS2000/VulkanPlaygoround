add_library(vma OBJECT
        vma/vm_mem_alloc.cpp)

target_compile_definitions(vma PRIVATE
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=1)
target_include_directories(vma PUBLIC
        vma)
