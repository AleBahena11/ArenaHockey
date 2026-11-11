/*
 * Proyecto Final: Arena de Hockey sobre Hielo
 * Laboratorio de Computación Gráfica e Interacción Humano-Computadora, FI UNAM
 *
 * Iteración 5: animación por keyframes.
 * Contiene la jugada completa hecha con keyframes, como en la práctica: con la tecla G el equipo local
 * sale del centro, un defensor lo persigue, da el pase, su compañero tira y anota (sube el marcador,
 * suena la bocina y se prende la luz de gol) y festeja. Después, la misma jugada la hace el equipo
 * visitante hacia la otra portería.
 *
 * Se ve la arena completa, con jugadores en la pista y en las bancas.
 *
 * Qué se puede hacer:
 *   W A S D moverse        Mouse mirar        ESC salir
 *   G reproducir la jugada (primero anota el local y luego el visitante); si se está reproduciendo, la detiene
 *   C cambiar entre la cámara libre y la cámara de transmisión
 *   V ver desde los ojos del jugador que tira (otra vez V regresa)
 *   L prender y apagar los reflectores del techo
 *   1 / 2 gol del local / del visitante (sube el marcador, prende la luz de gol y suena la bocina)
 *   0 regresar el marcador a 0 - 0
 */

#include <iostream>
#include <string>
#include <cmath>

// Sonido de gol: PlaySound viene en Windows (biblioteca winmm)
#define NOMINMAX
#include <windows.h>
#pragma comment(lib, "winmm.lib")

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

// Cámara desde los ojos del jugador que tira en la jugada de gol (tecla V)
bool camJugador = false;

// Posiciones de las luces puntuales
glm::vec3 pointLightPositions[] = {
	glm::vec3(-15.0f, 15.0f, 0.0f),   // reflector del techo izquierdo (colgado de la viga)
	glm::vec3(15.0f, 15.0f, 0.0f),    // reflector del techo derecho (colgado de la viga)
	glm::vec3(-29.5f, 3.2f, 0.0f),    // luz de gol izquierda (detrás de la portería)
	glm::vec3(29.5f, 3.2f, 0.0f)      // luz de gol derecha
};
bool reflectores = true;

// Marcador y luces de gol
int golesLocal = 0;
int golesVisita = 0;
float tiempoGolIzq = 0.0f;   // segundos que le quedan encendida a cada luz de gol
float tiempoGolDer = 0.0f;

// Piernas y brazos de los que patinan: suben y bajan como la caminata del perro (máquina de estados)
float piernaJug = 0.0f;                        // ángulo de las piernas
float brazoJug = 0.0f;                         // ángulo de los brazos
bool paso = false;                             // sentido del movimiento de piernas

// KeyFrames: la jugada completa (igual que en la práctica de keyframes).
// Las poses son las del ataque del local hacia la portería derecha; para el ataque del visitante
// se usan las mismas poses al revés (x y z con signo contrario y el giro + 180 grados).
float pasPosX, pasPosZ;     // posición del jugador que da el pase
float rotPas;               // hacia dónde mira el que da el pase
float jugPosX, jugPosZ;     // posición del jugador que tira
float rotJug;               // hacia dónde mira
float brazo;                // ángulo de los brazos (levanta el palo y tira)
float discoX, discoZ;       // posición del disco de la jugada
float porteroZ;             // posición del portero que defiende (se mueve hacia el lado del tiro)
int equipo = 0;             // equipo que ataca: 0 local (rojo), 1 visitante (azul)

#define MAX_FRAMES 9
int i_max_steps = 45;
int i_curr_steps = 0;
typedef struct _frame {
	float pasPosX;
	float pasPosZ;
	float rotPas;
	float jugPosX;
	float jugPosZ;
	float rotJug;
	float brazo;
	float discoX;
	float discoZ;
	float porteroZ;
	float pasXInc;
	float pasZInc;
	float rotPasInc;
	float incX;
	float incZ;
	float rotJugInc;
	float brazoInc;
	float discoXInc;
	float discoZInc;
	float porteroZInc;
}FRAME;
FRAME KeyFrame[MAX_FRAMES];
int FrameIndex = 0;
bool play = false;
int playIndex = 0;

// Tipos de persona
const int JUGADOR = 0;
const int PORTERO = 1;
const int ARBITRO = 2;
const int SENTADO = 3;   // jugador sentado en la banca
const int TIRADO = 4;    // jugador acostado en el hielo (la pelea)

// Texturas que también usan las personas
GLuint texPiel, texNegro, texMadera, texMetal, texBlancoTela;

// Tiempo entre cuadros
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Guarda la pose actual como un cuadro clave
void saveFrame(void)
{
	std::cout << "frameindex " << FrameIndex << std::endl;
	KeyFrame[FrameIndex].pasPosX = pasPosX;
	KeyFrame[FrameIndex].pasPosZ = pasPosZ;
	KeyFrame[FrameIndex].rotPas = rotPas;
	KeyFrame[FrameIndex].jugPosX = jugPosX;
	KeyFrame[FrameIndex].jugPosZ = jugPosZ;
	KeyFrame[FrameIndex].rotJug = rotJug;
	KeyFrame[FrameIndex].brazo = brazo;
	KeyFrame[FrameIndex].discoX = discoX;
	KeyFrame[FrameIndex].discoZ = discoZ;
	KeyFrame[FrameIndex].porteroZ = porteroZ;
	FrameIndex++;
}

