Gomas — emulador ZX Spectrum 48K con Allegro 5

El núcleo procede de YASS (referencia incluida en ../yass).

Compilar y ejecutar
------------------
Requiere C++11, make, pkg-config y las bibliotecas de desarrollo allegro-5,
allegro_audio-5, allegro_dialog-5, allegro_font-5 y allegro_primitives-5.
No necesita acodec.

  make -C bin
  ./bin/gomas

Al arrancar se restaura por defecto la última ROM abierta correctamente. Si no
hay una guardada o ya no está disponible, se usa roms/48.rom (o roms/48k.rom),
buscada respecto al ejecutable o al directorio de trabajo. También funciona al
ejecutar bin/gomas desde fuera del proyecto. Un --rom explícito tiene prioridad.
También se conserva la apertura por línea de comandos:

  ./bin/gomas bin/manic.tap
  ./bin/gomas bin/manic.tap roms/48.rom
  ./bin/gomas --rom roms/48.rom
  ./bin/gomas bin/manic.tap --rom roms/48.rom

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
Settings > ROM > Open ROM... identifica el modelo por el contenido de la ROM elegida.
Settings > ROM > Open custom 48K ROM... permite una ROM alternativa de 16 KB, asociada
explícitamente a hardware 48K. La carga rápida se deshabilita si la ROM no tiene
una rutina reconocida; usar Fast load OFF y Play para cargar cintas.
Settings > ROM > Information... muestra el perfil activo, archivo y compatibilidad
con carga rápida. El modelo aparece también en el título de la ventana.
View sigue deshabilitado, sin opciones todavía.
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

ROMs y perfiles
---------------
El directorio roms/ es la ubicación centralizada. Los perfiles conocidos son:
48.rom; 128-0/128-1; 128-spanish-0/128-spanish-1; plus2-0/plus2-1 y plus3-0..3.
Se identifican por huellas FNV-1a del contenido, no por el nombre del archivo.
Estas huellas identifican versiones; no son una firma de autenticidad.

Se puede elegir cualquier página de un conjunto. Las restantes se buscan en el
mismo directorio con sus nombres conocidos y se validan antes de cambiar nada.
También se admiten archivos combinados de 32 KB (128K/+2) o 64 KB (+3), ordenados
por página 0, 1, etc. Una ROM desconocida no se asigna a un modelo automáticamente;
para una alternativa de 48K usar la opción explícita de Settings o:

  ./bin/gomas --custom-48-rom /ruta/alternativa.rom

Actualmente solo se ejecuta el perfil 48K. Los conjuntos 128K, +2 y +3 se reconocen
y validan, pero su activación informa de que falta implementar su hardware y
conserva la máquina actual. if1-2.rom pertenece a Interface 1 y no sirve como ROM
de sistema. Una selección inválida o cancelada conserva ROM, CPU, RAM y cinta.
Una selección correcta reinicia la máquina y rebobina la cinta montada, igual que
CPU > Reset. La última selección válida se guarda entre ejecuciones, incluyendo si era una
ROM alternativa de 48K. En Settings > ROM > On startup se elige Last opened ROM
o Spectrum 48K. Esto cambia el próximo arranque, sin reiniciar la sesión actual.
Settings > ROM > ZX Spectrum 48K permite cargar directamente la ROM de 48K.
Los perfiles 128K/+2/+3 aparecen deshabilitados hasta implementar su hardware.

Las preferencias se guardan en el directorio de configuración de usuario de
Gomas (normalmente ~/.config/gomas/settings.ini en Linux). Las rutas de las ROMs
se guardan absolutas para poder restaurarlas desde otro directorio. Un archivo
inválido o una selección cancelada no sustituye la preferencia anterior. Si una
ROM guardada desaparece, se arranca con 48K y se conserva su ruta por si vuelve
a estar disponible. Un --rom inválido produce un error, sin sustituirse por otra
ROM silenciosamente. GOMAS_CONFIG permite indicar otro archivo de preferencias;
un valor vacío desactiva su lectura y guardado, por ejemplo para pruebas.

Configuración interna por modelo
--------------------------------
src/machine_config.h contiene los perfiles inmutables de hardware. El idioma y
las huellas de la ROM pertenecen a rom_loader.h; inglés y español de 128K usan
el mismo perfil físico.

