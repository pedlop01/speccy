Gomas — emulador ZX Spectrum 48K / 128K / +2 / +3 con Allegro 5

El núcleo procede de YASS (referencia incluida en ../yass).

Compilar y ejecutar
------------------
Requiere C++11, make, pkg-config y las bibliotecas de desarrollo allegro-5,
allegro_audio-5, allegro_dialog-5, allegro_font-5, allegro_primitives-5, libarchive, zlib, libcurl, libcrypto (OpenSSL) y
nlohmann_json. El catálogo usa HTTPS con certificados verificados y un hilo de
trabajo; compilar/enlazar con soporte pthread (incluido en el Makefile).
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
File > Open... abre un selector de TAP, TZX, SNA, Z80, SZX, SLT, DSK o ZIP. Las cintas válidas se insertan
y reinician el Spectrum; los snapshots recuperan directamente la máquina. Cancelar o seleccionar un archivo inválido conserva
la partida actual. Para cargar, escribir LOAD "" en BASIC: J produce LOAD y
Right Shift + P produce cada comilla; terminar con Enter.
Las flechas del PC emulan los cursores Spectrum: izquierda = CAPS SHIFT+5,
abajo = CAPS SHIFT+6, arriba = CAPS SHIFT+7 y derecha = CAPS SHIFT+8. Permiten
navegar por el menú 128K/+2/+3; Enter confirma. En juegos equivalen a esas
combinaciones, no a un joystick Kempston.
File > Close cierra la cinta o snapshot y reinicia el Spectrum; se deshabilita sin archivo.
File > Exit termina el programa. Escape sale si está en ventana; en pantalla
completa vuelve a ventana. Cerrar la ventana también termina el programa.
CPU > Reset reinicia CPU, RAM, ULA y audio; conserva la cinta y la rebobina.
CPU > Pause (F5) congela toda la máquina, incluida cinta y sonido.
CPU > Speed up (F6) duplica la velocidad global hasta 16x.
CPU > Speed down (F7) la divide por dos hasta 0,25x (cuatro veces más lento).
CPU > Normal speed 1x (F8) vuelve directamente a velocidad normal.
CPU > Rewind 1 second (F9) vuelve aproximadamente un segundo, hasta 10 segundos.
CPU > CPU frequency permite elegir 1x/2x/4x e indica los MHz del modelo activo.
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
View > 2x / 4x ajusta el área de imagen a 640x480 / 1280x960.
View > Fullscreen (F11) alterna pantalla completa sin cambiar la resolución del
escritorio. Escape vuelve a ventana; en ventana sigue siendo Exit. Elegir 2x/4x
sale de pantalla completa y aplica ese tamaño. Se conserva el tamaño de ventana
anterior al salir de pantalla completa con F11 o Escape.
View > Filters ofrece None, Scanlines (light), Scanlines (strong) y Pixel grid.
Las opciones seleccionadas se marcan en el menú.
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

Se ejecutan los perfiles 48K, 128K inglés/español, +2 y +3, con sus ROMs
correspondientes, paginación y dispositivos. if1-2.rom pertenece a Interface 1 y no sirve como ROM
de sistema. Una selección inválida o cancelada conserva ROM, CPU, RAM y cinta.
Una selección correcta reinicia la máquina y rebobina la cinta montada, igual que
CPU > Reset. La última selección válida se guarda entre ejecuciones, incluyendo si era una
ROM alternativa de 48K. En Settings > ROM > On startup se elige Last opened ROM
o Spectrum 48K. Esto cambia el próximo arranque, sin reiniciar la sesión actual.
Settings > ROM > ZX Spectrum 48K permite cargar directamente la ROM de 48K.
128K inglés/español, +2 y +3 también se pueden seleccionar.

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
perfil no activa por sí solo hardware que falta. La paginación especial del +3
y su controladora de disco ya están registradas como capacidades implementadas. RAM paginada, pantalla alternativa, AY y tiempos
de vídeo/audio 128K están implementados y registrados como capacidades.

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
El reloj de audio gobierna el avance del núcleo según los Hz del modelo; se emula
el beeper del bit 4, la señal de cassette y el AY en 128K/+2, sin la contribución
analógica de MIC. La carga rápida solo actúa en la página ROM reconocida. La reproducción por pulsos
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
El bit de pantalla selecciona banco 5/7 en la ULA. El +3 añade 1FFD, cuatro
páginas ROM, cuatro mapas all-RAM y su decodificación de puertos específica.

