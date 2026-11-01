Gomas — emulador ZX Spectrum 48K con Allegro 5

El núcleo procede de YASS (referencia incluida en ../yass).

Compilar y ejecutar
------------------
Requiere C++11, make, pkg-config y las bibliotecas de desarrollo allegro-5 y
allegro_audio-5. Ya no necesita primitives ni acodec.

  make -C bin
  cd bin
  ./gomas manic.tap

El segundo argumento opcional permite indicar la ROM desde otro directorio:

  ./bin/gomas bin/manic.tap yass/ROM/48.rom

Escape o cerrar la ventana termina el programa. La ventana se puede redimensionar;
la imagen conserva su proporción y usa escala entera cuando cabe, sin suavizado.
LSHIFT corresponde a CAPS SHIFT y RSHIFT a SYMBOL SHIFT. Al perder el foco se
liberan las teclas para evitar que queden pulsadas.

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
de búferes y arranque de la ROM incluida durante 500 frames.

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
