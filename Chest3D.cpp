#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "cylinder.h"

using namespace std; // Standard namespace

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "CS 330 -  (Diego Bez Zambiazzi)"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint VAO;         // Handle for the vertex array object
        GLuint VBO;         // Handle for the vertex buffer object
        GLuint EBO;         // Handle for the element buffer object
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;
    // Mesh data
    GLMesh chestBodyMesh;
    GLMesh chestDecorMesh;

}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UCreateChestBodyMesh(GLMesh& mesh);
void UCreateChestDecorMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // build and compile our shader zprogram
    // ------------------------------------
    Shader myShader("shaderfiles/7.3.camera.vs", "shaderfiles/7.3.camera.fs");

    // Create the mesh
    UCreateChestBodyMesh(chestBodyMesh);
    UCreateChestDecorMesh(chestDecorMesh);

    static_meshes_3D::Cylinder cylinder(0.25, 20, 1.0, true, true, true);
    // Cylinder
    unsigned int cylinderVAO, cylinderVBO;

    glGenVertexArrays(1, &cylinderVAO);
    glBindVertexArray(cylinderVAO);
    glGenBuffers(1, &cylinderVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);

    // load and create a texture 
      // -------------------------
    unsigned int texture1, texture2;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* data = stbi_load("wood.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 2
    // ---------
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load("metal.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    myShader.use();
    myShader.setInt("texture1", 0);
    myShader.setInt("texture2", 1);
            
    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        // Enable z-depth
        glEnable(GL_DEPTH_TEST);

        // Clear the frame and z buffers
        glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Transforms the camera: move the camera back (z axis)
        glm::mat4 viewTranslation = glm::translate(glm::vec3(0.0f, 0.0f, -3.0f));
        // Rotates the view around the X axis
        glm::mat4 viewRotation = glm::rotate(3.141592f * 0.15f, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 view = viewTranslation * viewRotation;
        myShader.setMat4("view", view);
        // Creates a perspective projection
        glm::mat4 projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        myShader.setMat4("projection", projection);

        glm::mat4 scale = glm::scale(glm::vec3(1.5f, 2.0f, 2.0f));
        glm::mat4 rotation = glm::rotate(-3.141592f * 0.15f, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 model = translation * rotation * scale;
        myShader.setMat4("model", model);
        // Activate the VBOs contained within the mesh's VAO, draw elements, and deactivate the VAO
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(chestBodyMesh.VAO);
        glDrawElements(GL_TRIANGLES, chestBodyMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        translation = glm::translate(glm::vec3(0.0f, 0.39f, 0.0f));
        model = translation * rotation * scale;
        myShader.setMat4("model", model);
        glBindVertexArray(chestDecorMesh.VAO);
        glDrawElements(GL_TRIANGLES, chestDecorMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);

        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(cylinderVAO);
        scale = glm::scale(glm::vec3(1.0f, 1.5f, 2.0f));
        translation = glm::translate(glm::vec3(0.0f, 0.5f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(-3.141592f * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
        model = translation * rotation * rotationZ * scale;
        myShader.setMat4("model", model);
        
        
        cylinder.render();
        //glBindVertexArray(0);
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(chestBodyMesh);
    UDestroyMesh(chestDecorMesh);

    glDeleteVertexArrays(1, &cylinderVAO);
    glDeleteBuffers(1, &cylinderVBO);


    exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Implements the UCreateMesh function
void UCreateChestBodyMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat chestBodyV[] = {
        // Vertex Positions      // Colors (r,g,b,a)        // Texture Coords
         0.5f,  0.25f, -0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    1.0, 1.0,    // Vertex 0 Back Top Right
         0.5f, -0.25f, -0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    1.0, 0.0,    // Vertex 1 Back Bottom Right
        -0.5f, -0.25f, -0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    0.0, 0.0,    // Vertex 2 Back Bottom Left
        -0.5f,  0.25f, -0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    0.0, 1.0,    // Vertex 3 Back Top Left
         0.5f,  0.25f,  0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    1.0, 1.0,    // Vertex 4 Front Top Right
         0.5f, -0.25f,  0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    1.0, 0.0,    // Vertex 5 Front Bottom Right
        -0.5f, -0.25f,  0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    0.0, 0.0,    // Vertex 6 Front Bottom Left
        -0.5f,  0.25f,  0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    0.0, 1.0,    // Vertex 7 Front Top Left
    };

    // Index data to share position data
    GLushort chestBodyI[] = {
        0, 1, 2, 0, 2, 3, // Box Back       Triangles 1 and 2
        0, 1, 5, 0, 4, 5, // Box Right Side Triangles 3 and 4
        0, 3, 7, 0, 4, 7, // Box Top        Triangles 5 and 6
        1, 2, 6, 1, 5, 6, // Box Bottom     Triangles 7 and 8
        2, 3, 7, 2, 6, 7, // Box Left Side  Triangles 9 and 10
        4, 5, 6, 4, 6, 7, // Box Front      Triangles 11 and 12
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTex = 2;

    glGenVertexArrays(1, &mesh.VAO); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.VAO);
    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(chestBodyV), chestBodyV, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(chestBodyI) / sizeof(chestBodyI[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(chestBodyI), chestBodyI, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 8 (x, y, z, r, g, b, a, tc1, tc2). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor + floatsPerTex);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * (floatsPerVertex + floatsPerColor)));
    glEnableVertexAttribArray(2);

}

void UCreateChestDecorMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat chestDecorV[] = {
        // Vertex Positions        // Colors (r,g,b,a)           // Texture coords 
         0.51f,  0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 0 Back Top Right
         0.51f, -0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 1 Back Bottom Right
        -0.51f, -0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 2 Back Bottom Left
        -0.51f,  0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 3 Back Top Left
         0.51f,  0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 4 Front Top Right
         0.51f, -0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 5 Front Bottom Right
        -0.51f, -0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 6 Front Bottom Left
        -0.51f,  0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0, 1.0,    // Vertex 7 Front Top Left
    };

    // Index data to share position data
    GLushort chestDecorI[] = {
        0, 1, 2, 0, 2, 3, // Box Back       Triangles 1 and 2
        0, 1, 5, 0, 4, 5, // Box Right Side Triangles 3 and 4
        0, 3, 7, 0, 4, 7, // Box Top        Triangles 5 and 6
        1, 2, 6, 1, 5, 6, // Box Bottom     Triangles 7 and 8
        2, 3, 7, 2, 6, 7, // Box Left Side  Triangles 9 and 10
        4, 5, 6, 4, 6, 7, // Box Front      Triangles 11 and 12
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;
    const GLuint floatsPerTex = 2;

    glGenVertexArrays(1, &mesh.VAO); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.VAO);
    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(chestDecorV), chestDecorV, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(chestDecorI) / sizeof(chestDecorI[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(chestDecorI), chestDecorI, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 8 (x, y, z, r, g, b, a, tc1, tc2). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor + floatsPerTex);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * (floatsPerVertex + floatsPerColor)));
    glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
    glDeleteBuffers(1, &mesh.EBO);
}
