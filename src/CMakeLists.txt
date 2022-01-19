add_library(BaseEngine OBJECT
        BaseEngine/BaseEngine.cpp
        BaseEngine/Debug.cpp
        BaseEngine/ChosenGPU.cpp
        BaseEngine/Presenter.cpp
        BaseEngine/Presenter.hpp
        )
target_link_libraries(BaseEngine
        SDL2::SDL2
        fmt::fmt
        spdlog::spdlog
        )
target_include_directories(BaseEngine PRIVATE
        ${Vulkan_INCLUDE_DIRS}
        )

target_include_directories(BaseEngine PUBLIC
        BaseEngine
        )
target_compile_definitions(BaseEngine PUBLIC
        VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
        )