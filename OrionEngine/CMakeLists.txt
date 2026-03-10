cmake_minimum_required(VERSION 3.23)

project(OrionEngine)

    # include all external dependency libraries' CMake files to build them
    add_subdirectory("engine/external/GLFW")

    # Engine .dll
    add_library(Engine SHARED)

        # sets output directory to something like OrionEngine/bin/Debug-x64-Windows
        set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>-$<PLATFORM_ID>-$<HOST_SYSTEM_PROCESSOR>")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

        # source files that define the engine library
        target_sources(OrionEngine PRIVATE
            "engine/src/Core/Console.cpp"

            "engine/src/ECS/ECS.cpp"
            
            "engine/src/Renderer/Renderer.cpp"
            "engine/src/Renderer/Mesh.cpp"

            "engine/src/Scene/Scene.cpp"

            "engine/external/Glad/src/glad.c" # compiles glad directly
        )

        target_link_libraries(OrionEngine PUBLIC
            glfw
        )

        # include all headers from the include directory
        target_include_directories(OrionEngine PUBLIC 
            "engine/include"
            "engine/external/Glad/include"
        )

    # Editor .exe
    add_executable(Editor "editor/src/main.cpp")

        target_include_directories(Editor PRIVATE
            "engine/src"
        )
        
        # link Editor.exe to Engine.dll
        target_link_libraries(Editor PRIVATE Engine)