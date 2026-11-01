# Plan de soporte Spectrum 128K

Estados: **Hecho**, **Pendiente**, **En curso** y **A la espera de ROMs**.
El estado describe el código integrado, salvo las tareas de diagnóstico y archivos.

| ID | Tarea | Estado | Resultado esperado / criterio de finalización |
|---|---|---|---|
| 0 | Diagnosticar `back3.tap` | Hecho | Las trazas y un prototipo temporal confirman accesos a bancos independientes mediante `0x7FFD` y al AY. El soporte 128K todavía no está integrado. |
| 1 | Centralizar los archivos de ROM en `roms/` | Hecho | `roms/48.rom` está presente y ocupa 16 KB. |
| 2 | Separar y ampliar la carga de ROMs | Hecho | Resolver rutas en `roms/` desde distintos directorios de ejecución, permitir seleccionar archivos y validar tamaño y lectura. Mantener la ROM anterior si la selección falla. Adaptar argumentos, mensajes, documentación y pruebas a la nueva ubicación. |
| 2a | Recordar ROM y configurar el arranque | Hecho | Restaurar la última ROM válida o arrancar siempre con 48K; prioridad de --rom y fallback seguro. Selección y preferencias en Settings > ROM. |
| 3 | Configurar el hardware según el perfil de ROM | Hecho | Descriptores inmutables de memoria, bancos de reset, páginas ROM, temporización, bus flotante y dispositivos para 48K/128K/+2/+3. La máquina aplica el perfil al cargar y al resetear; el cargador y el menú comparten la comprobación de funciones de hardware implementadas. Las ROMs inglesa/española de 128K comparten hardware. |
| 4 | Incorporar y verificar las ROMs 128K | Hecho | Admitir dos archivos de 16 KB o uno combinado de 32 KB. Verificar orden de páginas y asociación al perfil 128K. |
| 5 | Implementar RAM y paginación 128K | Hecho | Ocho bancos de RAM de 16 KB; bancos 5 y 2 fijos; banco seleccionable en `C000–FFFF`; dos páginas de ROM. Implementar selección y bloqueo de `0x7FFD`, con alias correctos de los bancos 2 y 5. |
| 6 | Adaptar vídeo y temporización | Hecho | Mostrar la pantalla del banco 5 o del 7, actualizarla al cambiar de banco y aplicar los tiempos y bus flotante del 128K. Mantener la frecuencia de muestreo de audio al cambiar la duración del fotograma. |
| 7 | Implementar sonido AY | Pendiente | Puertos y registros del AY, tres canales de tono, ruido, mezclador y envolvente. Mezclar la salida con beeper y cinta. |
| 8 | Añadir selección de modelo en Settings | En curso | Implementado `Settings > ROM`, selección automática del perfil, información de ROM y reinicio. La activación de modelos distintos de 48K queda pendiente de su hardware; no hay un segundo selector de modelo independiente. Si faltan ROMs, conservar la máquina actual e informar del archivo requerido. |
| 9 | Adaptar carga rápida y reproductor | En curso | Implementada la protección para la ROM 48K reconocida y la reproducción por pulsos para ROMs 48K alternativas. Pendiente soportar las páginas de los modelos nuevos. Usar reproducción por pulsos cuando la ROM elegida no tenga una trampa compatible. Mantener selección de bloques y multicarga. |
| 10 | Validar arranque y compatibilidad | Pendiente | Pruebas de paginación, bloqueo, alias, ROM, pantalla y AY; arranque del menú 128K y BASIC. Validar `back3.tap` en ambos modos de carga, introducción, música y partida; repetir Cobra y las pruebas 48K existentes. |

## Orden y entregas

1. **Carga de ROMs y perfiles:** tareas 2–4. La tarea 2 puede comenzar con la ROM 48K disponible; las pruebas que usan `48.rom` deben adaptarse también a `roms/`.
2. **Memoria 128K:** tarea 5, con sus pruebas de paginación y ROM.
3. **Vídeo y temporización:** tarea 6, con sus pruebas de bancos de pantalla y tiempos.
4. **Sonido AY:** tarea 7, con pruebas de registros, generadores y mezcla.
5. **Integración y compatibilidad:** tareas 8–10.

Cada entrega incluirá sus pruebas. Los prototipos de diagnóstico no cuentan como soporte terminado. El primer hito funcional será arrancar el menú de la ROM 128K con su mapa de memoria correcto.

## Situación actual de la selección de ROM

La aplicación restaura la última ROM válida o arranca con 48K según la preferencia de Settings > ROM > On startup. Si falta la última ROM, busca `roms/48.rom` o `roms/48k.rom` respecto al ejecutable o al directorio de trabajo. Las pruebas usan la ubicación centralizada. También se admite `--rom archivo` sin necesidad de indicar un TAP y se conserva el argumento posicional antiguo.

Settings permite seleccionar una ROM conocida o una ROM alternativa de 48K de forma explícita. Se identifica el perfil por las huellas del contenido. Las páginas de un conjunto se buscan en el mismo directorio con sus nombres conocidos y se validan; se admiten conjuntos combinados de 32/64 KB en el orden de páginas correspondiente. Interface 1 se rechaza como ROM de sistema.

