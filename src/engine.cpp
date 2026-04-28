#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <random>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float tanh_activation(float x) { return std::tanh(x); }

// --- 1. THE BRAIN ---
class NeuralNetwork {
public:
    // We now have 3 inputs: Sensor Left, Sensor Right, and PAIN
    int num_inputs = 3; 
    int num_hidden = 5;
    int num_outputs = 2;

    std::vector<std::vector<float>> weights_ih;
    std::vector<std::vector<float>> weights_ho;
    std::vector<float> bias_h;
    std::vector<float> bias_o;

    NeuralNetwork() {
        weights_ih.resize(num_inputs, std::vector<float>(num_hidden, 0.0f));
        weights_ho.resize(num_hidden, std::vector<float>(num_outputs, 0.0f));
        bias_h.resize(num_hidden, 0.0f);
        bias_o.resize(num_outputs, 0.0f);
    }

    void randomize(std::mt19937& gen) {
        std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
        for (int i = 0; i < num_inputs; ++i)
            for (int j = 0; j < num_hidden; ++j) weights_ih[i][j] = dis(gen);
        for (int i = 0; i < num_hidden; ++i)
            for (int j = 0; j < num_outputs; ++j) weights_ho[i][j] = dis(gen);
        for (int i = 0; i < num_hidden; ++i) bias_h[i] = dis(gen);
        for (int i = 0; i < num_outputs; ++i) bias_o[i] = dis(gen);
    }

    void mutate(float mutation_rate, std::mt19937& gen) {
        std::uniform_real_distribution<float> prob(0.0f, 1.0f);
        std::uniform_real_distribution<float> tweak(-0.5f, 0.5f);

        for (int i = 0; i < num_inputs; ++i)
            for (int j = 0; j < num_hidden; ++j) 
                if (prob(gen) < mutation_rate) weights_ih[i][j] += tweak(gen);

        for (int i = 0; i < num_hidden; ++i)
            for (int j = 0; j < num_outputs; ++j) 
                if (prob(gen) < mutation_rate) weights_ho[i][j] += tweak(gen);

        for (int i = 0; i < num_hidden; ++i) 
            if (prob(gen) < mutation_rate) bias_h[i] += tweak(gen);

        for (int i = 0; i < num_outputs; ++i) 
            if (prob(gen) < mutation_rate) bias_o[i] += tweak(gen);
    }

    std::vector<float> think(const std::vector<float>& inputs) {
        std::vector<float> hidden(num_hidden, 0.0f);
        for (int j = 0; j < num_hidden; ++j) {
            float sum = bias_h[j];
            for (int i = 0; i < num_inputs; ++i) sum += inputs[i] * weights_ih[i][j];
            hidden[j] = tanh_activation(sum);
        }

        std::vector<float> outputs(num_outputs, 0.0f);
        for (int j = 0; j < num_outputs; ++j) {
            float sum = bias_o[j];
            for (int i = 0; i < num_hidden; ++i) sum += hidden[i] * weights_ho[i][j];
            outputs[j] = tanh_activation(sum);
        }
        return outputs;
    }
};

struct Food {
    glm::vec2 position;
    float radius = 0.05f; // How close the ant needs to be to "eat" it
};

// --- 2. THE HOMEOSTATIC ANT ---
class Ant {
public:
    glm::vec2 position;
    float heading;
    float max_speed = 1.0f;
    float max_turn_rate = 4.0f;
    
    float motor_forward, motor_turn;
    float sensor_left, sensor_right;
    float antenna_length = 0.15f;
    float antenna_angle = 0.5f;

    // --- HOMEOSTASIS VARIABLES ---
    float pain = 0.0f;
    float pain_threshold = 100.0f; // If pain hits this, the ant dies
    float pain_increase_rate = 1.0f; 

    NeuralNetwork brain;
    NeuralNetwork best_surviving_brain; 

    int lifetimes = 1;
    int meals_eaten = 0;

    Ant() {}

    void reset_physics() {
        position = glm::vec2(0.0f, 0.0f);
        heading = 0.0f;
        motor_forward = 0.0f;
        motor_turn = 0.0f;
        pain = 0.0f;
    }

