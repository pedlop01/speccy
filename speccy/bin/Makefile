gomas: ../src/main.cpp ../src/z80.cpp ula.o
	g++ -o gomas ../src/main.cpp ../src/z80.cpp ula.o -O3 -std=c++11 -static -lallegro-5.0.10-monolith-md
	#g++ -o gomas ../src/main.cpp ../src/z80.cpp ula.o -std=c++11 `pkg-config --libs allegro-5.0 allegro_primitives-5.0`

ula.o: ../src/ula.cpp
	g++ -c ../src/ula.cpp -O3