make test incluye paging-tests: bancos completos, protección de ROM, alias,
decodificación de todos los puertos, bloqueo, reset, configuraciones rechazadas
y arranque de las ROMs 128K inglesa/española y +2 en el núcleo. 128K/+2/+3 están habilitados en la interfaz.

Vídeo y audio por modelo (tarea 6)
----------------------------------
La ULA lee el banco de pantalla seleccionado, independientemente del mapa CPU.
Bitmap, atributos, FLASH y bus flotante usan la misma fuente; cambiar de pantalla
redibuja toda la imagen y las escrituras ocultas no la modifican. Se conserva el
recorte 320x240, situado respecto al inicio de pantalla del perfil. El borde
sigue siendo por scanline, sin contención ni cambios por t-state.

El audio se muestrea a 48000 Hz respecto al reloj emulado, conservando la fase
entre frames. FrameAudioCount sustituye la suposición de 960 muestras por frame;
ReadAudio conserva las muestras sobrantes para rellenar fragmentos Allegro de
960, también tras diálogos, y reset las descarta. El refresco de presentación
sigue a 50 Hz, pero no fija la velocidad del núcleo. video128-tests comprueba
pantallas, FLASH, bus flotante, recorte y muestras exactas con distintos lotes
de ciclos y tamaños de fragmento. 128K/+2 ya incorporan AY.

AY y cinta en 128K/+2
---------------------
ay.h emula los tres tonos, ruido de 17 bits, mezclador y 16 formas de envolvente
del AY-3-8912. SpectrumIO decodifica FFFD/BFFD y sus alias; el AY solo responde
en modelos que lo tienen. El sonido se mezcla en mono con beeper y cinta a
48 kHz, conservando los eventos del chip entre instrucciones. La curva DAC es
aproximada y no reproduce la red analógica exacta. El puerto paralelo devuelve
pull-ups cuando es entrada; no se conectan periféricos serie externos.

La carga rápida reconoce la página de BASIC (0 en 48K, 1 en 128K/+2) y no
intercepta la ROM de menú. Las ROMs alternativas siguen usando pulsos. Los
pulsos TAP se convierten de 3,5 MHz al reloj CPU elegido con un resto entero,
sin acumular redondeo en cada instrucción. Pausa/reanudación conserva la fase;
seleccionar otro bloque o rebobinar la reinicia.

En las ROMs inglesa 128K y +2, Enter selecciona Tape Loader al arrancar.
Para BASIC, usar CAPS SHIFT + 6 para bajar a 128 BASIC y pulsar Enter. En el
editor 128K se escriben las palabras completas. La ROM española entra en BASIC
directamente. Fast load funciona al llegar a la rutina ROM reconocida.

back3.tap se valida en ambos modos hasta su primera fase jugable. En pulsos,
detener en el bloque 9 durante la introducción y continuar con Space; esperar
al siguiente pedido de carga antes de Play. Detener tras los bloques 9/10 para
conservar el resto de la cinta para las fases siguientes. Cue selected permite
volver a un bloque si el juego indica WRONG BLOCK. No hay reconocimiento
específico del juego ni pausa automática de multicarga.

make test incluye ay-tests, tape128-tests y compat128-tests: las tres ROMs nuevas,
PRINT 1+1 desde teclado, 48 BASIC, cargas en bancos separados, música variable y
partida de back3 en ambos modos. Se mantiene la regresión completa de Cobra.
La escucha y una partida de todas las fases siguen siendo comprobaciones manuales.

Estabilidad del menú de ROM
---------------------------
Todas las opciones de modelo se crean como casillas desde el inicio: Allegro
no permite cambiar CHECKBOX después de crear una opción. Intentarlo en cada
refresco reconstruía continuamente el menú nativo, causando parpadeo y pérdidas
de selección. test-ui comprueba que un menú estable no recibe escrituras de
flags durante los ticks de refresco, además de verificar los cambios de modelo.

Reinicio de Capitán Trueno al empezar
-------------------------------------
capitan1.tap usa DEC IYL (FD 2D) seguido de JP NZ para controlar bucles de
copia de memoria. El núcleo reducía IYL pero dejaba los flags anteriores, así
que Z no reflejaba cuándo el contador llegaba a cero. El bucle escribía fuera
de su destino y dañaba código; después se ejecutaba CALL 0000 y arrancaba la ROM.
Afectaba tanto a carga rápida como a reproducción por pulsos en 48K.

