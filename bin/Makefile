gomas: ../src/main.cpp ../src/z80.cpp ula.o
	#g++ -o gomas ../src/main.cpp ../src/z80.cpp ula.o -O3 -std=c++11 -static -lallegro-5.0.10-monolith-md
	g++ -o gomas ../src/main.cpp ../src/z80.cpp ula.o -std=c++11 `pkg-config --libs allegro-5 allegro_primitives-5 allegro_audio-5 allegro_acodec-5`

ula.o: ../src/ula.cpp
	g++ -c ../src/ula.cpp -O3

clean:
	rm -f *.o gomas
