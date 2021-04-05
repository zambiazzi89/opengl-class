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

#include "cylinder.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

const char* const WINDOW_TITLE = "CS 330 Project - (Diego Bez Zambiazzi)"; // Macro for window title

// Variables for window width and height
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Ortho default is false
bool ortho = false;

// --- CAMERA ---

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UPWARD,
    DOWNWARD
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    glm::vec3 OrthoWorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        OrthoWorldUp = -up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        OrthoWorldUp = glm::vec3(-upX, -upY, -upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix(bool ortho)
    {
        if (ortho) {
            updateCameraVectors();
            return glm::lookAt(Position, Position + Front, Up);
        }
        else {
            return glm::lookAt(Position, Position + Front, Up);
        }
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UPWARD)
            if (ortho) {
                Position -= Up * velocity;
            }
            else {
                Position += Up * velocity;
            }
        if (direction == DOWNWARD)
            if (ortho) {
                Position += Up * velocity;
            }
            else {
                Position -= Up * velocity;
            }
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        // Mouse Scroll changes speed
        MovementSpeed += yoffset;
        if (MovementSpeed < 1.0f)
            MovementSpeed = 1.0f;
        if (MovementSpeed > 50.0f)
            MovementSpeed = 50.0f;
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        if (ortho) {
            Right = glm::normalize(glm::cross(Front, OrthoWorldUp));
        }
        else {
            Right = glm::normalize(glm::cross(Front, WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        }
        Up = glm::normalize(glm::cross(Right, Front));
    }
};

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));
float lastX = WINDOW_WIDTH / 2.0f;
float lastY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// Stores the GL data relative to a given mesh
struct GLMesh
{
    GLuint VAO;         // Handle for the vertex array object
    GLuint VBO;         // Handle for the vertex buffer object
    GLuint EBO;         // Handle for the element buffer object
    GLuint nIndices;    // Number of indices of the mesh
};

// Mesh data
GLMesh chestBodyMesh;
GLMesh chestDecorMesh;
GLMesh planeMesh;

// Texture ID
GLuint chestWoodTexture;
GLuint chestMetalTexture;
GLuint marbleTexture;
GLuint pinkMarbleTexture;
GLuint ornamentTexture;

// Shader program
GLuint shaderProgramId;

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
void UProcessInput(GLFWwindow* window);
void UCreateChestBodyMesh(GLMesh& mesh);
void UCreateChestDecorMesh(GLMesh& mesh);
void UCreatePlaneMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);


/* Vertex Shader Source Code*/
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 2) in vec2 textureCoordinate;

    out vec2 vertexTextureCoordinate;

    //Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
        vertexTextureCoordinate = textureCoordinate;
    }
);


/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    uniform sampler2D uTextureBase;
    uniform sampler2D uTextureExtra;
    uniform bool multipleTextures;

    void main()
    {
        fragmentColor = texture(uTextureBase, vertexTextureCoordinate);
        if (multipleTextures)
        {
            vec4 extraTexture = texture(uTextureExtra, vertexTextureCoordinate);
            if (extraTexture.a != 0.0)
                fragmentColor = mix(texture(uTextureBase, vertexTextureCoordinate), extraTexture, 0.2);
        }
    }
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}


