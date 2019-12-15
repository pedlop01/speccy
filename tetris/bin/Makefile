PLATFORM := $(shell uname -o)
ifeq  ($(PLATFORM),GNU/Linux)
tetris: ../src/tetris.c
	gcc -o tetris ../src/tetris.c -O3 `pkg-config --libs allegro-5 allegro_primitives-5 allegro_image-5 allegro_font-5 allegro_ttf-5`
else
tetris: ../src/tetris.c
	gcc -o tetris ../src/tetris.c -O3 -static -lallegro-5.0.10-monolith-md
endif