Modelo       RAM       ROM       Línea x líneas      AY      Disquetera
48K          48 KB     16 KB     224 x 312            no      no
128K         128 KB    32 KB     228 x 311            sí      no
+2           128 KB    32 KB     228 x 311            sí      no
+3           128 KB    64 KB     228 x 311            sí      sí

Los perfiles definen además los bancos iniciales, la página ROM de arranque,
los bancos de pantalla normal/alternativa, el tipo de paginación, el reloj de CPU,
el pulso de interrupción y la fase del bus flotante. +3 tiene política de bus
inactivo y temporización ASIC, diferenciadas de 128K/+2. Referencias de tiempos:
https://github.com/fuse-emulator/libspectrum/blob/master/timings.c
https://github.com/fuse-emulator/fuse/blob/master/spectrum.c

HardwareFor(model) devuelve el descriptor; SpectrumMachine::Configuration()
expone el activo sin permitir modificarlo. Se comprueba que las páginas de ROM
coinciden con el hardware antes de sustituirlo. La máquina aplica la configuración
al cargar la ROM y al resetear. La ULA consume ya los tiempos por línea/fotograma
y el origen/política del bus flotante definidos por el perfil.

Los bits de capacidades describen extensiones a la base 48K ya implementada.
RequiredHardwareFeatures y ImplementedHardwareFeatures permiten que cargador y
menú decidan con una misma regla qué modelos se pueden activar. Declarar un
perfil no activa por sí solo hardware que falta: RAM paginada, pantalla alternativa,
sonido AY, paginación especial de +3 y disquetera siguen pendientes. El recorte
actual de vídeo y el audio de 960 muestras por fotograma a 50 Hz se conservan;
el ajuste preciso de vídeo/audio 128K se hará en la tarea correspondiente.

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
Las pruebas de ROM validan perfiles, páginas sueltas/combinadas, orden de páginas,
archivos renombrados, tamaños, fallos transaccionales, ROMs personalizadas y
resolución de rutas desde otros directorios. Las pruebas de preferencias
comprueban restauración de la última ROM, arranque fijo 48K, prioridad de --rom,
ROMs alternativas, rutas absolutas, guardado y fallback si falta una ROM. La prueba
de interfaz utiliza un archivo temporal aislado y verifica las opciones de arranque.
Las pruebas de hardware verifican los descriptores, los requisitos de cada ROM,
la disponibilidad de modelos, límites de fotograma, fase del bus flotante y
reaplicación de la configuración en reset.

Prueba opcional de interfaz con ratón real (requiere X11, audio y las bibliotecas
de desarrollo x11/xtst):

  make -C bin test-ui

Abre la aplicación, desactiva Fast load, envía 1000 movimientos de ratón mediante
XTest y comprueba que Play responde en menos de 750 ms. También pulsa Pause,
Stop, selecciona un bloque y hace Cue selected por coordenadas de la ventana.
También abre el selector nativo de ROMs y cambia la ROM conservando la cinta.
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

Memoria 128K integrada (tarea 5)
--------------------------------
spectrum_memory.h centraliza el mapa de CPU: ocho bancos de 16 KB, bancos fijos
5/2, banco superior seleccionable y dos páginas ROM para 128K/+2. El puerto
7FFD tiene decodificación parcial, selección de ROM y bloqueo hasta reset.
Los alias de los bancos 2/5 comparten los mismos bytes; escribir al banco 5
por C000 también actualiza la ULA. SpectrumIO distribuye las escrituras a ULA
y paginación, incluidas las direcciones compartidas. En 48K no hay paginación.
El bit de pantalla se conserva; mostrar banco 7 y completar vídeo/audio 128K
queda para la siguiente tarea. +3 requiere su propia paginación adicional.

make test incluye paging-tests: bancos completos, protección de ROM, alias,
decodificación de todos los puertos, bloqueo, reset, configuraciones rechazadas
y arranque de las ROMs 128K inglesa/española y +2 en el núcleo. Los perfiles
siguen deshabilitados en la interfaz hasta completar las capacidades pendientes.
