#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// --- 1. THE ENVIRONMENT (What the Ant senses) ---
// For now, let's just create a single piece of "food" in the world
struct Food {
    glm::vec2 position;
    float radius;
};

// --- 2. THE ANT BODY (Sensors and Actuators) ---
class Ant {
public:
    // --- Core Physical State ---
    glm::vec2 position;
    float heading;
    
    // Physical constraints of the body
    float max_speed = 1.0f;
    float max_turn_rate = 3.0f; // Radians per second
    
    // --- Actuators (Motor Outputs) ---
    // The brain will write to these values. The body will read them to move.
    // They are normalized between -1.0 and 1.0
    float motor_forward; // 1.0 = max speed forward, -1.0 = max speed backward
    float motor_turn;    // 1.0 = turn left max, -1.0 = turn right max

    // --- Sensors (Inputs) ---
    // The body will write to these values. The brain will read them to think.
    // We give the ant two antennae, placed slightly ahead and to the sides of its center.
    float sensor_left;   // 1.0 = smelling food strongly on the left, 0.0 = nothing
    float sensor_right;  // 1.0 = smelling food strongly on the right, 0.0 = nothing
    
    // Where the antennae physically are relative to the ant's center
    float antenna_length = 0.15f;
    float antenna_angle = 0.5f; // Roughly 30 degrees outward from the center line

    Ant() {
        position = glm::vec2(0.0f, 0.0f);
        heading = 0.0f;
        motor_forward = 0.0f;
        motor_turn = 0.0f;
        sensor_left = 0.0f;
        sensor_right = 0.0f;
    }

    // --- The Physics Step ---
    void update(float dt, const Food& food) {
        // 1. UPDATE SENSORS (Read the world)
        // Calculate exactly where the left and right antennae are in the 2D world
        glm::vec2 left_antenna_pos = position + glm::vec2(
            std::cos(heading + antenna_angle) * antenna_length,
            std::sin(heading + antenna_angle) * antenna_length
        );
        
        glm::vec2 right_antenna_pos = position + glm::vec2(
            std::cos(heading - antenna_angle) * antenna_length,
            std::sin(heading - antenna_angle) * antenna_length
        );

        // Calculate distance from each antenna to the food
        // The closer the food, the stronger the signal (1.0 = touching, 0.0 = far away)
        float dist_left = glm::distance(left_antenna_pos, food.position);
        float dist_right = glm::distance(right_antenna_pos, food.position);
        
        // Simple inverse-square law for smell intensity
        sensor_left = 1.0f / (1.0f + (dist_left * 5.0f));
        sensor_right = 1.0f / (1.0f + (dist_right * 5.0f));

        // 2. APPLY ACTUATORS (Move the body)
        // The brain has set 'motor_forward' and 'motor_turn'. We apply them to physics.
        
        // Clamp motor values just in case the brain goes crazy
        motor_forward = glm::clamp(motor_forward, -1.0f, 1.0f);
        motor_turn = glm::clamp(motor_turn, -1.0f, 1.0f);

        // Turn the body
        heading += (motor_turn * max_turn_rate) * dt;

        // Move the body forward
        float current_speed = motor_forward * max_speed;
        position.x += std::cos(heading) * current_speed * dt;
        position.y += std::sin(heading) * current_speed * dt;
    }
};

// --- 3. THE SHADERS ---
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 transform;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = transform * vec4(aPos, 1.0);\n"
    "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n" // We add a color uniform so we can draw the food differently
    "void main()\n"
    "{\n"
    "   FragColor = color;\n"
    "}\n\0";

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "The Ant Universe", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // --- COMPILE SHADERS ---
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- VBO & VAO (The Ant Shape) ---
    float ant_vertices[] = {
         0.05f,  0.0f,  0.0f, // Head
        -0.05f,  0.03f, 0.0f, // Back Left
        -0.05f, -0.03f, 0.0f  // Back Right
    };

    unsigned int antVBO, antVAO;
    glGenVertexArrays(1, &antVAO);
    glGenBuffers(1, &antVBO);
    glBindVertexArray(antVAO);
    glBindBuffer(GL_ARRAY_BUFFER, antVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ant_vertices), ant_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- VBO & VAO (The Food Shape - a simple square for now) ---
    float food_vertices[] = {
        -0.02f, -0.02f, 0.0f,
         0.02f, -0.02f, 0.0f,
         0.02f,  0.02f, 0.0f,
        -0.02f, -0.02f, 0.0f,
         0.02f,  0.02f, 0.0f,
        -0.02f,  0.02f, 0.0f
    };
    unsigned int foodVBO, foodVAO;
    glGenVertexArrays(1, &foodVAO);
    glGenBuffers(1, &foodVBO);
    glBindVertexArray(foodVAO);
    glBindBuffer(GL_ARRAY_BUFFER, foodVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(food_vertices), food_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- INITIALIZE UNIVERSE ---
    Ant myAnt;
    myAnt.position = glm::vec2(-0.5f, -0.5f); // Start bottom left
    myAnt.heading = 0.5f; // Point roughly towards center

    Food myFood;
    myFood.position = glm::vec2(0.5f, 0.5f); // Place food top right
    
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // --- ENGINE LOOP ---
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- THE "BRAIN" (Temporary Hardcoded Logic) ---
        // We haven't built the neural network yet.
        // For now, we write a simple "Braitenberg Vehicle" script to test the sensors and actuators.
        // If the left sensor smells more food, turn left. If right smells more, turn right.
        
        myAnt.motor_forward = 0.5f; // Always walk forward
        
        // The difference between sensors drives the steering wheel
        myAnt.motor_turn = (myAnt.sensor_left - myAnt.sensor_right) * 2.0f; 

        // --- UPDATE PHYSICS ---
        myAnt.update(deltaTime, myFood);

        // --- RENDER ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");

        // 1. Draw the Food (Green)
        glm::mat4 foodTrans = glm::mat4(1.0f);
        foodTrans = glm::translate(foodTrans, glm::vec3(myFood.position.x, myFood.position.y, 0.0f));
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(foodTrans));
        glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f); // Green
        
        glBindVertexArray(foodVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 2. Draw the Ant (Orange)
        glm::mat4 antTrans = glm::mat4(1.0f);
        antTrans = glm::translate(antTrans, glm::vec3(myAnt.position.x, myAnt.position.y, 0.0f));
        antTrans = glm::rotate(antTrans, myAnt.heading, glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(antTrans));
        glUniform4f(colorLoc, 1.0f, 0.5f, 0.2f, 1.0f); // Orange
        
        glBindVertexArray(antVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