// Regresa al jugador y al disco a la pose del primer cuadro
void resetElements(void)
{
	pasPosX = KeyFrame[0].pasPosX;
	pasPosZ = KeyFrame[0].pasPosZ;
	rotPas = KeyFrame[0].rotPas;
	jugPosX = KeyFrame[0].jugPosX;
	jugPosZ = KeyFrame[0].jugPosZ;
	rotJug = KeyFrame[0].rotJug;
	brazo = KeyFrame[0].brazo;
	discoX = KeyFrame[0].discoX;
	discoZ = KeyFrame[0].discoZ;
	porteroZ = KeyFrame[0].porteroZ;
}

// Calcula cuánto cambia cada variable en cada paso entre el cuadro actual y el siguiente
void interpolation(void)
{
	KeyFrame[playIndex].pasXInc = (KeyFrame[playIndex + 1].pasPosX - KeyFrame[playIndex].pasPosX) / i_max_steps;
	KeyFrame[playIndex].pasZInc = (KeyFrame[playIndex + 1].pasPosZ - KeyFrame[playIndex].pasPosZ) / i_max_steps;
	KeyFrame[playIndex].rotPasInc = (KeyFrame[playIndex + 1].rotPas - KeyFrame[playIndex].rotPas) / i_max_steps;
	KeyFrame[playIndex].incX = (KeyFrame[playIndex + 1].jugPosX - KeyFrame[playIndex].jugPosX) / i_max_steps;
	KeyFrame[playIndex].incZ = (KeyFrame[playIndex + 1].jugPosZ - KeyFrame[playIndex].jugPosZ) / i_max_steps;
	KeyFrame[playIndex].rotJugInc = (KeyFrame[playIndex + 1].rotJug - KeyFrame[playIndex].rotJug) / i_max_steps;
	KeyFrame[playIndex].brazoInc = (KeyFrame[playIndex + 1].brazo - KeyFrame[playIndex].brazo) / i_max_steps;
	KeyFrame[playIndex].discoXInc = (KeyFrame[playIndex + 1].discoX - KeyFrame[playIndex].discoX) / i_max_steps;
	KeyFrame[playIndex].discoZInc = (KeyFrame[playIndex + 1].discoZ - KeyFrame[playIndex].discoZ) / i_max_steps;
	KeyFrame[playIndex].porteroZInc = (KeyFrame[playIndex + 1].porteroZ - KeyFrame[playIndex].porteroZ) / i_max_steps;
}