Las ROMs 128K inglés/español y +2/+3 disponibles ya se han validado mediante pruebas. Su activación sigue pendiente: el cargador devuelve el perfil detectado y la aplicación conserva la máquina actual mientras falte el hardware. Una ROM nueva solo sustituye la anterior tras validarse por completo; una carga correcta reinicia y conserva la cinta montada, rebobinada.

Las preferencias se guardan por usuario. Una ROM elegida por --rom tiene prioridad; los cambios fallidos no sustituyen la última ROM guardada.

## Configuración interna de hardware (tarea 3)

`src/machine_config.h` separa la identidad del hardware de la versión o idioma de ROM. `HardwareFor(model)` devuelve un descriptor inmutable; `SpectrumMachine::Configuration()` expone el perfil activo. Cada descriptor define RAM/ROM en páginas de 16 KB, bancos visibles al reset, página ROM inicial, bancos de pantalla, tipo de paginación, tiempos de ULA y dispositivos AY/disquetera.

`ValidateROMHardware` comprueba que el conjunto de ROM encaja en ese descriptor antes de activar nada. `RequiredHardwareFeatures` y `ImplementedHardwareFeatures` mantienen separados los requisitos físicos de las funciones que ya están implementadas. El menú y el cargador consultan la misma disponibilidad. Un modelo pendiente se rechaza conservando el perfil, CPU, RAM, ULA y cinta actuales.

La ULA ya obtiene del perfil los ciclos por línea/fotograma y la fase/política del bus flotante. El reset reaplica el perfil activo. La salida conserva 320×240 píxeles. Las tareas 5 y 6 ya registran memoria, pantalla alternativa y temporización como capacidades implementadas; AY sigue pendiente antes de habilitar 128K/+2.

## Memoria paginada (tarea 5)

`src/spectrum_memory.h` implementa el mapa 48K y el de 128K/+2. Los bancos 5 y 2 quedan fijos en `4000` y `8000`; los tres bits bajos de `0x7FFD` seleccionan el banco de `C000`. El bit 4 selecciona ROM y el bit 5 bloquea todos los cambios posteriores hasta reset. El bit 3 selecciona la pantalla 5/7, conectada a la ULA en la tarea 6. La decodificación parcial `(puerto & 0x8002) == 0` incluye los alias y permite que un puerto par alcance también la ULA. Las lecturas siguen pasando por teclado/EAR/bus flotante, sin devolver el registro de paginación.

Los alias comparten almacenamiento, incluido el banco 5 utilizado por la ULA. Reset borra toda la RAM, conserva las ROMs y restaura ROM 0, banco superior 0 y paginación desbloqueada. El modo 48K ignora el puerto. La paginación especial del +3 continúa pendiente.

`paging-tests` recorre los ocho bancos completos, ambas ROMs, alias en ambos sentidos, todos los puertos, bloqueo, reset y fallos de configuración. Ejecuta también las ROMs inglesa/española 128K y +2 en el núcleo, con los mismos componentes de memoria y E/S que usa la aplicación. La selección de estos modelos en la aplicación sigue protegida hasta completar AY; este arranque de diagnóstico no equivale a soporte completo ni valida todavía back3.

Referencia del comportamiento: https://github.com/fuse-emulator/fuse/blob/master/machines/spec128.c

## Vídeo y temporización (tarea 6)

La ULA obtiene bitmap, atributos, FLASH y bus flotante del banco de pantalla activo (5/7), sin depender del banco visible en `C000`. Cambiar el bit 3 de `0x7FFD` redibuja la pantalla completa; escribir un banco oculto conserva sus datos sin alterar la imagen. Las escrituras al banco 5 por cualquiera de sus alias mantienen el mismo comportamiento. El bloqueo de paginación protege también la selección de pantalla y reset restaura la primaria.

Se aplican las 228 t-states por línea y 311 líneas por fotograma del 128K/+2 y la primera lectura de bus flotante en 14364. El recorte de 24 líneas de borde superior se sitúa respecto a la fase de pantalla del modelo, manteniendo 320×240. El borde sigue representándose por línea y las escrituras se aplican por instrucción: no se introduce contención ni un renderizado por t-state.

El audio integra 48000 muestras por segundo de reloj emulado (3500000 Hz en 48K, 3546900 Hz en 128K/+2), conservando la fase fraccionaria entre fotogramas. `FrameAudioCount` indica cuántas muestras hay en el frame terminado. `SpectrumMachine::ReadAudio` conserva sobrantes entre los fragmentos fijos de 960 muestras de Allegro y los descarta al resetear; no hay relleno ni ajuste de tono para forzar 50 frames/s. El temporizador de presentación sigue a 50 Hz, independiente del avance del núcleo, gobernado por audio.

`video128-tests` comprueba píxeles de ambos bancos, escrituras ocultas, FLASH, cambio de pantalla, bloqueo/reset, todos los ciclos de bus flotante de las 192 líneas, fase del recorte, recuento exacto de muestras durante 300 frames y equivalencia al dividir ciclos y fragmentos de audio. Continúan las pruebas de ROMs y Cobra rápido/por pulsos. 128K/+2 siguen deshabilitados en Settings hasta la tarea 7 (AY); +3 requiere además sus modos de paginación y disquetera.
