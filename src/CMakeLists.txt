add_library(AssetsManager OBJECT
        AssetsManager/ShaderModule.cpp
        AssetsManager/TextureModule.cpp
        AssetsManager/OneTimeCommand.cpp
        )
target_link_libraries(AssetsManager
        fmt::fmt
        spdlog::spdlog
        glm::glm
        vma
        stb
        )
target_compile_definitions(AssetsManager PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        )

add_library(BaseEngine OBJECT
        BaseEngine/BaseEngine.cpp
        BaseEngine/Debug.cpp
        BaseEngine/ChosenGPU.cpp
        BaseEngine/Presenter.cpp
        BaseEngine/DefaultPipeline.cpp
        )
target_link_libraries(BaseEngine
        SDL2::SDL2
        fmt::fmt
        spdlog::spdlog
        vma
        stb
        $<TARGET_OBJECTS:AssetsManager>
        )
target_include_directories(BaseEngine PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        AssetsManager
        )

target_include_directories(BaseEngine PUBLIC
        BaseEngine
        )
target_compile_definitions(BaseEngine PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        )