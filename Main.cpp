/*
 * Proyecto Final: Arena de Hockey sobre Hielo
 * Laboratorio de Computación Gráfica e Interacción Humano-Computadora, FI UNAM
 *
 * Iteración 4: jugadores, cámaras e iluminación.
 * Contiene los jugadores, los porteros y el árbitro hechos con modelado jerárquico, la animación
 * del jugador que patina, la cámara de transmisión y la iluminación de la práctica de
 * Iluminación II (luz direccional, reflectores del techo, luces de gol y un reflector que sigue al jugador).
 *
 * Se ve la arena con los dos equipos en la pista y un jugador con el disco iluminado desde el techo.
 *
 * Qué se puede hacer:
 *   W A S D moverse        Mouse mirar        ESC salir
 *   P el jugador empieza o deja de patinar con el disco
 *   C cambiar entre la cámara libre y la cámara de transmisión
 *   L prender y apagar los reflectores del techo
 *   1 / 2 gol del local / del visitante (sube el marcador y prende la luz de gol)
 *   0 regresar el marcador a 0 - 0
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
#include "Model.h"

// stb_image carga las texturas del escenario
#include "SOIL2/SOIL2.h"
#include "stb_image.h"

// Prototipos de las funciones
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void DoMovement();
void Animation();
GLuint CargarTextura(const char *ruta);
void DibujarCubo(Shader &shader, glm::vec3 posicion, glm::vec3 escala, GLuint textura, float rotacionY = 0.0f);
void DibujarParte(Shader &shader, glm::mat4 model, glm::vec3 escala, GLuint textura);
void DibujarPersona(Shader &shader, glm::vec3 posicion, float rotacion, float angPierna, float angBrazo,
	GLuint texCamiseta, GLuint texPantalon, int tipo);

// Tamaño de la ventana
const GLuint WIDTH = 1280, HEIGHT = 720;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// Cámara en primera persona. Empieza en las gradas
Camera camera(glm::vec3(0.0f, 3.1f, 18.5f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -12.0f);
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool keys[1024];
bool firstMouse = true;

// Cámara de transmisión: sigue al jugador desde arriba, como en la tele
bool camTransmision = false;

// Posiciones de las luces puntuales
glm::vec3 pointLightPositions[] = {
	glm::vec3(-15.0f, 10.0f, 0.0f),   // reflector del techo izquierdo
	glm::vec3(15.0f, 10.0f, 0.0f),    // reflector del techo derecho
	glm::vec3(-29.5f, 3.2f, 0.0f),    // luz de gol izquierda (detrás de la portería)
	glm::vec3(29.5f, 3.2f, 0.0f)      // luz de gol derecha
};
bool reflectores = true;

// Marcador y luces de gol
int golesLocal = 0;
int golesVisita = 0;
float tiempoGolIzq = 0.0f;   // segundos que le quedan encendida a cada luz de gol
float tiempoGolDer = 0.0f;

// Variables para animar al jugador (igual que la caminata del perro)
bool animJugador = false;
float piernaJug = 0.0f;                        // ángulo de las piernas
float brazoJug = 0.0f;                         // ángulo de los brazos
bool paso = false;                             // sentido del movimiento de piernas
glm::vec3 posJugador(-20.0f, 0.0f, -4.0f);     // posición del jugador
float rotJugador = 90.0f;                      // 90 = mira hacia +x
int estadoJug = 0;                             // 0 avanza, 1 gira, 2 regresa, 3 gira

// Tipos de persona
const int JUGADOR = 0;
const int PORTERO = 1;
const int ARBITRO = 2;

// Texturas que también usan las personas
GLuint texPiel, texNegro, texMadera, texMetal, texBlancoTela;

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

	GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Proyecto Final - Arena de Hockey", nullptr, nullptr);
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
	Shader lampShader("Shader/lamp.vs", "Shader/lamp.frag");

	// Cargamos los modelos .obj
	Model aficionadoLocal((char *)"Models/aficionado_local.obj");
	Model aficionadoVisita((char *)"Models/aficionado_visita.obj");
	Model palo((char *)"Models/palo.obj");
	Model botella((char *)"Models/botella.obj");
	Model disco((char *)"Models/disco.obj");

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
	glEnableVertexAttribArray(0);
	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Coordenadas de textura
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// Cargamos las texturas
	GLuint texHielo = CargarTextura("Textures/pista_hielo.png");
	GLuint texValla1 = CargarTextura("Textures/valla1.png");
	GLuint texValla2 = CargarTextura("Textures/valla2.png");
	GLuint texRed = CargarTextura("Textures/red.png");
	GLuint texCristal = CargarTextura("Textures/cristal.png");
	GLuint texMarcador = CargarTextura("Textures/marcador.png");
	GLuint texRojo = CargarTextura("Textures/rojo.png");
	GLuint texAzul = CargarTextura("Textures/azul.png");
	GLuint texGris = CargarTextura("Textures/gris.png");
	GLuint texHule = CargarTextura("Textures/hule.png");
	GLuint texGradaRoja = CargarTextura("Textures/grada_roja.png");
	GLuint texGradaAzul = CargarTextura("Textures/grada_azul.png");
	GLuint texRayas = CargarTextura("Textures/rayas.png");
	texPiel = CargarTextura("Textures/piel.png");
	texNegro = CargarTextura("Textures/negro.png");
	texMadera = CargarTextura("Textures/madera.png");
	texMetal = CargarTextura("Textures/metal.png");
	texBlancoTela = CargarTextura("Textures/blanco_tela.png");
	GLuint texNumeros[10];
	for (int i = 0; i < 10; i++)
	{
		std::string ruta = "Textures/numeros/" + std::to_string(i) + ".png";
		texNumeros[i] = CargarTextura(ruta.c_str());
	}

	glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 200.0f);

	// Ciclo principal
	while (!glfwWindowShouldClose(window))
	{
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		DoMovement();
		Animation();

		glClearColor(0.05f, 0.06f, 0.09f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Vista: usamos la cámara libre o la de transmisión, según la tecla C
		glm::mat4 view;
		glm::vec3 posCamara;
		if (camTransmision)
		{
			posCamara = glm::vec3(posJugador.x * 0.7f, 9.0f, 21.0f);
			view = glm::lookAt(posCamara, posJugador + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else
		{
			posCamara = camera.GetPosition();
			view = camera.GetViewMatrix();
		}

		lightingShader.Use();
		glUniform1i(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "material.specular"), 0);
		GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
		glUniform3f(viewPosLoc, posCamara.x, posCamara.y, posCamara.z);

		// Luz direccional: es la luz general de la arena, la dejamos tenue
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.2f, -1.0f, -0.3f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), 0.15f, 0.15f, 0.17f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), 0.2f, 0.2f, 0.22f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.1f, 0.1f, 0.1f);

		// Luces puntuales 0 y 1: los reflectores del techo (se apagan con la L)
		glm::vec3 luzReflector = reflectores ? glm::vec3(0.6f, 0.6f, 0.58f) : glm::vec3(0.0f);
		for (int i = 0; i < 2; i++)
		{
			std::string n = "pointLights[" + std::to_string(i) + "].";
			glUniform3f(glGetUniformLocation(lightingShader.Program, (n + "position").c_str()), pointLightPositions[i].x, pointLightPositions[i].y, pointLightPositions[i].z);
			glUniform3f(glGetUniformLocation(lightingShader.Program, (n + "ambient").c_str()), luzReflector.x * 0.3f, luzReflector.y * 0.3f, luzReflector.z * 0.3f);
			glUniform3f(glGetUniformLocation(lightingShader.Program, (n + "diffuse").c_str()), luzReflector.x, luzReflector.y, luzReflector.z);
			glUniform3f(glGetUniformLocation(lightingShader.Program, (n + "specular").c_str()), luzReflector.x * 0.5f, luzReflector.y * 0.5f, luzReflector.z * 0.5f);
			glUniform1f(glGetUniformLocation(lightingShader.Program, (n + "constant").c_str()), 1.0f);
			glUniform1f(glGetUniformLocation(lightingShader.Program, (n + "linear").c_str()), 0.014f);
			glUniform1f(glGetUniformLocation(lightingShader.Program, (n + "quadratic").c_str()), 0.0007f);
		}

		// Luces puntuales 2 y 3: las luces rojas de gol. Se prenden 3 segundos cuando hay gol
		// en esa portería y el resto del tiempo están apagadas (color negro).
		glm::vec3 rojoIzq = (tiempoGolIzq > 0.0f) ? glm::vec3(2.0f, 0.1f, 0.1f) : glm::vec3(0.0f);
		glm::vec3 rojoDer = (tiempoGolDer > 0.0f) ? glm::vec3(2.0f, 0.1f, 0.1f) : glm::vec3(0.0f);
		// Luz de gol izquierda
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), pointLightPositions[2].x, pointLightPositions[2].y, pointLightPositions[2].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), rojoIzq.x * 0.15f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), rojoIzq.x, rojoIzq.y, rojoIzq.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), rojoIzq.x * 0.5f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.09f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.032f);
		// Luz de gol derecha
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), pointLightPositions[3].x, pointLightPositions[3].y, pointLightPositions[3].z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), rojoDer.x * 0.15f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), rojoDer.x, rojoDer.y, rojoDer.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), rojoDer.x * 0.5f, 0.0f, 0.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.09f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.032f);

		// Reflector de cono: está en el techo y siempre apunta al jugador
		glm::vec3 posCanon(0.0f, 16.0f, 10.0f);
		glm::vec3 dirCanon = glm::normalize(posJugador + glm::vec3(0.0f, 1.0f, 0.0f) - posCanon);
		glm::vec3 luzCanon = reflectores ? glm::vec3(1.0f, 1.0f, 0.9f) : glm::vec3(0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.position"), posCanon.x, posCanon.y, posCanon.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.direction"), dirCanon.x, dirCanon.y, dirCanon.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), luzCanon.x, luzCanon.y, luzCanon.z);
		glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), luzCanon.x, luzCanon.y, luzCanon.z);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.014f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.0007f);
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(4.0f)));
		glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(6.0f)));

		// Brillo del material y transparencia para red y cristal
		glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 32.0f);
		glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 1);

		// Manda la vista y la proyección al shader
		glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(lightingShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);

		// Zona 1: la pista de juego

		// Piso de toda la arena
		DibujarCubo(lightingShader, glm::vec3(0.0f, -0.05f, 0.0f), glm::vec3(90.0f, 0.1f, 50.0f), texGris);

		// Hielo: la textura ya trae las líneas, los círculos y las áreas de portero
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(60.96f, 0.02f, 25.91f), texHielo);

		// Vallas: cada tramo es un cubo blanco y un panel delgado con la publicidad del lado de
		// la pista (si le ponemos la textura a todo el cubo, desde las gradas se ve al revés).
		// Lados largos: 15 tramos de 4.064 m
		for (int i = 0; i < 15; i++)
		{
			float x = -30.48f + 4.064f * (i + 0.5f);
			GLuint tex = (i % 2 == 0) ? texValla1 : texValla2;
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, -13.03f), glm::vec3(4.064f, 1.07f, 0.15f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, 13.03f), glm::vec3(4.064f, 1.07f, 0.15f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, -12.949f), glm::vec3(4.064f, 1.07f, 0.01f), tex, 0.0f);
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, 12.949f), glm::vec3(4.064f, 1.07f, 0.01f), tex, 180.0f);
		}
		// Fondos: 6 tramos de 4.318 m (los paneles se giran para que la publicidad mire a la pista)
		for (int i = 0; i < 6; i++)
		{
			float z = -12.955f + 4.318f * (i + 0.5f);
			GLuint tex = (i % 2 == 0) ? texValla2 : texValla1;
			DibujarCubo(lightingShader, glm::vec3(30.555f, 0.535f, z), glm::vec3(0.15f, 1.07f, 4.318f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(-30.555f, 0.535f, z), glm::vec3(0.15f, 1.07f, 4.318f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(30.474f, 0.535f, z), glm::vec3(4.318f, 1.07f, 0.01f), tex, -90.0f);
			DibujarCubo(lightingShader, glm::vec3(-30.474f, 0.535f, z), glm::vec3(4.318f, 1.07f, 0.01f), tex, 90.0f);
		}

		// Porterías: una en cada línea de gol (a 3.35 m del fondo)
		for (int lado = -1; lado <= 1; lado += 2)
		{
			float gx = 27.13f * lado;      // línea de gol
			float atras = 1.0f * lado;     // la red va hacia el fondo de la pista
			// Postes y travesaño (rojos)
			DibujarCubo(lightingShader, glm::vec3(gx, 0.61f, -0.915f), glm::vec3(0.06f, 1.22f, 0.06f), texRojo);
			DibujarCubo(lightingShader, glm::vec3(gx, 0.61f, 0.915f), glm::vec3(0.06f, 1.22f, 0.06f), texRojo);
			DibujarCubo(lightingShader, glm::vec3(gx, 1.22f, 0.0f), glm::vec3(0.06f, 0.06f, 1.89f), texRojo);
			// Red: atrás, arriba y a los lados
			DibujarCubo(lightingShader, glm::vec3(gx + atras, 0.61f, 0.0f), glm::vec3(0.02f, 1.22f, 1.83f), texRed);
			DibujarCubo(lightingShader, glm::vec3(gx + atras / 2, 1.22f, 0.0f), glm::vec3(1.0f, 0.02f, 1.83f), texRed);
			DibujarCubo(lightingShader, glm::vec3(gx + atras / 2, 0.61f, -0.915f), glm::vec3(1.0f, 1.22f, 0.02f), texRed);
			DibujarCubo(lightingShader, glm::vec3(gx + atras / 2, 0.61f, 0.915f), glm::vec3(1.0f, 1.22f, 0.02f), texRed);
			// Base de la luz de gol, sobre el cristal detrás de la portería
			DibujarCubo(lightingShader, glm::vec3(29.5f * lado, 2.95f, 0.0f), glm::vec3(0.3f, 0.2f, 0.3f), texNegro);
		}

		// Personas en la pista (modelado jerárquico)
		// Jugador animado del equipo local (tecla P)
		DibujarPersona(lightingShader, posJugador, rotJugador, piernaJug, brazoJug, texRojo, texNegro, JUGADOR);
		// Otros jugadores quietos, en posición de juego (el azul de abajo)
		DibujarPersona(lightingShader, glm::vec3(-8.0f, 0.0f, 6.0f), 90.0f, 5.0f, 0.0f, texRojo, texNegro, JUGADOR);
		DibujarPersona(lightingShader, glm::vec3(12.0f, 0.0f, 5.0f), -110.0f, 10.0f, 0.0f, texAzul, texNegro, JUGADOR);
		// Porteros dentro de su área, mirando al centro
		DibujarPersona(lightingShader, glm::vec3(-26.3f, 0.0f, 0.0f), 90.0f, 0.0f, 0.0f, texRojo, texNegro, PORTERO);
		DibujarPersona(lightingShader, glm::vec3(26.3f, 0.0f, 0.0f), -90.0f, 0.0f, 0.0f, texAzul, texNegro, PORTERO);
		// Defensor azul: marca al jugador rojo que patina (tecla P). Usa la misma posición del rojo,
		// recorrida 1.6 m hacia las bancas, y los mismos giros y ángulos de piernas y brazos.
		DibujarPersona(lightingShader, posJugador + glm::vec3(0.0f, 0.0f, -1.6f), rotJugador, piernaJug, brazoJug, texAzul, texNegro, JUGADOR);
		// Árbitro: sigue la jugada por la orilla del frente (va a la mitad de la x del jugador rojo)
		DibujarPersona(lightingShader, glm::vec3(posJugador.x * 0.5f, 0.0f, 8.0f), 180.0f, piernaJug, 0.0f, texRayas, texNegro, ARBITRO);

		// Zona 2: bancas de los equipos, cajas de castigo y mesa de oficiales

		// Bancas de los equipos (detrás de la valla de atrás, z negativo)
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.03f, -14.35f), glm::vec3(24.0f, 0.06f, 2.5f), texHule);  // piso de hule
		for (int equipo = -1; equipo <= 1; equipo += 2)
		{
			float xc = 7.0f * equipo;
			GLuint tex = (equipo < 0) ? texRojo : texAzul;
			DibujarCubo(lightingShader, glm::vec3(xc, 0.7f, -15.6f), glm::vec3(9.4f, 1.4f, 0.1f), tex);          // muro de atrás
			DibujarCubo(lightingShader, glm::vec3(xc - 4.7f, 0.6f, -14.35f), glm::vec3(0.1f, 1.2f, 2.5f), tex);   // muros de los lados
			DibujarCubo(lightingShader, glm::vec3(xc + 4.7f, 0.6f, -14.35f), glm::vec3(0.1f, 1.2f, 2.5f), tex);
			DibujarCubo(lightingShader, glm::vec3(xc, 0.42f, -15.05f), glm::vec3(9.0f, 0.08f, 0.45f), texMadera); // asiento de madera
			DibujarCubo(lightingShader, glm::vec3(xc, 0.2f, -15.05f), glm::vec3(8.6f, 0.4f, 0.3f), texHule);      // base
			// Toallas
			for (int i = -3; i <= 3; i += 3)
				DibujarCubo(lightingShader, glm::vec3(xc + i + 0.6f, 0.47f, -15.05f), glm::vec3(0.4f, 0.02f, 0.3f), texBlancoTela);
		}

		// Cajas de castigo y mesa de oficiales (al frente, z positivo)
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.03f, 14.35f), glm::vec3(12.0f, 0.06f, 2.5f), texHule);         // piso
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.7f, 15.6f), glm::vec3(12.0f, 1.4f, 0.1f), texGris);            // muro de atrás
		DibujarCubo(lightingShader, glm::vec3(-4.2f, 0.42f, 15.05f), glm::vec3(3.2f, 0.08f, 0.45f), texRojo);        // banca castigo local
		DibujarCubo(lightingShader, glm::vec3(4.2f, 0.42f, 15.05f), glm::vec3(3.2f, 0.08f, 0.45f), texAzul);         // banca castigo visitante
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.75f, 13.8f), glm::vec3(3.6f, 0.06f, 0.8f), texBlancoTela);     // mesa de oficiales
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.37f, 13.45f), glm::vec3(3.6f, 0.74f, 0.05f), texNegro);        // frente de la mesa
		for (int i = -1; i <= 1; i += 2)
		{
			DibujarCubo(lightingShader, glm::vec3(0.8f * i, 0.45f, 14.6f), glm::vec3(0.45f, 0.08f, 0.45f), texNegro);  // silla
			DibujarCubo(lightingShader, glm::vec3(0.8f * i, 0.75f, 14.82f), glm::vec3(0.45f, 0.55f, 0.05f), texNegro); // respaldo
		}

		// Zona 3: gradas con público y marcador

		// Gradas: 5 escalones en cada lado; cada escalón es un cubo más alto que el anterior.
		// Se pintan del color de la afición que se sienta ahí: rojo el local (izquierda) y azul el visitante (derecha).
		for (int fila = 0; fila < 5; fila++)
		{
			float alto = 0.5f * (fila + 1);
			DibujarCubo(lightingShader, glm::vec3(-17.0f, alto / 2, 16.5f + fila), glm::vec3(34.0f, alto, 1.0f), texGradaRoja); // frente, izquierda
			DibujarCubo(lightingShader, glm::vec3(17.0f, alto / 2, 16.5f + fila), glm::vec3(34.0f, alto, 1.0f), texGradaAzul);  // frente, derecha
			DibujarCubo(lightingShader, glm::vec3(-17.0f, alto / 2, -16.5f - fila), glm::vec3(34.0f, alto, 1.0f), texGradaRoja); // atrás, izquierda
			DibujarCubo(lightingShader, glm::vec3(17.0f, alto / 2, -16.5f - fila), glm::vec3(34.0f, alto, 1.0f), texGradaAzul);  // atrás, derecha
			DibujarCubo(lightingShader, glm::vec3(34.5f + fila, alto / 2, 0.0f), glm::vec3(1.0f, alto, 42.0f), texGradaAzul);    // fondo derecho
			DibujarCubo(lightingShader, glm::vec3(-34.5f - fila, alto / 2, 0.0f), glm::vec3(1.0f, alto, 42.0f), texGradaRoja);   // fondo izquierdo
		}

		// Marcador central colgante
		DibujarCubo(lightingShader, glm::vec3(0.0f, 13.0f, 0.0f), glm::vec3(5.0f, 2.6f, 2.0f), texNegro);     // cuerpo
		DibujarCubo(lightingShader, glm::vec3(-2.0f, 19.0f, 0.0f), glm::vec3(0.05f, 9.4f, 0.05f), texMetal);  // cables
		DibujarCubo(lightingShader, glm::vec3(2.0f, 19.0f, 0.0f), glm::vec3(0.05f, 9.4f, 0.05f), texMetal);
		// Pantallas (una de cada lado) y números de los goles (cada número es una textura)
		DibujarCubo(lightingShader, glm::vec3(0.0f, 13.0f, 1.01f), glm::vec3(4.6f, 2.4f, 0.02f), texMarcador, 0.0f);
		DibujarCubo(lightingShader, glm::vec3(0.0f, 13.0f, -1.01f), glm::vec3(4.6f, 2.4f, 0.02f), texMarcador, 180.0f);
		DibujarCubo(lightingShader, glm::vec3(-1.4f, 12.8f, 1.03f), glm::vec3(0.8f, 1.1f, 0.01f), texNumeros[golesLocal], 0.0f);
		DibujarCubo(lightingShader, glm::vec3(1.4f, 12.8f, 1.03f), glm::vec3(0.8f, 1.1f, 0.01f), texNumeros[golesVisita], 0.0f);
		DibujarCubo(lightingShader, glm::vec3(1.4f, 12.8f, -1.03f), glm::vec3(0.8f, 1.1f, 0.01f), texNumeros[golesLocal], 180.0f);
		DibujarCubo(lightingShader, glm::vec3(-1.4f, 12.8f, -1.03f), glm::vec3(0.8f, 1.1f, 0.01f), texNumeros[golesVisita], 180.0f);

		// Modelos cargados con Assimp
		glm::mat4 model(1);
		GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");

		// Disco: va delante del palo del jugador animado, así que también hereda su movimiento
		model = glm::mat4(1);
		model = glm::translate(model, posJugador);
		model = glm::rotate(model, glm::radians(rotJugador), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::translate(model, glm::vec3(0.13f, 0.012f, 1.75f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		disco.Draw(lightingShader);

		// Botellas y palos en las bancas
		for (int equipo = -1; equipo <= 1; equipo += 2)
		{
			float xc = 7.0f * equipo;
			for (int i = -4; i <= 4; i += 2)
			{
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(xc + i, 0.46f, -14.95f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				botella.Draw(lightingShader);
			}
			for (int i = 0; i < 4; i++)
			{
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(xc - 1.5f + i, 0.06f, -15.4f));
				model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // recargado en el muro
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				palo.Draw(lightingShader);
			}
		}

		// Público en las filas 1, 3 y 5 de las gradas (afición local a la izquierda, visitante a la derecha)
		for (int fila = 0; fila < 5; fila += 2)
		{
			float y = 0.5f * (fila + 1);
			// Lados largos
			for (float x = -27.5f; x <= 27.5f; x += 5.0f)
			{
				Model &aficionado = (x < 0) ? aficionadoLocal : aficionadoVisita;
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(x, y, 16.5f + fila));
				model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // mira hacia la pista
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				aficionado.Draw(lightingShader);

				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(x, y, -16.5f - fila));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				aficionado.Draw(lightingShader);
			}
			// Fondos
			for (float z = -12.5f; z <= 12.5f; z += 5.0f)
			{
				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(34.5f + fila, y, z));
				model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				aficionadoVisita.Draw(lightingShader);

				model = glm::mat4(1);
				model = glm::translate(model, glm::vec3(-34.5f - fila, y, z));
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
				aficionadoLocal.Draw(lightingShader);
			}
		}

		// Cristal protector: va al final porque es transparente
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBindVertexArray(VAO);
		DibujarCubo(lightingShader, glm::vec3(0.0f, 1.97f, -13.03f), glm::vec3(60.96f, 1.8f, 0.02f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(0.0f, 1.97f, 13.03f), glm::vec3(60.96f, 1.8f, 0.02f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(30.555f, 1.97f, 0.0f), glm::vec3(0.02f, 1.8f, 25.91f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(-30.555f, 1.97f, 0.0f), glm::vec3(0.02f, 1.8f, 25.91f), texCristal);
		// Separadores de cristal de las cajas de castigo
		DibujarCubo(lightingShader, glm::vec3(-2.45f, 1.1f, 14.35f), glm::vec3(0.03f, 2.2f, 2.5f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(2.45f, 1.1f, 14.35f), glm::vec3(0.03f, 2.2f, 2.5f), texCristal);
		glDisable(GL_BLEND);

		// Cubos blancos que marcan dónde están las luces
		lampShader.Use();
		glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glBindVertexArray(VAO);
		for (int i = 0; i < 4; i++)
		{
			// Los reflectores se ven si están encendidos; las luces de gol, solo mientras están prendidas
			if (i < 2 && !reflectores) continue;
			if (i == 2 && tiempoGolIzq <= 0.0f) continue;
			if (i == 3 && tiempoGolDer <= 0.0f) continue;
			model = glm::mat4(1);
			model = glm::translate(model, pointLightPositions[i]);
			model = glm::scale(model, (i < 2) ? glm::vec3(1.0f) : glm::vec3(0.25f));
			glUniformMatrix4fv(glGetUniformLocation(lampShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glfwTerminate();
	return 0;
}

// Dibuja el cubo en una posición, con su tamaño, su textura y un giro en Y
void DibujarCubo(Shader &shader, glm::vec3 posicion, glm::vec3 escala, GLuint textura, float rotacionY)
{
	glm::mat4 model(1);
	model = glm::translate(model, posicion);
	model = glm::rotate(model, glm::radians(rotacionY), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, escala);
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glBindTexture(GL_TEXTURE_2D, textura);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

// Dibuja una parte de una persona: recibe la matriz de su articulación y solo le aplica la escala
void DibujarParte(Shader &shader, glm::mat4 model, glm::vec3 escala, GLuint textura)
{
	model = glm::scale(model, escala);
	glUniformMatrix4fv(glGetUniformLocation(shader.Program, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glBindTexture(GL_TEXTURE_2D, textura);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

// Persona hecha con modelado jerárquico (igual que el brazo y el perro de las prácticas).
// tipo: JUGADOR, PORTERO o ARBITRO.
//   cuerpo -> torso -> cabeza -> casco
//          -> torso -> brazo -> antebrazo -> mano -> palo
//          -> cadera -> muslo -> pierna -> patín
// Cada pieza parte de la matriz guardada de su "padre" (modelTemp), le suma su traslación
// hasta la articulación y luego su propia rotación.
void DibujarPersona(Shader &shader, glm::vec3 posicion, float rotacion, float angPierna, float angBrazo,
	GLuint texCamiseta, GLuint texPantalon, int tipo)
{
	glm::mat4 model(1);
	glm::mat4 modelTemp(1);    // cuerpo (posición y giro de la persona)
	glm::mat4 modelTemp2(1);   // torso
	glm::mat4 modelTemp3(1);   // articulación actual (brazo o pierna)

	// Medidas: el portero es más ancho por su equipo de protección
	float anchoPierna = (tipo == PORTERO) ? 0.3f : 0.18f;
	float separacion = (tipo == PORTERO) ? 0.2f : 0.12f;
	GLuint texPierna = (tipo == PORTERO) ? texBlancoTela : texPantalon;

	// Cuerpo
	modelTemp = model = glm::translate(glm::mat4(1), posicion);
	modelTemp = model = glm::rotate(model, glm::radians(rotacion), glm::vec3(0.0f, 1.0f, 0.0f));

	// Torso: un poco inclinado hacia adelante, como patinador
	model = glm::translate(modelTemp, glm::vec3(0.0f, 0.98f, 0.0f));
	model = glm::rotate(model, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelTemp2 = model = glm::translate(model, glm::vec3(0.0f, 0.3f, 0.0f));
	DibujarParte(shader, model, (tipo == PORTERO) ? glm::vec3(0.6f, 0.6f, 0.4f) : glm::vec3(0.46f, 0.6f, 0.28f), texCamiseta);

	// Cabeza y casco
	model = glm::translate(modelTemp2, glm::vec3(0.0f, 0.42f, 0.0f));
	DibujarParte(shader, model, glm::vec3(0.22f, 0.24f, 0.22f), texPiel);
	model = glm::translate(model, glm::vec3(0.0f, 0.1f, -0.01f));
	DibujarParte(shader, model, glm::vec3(0.26f, 0.12f, 0.27f), (tipo == PORTERO) ? texBlancoTela : texNegro);

	// Brazos (derecho = lado 1, izquierdo = lado -1). Los brazos se mueven al revés de las piernas.
	for (int lado = -1; lado <= 1; lado += 2)
	{
		// Hombro
		model = glm::translate(modelTemp2, glm::vec3(0.31f * lado, 0.24f, 0.0f));
		model = glm::rotate(model, glm::radians(-45.0f + angBrazo * lado), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.17f, 0.0f));
		DibujarParte(shader, model, glm::vec3(0.13f, 0.34f, 0.13f), texCamiseta);
		// Codo y antebrazo
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.34f, 0.0f));
		model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.15f, 0.0f));
		DibujarParte(shader, model, glm::vec3(0.11f, 0.3f, 0.11f), texCamiseta);
		// Mano (guante)
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.33f, 0.0f));
		modelTemp3 = model;
		DibujarParte(shader, model, glm::vec3(0.12f, 0.12f, 0.12f), texNegro);

		// Palo: lo sostiene la mano derecha (el árbitro no lleva palo)
		if (lado == 1 && tipo != ARBITRO)
		{
			model = glm::rotate(modelTemp3, glm::radians(13.0f - angBrazo), glm::vec3(1.0f, 0.0f, 0.0f));  // compensa el brazo para que la pala siga en el hielo
			model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelTemp3 = model = glm::translate(model, glm::vec3(0.0f, -0.7f, 0.0f));
			DibujarParte(shader, model, glm::vec3(0.035f, 1.4f, 0.03f), texMadera);
			// Pala: se endereza para quedar acostada sobre el hielo
			model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.7f, 0.0f));
			model = glm::rotate(model, glm::radians(47.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.12f));
			DibujarParte(shader, model, glm::vec3(0.03f, 0.07f, 0.3f), texNegro);
		}
	}

	// Piernas: se mueven en sentidos opuestos, como las patas del perro
	for (int lado = -1; lado <= 1; lado += 2)
	{
		// Cadera y muslo
		model = glm::translate(modelTemp, glm::vec3(separacion * lado, 0.98f, 0.0f));
		model = glm::rotate(model, glm::radians(angPierna * lado - 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.22f, 0.0f));
		DibujarParte(shader, model, glm::vec3(anchoPierna, 0.44f, anchoPierna), texPierna);
		// Rodilla y pierna
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.44f, 0.0f));
		model = glm::rotate(model, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.2f, 0.0f));
		DibujarParte(shader, model, glm::vec3(anchoPierna * 0.85f, 0.4f, anchoPierna * 0.85f), (tipo == PORTERO) ? texBlancoTela : texCamiseta);
		// Patín (bota y cuchilla)
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.44f, 0.06f));
		model = glm::rotate(model, glm::radians(-angPierna * lado), glm::vec3(1.0f, 0.0f, 0.0f));
		DibujarParte(shader, model, glm::vec3(0.11f, 0.1f, 0.3f), texNegro);
		model = glm::translate(model, glm::vec3(0.0f, -0.07f, 0.0f));
		DibujarParte(shader, model, glm::vec3(0.02f, 0.04f, 0.3f), texMetal);
	}
}

// Carga una imagen con stb_image y la convierte en textura
GLuint CargarTextura(const char *ruta)
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
	unsigned char *image = stbi_load(ruta, &textureWidth, &textureHeight, &nrChannels, 4);
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

// Animaciones: se llama en cada cuadro, igual que en la práctica de animación
void Animation()
{
	// Jugador: máquina de estados como la caminata del perro
	if (animJugador)
	{
		// Piernas y brazos oscilan entre -25 y 25 grados
		if (!paso)
		{
			piernaJug += 60.0f * deltaTime;
			if (piernaJug > 25.0f)
				paso = !paso;
		}
		else
		{
			piernaJug -= 60.0f * deltaTime;
			if (piernaJug < -25.0f)
				paso = !paso;
		}
		brazoJug = -piernaJug * 0.6f;

		// Recorrido: avanza, da la vuelta, regresa y vuelve a dar la vuelta
		if (estadoJug == 0)        // avanza hacia +x
		{
			posJugador.x += 6.0f * deltaTime;
			if (posJugador.x > 20.0f)
				estadoJug = 1;
		}
		else if (estadoJug == 1)   // gira 180 grados
		{
			rotJugador += 180.0f * deltaTime;
			if (rotJugador >= 270.0f)
			{
				rotJugador = 270.0f;
				estadoJug = 2;
			}
		}
		else if (estadoJug == 2)   // regresa hacia -x
		{
			posJugador.x -= 6.0f * deltaTime;
			if (posJugador.x < -20.0f)
				estadoJug = 3;
		}
		else if (estadoJug == 3)   // gira 180 grados
		{
			rotJugador -= 180.0f * deltaTime;
			if (rotJugador <= 90.0f)
			{
				rotJugador = 90.0f;
				estadoJug = 0;
			}
		}
	}

	// Luces de gol: se apagan solas después de 3 segundos
	if (tiempoGolIzq > 0.0f)
		tiempoGolIzq -= deltaTime;
	else
		tiempoGolIzq = 0.0f;
	if (tiempoGolDer > 0.0f)
		tiempoGolDer -= deltaTime;
	else
		tiempoGolDer = 0.0f;
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
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
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

	// Encender y apagar los reflectores
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
		reflectores = !reflectores;

	// Cambiar de cámara
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
		camTransmision = !camTransmision;

	// El jugador empieza o deja de patinar
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
		animJugador = !animJugador;

	// Goles: suben el marcador (del 0 al 9) y encienden la luz de gol de esa portería.
	// El local ataca la portería derecha y el visitante la izquierda.
	if (key == GLFW_KEY_1 && action == GLFW_PRESS && golesLocal < 9)
	{
		golesLocal++;
		tiempoGolDer = 3.0f;
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS && golesVisita < 9)
	{
		golesVisita++;
		tiempoGolIzq = 3.0f;
	}
	if (key == GLFW_KEY_0 && action == GLFW_PRESS)
	{
		golesLocal = 0;
		golesVisita = 0;
	}
}

// Hacia dónde mira la cámara
void MouseCallback(GLFWwindow *window, double xPos, double yPos)
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
