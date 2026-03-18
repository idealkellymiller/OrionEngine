cmake_minimum_required(VERSION 3.23)
# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
project(OrionEngine LANGUAGES CXX C)

    # include all external dependency libraries' CMake files to build them

    add_library(Glad STATIC "engine/external/Glad/src/glad.c")
        target_include_directories(Glad PUBLIC 
        "engine/external/Glad/include"
        "engine/external/Glad/include/glad"
        "engine/external/Glad/include/KHR"
    )

    add_subdirectory("engine/external/GLFW")
    add_subdirectory("engine/external/spdlog")

    # Engine .dll
    add_library(Engine SHARED)

        # sets output directory to something like OrionEngine/bin/Debug-x64-Windows
        set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>-${CMAKE_GENERATOR_PLAFORM}-$<PLATFORM_ID>")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

        # include all headers from the include directory
        target_include_directories(Engine 
        PUBLIC 
            "engine/include"
        PRIVATE
            "engine/src"
        )

        # source files that define the engine library 
        # (PUT ALL OUR WRITTEN .CPPs HERE)
        target_sources(Engine PRIVATE
            "engine/src/Core/Console.cpp"
            "engine/src/Application.cpp"
            "engine/src/AppWindow.cpp"

            "engine/src/Layers/Layer.cpp"
            "engine/src/Layers/LayerStack.cpp"

            "engine/src/Log/Log.cpp"
        )

        target_compile_definitions(Engine PRIVATE ORN_BUILD_DLL
        PUBLIC
            ORN_PLATFORM_WINDOWS
            ORN_ENABLE_ASSERTS
        )

        target_compile_definitions(Engine INTERFACE ORION_PLATFORM_WINDOWS)

        target_link_libraries(Engine PUBLIC
            Glad
            glfw
            spdlog
        )

        get_target_property(OUT_DEFS Engine COMPILE_DEFINITIONS)
        message(STATUS "Engine Definitions: ${OUT_DEFS}")

    # Editor .exe
    add_executable(Editor "editor/src/main.cpp")
        
        target_include_directories(Editor PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
        
        # link Editor.exe to Engine.dll
        target_link_libraries(Editor PRIVATE Engine)

# this post-build command copies Engine.dll to the dir where Editor.exe is
# for easy Windows execution!
add_custom_command(TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Engine>
    $<TARGET_FILE_DIR:Editor>
)