    void update(float dt, const Food& food) {
        // 1. INCREASE PAIN (The drive to survive)
        pain += pain_increase_rate * dt;

        // 2. SENSORS
        glm::vec2 left_antenna_pos = position + glm::vec2(
            std::cos(heading + antenna_angle) * antenna_length,
            std::sin(heading + antenna_angle) * antenna_length
        );
        glm::vec2 right_antenna_pos = position + glm::vec2(
            std::cos(heading - antenna_angle) * antenna_length,
            std::sin(heading - antenna_angle) * antenna_length
        );

        float dist_left = glm::distance(left_antenna_pos, food.position);
        float dist_right = glm::distance(right_antenna_pos, food.position);
        
        sensor_left = 1.0f / (1.0f + (dist_left * 5.0f));
        sensor_right = 1.0f / (1.0f + (dist_right * 5.0f));

        // 3. THE BRAIN THINKS
        // We feed the brain its sensors AND its current pain level (normalized 0 to 1)
        std::vector<float> inputs = {sensor_left, sensor_right, pain / pain_threshold};
        std::vector<float> outputs = brain.think(inputs);

        motor_forward = glm::clamp(outputs[0], -1.0f, 1.0f);
        motor_turn = glm::clamp(outputs[1], -1.0f, 1.0f);

        // 4. MOVE
        heading += (motor_turn * max_turn_rate) * dt;
        float current_speed = motor_forward * max_speed;
        position.x += std::cos(heading) * current_speed * dt;
        position.y += std::sin(heading) * current_speed * dt;
        
        // Keep ant inside the screen bounds so it doesn't get lost forever
        position.x = glm::clamp(position.x, -0.9f, 0.9f);
        position.y = glm::clamp(position.y, -0.9f, 0.9f);
    }
};

// --- SHADERS ---
const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 transform;\n"
    "void main() { gl_Position = transform * vec4(aPos, 1.0); }\0";

const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 color;\n"
    "void main() { FragColor = color; }\n\0";

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "The Homeostatic Ant", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

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

    float ant_vertices[] = { 0.05f, 0.0f, 0.0f, -0.05f, 0.03f, 0.0f, -0.05f, -0.03f, 0.0f };
    unsigned int antVBO, antVAO;
    glGenVertexArrays(1, &antVAO); glGenBuffers(1, &antVBO);
    glBindVertexArray(antVAO); glBindBuffer(GL_ARRAY_BUFFER, antVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(ant_vertices), ant_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float food_vertices[] = { -0.02f, -0.02f, 0.0f, 0.02f, -0.02f, 0.0f, 0.02f, 0.02f, 0.0f, -0.02f, -0.02f, 0.0f, 0.02f, 0.02f, 0.0f, -0.02f, 0.02f, 0.0f };
    unsigned int foodVBO, foodVAO;
    glGenVertexArrays(1, &foodVAO); glGenBuffers(1, &foodVBO);
    glBindVertexArray(foodVAO); glBindBuffer(GL_ARRAY_BUFFER, foodVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(food_vertices), food_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> pos_dist(-0.7f, 0.7f);

    Ant myAnt;
    myAnt.brain.randomize(gen);
    myAnt.best_surviving_brain = myAnt.brain; // Start with the random brain
    myAnt.reset_physics();

    Food myFood;
    myFood.position = glm::vec2(0.5f, 0.5f);
    
    float deltaTime = 0.0f;
    float lastFrame = glfwGetTime();

    std::cout << "Ant born. Pain threshold: " << myAnt.pain_threshold << "\n";

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        myAnt.update(deltaTime, myFood);

        // --- THE SURVIVAL LOGIC ---
        
        if (glm::distance(myAnt.position, myFood.position) < myFood.radius) {
            myAnt.pain = 0.0f; 
            myAnt.meals_eaten++;
            
            myAnt.best_surviving_brain = myAnt.brain; 
            
            std::cout << "Ant ATE! Meals total: " << myAnt.meals_eaten << ". Brain saved.\n";

            myFood.position = glm::vec2(pos_dist(gen), pos_dist(gen));
        }

        if (myAnt.pain >= myAnt.pain_threshold) {
            myAnt.lifetimes++;
            std::cout << "Ant DIED of starvation. Lifetime: " << myAnt.lifetimes << ". Mutating from last good brain...\n";
            
            myAnt.brain = myAnt.best_surviving_brain;
            
            myAnt.brain.mutate(0.3f, gen); 
            
            myAnt.reset_physics();
            myFood.position = glm::vec2(pos_dist(gen), pos_dist(gen)); 
        }

        // --- RENDERING ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");

        // Draw Food (Green)
        glm::mat4 foodTrans = glm::mat4(1.0f);
        foodTrans = glm::translate(foodTrans, glm::vec3(myFood.position.x, myFood.position.y, 0.0f));
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(foodTrans));
        glUniform4f(colorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
        glBindVertexArray(foodVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw Ant
        glm::mat4 antTrans = glm::mat4(1.0f);
        antTrans = glm::translate(antTrans, glm::vec3(myAnt.position.x, myAnt.position.y, 0.0f));
        antTrans = glm::rotate(antTrans, myAnt.heading, glm::vec3(0.0f, 0.0f, 1.0f));
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(antTrans));
        
        // VISUAL FEEDBACK: The ant turns RED as it experiences more pain
        float pain_ratio = myAnt.pain / myAnt.pain_threshold;
        glUniform4f(colorLoc, 1.0f, 1.0f - pain_ratio, 1.0f - pain_ratio, 1.0f); 
        
        glBindVertexArray(antVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
