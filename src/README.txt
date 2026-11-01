Gomas — emulador ZX Spectrum 48K con Allegro 5

El núcleo procede de YASS (referencia incluida en ../yass).

Compilar y ejecutar
------------------
Requiere C++11, make, pkg-config y las bibliotecas de desarrollo allegro-5,
allegro_audio-5 y allegro_dialog-5. Ya no necesita primitives ni acodec.

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

No requieren ventana ni dispositivo de audio. Comprueban colores, BRIGHT, FLASH,
la distribución de VRAM, límites del borde, ciclos exactos y sobrantes, muestreo
independiente del reparto en instrucciones, eliminación de continua, separación
de búferes y arranque de la ROM incluida durante 500 frames. También se comprueban
Open, Reset y Close (incluido conservar la partida si Open falla), y
los bytes de bitmap/atributos del bus flotante en las 192 líneas, los intervalos
inactivos y el bucle de sincronización usado por Cobra.

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
el beeper del bit 4, sin AY ni la contribución analógica de MIC. La carga TAP es
rápida y específica de la ROM 48K; no reproduce pulsos de cinta ni loaders propios.
Las pruebas automáticas no sustituyen la comprobación visual y auditiva en el
equipo de destino, especialmente de latencia y posibles underruns del driver.
