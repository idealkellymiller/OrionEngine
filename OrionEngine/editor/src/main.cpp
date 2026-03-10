#include <Orion.h>

// Prototypes
unsigned int make_shader(const std::string& vertex_filepath, const std::string& fragment_filepath);
unsigned int make_module(const std::string& filepath, unsigned int module_type);


int main() {
	// 0) GLFW Window Initialization
	if (!glfwInit())
		return -1;

	GLFWwindow* window = glfwCreateWindow(800, 600, "Orion Editor", nullptr, nullptr);

	if (!window) {
		glfwTerminate();
		return -1;
	}

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return -1;
    }

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f); // RGBA
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);


	// 1) Renderer Initialization
	Renderer::Init();
	
	// Create a triangle (mesh) object
	//Mesh* triangle = new Mesh();

	// create the shader (vertex and fragment)
    /*
	unsigned int shader = make_shader(
		"../../engine/shaders/vertex.glsl",
        "../../engine/shaders/fragment.glsl"
	);
    */



	// 2) Main Loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();


	}


	// 3) Shutdown
    Renderer::Shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();
    return 0;
}


unsigned int make_shader(const std::string& vertex_filepath, const std::string& fragment_filepath) {

    std::vector<unsigned int> modules;
    modules.push_back(make_module(vertex_filepath, GL_VERTEX_SHADER));
    modules.push_back(make_module(fragment_filepath, GL_FRAGMENT_SHADER));

    unsigned int shader = glCreateProgram();
    for (unsigned int shaderModule : modules) {
        glAttachShader(shader, shaderModule);
    }
    glLinkProgram(shader);

    // Check for error
    int success;
    glGetProgramiv(shader, GL_LINK_STATUS, &success);
    if (!success) {
        char errorLog[1024];
        glGetProgramInfoLog(shader, 1024, NULL, errorLog);
        std::cout << "Shader Linking error:\n" << errorLog << std::endl;
    }

    for (unsigned int shaderModule : modules) {
        glDeleteShader(shaderModule);
    }

    return shader;
}

// Takes a filepath and makes a Shader Module
unsigned int make_module(const std::string& filepath, unsigned int module_type) {
    // Load file into a string
    std::ifstream file;
    std::stringstream bufferedLines;
    std::string line;

    file.open(filepath);
    while (std::getline(file, line)) {
        bufferedLines << line << "\n";
    }
    std::string shaderSource = bufferedLines.str();
    // convert from C++ standard library string into a C style character pointer string
    const char* shaderSrc = shaderSource.c_str();
    bufferedLines.str("");
    file.close();

    // compile the shader module
    unsigned int shaderModule = glCreateShader(module_type);
    glShaderSource(shaderModule, 1, &shaderSrc, NULL);
    glCompileShader(shaderModule);

    // Check if that compilation was successful
    int success;
    glGetShaderiv(shaderModule, GL_COMPILE_STATUS, &success);
    if (!success) {
        char errorLog[1024];
        glGetShaderInfoLog(shaderModule, 1024, NULL, errorLog);
        std::cout << "Shader Module compilation error:\n" << errorLog << std::endl;
    }

    return shaderModule;
}