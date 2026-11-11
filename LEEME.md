# Arena de Hockey sobre Hielo

Proyecto Final de Computación Gráfica e Interacción Humano-Computadora (FI UNAM).
Iteración 3: bancas, gradas, público, marcador y carga de modelos.

## Qué tiene esta versión

- **Zona 1, pista de juego:** hielo con sus marcas, vallas, cristal y porterías.
- **Zona 2, bancas:** bancas de madera de los dos equipos con toallas, botellas y palos; cajas de castigo y mesa de oficiales.
- **Zona 3, espectadores:** gradas de concreto de 5 escalones en los cuatro lados, público y marcador central colgante (por ahora en 0 - 0).
- **Carga de modelos:** público (sentado en su asiento), palos, botellas y disco, hechos en Blender y exportados a `.obj`; se cargan con Assimp y la clase `Model`, y sus texturas se leen con SOIL2.

## Controles

| Tecla | Acción |
| --- | --- |
| W A S D | Moverse |
| Mouse | Mirar |
| ESC | Salir |

## Archivos

- `Main.cpp`: el programa.
- `Shader.h`, `Camera.h`, `Mesh.h`, `Model.h`, `stb_image.h`: clases de las prácticas.
- `Shader/`: `lighting.vs` y `lighting.frag`.
- `Textures/`: hielo, vallas, red, cristal, materiales, marcador y números del 0 al 9.
- `Models/`: aficionados, palo, botella y disco (`.obj`), hechos en Blender 4.2. En `Models/blender/` están los `.blend` de cada uno y el script con el que se armaron.
- `docs/`: acta y plano.

## Cómo abrirlo en Visual Studio 2022

1. Proyecto vacío de C++ con la casilla "Colocar la solución y el proyecto en el mismo directorio".
2. Copiar todo esto junto al `.vcxproj`.
3. Copiar `External Libraries` (GLEW, GLFW, GLM, assimp y SOIL2) de una práctica que corra, y la carpeta `SOIL2` con `SOIL2.h` (la que va junto al `.cpp` en la práctica de carga de modelos).
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

## Lo que sigue

- Iteración 4: jugadores, portero y árbitro, cámaras e iluminación.
- Iteración 5: animación por cuadros clave, interacción, pruebas, documentación y video.
