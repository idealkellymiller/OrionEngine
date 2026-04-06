cmake_minimum_required(VERSION 3.23)
# set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
project(OrionEngine LANGUAGES CXX C)

    set(CMAKE_CXX_STANDARD 20)

    # include all external dependency libraries' CMake files to build them

    add_library(Glad STATIC "engine/external/Glad/src/glad.c")
        target_include_directories(Glad PUBLIC 
            "engine/external/Glad/include"
            "engine/external/Glad/include/glad"
            "engine/external/Glad/include/KHR"
        )

   set(IMGUI_PATH "engine/external/ImGUI")
    file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp)
    add_library(ImGUI STATIC ${IMGUI_SOURCES})
        target_include_directories(ImGUI PUBLIC ${IMGUI_PATH})

    add_subdirectory("engine/external/GLFW")
    add_subdirectory("engine/external/spdlog")
    add_subdirectory("engine/external/glm")
    add_subdirectory(engine/external/json-3.12.0)

    # Engine .dll
    add_library(Engine SHARED)

        # sets output directory to something like OrionEngine/bin/Debug-x64-Windows
        set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>-${CMAKE_GENERATOR_PLATFORM}-$<PLATFORM_ID>")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

        # include all headers from the include directory
        target_include_directories(Engine 
        PUBLIC 
            "engine/include"
            ${CMAKE_CURRENT_SOURCE_DIR}/engine/external
        PRIVATE
            "engine/src"
        )

        # source files that define the engine library 
        # (PUT ALL OUR WRITTEN .CPPs HERE)
        target_sources(Engine PRIVATE
            "engine/src/Application.cpp"
            "engine/src/AppWindow.cpp"

            "engine/src/Core/Console.cpp"

            "engine/src/Layers/Layer.cpp"
            "engine/src/Layers/LayerStack.cpp"
            "engine/src/Layers/ImGuiLayer.cpp"
            "engine/src/Layers/EditorLayer.cpp"
            "engine/src/Layers/RuntimeLayer.cpp"

            "engine/src/Log/Log.cpp"

            "engine/src/Renderer/Camera.cpp"
            "engine/src/Renderer/EditorCamera.cpp"
            "engine/src/Renderer/Framebuffer.cpp"
            "engine/src/Renderer/Frustum.cpp"
	        "engine/src/Renderer/Gizmo.cpp"
            "engine/src/Renderer/IndexBuffer.cpp"
            "engine/src/Renderer/Material.cpp"
            "engine/src/Renderer/Mesh.cpp"
            "engine/src/Renderer/OBJLoader.cpp"
            "engine/src/Renderer/Renderer.cpp"
            "engine/src/Renderer/RenderPass.cpp"
            "engine/src/Renderer/RenderScene.cpp"
            "engine/src/Renderer/Shader.cpp"
            "engine/src/Renderer/stb_image.cpp"
            "engine/src/Renderer/Texture.cpp"
            "engine/src/Renderer/VertexArray.cpp"
            "engine/src/Renderer/VertexBuffer.cpp"
            
            "engine/src/Assets/AssetManager.cpp"

            "engine/src/ECS/Scene.cpp"
            "engine/src/ECS/SceneManager.cpp"
        )

        target_compile_definitions(Engine 
        PRIVATE 
            ORN_BUILD_DLL
        PUBLIC
            ORN_PLATFORM_WINDOWS
            ORN_ENABLE_ASSERTS
        )

        

        target_compile_definitions(Engine INTERFACE ORION_PLATFORM_WINDOWS)

        target_link_libraries(Engine PUBLIC
            Glad
            ImGUI
            glfw
            spdlog
            nlohmann_json::nlohmann_json
        )

        get_target_property(OUT_DEFS Engine COMPILE_DEFINITIONS)
        message(STATUS "Engine Definitions: ${OUT_DEFS}")

    # Editor .exe
    add_executable(Editor "editor/src/main.cpp")
        
        target_include_directories(Editor PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/editor/src
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