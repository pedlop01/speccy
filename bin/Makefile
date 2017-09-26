gomas: ../src/main.cpp ../src/z80.cpp ula.o
	g++ -o gomas ../src/main.cpp ../src/z80.cpp ula.o `pkg-config --libs allegro-5.0 allegro_primitives-5.0`

ula.o: ../src/ula.cpp
	g++ -c ../src/ula.cpp
