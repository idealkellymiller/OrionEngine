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

    Renderer::SetClearColor(0.6f, 0.6f, 0.6f, 1.0f);








    Mesh cubeMesh;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    OBJLoader::Load("C:/dev/OrionRenderer/engine/engineAssets/primitives/cube.obj", vertices, indices);
    if (!cubeMesh.Create(vertices, indices)) {
        std::cout << "Couldn't make cubeMesh\n";
        return -1;
    }

    Mesh monkeyMesh;
    OBJLoader::Load("C:/dev/OrionRenderer/engine/engineAssets/primitives/monkey.obj", vertices, indices);
    if (!monkeyMesh.Create(vertices, indices)) {
        std::cout << "Couldn't make monkeyMesh\n";
        return -1;
    }

    //Mesh eratoMesh;
    //OBJLoader::Load("C:/dev/OrionRenderer/engine/engineAssets/primitives/erato.obj", vertices, indices);
    //if (!eratoMesh.Create(vertices, indices)) {
    //    std::cout << "Couldn't make eratoMesh\n";
    //    return -1;
    //}







    //Shader litShader;
    //if (!litShader.CreateFromFiles(
    //    "C:/dev/OrionRenderer/engine/engineAssets/shaders/Lit.vert",
    //    "C:/dev/OrionRenderer/engine/engineAssets/shaders/Lit.frag"))
    //{
    //    std::cout << "Failed to create lit Shader\n";
    //    Renderer::Shutdown();
    //    glfwDestroyWindow(window);
    //    glfwTerminate();
    //    return -1;
    //}

    //Shader shadowShader;
    //if (!shadowShader.CreateFromFiles(
    //    "C:/dev/OrionRenderer/engine/engineAssets/shaders/Shadow.vert",
    //    "C:/dev/OrionRenderer/engine/engineAssets/shaders/Shadow.frag"))
    //{
    //    std::cout << "Failed to create shadow shader\n";
    //    Renderer::Shutdown();
    //    glfwDestroyWindow(window);
    //    glfwTerminate();
    //    return -1;
    //}
    //// Send shadow shader reference to the renderer.
    //Renderer::SetShadowShader(&shadowShader);





    Texture stoneTex;
    if (!stoneTex.LoadFromFile("C:/dev/OrionRenderer/engine/engineAssets/textures/mossy_stones.png")) {
        std::cout << "Failed to load texture\n";
    }

    Material defaultMat;
    defaultMat.SetShader(Renderer::GetLitShader());
    defaultMat.SetDiffuseTexture(nullptr);
    defaultMat.SetColor(glm::vec4(0.9f, 0.9f, 0.9f, 1.0f));
    defaultMat.SetSpecularColor(glm::vec3(0.5f, 0.5f, 0.5f));      // painted/plastic-ish
    defaultMat.SetShininess(32.0f);
    defaultMat.SetTransparent(false);

    Material stoneMat;
    stoneMat.SetShader(Renderer::GetLitShader());
    stoneMat.SetDiffuseTexture(&stoneTex);
    stoneMat.SetColor(glm::vec4(1.0f)); // white tint = original texture colors
    stoneMat.SetSpecularColor(glm::vec3(0.15f, 0.15f, 0.15f));    // stone = low specular
    stoneMat.SetShininess(8.0f);                                  // broad dull highlight
    stoneMat.SetTransparent(false);

    Material glassMat;
    glassMat.SetShader(Renderer::GetLitShader());
    glassMat.SetDiffuseTexture(nullptr);
    glassMat.SetColor(glm::vec4(0.0157f, 0.9059f, 1.0f, 0.3f));
    glassMat.SetSpecularColor(glm::vec3(1.0f, 1.0f, 1.0f));     // glass = strong highlight
    glassMat.SetShininess(128.0f);                              // tight sharp highlight
    glassMat.SetTransparent(true);




    // Create and configure camera once.
    Camera camera;
    camera.SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
    camera.SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));

    EditorCamera editorCamera;
    editorCamera.SetPosition(glm::vec3(0.0f, 2.0f, 8.0f));
    editorCamera.SetYawPitch(-90.0f, -10.0f);




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
            camera,
            deltaTime,
            viewportHovered,
            viewportFocused
        );


        // Update camera projection if window size changes
        float aspectRatio = (float)viewportFramebuffer.GetWidth() / (float)viewportFramebuffer.GetHeight();
        camera.SetPerspective(45.0f, aspectRatio, 0.1f, 100.0f);

        RenderScene renderScene;
        renderScene.SetActiveCamera(&camera);


        // Create a sun-like directional light.
        DirectionalLight sun;
        sun.Direction = glm::vec3(-0.5f, -1.0f, -0.2f);
        sun.Color = glm::vec3(1.0f, 0.95f, 0.85f);
        sun.Intensity = 1.2f;

        renderScene.SetDirectionalLight(sun);


        Renderable stone;
        stone.MeshPtr = &cubeMesh;
        stone.MaterialPtr = &stoneMat;
        glm::mat4 modelA = glm::mat4(1.0f);
        modelA = glm::translate(modelA, glm::vec3(sin((float)glfwGetTime() * 3), -1.0f, 0.0f));
        modelA = glm::rotate(modelA, (float)glfwGetTime(), glm::vec3(3.0f, 2.5f, 2.0f));
        modelA = glm::scale(modelA, glm::vec3(1.0f, 1.0f, 1.0f));
        stone.ModelMatrix = modelA;
        renderScene.AddRenderable(stone);

        Renderable monkey;
        monkey.MeshPtr = &monkeyMesh;
        monkey.MaterialPtr = &glassMat;
        glm::mat4 modelB = glm::mat4(1.0f);
        modelB = glm::translate(modelB, glm::vec3(3.5f, 0.0f, -2.0f));
        modelB = glm::rotate(modelB, cos((float)glfwGetTime() * 3), glm::vec3(0.0f, 1.0f, 0.0f));
        modelB = glm::scale(modelB, glm::vec3(1.0f, 1.0f, 1.0f));
        monkey.ModelMatrix = modelB;
        renderScene.AddRenderable(monkey);

        Renderable floor;
        floor.MeshPtr = &cubeMesh;
        floor.MaterialPtr = &defaultMat;
        glm::mat4 modelC = glm::mat4(1.0f);
        modelC = glm::translate(modelC, glm::vec3(0.0f, -4.0f, 0.0f));
        modelC = glm::rotate(modelC, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        modelC = glm::scale(modelC, glm::vec3(5.0f, 1.0f, 5.0f));
        floor.ModelMatrix = modelC;
        renderScene.AddRenderable(floor);

        Renderable box;
        box.MeshPtr = &cubeMesh;
        box.MaterialPtr = &defaultMat;
        glm::mat4 modelD = glm::mat4(1.0f);
        modelD = glm::translate(modelD, glm::vec3(3.5f, -2.0f, -2.0f));
        modelD = glm::rotate(modelD, 0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
        modelD = glm::scale(modelD, glm::vec3(1.0f, 1.0f, 1.0f));
        box.ModelMatrix = modelD;
        renderScene.AddRenderable(box);


        Renderer::Render(renderScene);
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