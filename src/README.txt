Gomas — emulador ZX Spectrum 48K con Allegro 5

El núcleo procede de YASS (referencia incluida en ../yass).

Compilar y ejecutar
------------------
Requiere C++11, make, pkg-config y las bibliotecas de desarrollo allegro-5,
allegro_audio-5, allegro_dialog-5, allegro_font-5 y allegro_primitives-5.
No necesita acodec.

  make -C bin
  ./bin/gomas

La ROM 48.rom se busca en el directorio de trabajo y junto al ejecutable.
También se conserva la apertura por línea de comandos:

  ./bin/gomas bin/manic.tap
  ./bin/gomas bin/manic.tap yass/ROM/48.rom

Menús
-----
File > Open... abre un selector de TAP. Al aceptar un archivo válido se inserta la
cinta y se reinicia el Spectrum. Cancelar o seleccionar un archivo inválido conserva
la partida actual. Para cargar, escribir LOAD "" en BASIC: J produce LOAD y
Right Shift + P produce cada comilla; terminar con Enter.
File > Close retira la cinta y reinicia el Spectrum; se deshabilita sin cinta.
File > Exit termina el programa, igual que Escape o cerrar la ventana.
CPU > Reset reinicia CPU, RAM, ULA y audio; conserva la cinta y la rebobina.
Tape > Fast load está activado por defecto y mantiene la carga instantánea.
Al desactivarlo se abre el reproductor y se habilitan Play, Pause y Stop.
Tape > Show tape deck... permite abrir el panel en cualquiera de los dos modos.
Tape > Rewind vuelve al primer bloque, sin reiniciar el Spectrum.
View y Settings se muestran deshabilitados, sin opciones todavía.
Help > About / Help muestra instrucciones y los créditos de YASS.

Los diálogos de archivo y ayuda pausan el audio y la emulación. Tras Open, Close
o Reset se descarta todo el audio pendiente de la sesión anterior. El nombre de
la cinta aparece en el título de la ventana.

En Linux los menús nativos de Allegro requieren X11/GLX y GTK. El programa utiliza
ALLEGRO_GTK_TOPLEVEL y selecciona GDK_BACKEND=x11 dentro de su propio proceso,
también en escritorios Wayland con XWayland. No modifica la configuración del
escritorio. Referencia: https://liballeg.org/a5docs/trunk/native_dialog.html

La ventana se puede redimensionar; la imagen conserva su proporción y usa escala
entera cuando cabe, sin suavizado. LSHIFT corresponde a CAPS SHIFT y RSHIFT a
SYMBOL SHIFT. Al perder el foco se liberan las teclas.

Reproductor de cinta y multicarga
--------------------------------
Con Fast load desactivado, escribir LOAD "" y pulsar Play. Los datos se entregan
como pulsos a la entrada EAR (bit 6 del puerto de la ULA); la ROM ejecuta la carga
real, con sus franjas de borde y sonido de cinta. El Spectrum sigue funcionando
con el reproductor parado o pausado.

Play continúa desde la posición actual. Pause conserva el punto exacto dentro
del pulso; Play lo reanuda. Stop para y vuelve al principio del bloque actual.
Al finalizar se muestra End of tape; no se rebobina automáticamente.

El panel muestra todos los bloques: número, nombre y tipo de cabecera si existen,
bytes de datos, duración y aviso si el checksum está dañado. La flecha > señala
el bloque actual. La barra inferior muestra su progreso. La rueda desplaza la
lista; pulsar una fila la selecciona y Cue selected sitúa la cinta al inicio de
ese bloque. Esto no borra RAM ni reinicia la CPU, y sirve para cargar otra fase
de un juego multicarga. Después, cuando el juego pida la cinta, pulsar Play.

En el panel: flechas arriba/abajo seleccionan, Enter hace Cue selected, Home
rebobina, Espacio alterna Play/Pause y Escape cierra solo el reproductor. Cerrar
su ventana no detiene la cinta ni sale del emulador; se puede volver a abrir.
Play/Pause/Stop se deshabilitan con Fast load activado, pero se puede seleccionar
un bloque para que la siguiente carga rápida empiece ahí.

Open, Close y CPU > Reset paran y rebobinan la cinta. Conservan la elección de
Fast load. Cambiar de modo para la cinta y la coloca al comienzo del bloque
actual; conviene elegir el modo antes de iniciar una carga. El modo no se guarda
entre ejecuciones. No hay detección automática de cuándo un juego necesita cinta:
para una multicarga se controla el reproductor manualmente.

Los TAP contienen bytes, no tiempos de audio ni marcadores de fases. El panel
muestra sus bloques reales sin intentar adivinar qué grupos forman cada nivel.
La señal se sintetiza con el estándar Spectrum: piloto de 2168 t-states (8063
pulsos para cabecera, 3223 para datos), sincronización 667/735, dos pulsos por bit
de 855 o 1710 y una pausa de un segundo entre bloques. Referencia:
https://worldofspectrum.net/faq/reference/48kreference.htm

Revisión de vídeo y audio
------------------------
Antes se modificaba el mismo búfer que Allegro estaba reproduciendo, se arrancaba
un sample nuevo cada frame y se esperaba su final con un bucle ocupado. Eso dejaba
huecos entre clips y mezclaba generación con reproducción. Ahora un stream mono
de 48 kHz usa tres fragmentos de 960 muestras: el dispositivo de audio marca el
ritmo, como la cola de audio de YASS. Se rellenan antes de arrancar y los eventos
de Allegro solicitan los siguientes. Hay hasta unos 60 ms de audio en cola, más
la latencia del sistema. No hay espera activa.