int main(int argc, char* argv[])
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
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

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

    // Create the mesh
    UCreateChestBodyMesh(chestBodyMesh);
    UCreateChestDecorMesh(chestDecorMesh);
    UCreatePlaneMesh(planeMesh);

    // Cylinder
    static_meshes_3D::Cylinder cylinder(0.25, 20, 1.0, true, true, true);
    unsigned int cylinderVAO, cylinderVBO;

    glGenVertexArrays(1, &cylinderVAO);
    glBindVertexArray(cylinderVAO);
    glGenBuffers(1, &cylinderVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);


    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, shaderProgramId))
        return EXIT_FAILURE;

    // Load textures
    const char* texFilename = "wood.jpg";
    if (!UCreateTexture(texFilename, chestWoodTexture))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "metal.jpg";
    if (!UCreateTexture(texFilename, chestMetalTexture))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "marble.gif";
    if (!UCreateTexture(texFilename, marbleTexture))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "pinkMarble.jpg";
    if (!UCreateTexture(texFilename, pinkMarbleTexture))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    texFilename = "ornament.jpg";
    if (!UCreateTexture(texFilename, ornamentTexture))
    {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }
    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    glUseProgram(shaderProgramId);

    glUniform1i(glGetUniformLocation(shaderProgramId, "uTextureBase"), 0);
    glUniform1i(glGetUniformLocation(shaderProgramId, "uTextureExtra"), 1);

            
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        UProcessInput(window);

        // Clear the frame and z buffers
        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                
        // Retrieves and passes transform matrices to the Shader program
        GLint viewLoc = glGetUniformLocation(shaderProgramId, "view");
        GLint projLoc = glGetUniformLocation(shaderProgramId, "projection");
        GLint modelLoc = glGetUniformLocation(shaderProgramId, "model");
        GLuint multipleTexturesLoc = glGetUniformLocation(shaderProgramId, "multipleTextures");

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix(ortho);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 projection;
        // Creates a perspective projection
        // Condition if orthographic
        if (ortho) {
            float scale = 200;
            float scaledWidth = (GLfloat)WINDOW_WIDTH / scale;
            float scaledHeight = (GLfloat)WINDOW_HEIGHT/ scale;
            projection = glm::ortho(-scaledWidth, scaledWidth, scaledHeight, -scaledHeight, -4.0f, 10.0f);
        }
        else {
            projection = glm::perspective(45.0f, (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
        }
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Render Chest
        glm::mat4 scale = glm::scale(glm::vec3(1.5f, 2.0f, 2.0f));
        glm::mat4 rotation = glm::rotate(-3.141592f * 0.15f, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 translation = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 model = translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        // Activate the VBOs contained within the mesh's VAO, draw elements, and deactivate the VAO
        glUniform1i(multipleTexturesLoc, false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, chestWoodTexture);
        glBindVertexArray(chestBodyMesh.VAO);
        glDrawElements(GL_TRIANGLES, chestBodyMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        translation = glm::translate(glm::vec3(0.0f, 0.39f, 0.0f));
        model = translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(multipleTexturesLoc, false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, chestMetalTexture);
        glBindVertexArray(chestDecorMesh.VAO);
        glDrawElements(GL_TRIANGLES, chestDecorMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
       
        // Render Cylinder
        scale = glm::scale(glm::vec3(1.0f, 1.5f, 2.0f));
        translation = glm::translate(glm::vec3(0.0f, 0.5f, 0.0f));
        glm::mat4 rotationZ = glm::rotate(-3.141592f * 0.5f, glm::vec3(0.0f, 0.0f, 1.0f));
        model = translation * rotation * rotationZ * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(multipleTexturesLoc, false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, chestWoodTexture);
        glBindVertexArray(cylinderVAO);
        cylinder.render();
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Pink Marble box
        scale = glm::scale(glm::vec3(0.6f, 0.4f, 0.6f));
        translation = glm::translate(glm::vec3(-1.0f, -0.4f, 1.0f));
        rotation = glm::rotate(-3.141592f * -0.15f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(multipleTexturesLoc, false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pinkMarbleTexture);
        glBindVertexArray(chestBodyMesh.VAO);
        glDrawElements(GL_TRIANGLES, chestBodyMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Pink Marble box lid
        scale = glm::scale(glm::vec3(0.62f, 0.15f, 0.62f));
        translation = glm::translate(glm::vec3(-1.0f, -0.31f, 1.0f));
        rotation = glm::rotate(-3.141592f * -0.15f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(multipleTexturesLoc, false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pinkMarbleTexture);
        glBindVertexArray(chestBodyMesh.VAO);
        glDrawElements(GL_TRIANGLES, chestBodyMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Top pink marble box
        scale = glm::scale(glm::vec3(0.62f, 0.0f, 0.32f));
        translation = glm::translate(glm::vec3(-1.0f, -0.27f, 1.0f));
        rotation = glm::rotate(-3.141592f * -0.15f, glm::vec3(0.0f, 1.0f, 0.0f));
        model = translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(multipleTexturesLoc, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pinkMarbleTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ornamentTexture);
        glBindVertexArray(planeMesh.VAO);
        glDrawElements(GL_TRIANGLES, planeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Render Plane
        scale = glm::scale(glm::vec3(10.0f, 10.0f, 10.0f));
        translation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
        model = translation * rotation * scale;
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(multipleTexturesLoc, false);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, marbleTexture);
        glBindVertexArray(planeMesh.VAO);
        glDrawElements(GL_TRIANGLES, planeMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);    // Flips the the back buffer with the front buffer every frame.
        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(chestBodyMesh);
    UDestroyMesh(chestDecorMesh);
    UDestroyMesh(planeMesh);

    glDeleteVertexArrays(1, &cylinderVAO);
    glDeleteBuffers(1, &cylinderVBO);

    // Release texture
    UDestroyTexture(chestWoodTexture);
    UDestroyTexture(chestMetalTexture);
    UDestroyTexture(marbleTexture);

    // Release shader program
    UDestroyShaderProgram(shaderProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Implements the UCreateMesh function
void UCreateChestBodyMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat chestBodyV[] = {
        // Vertex Positions      // Colors (r,g,b,a)        // Texture Coords
         0.5f,  0.25f, -0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    1.0f, 1.0f,    // Vertex 0 Back Top Right
         0.5f, -0.25f, -0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    1.0f, 0.0f,    // Vertex 1 Back Bottom Right
        -0.5f, -0.25f, -0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    0.5f, 0.0f,    // Vertex 2 Back Bottom Left
        -0.5f,  0.25f, -0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    0.5f, 1.0f,    // Vertex 3 Back Top Left
         0.5f,  0.25f,  0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    0.5f, 1.0f,    // Vertex 4 Front Top Right
         0.5f, -0.25f,  0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    0.5f, 0.0f,    // Vertex 5 Front Bottom Right
        -0.5f, -0.25f,  0.25f,   0.7f, 0.4f, 0.0f, 1.0f,    0.0f, 0.0f,    // Vertex 6 Front Bottom Left
        -0.5f,  0.25f,  0.25f,   0.9f, 0.6f, 0.2f, 1.0f,    0.0f, 1.0f,    // Vertex 7 Front Top Left
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
         0.51f,  0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0f, 1.0f,    // Vertex 0 Back Top Right
         0.51f, -0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    1.0f, 0.0f,    // Vertex 1 Back Bottom Right
        -0.51f, -0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    0.5f, 0.0f,    // Vertex 2 Back Bottom Left
        -0.51f,  0.05f, -0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    0.5f, 1.0f,    // Vertex 3 Back Top Left
         0.51f,  0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    0.5f, 1.0f,    // Vertex 4 Front Top Right
         0.51f, -0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    0.5f, 0.0f,    // Vertex 5 Front Bottom Right
        -0.51f, -0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    0.0f, 0.0f,    // Vertex 6 Front Bottom Left
        -0.51f,  0.05f,  0.251f,   0.35f, 0.30f, 0.28f, 1.0f,    0.0f, 1.0f,    // Vertex 7 Front Top Left
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

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerColor)));
    glEnableVertexAttribArray(2);
}

void UCreatePlaneMesh(GLMesh& mesh)
{
    // Position and Color data
    GLfloat planeV[] = {
        // Vertex Positions   // Colors (r,g,b,a)      // Texture
         0.5f, 0.0f, -0.5f,   1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  // Vertex 0 Back Right
         0.5f, 0.0f,  0.5f,   1.0f, 1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  // Vertex 1 Front Right
        -0.5f, 0.0f,  0.5f,   1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  // Vertex 2 Front Left
        -0.5f, 0.0f, -0.5f,   1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  // Vertex 3 Back Left
    };

    // Index data to share position data
    GLushort planeI[] = {
        0, 1, 2, 0, 2, 3, // Plane Triangles 1 and 2
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeV), planeV, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(planeI) / sizeof(planeI[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeI), planeI, GL_STATIC_DRAW);

    // Strides between vertex coordinates is 8 (x, y, z, r, g, b, a, tc1, tc2). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor + floatsPerTex);// The number of floats before each

    // Position attribute
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerTex, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerColor)));
    glEnableVertexAttribArray(2);
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.VAO);
    glDeleteBuffers(1, &mesh.VBO);
    glDeleteBuffers(1, &mesh.EBO);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) // W: goes forward
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) // S: goes backward
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) // A: goes left
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) // D: goes right
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) // E: goes upward
        camera.ProcessKeyboard(UPWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) // Q: goes downward
        camera.ProcessKeyboard(DOWNWARD, deltaTime);
}

// Key callback to handle key "P" to change to Ortho
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) return; //only handle press events
    if (key == GLFW_KEY_P) 
        ortho = !ortho;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    // Mouse cursor changes orientation of the camera
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    /*
     * Mouse Scroll changes speed
     * Scrolling up increases speed
     * Scrolling down decreases speed
     */
    camera.ProcessMouseScroll(yoffset);
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}


void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
