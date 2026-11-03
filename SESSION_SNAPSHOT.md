# Punto de continuación hacia la versión 1.0

Fecha: 2026-11-03. Sesión detenida a petición de Pedro para realizar más
validación manual. No hay una tarea de implementación en curso.

## Estado del código

Último commit funcional: `60d1d4b` — Add debugger view and instruction stepping.
Los cambios funcionales están comiteados. `bin/sorteo.tap` es un archivo local
sin seguimiento y se ha dejado fuera de los commits; conservarlo.

Funciones disponibles:

- Modelos 48K, 128K inglés/español, +2 y +3; selección y preferencias de ROM.
- Vídeo y temporización por modelo, paginación, pantalla alternativa y AY.
- Menús, escalas 2x/4x, pantalla completa y filtros.
- Pausa, velocidad global 0,25x–16x, frecuencia de CPU y retroceso aproximado de 10 s.
- TAP/TZX, SNA/Z80/SZX/SLT y ZIP; selección interactiva de rutas TZX.
- Discos DSK/EDSK con controladora +3 y panel independiente de unidades A/B.
- Galería de partidas con miniaturas y casilla rápida; GSS v2 conserva lectura v1,
  discos, niveles SLT y decisiones TZX.
- Catálogo ZXArt con búsqueda, versiones, descargas verificadas, indicador Installed,
  biblioteca offline en games y arranque automático.
- Flechas del PC como cursores Spectrum (CAPS SHIFT + 5/6/7/8).
- Depurador con registros, flags, pila, RAM/mapa CPU, bancos físicos y pasos de
  instrucción/fotograma. Inspección sin lecturas de puertos ni cambios de paginación.

Atajos principales: F2/F3 guardar/cargar rápido; F5 pausa; F6/F7 velocidad;
F8 velocidad normal; F9 retroceso; F10 paso de instrucción; F4 siguiente fotograma;
F11 pantalla completa; F12 depurador. Detalles en src/README.txt.

## Validación disponible

La última implementación pasó compilación, suite completa, pruebas nativas de
interfaz y comprobaciones con AddressSanitizer/UndefinedBehaviorSanitizer.

Las pruebas incluyen cargas reales de cinta, regresiones de juegos, arranque de
un disco sintético mediante la ROM real del +3, recuperación a mitad de una
transferencia de disco, biblioteca offline, una descarga real de ZXArt y
comparación exacta entre ejecución por frames y por instrucciones.

Estos resultados no equivalen a validación universal de juegos comerciales.
Pedro ha confirmado, entre otros casos, Rick Dangerous 2 en TZX comprimido y 128K.

## Pendientes acordados para cerrar la 1.0

1. Remapeo configurable de teclado y joystick.
2. Validación manual adicional, especialmente DSK comerciales, protecciones,
   multicarga y cambios de disco/cinta. Corregir los fallos que aparezcan.
3. Revisar persistencia de preferencias (controles, vídeo, filtros y carga rápida).
4. Revisar el cierre/expulsión/cambio de sesión con discos modificados sin guardar.
5. Compilación e instalación reproducibles desde un entorno limpio; revisar rutas
   fuera del proyecto y documentar dependencias.
6. Preparar documentación de versión y una 1.0 RC para validación.

Los puntos 3–6 son una revisión pendiente: no asumir que ya están implementados.
No añadir funciones grandes antes de la 1.0 salvo necesidad surgida de validación.
Los refinamientos se decidirán con los resultados de las pruebas de Pedro.

## Límites actuales a tener presentes

- La controladora de disco funciona por sectores; las protecciones exóticas y
  todos los títulos DSK comerciales aún requieren validación.
- La precisión completa de contención y otras mejoras de fidelidad quedan para
  después, salvo que un juego detectado las necesite para funcionar.
- El depurador todavía no edita RAM/registros ni tiene desensamblador o breakpoints.
- El catálogo usa ZXArt; el modo offline contiene lo visitado/descargado, no todo
  el catálogo remoto.

## Al retomar

Leer este archivo, comprobar git status y recibir los resultados de validación.
Si no hay fallos nuevos prioritarios, la siguiente implementación será el remapeo
de teclado/joystick. Mantener los archivos personales fuera de los commits.
Identidad de Git acordada: Pedro Lopez <pit_ter@hotmail.com>.