Los INC/DEC de IXH, IXL, IYH e IYL reutilizan ahora las operaciones de 8 bits
que actualizan S, Z, H, P/V y N, conservando carry. No cambia INC/DEC de los
registros IX/IY completos, que no deben modificar flags. No se parchea el juego.
z80-index-flags-tests recorre los 256 operandos y ambos estados iniciales de
flags para los ocho opcodes y comprueba un bucle de copia con un centinela.
capitan-tests carga desde BASIC, elige 1 Teclado y verifica partida sin reinicio
en ambos modos, junto con la batería existente de Cobra y back3.

Corrupción de texto en Bomb Jack
--------------------------------
bombjack.tap usa CPI en EA13 para buscar los códigos de control de su rutina
de texto. CPI marcaba Z según el byte leído fuese cero, en vez de comparar A
con ese byte. Los códigos de color y posición no se reconocían, mezclando
caracteres y parámetros en el menú y la pantalla de juego.

CPI, CPD y CPIR reutilizan ahora la comparación de 8 bits que conserva carry;
S/Z/H reflejan A menos el byte, N se activa y P/V refleja BC tras decrementarlo.
CPD y CPIR también obtenían S del operando en vez del resultado; CPD/CPI tenían
cálculos de H incorrectos. Se mantiene el comportamiento de repetición y tiempos.
CPDR ya usaba esa comparación y queda cubierto por las mismas pruebas.

z80-block-compare-tests recorre las 65536 parejas de operandos, BC igual a 0/1/2
y flags iniciales a cero/uno para las cuatro instrucciones. Comprueba flags
documentados, carry, puntero, contador, tiempos y terminación de las búsquedas.
bombjack-tests carga desde BASIC en 48K en ambos modos, contrasta píxel a píxel
los textos del menú con la fuente del juego en sus posiciones esperadas y entra
en una partida con pantallas cambiantes. No se parchean el TAP ni la ROM.

Presentación y filtros de View
------------------------------
screen_view.h separa la presentación de la memoria y el renderizado de la ULA.
La ventana arranca a 2x (640x480); el escalado manual conserva proporciones,
usa factores enteros cuando caben y centra la imagen sobre negro. Los filtros
son máscaras sobre la imagen ampliada: scanlines al 25%/50% de opacidad y una
rejilla de un píxel de salida. Se dibujan en una sola llamada de primitivas,
sin suavizar el bitmap original. A escalas inferiores a 2x se omite la máscara
para conservar legibilidad. El tamaño y filtro elegidos se mantienen durante
la sesión, también al cambiar ROM o cinta.

Los cambios de View no reinician CPU, cinta ni audio. Pantalla completa usa
ALLEGRO_FULLSCREEN_WINDOW y permite volver con F11/Escape sin recrear display,
bitmap ni stream. Si el escritorio rechaza el cambio se muestra un error.
Referencia: https://liballeg.org/a5docs/trunk/display.html#al_set_display_flag

screen-view-tests usa bitmaps de memoria para comprobar factores 2x/4x,
proporciones, márgenes, intensidad de scanlines, rejilla y conservación de la
imagen fuente. test-ui verifica tamaños reales de ventana, selección de filtros,
pantalla completa y restauración por Escape/F11, con el menú estable después.

Pausa, velocidad y retroceso
-----------------------------
F5 pausa/reanuda el núcleo sin mover la cinta ni avanzar AY, vídeo o registros;
el audio entrega silencio durante la pausa. Los menús y View siguen disponibles.
La pausa de CPU es distinta de Tape > Pause, que solo pausa el cassette.

F6 sube la velocidad global por pasos: 0,25x -> 0,5x -> 1x -> 2x -> 4x -> 8x -> 16x.
Al llegar a 16x se mantiene ese valor. F7 baja por la misma escala hasta 0,25x; F8 vuelve
directamente a 1x desde cualquier velocidad. Los tres controles están juntos en
CPU y sus atajos son contiguos. Los extremos se deshabilitan en el menú.

