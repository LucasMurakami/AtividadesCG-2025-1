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
void renderAxes();

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
bool rotateX = false, rotateY = false, rotateZ = false;
glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
float scale = 1.0f;

// Variáveis para visualização
bool wireframeMode = false;
bool showAxes = true;

// Variáveis para câmera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
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
    cout << "X, Y, Z - Rotate around respective axis\n";
    cout << "W/S, A/D, I/J - Move in Z, X, Y axes\n";
    cout << "[/] - Scale down/up\n";
    cout << "SPACE - Add new cube instance\n";
    cout << "F - Toggle wireframe mode\n";
    cout << "G - Toggle coordinate axes\n";
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
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
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
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f); // cor de fundo mais escura para melhorar visualização
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

        // Renderiza os eixos de coordenadas
        if (showAxes) {
            renderAxes();
        }

        // Para cada instância
        for (int i = 0; i < currentInstances; i++)
        {
            // Transformações
            model = glm::mat4(1); // Reseta o modelo para a matriz identidade
            
            // Aplica a translação da instância
            model = glm::translate(model, instancePositions[i]);
            
            // Rotação
            float angle = (GLfloat)glfwGetTime();
            if (rotateX)
            {
                model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
            }
            else if (rotateY)
            {
                model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            else if (rotateZ)
            {
                model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            }
            
            // Translação
            model = glm::translate(model, translation);
            
            // Escala
            model = glm::scale(model, glm::vec3(scale));
            
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

        // Troca os buffers da tela
        glfwSwapBuffers(window);
    }
    
    // Limpa os recursos alocados
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void renderAxes() {
    // Pontos para representar eixos X (vermelho), Y (verde), e Z (azul)
    static const GLfloat axesVertices[] = {
        // X axis (red)
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        5.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        
        // Y axis (green)
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 5.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        
        // Z axis (blue)
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 1.0f
    };

    // Criar e configurar VAO e VBO para os eixos
    static GLuint axesVAO = 0, axesVBO;
    
    if (axesVAO == 0) {
        glGenVertexArrays(1, &axesVAO);
        glGenBuffers(1, &axesVBO);
        
        glBindVertexArray(axesVAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, axesVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);
        
        // Atributo posição (x, y, z)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)0);
        glEnableVertexAttribArray(0);
        
        // Atributo cor (r, g, b)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
    }
    
    // Salva o estado anterior
    GLboolean previousLineState;
    glGetBooleanv(GL_LINE_SMOOTH, &previousLineState);
    
    // Configurar para desenhar linhas
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(3.0f);
    
    // Desenhar eixos
    glBindVertexArray(axesVAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
    
    // Restaura estado anterior
    if (!previousLineState)
        glDisable(GL_LINE_SMOOTH);
}

// Função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Rotações
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        rotateX = !rotateX;
        rotateY = false;
        rotateZ = false;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        rotateX = false;
        rotateY = !rotateY;
        rotateZ = false;
    }

    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        rotateX = false;
        rotateY = false;
        rotateZ = !rotateZ;
    }

    // Translação no eixo X (A/D)
    if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
        translation.x -= 0.1f;
    if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
        translation.x += 0.1f;

    // Translação no eixo Y (I/J)
    if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
        translation.y += 0.1f;
    if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
        translation.y -= 0.1f;

    // Translação no eixo Z (W/S)
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
        translation.z -= 0.1f;
    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
        translation.z += 0.1f;

    // Escala ([ / ])
    if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        scale = max(0.1f, scale - 0.1f);
    if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
        scale = min(3.0f, scale + 0.1f);

    // Toggle wireframe mode (F)
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
        wireframeMode = !wireframeMode;
        
    // Toggle axis display (G)
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
        showAxes = !showAxes;
        
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
        rotateX = rotateY = rotateZ = false;
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

    // Limita o pitch para evitar que a câmera vire de cabeça para baixo
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

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
        // Face frontal (vermelha)
        // x    y    z    r    g    b
        -0.5, -0.5,  0.5, 1.0, 0.0, 0.0, // 0
         0.5, -0.5,  0.5, 1.0, 0.0, 0.0, // 1
         0.5,  0.5,  0.5, 1.0, 0.0, 0.0, // 2
        
        -0.5, -0.5,  0.5, 1.0, 0.0, 0.0, // 0
         0.5,  0.5,  0.5, 1.0, 0.0, 0.0, // 2
        -0.5,  0.5,  0.5, 1.0, 0.0, 0.0, // 3
        
        // Face traseira (verde)
        -0.5, -0.5, -0.5, 0.0, 1.0, 0.0, // 4
        -0.5,  0.5, -0.5, 0.0, 1.0, 0.0, // 7
         0.5,  0.5, -0.5, 0.0, 1.0, 0.0, // 6
        
        -0.5, -0.5, -0.5, 0.0, 1.0, 0.0, // 4
         0.5,  0.5, -0.5, 0.0, 1.0, 0.0, // 6
         0.5, -0.5, -0.5, 0.0, 1.0, 0.0, // 5
        
        // Face superior (azul)
        -0.5,  0.5, -0.5, 0.0, 0.0, 1.0, // 7
        -0.5,  0.5,  0.5, 0.0, 0.0, 1.0, // 3
         0.5,  0.5,  0.5, 0.0, 0.0, 1.0, // 2
        
        -0.5,  0.5, -0.5, 0.0, 0.0, 1.0, // 7
         0.5,  0.5,  0.5, 0.0, 0.0, 1.0, // 2
         0.5,  0.5, -0.5, 0.0, 0.0, 1.0, // 6
        
        // Face inferior (amarela)
        -0.5, -0.5, -0.5, 1.0, 1.0, 0.0, // 4
         0.5, -0.5, -0.5, 1.0, 1.0, 0.0, // 5
         0.5, -0.5,  0.5, 1.0, 1.0, 0.0, // 1
        
        -0.5, -0.5, -0.5, 1.0, 1.0, 0.0, // 4
         0.5, -0.5,  0.5, 1.0, 1.0, 0.0, // 1
        -0.5, -0.5,  0.5, 1.0, 1.0, 0.0, // 0
        
        // Face direita (magenta)
         0.5, -0.5, -0.5, 1.0, 0.0, 1.0, // 5
         0.5,  0.5, -0.5, 1.0, 0.0, 1.0, // 6
         0.5,  0.5,  0.5, 1.0, 0.0, 1.0, // 2
        
         0.5, -0.5, -0.5, 1.0, 0.0, 1.0, // 5
         0.5,  0.5,  0.5, 1.0, 0.0, 1.0, // 2
         0.5, -0.5,  0.5, 1.0, 0.0, 1.0, // 1
        
        // Face esquerda (ciano)
        -0.5, -0.5, -0.5, 0.0, 1.0, 1.0, // 4
        -0.5, -0.5,  0.5, 0.0, 1.0, 1.0, // 0
        -0.5,  0.5,  0.5, 0.0, 1.0, 1.0, // 3
        
        -0.5, -0.5, -0.5, 0.0, 1.0, 1.0, // 4
        -0.5,  0.5,  0.5, 0.0, 1.0, 1.0, // 3
        -0.5,  0.5, -0.5, 0.0, 1.0, 1.0, // 7
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