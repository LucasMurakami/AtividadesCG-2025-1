/* 
 * InstanciaObjects3D - 3D Cube Visualizer
 */

#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <sstream>
#include <iomanip>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Código fonte do Vertex Shader
const GLchar *vertexShaderSource = "#version 450\n"
                                   "layout (location = 0) in vec3 position;\n"
                                   "layout (location = 1) in vec3 color;\n"
                                   "uniform mat4 model;\n"
                                   "uniform mat4 view;\n"
                                   "uniform mat4 projection;\n"
                                   "out vec4 finalColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "gl_Position = projection * view * model * vec4(position, 1.0);\n"
                                   "finalColor = vec4(color, 1.0);\n"
                                   "}\0";

// Código fonte do Fragment Shader
const GLchar *fragmentShaderSource = "#version 450\n"
                                     "in vec4 finalColor;\n"
                                     "out vec4 color;\n"
                                     "void main()\n"
                                     "{\n"
                                     "color = finalColor;\n"
                                     "}\n\0";

// Variáveis para rotação, translação e escala
float rotationX = 0.0f;
float rotationY = 0.0f;
float rotationZ = 0.0f;
bool isRotating = false;
float targetRotationX = 0.0f;
float targetRotationY = 0.0f;
float targetRotationZ = 0.0f;
float rotationSpeed = 5.0f; // Degrees per frame
glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
float scale = 1.0f;

float rotationAngleX = 0.0f;
float rotationAngleY = 0.0f;
float rotationAngleZ = 0.0f;

// Variáveis para visualização
bool wireframeMode = false;
bool showAxes = true;

// Variáveis para câmera
glm::vec3 cameraPos = glm::vec3(3.5f, 2.5f, 5.0f); // Set to give a 3/4 view
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f); // Look at origin
glm::vec3 cameraFront = glm::normalize(cameraTarget - cameraPos);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
float fov = 45.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool cameraMode = false;

// Variáveis para instanciamento
const int MAX_INSTANCES = 10;
int currentInstances = 1;
vector<glm::vec3> instancePositions;