La velocidad global cambia cuánto tiempo emulado transcurre por segundo real,
incluida la cinta por pulsos. Por encima de 1x el sonido también se acelera.
El stream conserva 48 kHz: en los modos rápidos se generan tantas muestras
virtuales por muestra de salida como indique el multiplicador y se promedian. A 0,5x/0,25x cada muestra virtual se mantiene
2/4 muestras de salida, conservando los sobrantes entre fragmentos. La máquina
avanza menos tiempo y el sonido también se ralentiza. El título muestra la
velocidad activa, usando 0.5x o 0.25x para distinguir cámara lenta del retroceso
de F9.

CPU > CPU frequency modifica únicamente los ciclos que obtiene el Z80 respecto
a los dispositivos: 1x, 2x o 4x (3,50/7/14 MHz en 48K y aproximadamente
3,55/7,09/14,19 MHz en 128K/+2). ULA, AY y cassette conservan su reloj. El beeper
y los juegos que temporizan mediante bucles de CPU pueden cambiar su ritmo;
para cargas por pulsos y juegos sensibles a tiempos, usar 1x. Para acelerar una
carga, usar F6, que mantiene la relación de tiempos entre CPU y cinta.
Las dos velocidades son independientes y se pueden combinar; el título indica
pausa, velocidad global y multiplicador de CPU cuando están activos.

F9 recupera aproximadamente un segundo por pulsación, con un historial de hasta
500 frames (unos 10 segundos de tiempo emulado). Se guardan estados cada 10
frames, por lo que el salto puede variar unas décimas; si queda menos historial,
se vuelve al estado más antiguo disponible. El historial se mantiene en RAM,
con unas 51 copias como máximo (aproximadamente 3 MB en 48K / 7 MB en 128K).
Repetir F9 permite seguir retrocediendo. Si estaba pausado, permanece pausado.
Al continuar se descarta el futuro anterior y se crea una nueva rama de ejecución.

Los estados conservan registros, RAM y paginación, selección de pantalla, FLASH,
bordes por línea, AY, fase de audio, muestras pendientes, cinta, bloque/flanco,
carga rápida y frecuencia de CPU. La imagen se reconstruye sin copiar recursos
Allegro ni punteros al bus. Se vacía el audio de salida al cambiar pausa,
velocidad, frecuencia o posición temporal. Reset, abrir/cerrar cinta y cambiar
ROM borran el historial; los fallos de apertura conservan la sesión y el historial.
La frecuencia seleccionada y los controles de pausa/velocidad se mantienen hasta
que el usuario los cambia o cierra el programa.

playback-tests verifica la escala 0,25x-16x y sus límites, pausa sin avance, separación entre las dos velocidades,
carga real ROM por pulsos a velocidades 0,25x-16x, reproducción exacta tras restaurar RAM,
paginación/pantalla 7, AY, FLASH, vídeo, cinta y audio parcialmente consumido,
y límite/ramificación/invalidez del historial tras cambiar de sesión. test-ui
prueba los controles CPU y F5-F9, incluyendo subir/bajar/restaurar velocidad
y rebobinar la frecuencia anterior
sin salir de pausa, además de los menús y filtros existentes. También comprueba
cámara lenta, muestras pendientes y fragmentos de longitudes no divisibles por cuatro.

Partidas guardadas
------------------
File > Save game (Ctrl+S) abre una galería de 12 casillas con miniaturas,
nombre del juego y fecha. Selecciona una casilla con el ratón o las flechas y
pulsa Save / Replace (o Enter), o haz doble clic para guardar directamente.
La casilla QUICK está reservada para F2.
File > Load game (Ctrl+L) permite recuperar cualquier casilla, incluida QUICK.
El doble clic en una casilla también permite cargarla directamente.
F2 guarda directamente en QUICK y F3 recupera esa partida sin abrir la galería.
Escape o Cancel cierra la galería. La emulación se detiene mientras está abierta.

Los archivos slot-0.gss a slot-11.gss se guardan en el directorio de datos de
Allegro para Gomas, subdirectorio saves (habitualmente ~/.local/share/gomas/gomas/saves
en Linux). GOMAS_SAVE_DIR permite elegir otro directorio. Las casillas son
comunes a todos los juegos; slot-0.gss corresponde a QUICK.

