rick2: ../src/main.cpp world.o pugixml.o
	g++ -o rick2 ../src/main.cpp world.o pugixml.o -O3 -std=c++11 -static -lallegro-5.0.10-monolith-md

world.o: ../src/world.cpp
	g++ -c ../src/world.cpp

pugixml.o: ../src/pugixml.cpp
	g++ -c ../src/pugixml.cpp