// Função MAIN
int main()
{
    // Inicialização da GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criação da janela GLFW
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "M2 - Lucas M", nullptr, nullptr);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Fazendo o registro das funções de callback
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Configurações de mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // GLAD: carrega todos os ponteiros de funções da OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Obtendo as informações de versão
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *version = glGetString(GL_VERSION);
    cout << "Renderer: " << renderer << endl;
    cout << "OpenGL version supported " << version << endl;
    cout << "\n=== Controls ===\n";
    cout << "W/S - Rotate front face (X axis)\n";
    cout << "A/D - Rotate side face (Y axis)\n";
    cout << "I/J - Rotate top/bottom face (Z axis)\n";
    cout << "[/] - Scale down/up\n";
    cout << "SPACE - Add new cube instance\n";
    cout << "F - Toggle wireframe mode\n";
    cout << "C - Toggle camera mode (when active, use mouse to rotate camera)\n";
    cout << "R - Reset view\n";
    cout << "ESC - Exit\n";

    // Definindo as dimensões da viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Compilando e buildando o programa de shader
    GLuint shaderID = setupShader();

    // Gerando um buffer simples com a geometria de um cubo
    GLuint VAO = setupGeometry();

    glUseProgram(shaderID);

    // Configuração das matrizes de transformação
    glm::mat4 model = glm::mat4(1);
    glm::mat4 view = glm::lookAt(
        cameraPos,                      // Position
        glm::vec3(0.0f, 0.0f, 0.0f),   // Target (origin)
        cameraUp                        // Up vector
    );
    glm::mat4 projection = glm::perspective(glm::radians(fov), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
    
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projectionLoc = glGetUniformLocation(shaderID, "projection");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    // Inicializa a primeira instância
    instancePositions.push_back(glm::vec3(0.0f, 0.0f, 0.0f));

    // Loop da aplicação
    while (!glfwWindowShouldClose(window))
    {
        // Checa se houveram eventos de input
        glfwPollEvents();

        // Limpa o buffer de cor
        glClearColor(0.95f, 0.95f, 0.95f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualiza a matriz de visão da câmera
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        // Atualiza a matriz de projeção (para FOV dinâmico)
        projection = glm::perspective(glm::radians(fov), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Configura modo de renderização (wireframe ou preenchimento)
        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);

        // Define largura da linha e tamanho dos pontos
        glLineWidth(2);
        glPointSize(8);

        // Handle rotation animation
        if (isRotating) {
            // X rotation
            if (rotationX != targetRotationX) {
                float step = rotationSpeed;
                if (rotationX < targetRotationX) {
                    rotationX = min(rotationX + step, targetRotationX);
                } else {
                    rotationX = max(rotationX - step, targetRotationX);
                }
            }
            
            // Y rotation
            if (rotationY != targetRotationY) {
                float step = rotationSpeed;
                if (rotationY < targetRotationY) {
                    rotationY = min(rotationY + step, targetRotationY);
                } else {
                    rotationY = max(rotationY - step, targetRotationY);
                }
            }
            
            // Z rotation
            if (rotationZ != targetRotationZ) {
                float step = rotationSpeed;
                if (rotationZ < targetRotationZ) {
                    rotationZ = min(rotationZ + step, targetRotationZ);
                } else {
                    rotationZ = max(rotationZ - step, targetRotationZ);
                }
            }
            
            // Check if rotation is complete
            if (rotationX == targetRotationX && rotationY == targetRotationY && rotationZ == targetRotationZ) {
                isRotating = false;
            }
        }

        // Para cada instância
        for (int i = 0; i < currentInstances; i++)
        {
            // Transformações
            model = glm::mat4(1); // Reseta o modelo para a matriz identidade
            
            // Aplica a translação da instância
            model = glm::translate(model, instancePositions[i]);
            
            // Apply fixed rotations
            model = glm::rotate(model, glm::radians(rotationX), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotationY), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(rotationZ), glm::vec3(0.0f, 0.0f, 1.0f));
            
            // Aplicar translação local
            model = glm::translate(model, translation);
            
            // Escala
            model = glm::scale(model, glm::vec3(scale));
            
            // Add slight rotation to add 2.5D effect
            model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            // Passa a matriz para o shader
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            // Desenha o objeto
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36); 
            
            // Se estiver no modo wireframe, desenha também os pontos
            if (wireframeMode) {
                glDrawArrays(GL_POINTS, 0, 36);
            }
            glBindVertexArray(0);
        }

        // In the render loop, before drawing the cube:
        // Add a subtle outline for 2.5D effect
        if (!wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(1.0f);
            glUniform4f(glGetUniformLocation(shaderID, "outlineColor"), 0.0f, 0.0f, 0.0f, 1.0f);
            
            // Draw the outlines
            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            
            // Reset to fill mode
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }
    
    // Limpa os recursos alocados
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// Função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Only process rotation if we're not already rotating
    if (!isRotating && action == GLFW_PRESS) {
        // Front face rotation (W/S)
        if (key == GLFW_KEY_W) {
            targetRotationX = rotationX - 90.0f;
            isRotating = true;
        }
        if (key == GLFW_KEY_S) {
            targetRotationX = rotationX + 90.0f;
            isRotating = true;
        }
        
        // Side face rotation (A/D)
        if (key == GLFW_KEY_A) {
            targetRotationY = rotationY - 90.0f;
            isRotating = true;
        }
        if (key == GLFW_KEY_D) {
            targetRotationY = rotationY + 90.0f;
            isRotating = true;
        }
        
        // Top/Bottom face rotation (I/J)
        if (key == GLFW_KEY_I) {
            targetRotationZ = rotationZ + 90.0f;
            isRotating = true;
        }
        if (key == GLFW_KEY_J) {
            targetRotationZ = rotationZ - 90.0f;
            isRotating = true;
        }
    }

    // Escala ([ / ])
    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        scale = max(0.1f, scale - 0.1f);
    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        scale = min(3.0f, scale + 0.1f);

    // Toggle wireframe mode (F)
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        wireframeMode = !wireframeMode;
        
    // Toggle camera mode (C)
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        cameraMode = !cameraMode;
        if (cameraMode)
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    
    // Reset view (R)
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        translation = glm::vec3(0.0f);
        scale = 1.0f;
        rotationX = rotationY = rotationZ = 0.0f;
        targetRotationX = targetRotationY = targetRotationZ = 0.0f;
        isRotating = false;
        cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        pitch = 0.0f;
        fov = 45.0f;
    }

    // Criar nova instância (SPACE)
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && currentInstances < MAX_INSTANCES)
    {
        // Calculate position along camera's view direction
        float distance = 3.0f; // Place cube 3 units in front of camera
        glm::vec3 newPosition = cameraPos + cameraFront * distance;
        
        instancePositions.push_back(newPosition);
        currentInstances++;
        
        cout << "Novo cubo criado na posição: (" 
             << newPosition.x << ", " 
             << newPosition.y << ", " 
             << newPosition.z << ") Total: " 
             << currentInstances << endl;
    }
}

// Função de callback para movimento do mouse
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!cameraMode) return;
    
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertido, pois coordenadas y vão de baixo para cima
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Limit the pitch more strictly for 2.5D feel
    if (pitch > 45.0f)
        pitch = 45.0f;
    if (pitch < 15.0f)
        pitch = 15.0f;
    
    // Limit yaw to maintain 2.5D perspective
    if (yaw > -45.0f)
        yaw = -45.0f;
    if (yaw < -135.0f)
        yaw = -135.0f;

    // Calcula o vetor de direção da câmera
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

