#include "Renderer/Mesh.h"
#include <vector>


// Constructor
Mesh::Mesh() {
    // Screen coords are 0,0 in the center and 1,1 in the corner (NDC = Normalized Device Coordinates)
    std::vector<float> data = {
        // x      y     z        R     G     B
        -.5f, -0.5f, 0.0f,      1.0f, 0.0f, 0.0f,   // bottom left  red
         0.7f, 0.0f, 0.0f,      0.0f, 1.0f, 0.0f,   // bottom right green
         -0.1f,  0.5f, 0.0f,    0.0f, 0.0f, 1.0f   // top left     blue
    };
    vertex_count = 3;

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &VBO);  // Generate a buffer (of vertices)
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // put VBO on this worktable (buffer)
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float),
        data.data(), GL_STATIC_DRAW);


    // Atribute pointers are telling the gpu how to interperet the data it receives.
    // The GPU recieves a VBO (vertex buffer), which is just a string of 1s and 0s.  
    // (Attribute number, number of numbers in each attribute, data format, whether we should normalize it, the stride in bytes to get to each vertex, the offset to the beginning of the attribute)
    //position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glEnableVertexAttribArray(0);

    //color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12); // 12 bytes here because the color starts after the first 3 floats (position) in the vertex
    glEnableVertexAttribArray(1);
}

void Mesh::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}

// Deconstructor
Mesh::~Mesh() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// A Vertex Buffer (VBO) is the data (raw binary) and attribute pointers (which are indicating what the data is)
// A Vertex Array (VAO) wraps up all of the stuff needed to draw this (vertex buffer, as well as its attributes)