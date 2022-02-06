add_library(AssetsManager OBJECT
        AssetsManager/ShaderModule.cpp
        )
target_link_libraries(AssetsManager
        fmt::fmt
        spdlog::spdlog
        glm::glm
        )

add_library(BaseEngine OBJECT
        BaseEngine/BaseEngine.cpp
        BaseEngine/Debug.cpp
        BaseEngine/ChosenGPU.cpp
        BaseEngine/Presenter.cpp
        BaseEngine/DefaultPipeline.cpp
        BaseEngine/Scene.cpp
        )
target_link_libraries(BaseEngine
        SDL2::SDL2
        fmt::fmt
        spdlog::spdlog
        vma
        $<TARGET_OBJECTS:AssetsManager>
        )
target_include_directories(BaseEngine PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        AssetsManager
        )

target_include_directories(BaseEngine PUBLIC
        BaseEngine
        )
target_compile_definitions(BaseEngine PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        )