Cada archivo contiene ROM, cinta TAP, miniatura y estado completo del Spectrum,
por lo que se puede recuperar en otra sesión aunque los originales se hayan
movido. Al cargar se recuperan el modelo, RAM, CPU, frecuencia, vídeo, AY, audio
pendiente y posición exacta de cinta. La pausa, velocidad global y filtros de
View mantienen los valores actuales. Se inicia un historial de retroceso nuevo.
El formato propio GSS versión 2 (compatible con lectura de partidas v1) incluye límites y comprobación de integridad;
un fallo de lectura conserva la sesión actual. El guardado sustituye la casilla
mediante un archivo temporal para proteger el guardado anterior si falla la escritura.

Cintas comprimidas
------------------
File > Open... y la línea de comandos aceptan archivos .zip / .ZIP con
TAP, TZX, SNA, Z80, SZX, SLT y DSK:

  ./bin/gomas juegos/cobra.zip

Se lee en memoria la primera entrada compatible (sin distinguir mayúsculas), incluso
si está dentro de una carpeta del ZIP. Se ignoran otros archivos y no se extrae
nada al disco. Si hay varios archivos compatibles se usa el primero en el orden del ZIP.
El reproductor, la carga rápida y las partidas guardadas funcionan igual que
con un TAP suelto; el título muestra el nombre del ZIP. La partida guardada
incluye el TAP descomprimido, por lo que no necesita conservar el ZIP original.
Se admiten ZIP almacenados o comprimidos. Las entradas cifradas, cintas vacías,
archivos dañados y cintas de más de 16 MiB se rechazan conservando la sesión.
La apertura se limita a recorrer 4096 entradas. Los TAP sueltos comparten el
límite de 16 MiB para poder incluirlos también en las partidas guardadas.

TZX y snapshots SNA/Z80
----------------------
Los TZX conservan su estructura y tiempos originales en el reproductor. Se
admiten bloques estándar y turbo, tonos, secuencias de pulsos, datos puros,
grabación directa, CSW RLE/Z-RLE, datos generalizados, pausas/paradas, grupos,
saltos, bucles, llamadas/retornos, stop en 48K y selección del nivel EAR.
Los textos y metadatos no generan señal. Los bloques de selección interactiva 0x28 abren una lista de rutas; Enter o
doble clic elige una. Escape cancela manteniendo la cinta detenida; Tape >
Choose TZX route permite abrirla de nuevo. Las decisiones se conservan al guardar
y retroceder. Rebobinar al inicio descarta las decisiones para elegir otra ruta.
Los tipos obsoletos no implementados se rechazan sin cambiar la sesión. Los bucles se expanden con límites de bloques, símbolos y pulsos para
que una cinta dañada no bloquee el programa.

Fast load está disponible para TZX compuestos de bloques estándar, además de
TAP. En TZX con señales especiales se desactiva automáticamente y aparece el
deck; pulsa Play después de LOAD "". El deck permite elegir bloques, pausar y
reanudar; las paradas TZX se respetan y Play permite continuar. Los estados
GSS y el retroceso conservan también la posición dentro de una cinta TZX.

SNA admite 48K y 128K, incluidas las variantes de banco duplicado. Z80 admite
las versiones 1, 2 y 3, RAM sin comprimir o con RLE y modelos 48K/128K/+2/+3.
Se restauran registros, RAM, paginación/pantalla, borde y, cuando el formato los
incluye, registros AY y fase de vídeo. Son snapshots externos: no incluyen una
cinta, historial ni el estado de audio interno exacto que conserva GSS.
Al abrirlos se retira la cinta anterior y se selecciona automáticamente el
modelo; si hace falta cambiarlo se busca su ROM en roms, conservando la ROM
actual cuando ya corresponde al modelo. Un fallo de lectura o una ROM ausente
conserva la sesión anterior. El nombre del snapshot aparece en el título y en
la galería; File > Close cierra también un snapshot.

Los modelos y expansiones aún no emulados (por ejemplo Pentagon, SamRam,
TR-DOS o una ROM de expansión paginada) se rechazan explícitamente.
SZX admite modelos 48K/128K/+2/+3, RAM y ROM incrustadas con o sin zlib, estado
CPU (incluido HALT), paginación, ULA y AY. También recupera cintas TAP/TZX y
discos DSK incrustados. Los enlaces a cintas se resuelven respecto al SZX o al
miembro SZX dentro del ZIP. Un enlace ausente o inválido conserva la sesión.
No se emulan expansiones como Interface 1, Multiface, DivIDE o Beta Disk.

