# Arena de Hockey sobre Hielo

Proyecto Final de Computación Gráfica e Interacción Humano-Computadora (FI UNAM).
Versión de la Iteración 4, con los ajustes de la revisión (mallas, jugadores en la banca y vallas nuevas).

## Qué tiene hasta ahora

- **Zona 1, pista de juego:** hielo de 61 × 26 m con sus marcas, vallas con publicidad (FI UNAM, FOUR LOQO, BACACHO, TACOSCHAMPS y ESQUITES), cristal protector, mallas arriba del cristal de los fondos, dos porterías (modelo de Blender), jugadores de los dos equipos, 2 porteros, un árbitro y, en una esquina, una pelea (un jugador tirado junto a una mancha roja y el otro festejando).
- **Zona 2, bancas:** bancas de madera de los dos equipos con jugadores sentados, toallas, botellas y palos; cajas de castigo y mesa de oficiales con dos oficiales sentados.
- **Zona 3, espectadores:** gradas de 5 escalones en los cuatro lados, rojas del lado de la afición local y azules del visitante,, unos 340 aficionados y marcador central colgante.
- **Zona 4, túnel a vestidores:** entre las dos partes de las gradas de atrás, con su letrero, la puerta de los vestidores y la mascota en la entrada: el perro de las prácticas, parado en dos patas como botarga, saludando y moviendo la cola.
- **Recinto:** paredes de concreto, techo y tres vigas de metal; de ellas cuelgan las lámparas y el marcador.

## Qué se tomó de cada práctica

| Práctica | Dónde se usa en el proyecto |
| --- | --- |
| Modelado jerárquico (brazo) | `DibujarPersona()`: torso → cabeza, brazo → antebrazo → mano → palo, cadera → muslo → pierna → patín, con `modelTemp`. La misma función dibuja jugadores, porteros, árbitro y jugadores sentados en la banca. |
| Carga de modelos | Público, palos, botellas, disco y porterías, hechos en Blender y exportados a `.obj`, y la mascota (el perro en piezas de la práctica de animación), con la clase `Model`; sus texturas se leen con SOIL2, como en la `Model.h` de la práctica. |
| Texturas | Hielo, vallas con 5 anuncios (FI UNAM, FOUR LOQO, BACACHO, TACOSCHAMPS y ESQUITES), malla, cristal, madera, concreto, gradas roja y azul, hule, metal, playeras, rayas del árbitro, letrero de vestidores, marcador y números, cargadas con stb_image como en la práctica de texturizado. |
| Iluminación II | `lighting.frag` con `DirLight`, 4 `PointLight` y un `SpotLight`; cubos de lámpara con `lamp.frag`. |
| Animación | `Animation()` llamada en cada cuadro; el jugador patina con una máquina de estados como la caminata del perro, y un defensor azul y el árbitro lo siguen usando su misma posición; la luz de gol se prende 3 segundos con un contador que baja con `deltaTime`, y el número del marcador empieza 30% más grande y regresa a su tamaño con `glm::scale` (animación con escala); la mascota (parada en dos patas: el cuerpo gira 70° sobre la cadera y las patas traseras 70° al revés) saluda, voltea y mueve la cola con el seno del tiempo; `transparency` y `discard` para la red y `GL_BLEND` para el cristal. |
| Cámara sintética | Cámara en primera persona (`Camera.h`) y cámara de transmisión con `glm::lookAt`. |

## Luces

| Luz | Qué es en la arena |
| --- | --- |
| `dirLight` | Luz general del recinto, tenue. |
| `pointLights[0]` y `[1]` | Reflectores del techo. Se apagan con **L**. |
| `pointLights[2]` y `[3]` | Luces rojas de gol detrás de cada portería. Se prenden 3 segundos al anotar. |
| `spotLight` | Cañón de seguimiento: desde el techo apunta siempre al jugador. |

## Controles

| Tecla | Acción |
| --- | --- |
| W A S D | Moverse |
| Mouse | Mirar |
| C | Cambiar entre la cámara libre y la cámara de transmisión |
| P | El jugador empieza o deja de patinar |
| L | Encender y apagar los reflectores |
| 1 / 2 | Gol del local / visitante (marcador, luz de gol y bocina) |
| 0 | Reiniciar el marcador |
| ESC | Salir |

## Archivos

- `Main.cpp`: todo el programa.
- `Shader.h`, `Camera.h`, `Mesh.h`, `Model.h`, `stb_image.h`: clases de las prácticas (se pueden cambiar por las suyas).
- `Shader/`: `lighting.vs`, `lighting.frag`, `lamp.vs`, `lamp.frag`.
- `Textures/`: hielo, vallas, red, malla, cristal, colores de playera, materiales y números del 0 al 9.
- `SOIL2/`: encabezados de SOIL2 (la biblioteca `soil2-debug.lib` va en `External Libraries\SOIL2\lib`).
- `Sounds/gol.wav`: bocina de gol, se reproduce con `PlaySound` de Windows.
- `Models/`: aficionados, palo, botella, disco y portería (`.obj`), hechos en Blender 4.2. También van aquí las 7 piezas del perro de la práctica de animación (`DogBody`, `HeadDog`, `TailDog`, `F_RightLegDog`, `F_LeftLegDog`, `B_RightLegDog`, `B_LeftLegDog`, con sus `.mtl` y `Texture_albedo.jpg`), tomadas de esa práctica. En `Models/blender/` están los `.blend` de cada uno y el script con el que se armaron.
- `docs/`: acta y plano de la arena.

## Cómo abrirlo en Visual Studio 2022

1. Proyecto vacío de C++ con la casilla "Colocar la solución y el proyecto en el mismo directorio".
2. Copiar todo esto junto al `.vcxproj`.
3. Copiar `External Libraries` (GLEW, GLFW, GLM, assimp y SOIL2) de una práctica que corra. La carpeta `SOIL2` con los encabezados (`SOIL2.h`) ya viene en el proyecto.
4. Copiar `glew32.dll` y `assimp-vc140-mt.dll` junto al `.vcxproj`.
5. Agregar `Main.cpp` y los `.h` al proyecto.
6. Propiedades (Todas las configuraciones, Win32):
   - C/C++ > General > Directorios de inclusión adicionales:
     `$(ProjectDir)External Libraries\GLEW\include;$(ProjectDir)External Libraries\GLFW\include;$(ProjectDir)External Libraries\assimp\include;$(ProjectDir)External Libraries\GLM`
   - Vinculador > General > Directorios de bibliotecas adicionales:
     `$(ProjectDir)External Libraries\GLEW\lib\Release\Win32;$(ProjectDir)External Libraries\GLFW\lib-vc2015;$(ProjectDir)External Libraries\assimp\lib;$(ProjectDir)External Libraries\SOIL2\lib`
   - Vinculador > Entrada > Dependencias adicionales:
     `opengl32.lib;glew32.lib;glfw3.lib;assimp-vc140-mt.lib;soil2-debug.lib;`
7. Debug, x86 y F5.

## Pendiente (Iteración 5)

- Animación por keyframes (cuando veamos la práctica de keyframes, para hacerla igual que en clase).
- Pruebas, documentación en PDF y video.
