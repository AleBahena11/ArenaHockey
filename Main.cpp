/*
 * Proyecto Final: Arena de Hockey sobre Hielo
 * Laboratorio de Computación Gráfica e Interacción Humano-Computadora, FI UNAM
 *
 * Iteración 1: configuración del proyecto.
 * Contiene la ventana, la cámara en primera persona, el cubo con el que armaremos todo
 * el escenario y un shader con luz direccional y textura.
 *
 * Se ve el piso de la arena (90 x 50 m) y un cubo de prueba en el centro.
 *
 * Qué se puede hacer:
 *   W A S D moverse        Mouse mirar        ESC salir
 */

#include <iostream>
#include <string>
#include <cmath>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Clases de las prácticas
#include "Shader.h"
#include "Camera.h"

// stb_image carga las texturas del escenario
#include "stb_image.h"

// Prototipos de las funciones
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
GLuint CargarTextura(const char* ruta);
void DibujarCubo(Shader& shader, glm::vec3 posicion, glm::vec3 escala, GLuint textura, float rotacionY = 0.0f);

// Tamaño de la ventana
const GLuint WIDTH = 1280, HEIGHT = 720;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Cámara en primera persona. Empieza en las gradas
Camera camera(glm::vec3(0.0f, 3.1f, 18.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -12.0f);
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

// Tiempo entre cuadros
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

int main()
{
	// Iniciamos GLFW y creamos la ventana
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto Final - Arena de Hockey", nullptr, nullptr);
	if (nullptr == window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

	// Funciones que atienden el teclado y el mouse
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewExperimental = GL_TRUE;
	if (GLEW_OK != glewInit())
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return EXIT_FAILURE;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	glEnable(GL_DEPTH_TEST);

	Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");

	// Vértices del cubo
	GLfloat vertices[] =
	{
		// Atrás
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
		// Frente
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
		// Izquierda
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		// Derecha
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
		 // Abajo
		 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
		  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
		 -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
		 -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
		 // Arriba
		 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
		  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
		  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
		 -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
		 -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
	};

	GLuint VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// Posición
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Coordenadas de textura
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// Cargamos las texturas
	GLuint texGris = CargarTextura("Textures/gris.png");
	GLuint texConcreto = CargarTextura("Textures/concreto.png");

	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 200.0f);

	// Ciclo principal
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();

		glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Vista de la cámara en primera persona
		glm::mat4 view = camera.GetViewMatrix();
		glm::vec3 posCamara = camera.GetPosition();

		lightingShader.Use();
		glUniform1i(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "material.specular"), 0);
		GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
		glUniform3f(viewPosLoc, posCamara.x, posCamara.y, posCamara.z);

		// Luz direccional, única luz de la arena
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.45f, 0.45f, 0.47f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.6f, 0.6f, 0.6f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.3f, 0.3f, 0.3f);

		// Brillo del material y transparencia para red y cristal
		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 32.0f);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 1);

		// Manda la vista y la proyección al shader
		glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);

		// Escenario

		// Piso de toda la arena
		DibujarCubo(lightingShader, glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(90.0f, 0.1f, 50.0f), texGris);

		// Cubo de prueba con textura, para revisar la luz y la cámara
		DibujarCubo(lightingShader, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(2.0f, 2.0f, 2.0f), texConcreto);

		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	return 0;
}

// Dibuja el cubo en una posición, con su tamaño, su textura y un giro en Y
void DibujarCubo(Shader& shader, glm::vec3 posicion, glm::vec3 escala, GLuint textura, float rotacionY)
{
	glm::mat4 model(1);
	model = glm::translate(model, posicion);
	model = glm::rotate(model, glm::radians(rotacionY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, escala);
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glBindTexture(GL_TEXTURE_2D, textura);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

// Carga una imagen con stb_image y la convierte en textura
GLuint CargarTextura(const char* ruta)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int textureWidth, textureHeight, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* image = stbi_load(ruta, &textureWidth, &textureHeight, &nrChannels, 4);
	if (image)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "No se pudo cargar la textura: " << ruta << std::endl;
	}
	stbi_image_free(image);
	return texture;
}

// Movimiento de la cámara con el teclado
void DoMovement()
{
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// Teclas
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

// Hacia dónde mira la cámara
void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}
	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;
	lastX = xPos;
	lastY = yPos;
	camera.ProcessMouseMovement(xOffset, yOffset);
}