// Poses de la jugada del local (el visitante usa las mismas, al revés).
// En la práctica cada pose se acomodaba con el teclado y se guardaba con la tecla K;
// aquí cada pose se escribe y se guarda con la misma función saveFrame().
void cargarJugada(void)
{
	// Cuadro 0: sale del centro con el disco y el defensor lo persigue
	pasPosX = 0.0f; pasPosZ = -5.5f; rotPas = 80.0f;
	jugPosX = 6.0f; jugPosZ = 6.5f; rotJug = 90.0f; brazo = 0.0f;
	discoX = 1.72f; discoZ = -5.25f; porteroZ = 0.0f;
	saveFrame();

	// Cuadro 1: sigue avanzando con el disco
	pasPosX = 6.0f; pasPosZ = -4.5f; rotPas = 70.0f;
	jugPosX = 10.0f; jugPosZ = 5.8f; rotJug = 95.0f; brazo = 0.0f;
	discoX = 7.65f; discoZ = -3.95f; porteroZ = 0.0f;
	saveFrame();

	// Cuadro 2: da el pase a su compañero
	pasPosX = 9.5f; pasPosZ = -3.5f; rotPas = 45.0f;
	jugPosX = 13.0f; jugPosZ = 5.2f; rotJug = 100.0f; brazo = 0.0f;
	discoX = 10.77f; discoZ = -2.31f; porteroZ = 0.0f;
	saveFrame();

	// Cuadro 3: el compañero recibe el disco
	pasPosX = 12.0f; pasPosZ = -2.5f; rotPas = 60.0f;
	jugPosX = 16.0f; jugPosZ = 4.4f; rotJug = 100.0f; brazo = 0.0f;
	discoX = 17.71f; discoZ = 4.05f; porteroZ = 0.0f;
	saveFrame();

	// Cuadro 4: el que recibe avanza hacia la portería
	pasPosX = 14.0f; pasPosZ = -1.8f; rotPas = 75.0f;
	jugPosX = 19.0f; jugPosZ = 3.6f; rotJug = 95.0f; brazo = 0.0f;
	discoX = 20.73f; discoZ = 3.4f; porteroZ = 0.15f;
	saveFrame();

	// Cuadro 5: levanta el palo; el disco se queda adelante
	pasPosX = 15.0f; pasPosZ = -1.5f; rotPas = 80.0f;
	jugPosX = 19.8f; jugPosZ = 3.3f; rotJug = 105.0f; brazo = 70.0f;
	discoX = 21.87f; discoZ = 2.7f; porteroZ = 0.2f;
	saveFrame();

	// Cuadro 6: baja el palo y le pega al disco
	pasPosX = 16.0f; pasPosZ = -1.2f; rotPas = 85.0f;
	jugPosX = 20.2f; jugPosZ = 3.2f; rotJug = 105.0f; brazo = 0.0f;
	discoX = 21.87f; discoZ = 2.7f; porteroZ = 0.2f;
	saveFrame();

	// Cuadro 7: el disco entra por la esquina (gol); el portero se lanzó tarde
	pasPosX = 16.5f; pasPosZ = -1.0f; rotPas = 90.0f;
	jugPosX = 20.6f; jugPosZ = 3.05f; rotJug = 105.0f; brazo = -30.0f;
	discoX = 27.55f; discoZ = 0.5f; porteroZ = 0.45f;
	saveFrame();

	// Cuadro 8: festejo
	pasPosX = 17.0f; pasPosZ = -0.8f; rotPas = 90.0f;
	jugPosX = 22.5f; jugPosZ = 2.2f; rotJug = 140.0f; brazo = -100.0f;
	discoX = 27.55f; discoZ = 0.5f; porteroZ = 0.45f;
	saveFrame();
}

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
	glfwSwapInterval(1);   // 60 cuadros por segundo, para que los keyframes vayan a la misma velocidad
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
	Model porteria((char *)"Models/porteria.obj");
	// Mascota: el perro en piezas de la práctica de animación
	Model DogBody((char *)"Models/DogBody.obj");
	Model HeadDog((char *)"Models/HeadDog.obj");
	Model DogTail((char *)"Models/TailDog.obj");
	Model F_RightLeg((char *)"Models/F_RightLegDog.obj");
	Model F_LeftLeg((char *)"Models/F_LeftLegDog.obj");
	Model B_RightLeg((char *)"Models/B_RightLegDog.obj");
	Model B_LeftLeg((char *)"Models/B_LeftLegDog.obj");

	// KeyFrames
	for (int i = 0; i < MAX_FRAMES; i++)
	{
		KeyFrame[i].pasPosX = 0;
		KeyFrame[i].pasPosZ = 0;
		KeyFrame[i].rotPas = 0;
		KeyFrame[i].pasXInc = 0;
		KeyFrame[i].pasZInc = 0;
		KeyFrame[i].rotPasInc = 0;
		KeyFrame[i].jugPosX = 0;
		KeyFrame[i].jugPosZ = 0;
		KeyFrame[i].rotJug = 0;
		KeyFrame[i].brazo = 0;
		KeyFrame[i].discoX = 0;
		KeyFrame[i].discoZ = 0;
		KeyFrame[i].incX = 0;
		KeyFrame[i].incZ = 0;
		KeyFrame[i].rotJugInc = 0;
		KeyFrame[i].brazoInc = 0;
		KeyFrame[i].discoXInc = 0;
		KeyFrame[i].discoZInc = 0;
		KeyFrame[i].porteroZ = 0;
		KeyFrame[i].porteroZInc = 0;
	}
	cargarJugada();
	resetElements();   // el jugador espera en la pose del primer cuadro

	// Controles en la consola, para tenerlos a la vista
	std::cout << "\nControles:" << std::endl;
	std::cout << "  W A S D y mouse: moverse y mirar" << std::endl;
	std::cout << "  G: jugada completa (keyframes): anota el local y luego el visitante" << std::endl;
	std::cout << "  C: camara de transmision          V: ojos del jugador que tira" << std::endl;
	std::cout << "  L: reflectores                    1 / 2: gol local / visitante    0: marcador en 0" << std::endl;
	std::cout << "  ESC: salir" << std::endl;

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
	// Publicidad de las vallas (5 anuncios que se van repitiendo)
	GLuint texVallas[5];
	for (int i = 0; i < 5; i++)
	{
		std::string ruta = "Textures/valla" + std::to_string(i + 1) + ".png";
		texVallas[i] = CargarTextura(ruta.c_str());
	}
	GLuint texCristal = CargarTextura("Textures/cristal.png");
	GLuint texMalla = CargarTextura("Textures/malla.png");
	GLuint texLetrero = CargarTextura("Textures/vestidores.png");
	GLuint texMarcador = CargarTextura("Textures/marcador.png");
	GLuint texRojo = CargarTextura("Textures/rojo.png");
	GLuint texAzul = CargarTextura("Textures/azul.png");
	GLuint texGris = CargarTextura("Textures/gris.png");
	GLuint texHule = CargarTextura("Textures/hule.png");
	GLuint texConcreto = CargarTextura("Textures/concreto.png");
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

		// Lado del equipo que ataca: el visitante usa las mismas poses al revés
		float lado = (equipo == 0) ? 1.0f : -1.0f;
		float giroLado = (equipo == 0) ? 0.0f : 180.0f;
		// La cámara de transmisión y el reflector siguen al disco
		glm::vec3 objetivo(discoX * lado, 0.0f, discoZ * lado);

		// Vista: usamos la cámara libre o la de transmisión, según la tecla C
		glm::mat4 view;
		glm::vec3 posCamara;
		if (camJugador)
		{
			// Ojos del jugador que tira: a 1.75 m de alto, un poco adelante de su cabeza,
			// mirando hacia donde está volteando (un poco hacia el hielo)
			glm::vec3 adelante(sin(glm::radians(rotJug + giroLado)), 0.0f, cos(glm::radians(rotJug + giroLado)));
			posCamara = glm::vec3(jugPosX * lado, 1.75f, jugPosZ * lado) + adelante * 0.7f;
			view = glm::lookAt(posCamara, posCamara + adelante * 5.0f + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else if (camTransmision)
		{
			posCamara = glm::vec3(objetivo.x * 0.7f, 9.0f, 20.0f);
			view = glm::lookAt(posCamara, objetivo + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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
		glm::vec3 luzReflector = reflectores ? glm::vec3(0.75f, 0.75f, 0.72f) : glm::vec3(0.0f);
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
		glm::vec3 posCanon(0.0f, 15.0f, 10.0f);   // colgado del techo
		glm::vec3 dirCanon = glm::normalize(objetivo + glm::vec3(0.0f, 1.0f, 0.0f) - posCanon);
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
			GLuint tex = texVallas[i % 5];
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, -13.03f), glm::vec3(4.064f, 1.07f, 0.15f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, 13.03f), glm::vec3(4.064f, 1.07f, 0.15f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, -12.949f), glm::vec3(4.064f, 1.07f, 0.01f), tex, 0.0f);
			DibujarCubo(lightingShader, glm::vec3(x, 0.535f, 12.949f), glm::vec3(4.064f, 1.07f, 0.01f), tex, 180.0f);
		}
		// Fondos: 6 tramos de 4.318 m (los paneles se giran para que la publicidad mire a la pista)
		for (int i = 0; i < 6; i++)
		{
			float z = -12.955f + 4.318f * (i + 0.5f);
			GLuint tex = texVallas[(i + 2) % 5];
			DibujarCubo(lightingShader, glm::vec3(30.555f, 0.535f, z), glm::vec3(0.15f, 1.07f, 4.318f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(-30.555f, 0.535f, z), glm::vec3(0.15f, 1.07f, 4.318f), texBlancoTela);
			DibujarCubo(lightingShader, glm::vec3(30.474f, 0.535f, z), glm::vec3(4.318f, 1.07f, 0.01f), tex, -90.0f);
			DibujarCubo(lightingShader, glm::vec3(-30.474f, 0.535f, z), glm::vec3(4.318f, 1.07f, 0.01f), tex, 90.0f);
		}

		// Bases de las luces de gol, sobre el cristal detrás de cada portería.
		// Las porterías son un modelo de Blender y se dibujan junto con el cristal, porque su red es transparente.
		for (int lado = -1; lado <= 1; lado += 2)
			DibujarCubo(lightingShader, glm::vec3(29.5f * lado, 2.95f, 0.0f), glm::vec3(0.3f, 0.2f, 0.3f), texNegro);

		// Personas en la pista (modelado jerárquico)
		// La jugada (tecla G): el que da el pase, el que tira y el defensor que persigue al del pase
		GLuint texAtaca = (equipo == 0) ? texRojo : texAzul;
		GLuint texDefiende = (equipo == 0) ? texAzul : texRojo;
		glm::vec3 posPas(pasPosX * lado, 0.0f, pasPosZ * lado);
		DibujarPersona(lightingShader, posPas, rotPas + giroLado, piernaJug, brazoJug, texAtaca, texNegro, JUGADOR);
		DibujarPersona(lightingShader, glm::vec3(jugPosX * lado, 0.0f, jugPosZ * lado), rotJug + giroLado, piernaJug, brazo, texAtaca, texNegro, JUGADOR);
		// Defensor: va 1.5 m detrás del que da el pase, mirando hacia el mismo lado
		glm::vec3 haciaAdelante(sin(glm::radians(rotPas + giroLado)), 0.0f, cos(glm::radians(rotPas + giroLado)));
		DibujarPersona(lightingShader, posPas - 1.5f * haciaAdelante + glm::vec3(0.0f, 0.0f, -0.8f * lado), rotPas + giroLado, -piernaJug, brazoJug, texDefiende, texNegro, JUGADOR);
		// Porteros: el que defiende se mueve hacia el lado del tiro (keyframes); el otro se queda en medio
		DibujarPersona(lightingShader, glm::vec3(26.3f, 0.0f, (equipo == 0) ? porteroZ : 0.0f), -90.0f, 0.0f, 0.0f, texAzul, texNegro, PORTERO);
		DibujarPersona(lightingShader, glm::vec3(-26.3f, 0.0f, (equipo == 1) ? -porteroZ : 0.0f), 90.0f, 0.0f, 0.0f, texRojo, texNegro, PORTERO);
		// Árbitro: sigue al disco por la orilla del frente
		DibujarPersona(lightingShader, glm::vec3(discoX * lado * 0.5f, 0.0f, 8.0f), 180.0f, piernaJug, 0.0f, texRayas, texNegro, ARBITRO);

		// Detalle: una pelea en la esquina. El jugador azul quedó tirado junto a una mancha roja
		// (un cuadro rojo plano) y el rojo festeja con los brazos arriba.
		DibujarPersona(lightingShader, glm::vec3(-23.0f, 0.0f, 10.0f), 120.0f, 0.0f, 0.0f, texAzul, texNegro, TIRADO);
		DibujarPersona(lightingShader, glm::vec3(-21.3f, 0.0f, 9.0f), -60.0f, 10.0f, -100.0f, texRojo, texNegro, JUGADOR);
		DibujarCubo(lightingShader, glm::vec3(-24.4f, 0.012f, 10.8f), glm::vec3(0.8f, 0.004f, 0.8f), texRojo);   // mancha

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
			// Jugadores sentados en la banca, mirando hacia la pista
			DibujarPersona(lightingShader, glm::vec3(xc - 3.0f, -0.42f, -15.1f), 0.0f, 0.0f, 0.0f, tex, texNegro, SENTADO);
			DibujarPersona(lightingShader, glm::vec3(xc + 1.1f, -0.42f, -15.1f), 0.0f, 0.0f, 0.0f, tex, texNegro, SENTADO);
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
			// Oficial (anotador y cronometrista) sentado, mirando a la pista
			DibujarPersona(lightingShader, glm::vec3(0.8f * i, -0.39f, 14.65f), 180.0f, 0.0f, 0.0f, texBlancoTela, texNegro, SENTADO);
		}

		// Zona 3: gradas con público y marcador

		// Gradas: 5 escalones en cada lado; cada escalón es un cubo más alto que el anterior.
		// Se pintan del color de la afición que se sienta ahí: rojo el local (izquierda) y azul el visitante (derecha).
		for (int fila = 0; fila < 5; fila++)
		{
			float alto = 0.5f * (fila + 1);
			DibujarCubo(lightingShader, glm::vec3(-17.0f, alto / 2, 16.5f + fila), glm::vec3(34.0f, alto, 1.0f), texGradaRoja); // frente, izquierda
			DibujarCubo(lightingShader, glm::vec3(17.0f, alto / 2, 16.5f + fila), glm::vec3(34.0f, alto, 1.0f), texGradaAzul);  // frente, derecha
			DibujarCubo(lightingShader, glm::vec3(-18.0f, alto / 2, -16.5f - fila), glm::vec3(32.0f, alto, 1.0f), texGradaRoja); // atrás, izquierda
			DibujarCubo(lightingShader, glm::vec3(18.0f, alto / 2, -16.5f - fila), glm::vec3(32.0f, alto, 1.0f), texGradaAzul);  // atrás, derecha
			DibujarCubo(lightingShader, glm::vec3(34.5f + fila, alto / 2, 0.0f), glm::vec3(1.0f, alto, 42.0f), texGradaAzul);    // fondo derecho
			DibujarCubo(lightingShader, glm::vec3(-34.5f - fila, alto / 2, 0.0f), glm::vec3(1.0f, alto, 42.0f), texGradaRoja);   // fondo izquierdo
		}

		// Marcador central colgante
		DibujarCubo(lightingShader, glm::vec3(0.0f, 13.0f, 0.0f), glm::vec3(5.0f, 2.6f, 2.0f), texNegro);     // cuerpo
		DibujarCubo(lightingShader, glm::vec3(-2.0f, 14.85f, 0.0f), glm::vec3(0.05f, 1.1f, 0.05f), texMetal);  // cables a la viga
		DibujarCubo(lightingShader, glm::vec3(2.0f, 14.85f, 0.0f), glm::vec3(0.05f, 1.1f, 0.05f), texMetal);
		// Pantallas (una de cada lado) y números de los goles (cada número es una textura)
		DibujarCubo(lightingShader, glm::vec3(0.0f, 13.0f, 1.01f), glm::vec3(4.6f, 2.4f, 0.02f), texMarcador, 0.0f);
		DibujarCubo(lightingShader, glm::vec3(0.0f, 13.0f, -1.01f), glm::vec3(4.6f, 2.4f, 0.02f), texMarcador, 180.0f);
		// Animación con escala: al anotar, el número de ese equipo empieza 30% más grande
		// y regresa a su tamaño mientras se acaban los 3 segundos del gol
		float escalaLocal = 1.0f + 0.3f * (tiempoGolDer / 3.0f);
		float escalaVisita = 1.0f + 0.3f * (tiempoGolIzq / 3.0f);
		glm::vec3 numLocal(0.8f * escalaLocal, 1.1f * escalaLocal, 0.01f);
		glm::vec3 numVisita(0.8f * escalaVisita, 1.1f * escalaVisita, 0.01f);
		DibujarCubo(lightingShader, glm::vec3(-1.4f, 12.8f, 1.03f), numLocal, texNumeros[golesLocal], 0.0f);
		DibujarCubo(lightingShader, glm::vec3(1.4f, 12.8f, 1.03f), numVisita, texNumeros[golesVisita], 0.0f);
		DibujarCubo(lightingShader, glm::vec3(1.4f, 12.8f, -1.03f), numLocal, texNumeros[golesLocal], 180.0f);
		DibujarCubo(lightingShader, glm::vec3(-1.4f, 12.8f, -1.03f), numVisita, texNumeros[golesVisita], 180.0f);

		// Zona 4: túnel a los vestidores, entre las dos partes de las gradas de atrás
		DibujarCubo(lightingShader, glm::vec3(0.0f, 0.03f, -18.3f), glm::vec3(4.0f, 0.06f, 5.4f), texHule);        // piso
		DibujarCubo(lightingShader, glm::vec3(-1.9f, 1.5f, -18.3f), glm::vec3(0.2f, 3.0f, 5.4f), texConcreto);     // muro izquierdo
		DibujarCubo(lightingShader, glm::vec3(1.9f, 1.5f, -18.3f), glm::vec3(0.2f, 3.0f, 5.4f), texConcreto);      // muro derecho
		DibujarCubo(lightingShader, glm::vec3(0.0f, 3.1f, -18.3f), glm::vec3(4.0f, 0.2f, 5.4f), texConcreto);      // techo del túnel
		DibujarCubo(lightingShader, glm::vec3(0.0f, 1.2f, -20.94f), glm::vec3(2.2f, 2.4f, 0.1f), texMadera);      // puerta de los vestidores
		DibujarCubo(lightingShader, glm::vec3(0.0f, 3.55f, -15.62f), glm::vec3(3.6f, 0.7f, 0.05f), texLetrero);   // letrero

		// Recinto: paredes de concreto, techo y tres vigas de metal (de ellas cuelgan las lámparas y el marcador)
		DibujarCubo(lightingShader, glm::vec3(0.0f, 8.0f, -21.2f), glm::vec3(79.0f, 16.0f, 0.4f), texConcreto);   // pared de atrás
		DibujarCubo(lightingShader, glm::vec3(0.0f, 8.0f, 21.2f), glm::vec3(79.0f, 16.0f, 0.4f), texConcreto);    // pared del frente
		DibujarCubo(lightingShader, glm::vec3(-39.2f, 8.0f, 0.0f), glm::vec3(0.4f, 16.0f, 42.8f), texConcreto);   // pared izquierda
		DibujarCubo(lightingShader, glm::vec3(39.2f, 8.0f, 0.0f), glm::vec3(0.4f, 16.0f, 42.8f), texConcreto);    // pared derecha
		DibujarCubo(lightingShader, glm::vec3(0.0f, 16.2f, 0.0f), glm::vec3(79.0f, 0.4f, 42.8f), texMetal);       // techo
		for (int i = -1; i <= 1; i++)
			DibujarCubo(lightingShader, glm::vec3(15.0f * i, 15.7f, 0.0f), glm::vec3(0.5f, 0.6f, 42.4f), texMetal);  // vigas

		// Modelos cargados con Assimp
		glm::mat4 model(1);
		GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");

		// Disco: solo hay uno en juego (keyframes, tecla G)
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(discoX * lado, 0.012f, discoZ * lado));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		disco.Draw(lightingShader);

		// Mascota (botarga): el perro de la práctica, 2 veces más grande, parado en dos patas en la entrada
		// del túnel. Es el mismo modelado jerárquico de la práctica con dos cambios: el cuerpo se gira
		// 70 grados hacia atrás sobre la cadera, y las patas traseras y la cabeza se giran al revés para
		// que las patas queden paradas en el piso y la cabeza mire al frente. Saluda y mueve la cola.
		float colaMascota = 25.0f * sin(glfwGetTime() * 6.0f);
		float cabezaMascota = 15.0f * sin(glfwGetTime() * 2.0f);
		float saludo = -60.0f + 35.0f * sin(glfwGetTime() * 5.0f);
		glm::vec3 cadera(0.0f, -0.05f, -0.225f);   // punto entre las patas traseras (medidas del modelo)
		glm::mat4 modelTemp(1);
		model = glm::mat4(1);
		model = glm::translate(model, glm::vec3(0.0f, 0.81f, -14.35f));   // 0.81: las patas miden 0.376 x 2 + el piso de hule
		model = glm::scale(model, glm::vec3(2.0f));
		model = glm::translate(model, cadera);                            // el giro se hace sobre la cadera
		model = glm::rotate(model, glm::radians(-70.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::translate(model, -cadera);
		modelTemp = model;
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		DogBody.Draw(lightingShader);
		// Cabeza: se endereza para mirar al frente y voltea de un lado a otro
		model = modelTemp;
		model = glm::translate(model, glm::vec3(0.0f, 0.093f, 0.208f));
		model = glm::rotate(model, glm::radians(65.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(cabezaMascota), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		HeadDog.Draw(lightingShader);
		// Cola
		model = modelTemp;
		model = glm::translate(model, glm::vec3(0.0f, 0.026f, -0.288f));
		model = glm::rotate(model, glm::radians(colaMascota), glm::vec3(0.0f, 0.0f, -1.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		DogTail.Draw(lightingShader);
		// Patas delanteras: la izquierda saluda y la derecha queda doblada
		model = modelTemp;
		model = glm::translate(model, glm::vec3(0.112f, -0.044f, 0.074f));
		model = glm::rotate(model, glm::radians(saludo), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		F_LeftLeg.Draw(lightingShader);
		model = modelTemp;
		model = glm::translate(model, glm::vec3(-0.111f, -0.055f, 0.074f));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		F_RightLeg.Draw(lightingShader);
		// Patas traseras: se giran 70 grados al revés para quedar paradas en el piso
		model = modelTemp;
		model = glm::translate(model, glm::vec3(0.082f, -0.046f, -0.218f));
		model = glm::rotate(model, glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		B_LeftLeg.Draw(lightingShader);
		model = modelTemp;
		model = glm::translate(model, glm::vec3(-0.083f, -0.057f, -0.231f));
		model = glm::rotate(model, glm::radians(70.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		B_RightLeg.Draw(lightingShader);

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

		// Público en las 5 filas de las gradas, uno cada 2.5 m (afición local a la izquierda, visitante a la derecha).
		// En medio de los lados largos se deja libre: atrás está el túnel a vestidores y al frente un pasillo.
		for (int fila = 0; fila < 5; fila++)
		{
			float y = 0.5f * (fila + 1);
			// Lados largos
			for (float x = -31.25f; x <= 31.25f; x += 2.5f)
			{
				if (fabs(x) < 3.0f)
					continue;
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
			for (float z = -18.75f; z <= 18.75f; z += 2.5f)
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
		// Porterías (modelo de Blender): la derecha tal cual y la izquierda girada 180 grados
		for (int lado = -1; lado <= 1; lado += 2)
		{
			model = glm::mat4(1);
			model = glm::translate(model, glm::vec3(27.13f * lado, 0.01f, 0.0f));
			if (lado < 0)
				model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			porteria.Draw(lightingShader);
		}
		glBindVertexArray(VAO);
		DibujarCubo(lightingShader, glm::vec3(0.0f, 1.97f, -13.03f), glm::vec3(60.96f, 1.8f, 0.02f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(0.0f, 1.97f, 13.03f), glm::vec3(60.96f, 1.8f, 0.02f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(30.555f, 1.97f, 0.0f), glm::vec3(0.02f, 1.8f, 25.91f), texCristal);
		DibujarCubo(lightingShader, glm::vec3(-30.555f, 1.97f, 0.0f), glm::vec3(0.02f, 1.8f, 25.91f), texCristal);
		// Mallas de protección arriba del cristal de los fondos (detienen los discos que salen volando)
		// (se giran igual que las vallas de los fondos, así que el ancho va en x)
		DibujarCubo(lightingShader, glm::vec3(30.555f, 5.37f, 0.0f), glm::vec3(25.91f, 5.0f, 0.02f), texMalla, -90.0f);
		DibujarCubo(lightingShader, glm::vec3(-30.555f, 5.37f, 0.0f), glm::vec3(25.91f, 5.0f, 0.02f), texMalla, 90.0f);
		DibujarCubo(lightingShader, glm::vec3(30.555f, 7.9f, 0.0f), glm::vec3(0.06f, 0.06f, 25.91f), texMetal);   // tubo de donde cuelga
		DibujarCubo(lightingShader, glm::vec3(-30.555f, 7.9f, 0.0f), glm::vec3(0.06f, 0.06f, 25.91f), texMetal);
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
			model = glm::scale(model, (i < 2) ? glm::vec3(3.0f, 0.3f, 1.5f) : glm::vec3(0.25f));   // lámpara plana del techo o foco de gol
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
// tipo: JUGADOR, PORTERO, ARBITRO, SENTADO (en la banca o en una silla, sin palo) o TIRADO (acostado, sin palo).
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
	// Jugador tirado: todo el cuerpo se acuesta de espaldas (giro de 90 grados en X)
	if (tipo == TIRADO)
	{
		modelTemp = glm::translate(modelTemp, glm::vec3(0.0f, 0.15f, 0.0f));
		modelTemp = glm::rotate(modelTemp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	// Ángulos de la postura: patinando (de pie) o sentado en la banca
	float angTorso = (tipo == SENTADO) ? 5.0f : (tipo == TIRADO) ? 0.0f : 20.0f;
	float angHombro = (tipo == SENTADO) ? -20.0f : (tipo == TIRADO) ? -5.0f : -45.0f;
	float angCodo = (tipo == SENTADO) ? -55.0f : (tipo == TIRADO) ? -10.0f : -35.0f;
	float angRodilla = (tipo == SENTADO) ? 90.0f : 20.0f;

	// Torso: un poco inclinado hacia adelante, como patinador
	model = glm::translate(modelTemp, glm::vec3(0.0f, 0.98f, 0.0f));
	model = glm::rotate(model, glm::radians(angTorso), glm::vec3(1.0f, 0.0f, 0.0f));
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
		model = glm::rotate(model, glm::radians(angHombro + angBrazo * lado), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.17f, 0.0f));
		DibujarParte(shader, model, glm::vec3(0.13f, 0.34f, 0.13f), texCamiseta);
		// Codo y antebrazo
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.34f, 0.0f));
		model = glm::rotate(model, glm::radians(angCodo), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.15f, 0.0f));
		DibujarParte(shader, model, glm::vec3(0.11f, 0.3f, 0.11f), texCamiseta);
		// Mano (guante)
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.33f, 0.0f));
		modelTemp3 = model;
		DibujarParte(shader, model, glm::vec3(0.12f, 0.12f, 0.12f), texNegro);

		// Palo: lo sostiene la mano derecha (el árbitro, los sentados y el tirado no llevan palo)
		if (lado == 1 && tipo != ARBITRO && tipo != SENTADO && tipo != TIRADO)
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
		if (tipo == SENTADO)
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));   // muslo hacia adelante
		else
			model = glm::rotate(model, glm::radians(angPierna * lado - 10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		modelTemp3 = model;
		model = glm::translate(model, glm::vec3(0.0f, -0.22f, 0.0f));
		DibujarParte(shader, model, glm::vec3(anchoPierna, 0.44f, anchoPierna), texPierna);
		// Rodilla y pierna
		model = glm::translate(modelTemp3, glm::vec3(0.0f, -0.44f, 0.0f));
		model = glm::rotate(model, glm::radians(angRodilla), glm::vec3(1.0f, 0.0f, 0.0f));
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
	// Jugada de gol con keyframes (igual que en la práctica)
	if (play)
	{
		if (i_curr_steps >= i_max_steps) // terminó la transición entre dos cuadros
		{
			playIndex++;
			// Al llegar al cuadro 7 el disco ya está dentro de la portería: gol del equipo que ataca
			if (playIndex == 7)
			{
				if (equipo == 0 && golesLocal < 9)
				{
					golesLocal++;
					tiempoGolDer = 3.0f;
				}
				if (equipo == 1 && golesVisita < 9)
				{
					golesVisita++;
					tiempoGolIzq = 3.0f;
				}
				PlaySound(TEXT("Sounds/gol.wav"), NULL, SND_FILENAME | SND_ASYNC);   // bocina de gol
			}
			if (playIndex > FrameIndex - 2) // terminó toda la animación
			{
				std::cout << "termina anim" << std::endl;
				playIndex = 0;
				if (equipo == 0)   // terminó el local: ahora ataca el visitante con las mismas poses
				{
					equipo = 1;
					resetElements();
					interpolation();
					i_curr_steps = 0;
				}
				else               // terminó el visitante: se acaba la jugada
				{
					equipo = 0;
					play = false;
				}
			}
			else // siguiente transición
			{
				i_curr_steps = 0;
				interpolation();
			}
		}
		else
		{
			// Se suma el incremento de cada variable
			jugPosX += KeyFrame[playIndex].incX;
			jugPosZ += KeyFrame[playIndex].incZ;
			rotJug += KeyFrame[playIndex].rotJugInc;
			brazo += KeyFrame[playIndex].brazoInc;
			discoX += KeyFrame[playIndex].discoXInc;
			discoZ += KeyFrame[playIndex].discoZInc;
			porteroZ += KeyFrame[playIndex].porteroZInc;
			pasPosX += KeyFrame[playIndex].pasXInc;
			pasPosZ += KeyFrame[playIndex].pasZInc;
			rotPas += KeyFrame[playIndex].rotPasInc;
			i_curr_steps++;
		}
	}

	// Piernas de los que patinan: suben y bajan entre -25 y 25 grados mientras corre la jugada,
	// con la misma máquina de estados de la caminata del perro
	if (play)
	{
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
	{
		camTransmision = !camTransmision;
		camJugador = false;
	}

	// Cámara desde los ojos del jugador que tira (jugada G)
	if (key == GLFW_KEY_V && action == GLFW_PRESS)
	{
		camJugador = !camJugador;
		camTransmision = false;
	}

	// Jugada con keyframes: G la reproduce desde el primer cuadro (empieza el local); si ya se reproduce, la detiene
	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		if (play == false && (FrameIndex > 1))
		{
			equipo = 0;
			resetElements();
			interpolation();   // primera transición
			play = true;
			playIndex = 0;
			i_curr_steps = 0;
		}
		else
		{
			play = false;
		}
	}

	// Goles: suben el marcador (del 0 al 9) y encienden la luz de gol de esa portería.
	// El local ataca la portería derecha y el visitante la izquierda.
	if (key == GLFW_KEY_1 && action == GLFW_PRESS && golesLocal < 9)
	{
		golesLocal++;
		tiempoGolDer = 3.0f;
		PlaySound(TEXT("Sounds/gol.wav"), NULL, SND_FILENAME | SND_ASYNC);   // bocina de gol
	}
	if (key == GLFW_KEY_2 && action == GLFW_PRESS && golesVisita < 9)
	{
		golesVisita++;
		tiempoGolIzq = 3.0f;
		PlaySound(TEXT("Sounds/gol.wav"), NULL, SND_FILENAME | SND_ASYNC);   // bocina de gol
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
