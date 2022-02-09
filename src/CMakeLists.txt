add_library(BaseEngine OBJECT
        BaseEngine/BaseEngine.cpp
        BaseEngine/Debug.cpp
        BaseEngine/ChosenGPU.cpp
        BaseEngine/Presenter.cpp
        BaseEngine/DefaultPipeline.cpp

        AssetsManager/ShaderModule.cpp
        AssetsManager/TextureModule.cpp
        AssetsManager/OneTimeCommand.cpp
        )
target_link_libraries(BaseEngine
        SDL2::SDL2
        fmt::fmt
        spdlog::spdlog
        glm::glm
        vma
        stb
        )
target_include_directories(BaseEngine PUBLIC
        ${Vulkan_INCLUDE_DIRS}
        BaseEngine
        AssetsManager
        )

target_include_directories(BaseEngine PUBLIC
        BaseEngine
        )
target_compile_definitions(BaseEngine PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        )
target_precompile_headers(BaseEngine PRIVATE
        <algorithm>
        <vector>
        <array>
        <vulkan/vulkan.hpp>
        <spdlog/spdlog.h>
        )