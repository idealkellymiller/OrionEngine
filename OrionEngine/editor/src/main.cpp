#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


#include "Renderer.hpp"
#include "EditorCamera.hpp"
#include "EditorCameraInput.hpp"
#include "AssetManager.h"
#include "SceneManager.h"


// For EditorCamera scrolling ot change speed
void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    (void)window;
    (void)xOffset;

    // Store wheel delta so editor camerae can use it during update
    EditorCameraInput::AddScrollDelta(static_cast<float>(yOffset));
}



int main()
{
    // Initialize GLFW.
    // GLFW handles window creation, input, and OpenGL context creation.
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }

    // Tell GLFW what kind of OpenGL context we want.
    // Here we ask for OpengGL 4.6 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create the main application window.
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Orion Editor", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    // For editor camera scroll zooming
    glfwSetScrollCallback(window, ScrollCallback);

    // Make this window's OpenGL context current on this thread.
    glfwMakeContextCurrent(window);

    // Load OpenGL function pointers through GLAD.
    // Without this, functions like glClear, glEnable, glViewport, etc.
    // may be null/unavailable.
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Enable VSync.
    // With VSync on, buffer swaps wiat for the monitor refresh.
    // So if your display is 60 Hz, the game usually presents around 60 fps isntead 800+ fps.
    // 1 means wait for 1 vertical refresh before swapping buffers.
    glfwSwapInterval(1);





    // ------- ImGui --------
        // Create the Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Access global ImGui IO settings
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Enable keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Enable docking so windows can snap together
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Optional: enable multi-viewport support later
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Use ImGui's dark color theme
    ImGui::StyleColorsDark();

    // Initialize platform backend (GLFW)
    ImGui_ImplGlfw_InitForOpenGL(window, true);

    // Initialize renderer backend (OpenGL3)
    // "#version 460" matches OpenGL 4.6 shaders
    ImGui_ImplOpenGL3_Init("#version 460");

    // Create the editor viewport framebuffer
    Framebuffer viewportFramebuffer;
    viewportFramebuffer.Create(1280, 720);







    // Initialize the renderer after OpenGL is available.
    if (!Renderer::Init()) {
        std::cout << "Renderer init failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }








    EditorCamera editorCamera;
    editorCamera.SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
    editorCamera.SetYawPitch(-90.0f, -10.0f);


    // Load assets
    // Working directory is "OrionEngine/editor/"
    AssetManager::SetAssetsFolderPath("assets\\");
    AssetManager::LoadAssetsFolder();

    // Load scene
    SceneManager::LoadScene(AssetManager::GetAssetsFolderPath() + "default.scene");


    // Game loop
    while (!glfwWindowShouldClose(window))
    {
        // -------- UI BUILD --------
        // Process OS/window/input events
        glfwPollEvents();

        // Start a new ImGui frame for both backends
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a fullscreen dockspace window (dockspace root)
        {
            ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_MenuBar |
                ImGuiWindowFlags_NoDocking;

            // Get main viewport so this fills the whole app window
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);

            windowFlags |= ImGuiWindowFlags_NoTitleBar;
            windowFlags |= ImGuiWindowFlags_NoCollapse;
            windowFlags |= ImGuiWindowFlags_NoResize;
            windowFlags |= ImGuiWindowFlags_NoMove;
            windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
            windowFlags |= ImGuiWindowFlags_NoNavFocus;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            ImGui::Begin("DockSpaceRoot", nullptr, windowFlags);

            ImGui::PopStyleVar(2);

            // Create the docking space node
            ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
            ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

            // Simple menu bar
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Exit")) {
                        glfwSetWindowShouldClose(window, true);
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        // Viewport window
        static ImVec2 viewportSize = ImVec2(0.0f, 0.0f);
        bool viewportHovered = false;
        bool viewportFocused = false;

        ImGui::Begin("Viewport");

        viewportHovered = ImGui::IsWindowHovered();
        viewportFocused = ImGui::IsWindowFocused();

        // Get the size available inside this window for content
        viewportSize = ImGui::GetContentRegionAvail();

        // Prevent weird zero-cases when minimized/collapsed
        if (viewportSize.x < 1.0f) viewportSize.x = 1.0f;
        if (viewportSize.y < 1.0f) viewportSize.y = 1.0f;

        // Resize the framebuffer if panel size changed
        viewportFramebuffer.Resize(
            static_cast<unsigned int>(viewportSize.x),
            static_cast<unsigned int>(viewportSize.y)
        );

        // Show the framebuffer's color texture inside ImGui
        // ImGui uses ImTextureID, and for OpenGL that is just the texture handle cast.
        // UVs are flipped vertically because OpenGL texture origin is bottom-left,
        // while ImGui expects top-left style display.
        ImGui::Image(
            (ImTextureID)(intptr_t)viewportFramebuffer.GetColorAttachment(),
            ImVec2(viewportSize.x, viewportSize.y),
            ImVec2(0, 1),   // UV top-left
            ImVec2(1, 0)    // UV bottom-right
        );

        ImGui::End();
        

        // Inspector window
        {
            ImGui::Begin("Inspector");
            ImGui::Text("Selected object info goes here.");
            ImGui::Separator();

            static float testValue = 1.0f;
            ImGui::DragFloat("Test Value", &testValue, 0.1f);

            ImGui::End();
        }

        // Example hierarchy window
        {
            ImGui::Begin("Hierarchy");
            ImGui::Selectable("Camera");
            ImGui::Selectable("Cube");
            ImGui::Selectable("Light");
            ImGui::End();
        }

        // Tell ImGui to finalize all UI draw data
        ImGui::Render();





        // -------- VIEWPORT RENDER --------
        // Render scene into viewport framebuffer
        viewportFramebuffer.Bind();


        // Set viewport as the framebuffer size
        Renderer::SetViewport(0, 0,
            viewportFramebuffer.GetWidth(),
            viewportFramebuffer.GetHeight());

        glEnable(GL_DEPTH_TEST);
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);




        //// Keep viewport synced to the frame
        Renderer::SetViewport(0, 0, viewportFramebuffer.GetWidth(), viewportFramebuffer.GetHeight());
        Renderer::BeginFrame();

        static float lastFrameTime = 0.0f;
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;

        // Update editor camera using viewport interaction state
        editorCamera.Update(
            window,
            *Renderer::GetActiveCamera(),
            deltaTime,
            viewportHovered,
            viewportFocused
        );

        // Update camera projection if window size changes
        float aspectRatio = (float)viewportFramebuffer.GetWidth() / (float)viewportFramebuffer.GetHeight();
        Renderer::GetActiveCamera()->SetPerspective(45.0f, aspectRatio, 0.1f, 100.0f);


        Renderer::Render();
        Renderer::EndFrame();

        viewportFramebuffer.Unbind();



        // -------- UI RENDERER --------
        // Render ImGui to the actual window backbuffer
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);

        // Set viewport back to the real windwo size
        glViewport(0, 0, displayW, displayH);
        // Usually editor background does not need depth test
        glDisable(GL_DEPTH_TEST);

        // Clear the actual screen
        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw all ImGui windows, including the Viewport image
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Present the final image
        glfwSwapBuffers(window);
    }





    // Shutdown ImGui backends
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    // Destroy ImGui context
    ImGui::DestroyContext();

    // Destroy GLFW window and terminate GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
    
    
}