La ULA integra el nivel del beeper en el tiempo de cada muestra, elimina la
componente continua y mantiene separado el audio en construcción del frame
terminado. Antes se sobrescribía una muestra por instrucción y el filtro dependía
de cuántas instrucciones se ejecutasen. ULAIOData estaba además sin inicializar.
Los límites de scanline ahora usan >=, conservan los ciclos sobrantes y contabilizan
los ciclos de aceptación de la interrupción. FLASH cambia cada 16 frames.

El vídeo se genera en un búfer RGBA de CPU y se sube una vez por actualización,
respetando el pitch del bitmap. Ya no se llama a Allegro por cada píxel de cada
escritura de RAM. La marca de cambios se limpia al subirlo. Se corrige también la
línea 240 del borde, que quedaba fuera del área de 320x240.

Se comprueba el tamaño de la ROM y el encuadre de los bloques TAP antes de arrancar.
La carga rápida comprueba flag/checksum y fin de cinta, y avanza al siguiente
bloque completo. Los recursos Allegro se liberan también al salir por un error.

Pruebas
-------
  make -C bin test

Las pruebas de `make test` no requieren ventana ni dispositivo de audio. Comprueban colores, BRIGHT, FLASH,
la distribución de VRAM, límites del borde, ciclos exactos y sobrantes, muestreo
independiente del reparto en instrucciones, eliminación de continua, separación
de búferes y arranque de la ROM incluida durante 500 frames. También se comprueban
Open, Reset y Close (incluido conservar la partida si Open falla), y
los bytes de bitmap/atributos del bus flotante en las 192 líneas, los intervalos
inactivos y el bucle de sincronización usado por Cobra. Las pruebas de cinta
comprueban pulsos, pausa/reanudación, fin de cinta, selección de bloques sin reset
y dos cargas a través de la ROM real. La prueba completa carga Cobra desde BASIC
en ambos modos, contesta al trainer y comprueba que la partida actualiza la
pantalla. La reproducción lenta se prueba sin esperar al reloj de pared.

Prueba opcional de interfaz con ratón real (requiere X11, audio y las bibliotecas
de desarrollo x11/xtst):

  make -C bin test-ui

Abre la aplicación, desactiva Fast load, envía 1000 movimientos de ratón mediante
XTest y comprueba que Play responde en menos de 750 ms. También pulsa Pause,
Stop, selecciona un bloque y hace Cue selected por coordenadas de la ventana.
Esta prueba abre ventanas y mueve el puntero; no forma parte de `make test`.

Respuesta del reproductor
------------------------
La entrada se procesa independientemente del dibujo: un temporizador limita el
refresco del Spectrum a 50 Hz y el del reproductor a 25 Hz. Se descartan los ticks
de dibujo antiguos después de un diálogo o un frame lento. Mover el puntero no
provoca redibujados; el panel solo se pinta si cambia su estado o progreso, si se
redimensiona o si necesita volver a mostrarse. Su fuente reside en vídeo y se
recrea al volver a abrir la ventana. Esto evita que el dibujo de los caracteres
y los eventos de movimiento acumulen retrasos delante de los clics.

Bloqueo de Cobra al comenzar la partida
---------------------------------------
Cobra espera al bus flotante en 0x95FE-0x9607:

  LD BC,0x3F28
  LD A,C
  IN A,(0xFF)
  CP B
  JR NC,0x9601

Con una respuesta constante de 0xFF el salto se repite indefinidamente. El menú
puede funcionar y reproducir música porque no ejecuta todavía esta espera.
ULA::IORead ahora devuelve los bytes que lee la ULA durante la zona de pantalla:
bitmap, atributo, bitmap siguiente, atributo siguiente y cuatro ciclos inactivos.
Fuera de esos accesos devuelve 0xFF. La primera lectura es en el t-state 14338,
con líneas de 224 t-states. Referencia de temporización:
https://github.com/fuse-emulator/fuse/blob/master/spectrum.c

Se reprodujo cargando bin/cobra.tap mediante LOAD "", contestando N al trainer y
eligiendo 1 (teclado). Antes la imagen permanecía idéntica y el PC se quedaba en
0x9601-0x9605. Después la partida actualiza la pantalla y avanza. No se ha cambiado
el TAP ni parcheado el código del juego. El acceso de E/S sigue resolviéndose con
la fase de la ULA al inicio de la instrucción; no es todavía un modelo de ciclos
de bus con contención completa.

Límites actuales
----------------
No es una emulación de ULA precisa por t-state: las escrituras de vídeo se aplican
al ejecutarse la instrucción y el borde se representa por scanline. No hay
contención de memoria ni ajuste de eventos de E/S dentro de una instrucción.
Se conserva la aproximación de 50 frames/s (69888 t-states por frame) y se emula
el beeper del bit 4 y la señal de cassette, sin AY ni la contribución analógica de
MIC. La carga rápida es específica de la ROM 48K. La reproducción por pulsos
soporta TAP estándar: los cargadores propios que requieran otras temporizaciones
no pueden reconstruirse con la información de un TAP. No hay soporte TZX/PZX ni
grabación de cinta.
Las pruebas automáticas no sustituyen la comprobación visual y auditiva en el
equipo de destino, especialmente de latencia y posibles underruns del driver.