// Função de callback para o scroll do mouse (zoom)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 90.0f)
        fov = 90.0f;
}

// Compilação e linkagem do shader
int setupShader()
{
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Checando erros de compilação
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Checando erros de compilação
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // Linkando os shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Checando por erros de linkagem
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria a geometria do cubo
int setupGeometry()
{
    // Vértices do cubo com cores diferentes para cada face (8 vértices * 6 faces)
    // Formato: x, y, z, r, g, b (posição e cor)
    GLfloat vertices[] = {
        // Face frontal (vermelho mais suave)
        -0.5, -0.5,  0.5, 0.9f, 0.2f, 0.2f,
         0.5, -0.5,  0.5, 0.9f, 0.2f, 0.2f,
         0.5,  0.5,  0.5, 0.9f, 0.2f, 0.2f,
        
        -0.5, -0.5,  0.5, 0.9f, 0.2f, 0.2f,
         0.5,  0.5,  0.5, 0.9f, 0.2f, 0.2f,
        -0.5,  0.5,  0.5, 0.9f, 0.2f, 0.2f,
        
        // Face traseira (verde mais suave)
        -0.5, -0.5, -0.5, 0.2f, 0.8f, 0.2f,
        -0.5,  0.5, -0.5, 0.2f, 0.8f, 0.2f,
         0.5,  0.5, -0.5, 0.2f, 0.8f, 0.2f,
        
        -0.5, -0.5, -0.5, 0.2f, 0.8f, 0.2f,
         0.5,  0.5, -0.5, 0.2f, 0.8f, 0.2f,
         0.5, -0.5, -0.5, 0.2f, 0.8f, 0.2f,
        
        // Face superior (azul mais suave)
        -0.5,  0.5, -0.5, 0.3f, 0.3f, 0.9f,
        -0.5,  0.5,  0.5, 0.3f, 0.3f, 0.9f,
         0.5,  0.5,  0.5, 0.3f, 0.3f, 0.9f,
        
        -0.5,  0.5, -0.5, 0.3f, 0.3f, 0.9f,
         0.5,  0.5,  0.5, 0.3f, 0.3f, 0.9f,
         0.5,  0.5, -0.5, 0.3f, 0.3f, 0.9f,
        
        // Face inferior (amarelo mais suave)
        -0.5, -0.5, -0.5, 0.9f, 0.9f, 0.2f,
         0.5, -0.5, -0.5, 0.9f, 0.9f, 0.2f,
         0.5, -0.5,  0.5, 0.9f, 0.9f, 0.2f,
        
        -0.5, -0.5, -0.5, 0.9f, 0.9f, 0.2f,
         0.5, -0.5,  0.5, 0.9f, 0.9f, 0.2f,
        -0.5, -0.5,  0.5, 0.9f, 0.9f, 0.2f,
        
        // Face direita (magenta mais suave)
         0.5, -0.5, -0.5, 0.9f, 0.2f, 0.9f,
         0.5,  0.5, -0.5, 0.9f, 0.2f, 0.9f,
         0.5,  0.5,  0.5, 0.9f, 0.2f, 0.9f,
        
         0.5, -0.5, -0.5, 0.9f, 0.2f, 0.9f,
         0.5,  0.5,  0.5, 0.9f, 0.2f, 0.9f,
         0.5, -0.5,  0.5, 0.9f, 0.2f, 0.9f,
        
        // Face esquerda (ciano mais suave)
        -0.5, -0.5, -0.5, 0.2f, 0.9f, 0.9f,
        -0.5, -0.5,  0.5, 0.2f, 0.9f, 0.9f,
        -0.5,  0.5,  0.5, 0.2f, 0.9f, 0.9f,
        
        -0.5, -0.5, -0.5, 0.2f, 0.9f, 0.9f,
        -0.5,  0.5,  0.5, 0.2f, 0.9f, 0.9f,
        -0.5,  0.5, -0.5, 0.2f, 0.9f, 0.9f,
    };

    GLuint VBO, VAO;

    // Geração do identificador do VBO
    glGenBuffers(1, &VBO);

    // Faz a conexão (vincula) do buffer como um buffer de array
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Envia os dados do array de floats para o buffer da OpenGL
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Geração do identificador do VAO
    glGenVertexArrays(1, &VAO);

    // Vincula o VAO primeiro, e em seguida conecta e seta o(s) buffer(s) de vértices
    // e os ponteiros para os atributos
    glBindVertexArray(VAO);

    // Atributo posição (x, y, z)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    // Atributo cor (r, g, b)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Desvincula o VAO para não modificar acidentalmente
    glBindVertexArray(0);

    return VAO;
}