SLT carga el snapshot Z80 y sus niveles comprimidos. La instrucción ED FB
selecciona el nivel por A y lo copia al destino indicado por HL usando el mapa
de memoria activo; no necesita archivos externos. Los niveles se incluyen en
las partidas GSS y se conservan al retroceder. SLT y SZX también abren desde ZIP.

Referencias de los lectores:
https://www.worldofspectrum.net/TZXformat.html
https://worldofspectrum.org/faq/reference/z80format.htm
https://worldofspectrum.net/zx-modules/fileformats/snaformat.html

Discos DSK / Extended DSK y Spectrum +3
--------------------------------------
File > Open... acepta DSK y EDSK (extensión .dsk), también dentro de ZIP. Se
selecciona +3 y se inserta el disco en A; en el menú de la ROM, Loader arranca
los discos preparados para ello. Se necesita el conjunto plus3-0..3.rom.

Disk > Show disk drives abre un panel independiente del reproductor de cinta,
con unidades A/B, nombre, pistas/caras, cilindro, motor y actividad. Sus botones
Insert, Eject, Save as y Write protect corresponden a las opciones del menú Disk.
Insertar en un +3 activo permite intercambiar discos sin reiniciar la CPU;
insertar desde otro modelo cambia a +3. File > Close cierra los medios y resetea.

Las escrituras del Spectrum modifican solo la imagen en memoria. Save as exporta
un Extended DSK mediante reemplazo temporal/atómico; no se sobrescribe un ZIP
original. El panel marca los cambios sin guardar. La protección contra escritura
se aplica también a los comandos de la controladora.

El +3 implementa los cuatro mapas RAM especiales de 1FFD, selección de cuatro
ROMs, bloqueo y motor. La controladora uPD765 usa transferencia no DMA y fases
command/data/result, con intervalos de disponibilidad por byte, búsqueda y
recalibrado, Sense Interrupt/Drive, Read ID/Data, Write Data y Format Track.
Se conservan flags de sector y geometrías de DSK/EDSK; no se pretende emulación
a nivel de flujo magnético ni compatibilidad universal con protecciones de disco.
Las partidas y el historial conservan ambos discos, modificaciones, protección,
cilindros y una transferencia en curso. Las imágenes se comparten entre estados
y solo se copian al modificarlas, evitando duplicar el disco en cada frame.

new-media-tests comprueba SZX/SLT/ZIP, enlaces e imágenes incrustadas, HALT,
compatibilidad GSS v1, selección TZX y restauración, paginación +3, lectura,
escritura, protección, búsqueda, formato y partida a mitad de sector. También
arranca un disco sintético con checksum usando la ROM real del +3. test-ui prueba
los botones de disco y el diálogo TZX con cancelación, doble clic y guardado.
Las protecciones comerciales y todos los juegos DSK requieren validación manual.

Referencias adicionales:
https://www.spectaculator.com/docs/zx-state/index.html
https://worldofspectrum.org/faq/reference/formats.htm
https://www.cpctech.cpcwiki.de/docs/dsk.html
https://github.com/fuse-emulator/fuse/blob/master/machines/specplus3.c

Catálogo de juegos y biblioteca local
--------------------------------------
File > Game catalog... abre el catálogo de ZXArt.ee. La primera versión usa
su API pública de juegos: buscador, orden A-Z/Z-A, páginas y lista de versiones
por juego. Selecciona un título y un archivo compatible (formato y 48K/128K/+2/+3)
y pulsa Install & load. Los archivos ofrecidos por el catálogo que requieren
hardware no soportado se muestran deshabilitados. No todos los títulos tienen
una descarga compatible disponible.

Las búsquedas y descargas trabajan en segundo plano; se puede seguir usando
la ventana o cancelar. La búsqueda se actualiza tras una pausa breve al escribir.
La rueda desplaza las listas; Tab cambia de panel, flechas recorren las listas,
Enter busca o carga y Escape cierra. El emulador queda pausado mientras se navega.
Una descarga y apertura válidas sustituyen la sesión; un fallo conserva la anterior.

