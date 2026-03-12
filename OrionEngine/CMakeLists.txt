cmake_minimum_required(VERSION 3.23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
project(OrionEngine LANGUAGES CXX C)

    # include all external dependency libraries' CMake files to build them
    add_subdirectory("engine/external/GLFW")

    add_library(Glad STATIC "engine/external/Glad/src/glad.c")
        target_include_directories(Glad PUBLIC 
        "engine/external/Glad/include"
        "engine/external/Glad/include/glad"
        "engine/external/Glad/include/KHR"
    )

    # Engine .dll
    add_library(Engine SHARED)

        # sets output directory to something like OrionEngine/bin/Debug-x64-Windows
        set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>-${CMAKE_GENERATOR_PLAFORM}-$<PLATFORM_ID>")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

        # source files that define the engine library 
        # (PUT ALL OUR WRITTEN .CPPs HERE)
        target_sources(Engine PRIVATE
            "engine/src/Core/Console.cpp"

            "engine/src/ECS/ECS.cpp"
            
            "engine/src/Renderer/Renderer.cpp"
            "engine/src/Renderer/Mesh.cpp"

            "engine/src/Scene/Scene.cpp"
        )

        target_link_libraries(Engine PUBLIC
            glfw
            Glad
        )

        # include all headers from the include directory
        target_include_directories(Engine 
        PUBLIC 
            "engine/include"
        PRIVATE
            "engine/src"
        )

    # Editor .exe
    add_executable(Editor "editor/src/main.cpp")

        target_include_directories(Editor PRIVATE
            "engine/src"
        )
        
        # link Editor.exe to Engine.dll
        target_link_libraries(Editor PRIVATE 
            Engine)

# this post-build command copies Engine.dll to the dir where Editor.exe is
# for easy Windows execution!
add_custom_command(TARGET Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:Engine>
    $<TARGET_FILE_DIR:Editor>
)