Los archivos se guardan en games/zxart/<juego>/<versión-archivo>/<nombre>.
En el proyecto, games queda junto a roms; si esa ubicación no está disponible se
usa el directorio de datos del usuario, subdirectorio games. GOMAS_GAMES_DIR
permite elegir otra ubicación. games/catalog.json guarda los metadatos visitados,
versiones e instalaciones. No se descarga el catálogo entero al abrir la ventana.
Los nombres y rutas locales se construyen de forma segura, sin extraer rutas de
ZIP al disco. Se comprueban tamaño, MD5 cuando ZXArt lo proporciona, y formato;
el archivo definitivo se reemplaza mediante una escritura temporal/atómica.

Los juegos con alguna versión descargada muestran Installed. All games / Installed
permite ver solo la biblioteca local, sin consultas de red. Offline: ON usa los
metadatos ya visitados y las versiones instaladas; esa preferencia se conserva.
Los juegos descargados cargan desde disco, sin volver a solicitar el archivo.
Si falla la red, se ofrecen los resultados guardados que coincidan con la búsqueda.
El catálogo sin conexión contiene lo visitado/instalado, no todos los juegos del
servidor. Un archivo local dañado necesita una descarga nueva en modo online.

Load installed game abre directamente la versión instalada. Los snapshots
arrancan desde su estado; para TAP/TZX se elige la ROM indicada y se introduce
LOAD automáticamente usando el teclado de la ROM. En 128K/+2/+3 se usa Loader.
Se respeta Fast load; con reproducción real se pulsa Play automáticamente.
Los discos arrancan a través del Loader del +3. Las preguntas de un juego y las
rutas interactivas TZX siguen siendo decisiones del usuario. En juegos con varios
archivos, el catálogo permite elegir cada versión/archivo ofrecido por ZXArt.

catalog-tests usa respuestas de prueba sin red para verificar búsqueda, orden,
páginas, versiones, rutas, checksum, cancelación, archivos ZIP, reutilización
offline y carga transaccional. Prueba también el arranque de Cobra en los cuatro
modelos. catalog-tests --live es una comprobación opcional de consulta y descarga
real; la suite normal no necesita Internet. test-ui abre el catálogo con una
biblioteca aislada, busca y carga una versión instalada sin contactar con ZXArt.

Referencias de la integración:
https://github.com/moroz1999/zxart-api-bridge
https://hype.retroscene.org/blog/898.html
https://curl.se/libcurl/c/CURLOPT_XFERINFOFUNCTION.html

Vista de depuración
-------------------
Debug > Show debugger (F12) abre una ventana independiente y pausa la máquina.
Muestra AF/BC/DE/HL, registros alternativos, IX/IY/SP/PC, I/R, IFF1/IFF2, modo de
interrupción y todos los bits de F. También muestra fase de vídeo, frecuencia,
paginación 7FFD/1FFD, banco de pantalla, pila y el mapa activo de memoria.
La ventana se actualiza en vivo al reanudar con F5; cerrarla mantiene la pausa
actual. Escape cierra solo la ventana de depuración.

F10 / Step CPU avanza una instrucción del núcleo y conserva la pausa. F4 / Next
frame avanza hasta el siguiente límite de fotograma. Ambos siguen la misma ruta
que la ejecución normal, incluyendo interrupciones, overclock, ULA, AY, cassette
y disquetera. Se respetan las trampas de carga rápida y SLT; una trampa puede
copiar un bloque completo durante un paso. El audio de salida se descarta al
pasar al modo de pasos y permanece en silencio mientras la máquina está pausada.

El visor de memoria muestra 256 bytes en hexadecimal y ASCII. View alterna entre
el mapa CPU completo y cada banco físico de RAM, sin modificar la paginación.
Address/Offset acepta una dirección hexadecimal (0000-FFFF en CPU, 0000-3FFF en
bancos físicos): pulsa el campo, escribe y confirma con Enter o Go. La rueda,
las flechas y PgUp/PgDn desplazan la vista. Follow PC / Home sigue el contador
de programa; Go to SP muestra la pila. PC se resalta en cian y SP en violeta.
La inspección lee memoria, no puertos de E/S, y no consume datos de la cinta o
controladora. Esta primera vista permite inspección y avance; no edita RAM ni
registros y todavía no incluye desensamblador o puntos de ruptura.

debug-tests verifica que pasos individuales y frames producen el mismo estado
en 48K/128K/+2/+3, incluyendo interrupciones, audio, overclock, cinta, AY,
disquetera y retroceso. test-ui prueba apertura pausada, botones de paso,
navegación RAM sin cambios en la máquina y cierre independiente.
