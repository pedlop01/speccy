rick2: ../src/main.cpp world.o keyboard.o camera.o sprite.o animation.o character.o timer.o pugixml.o
	g++ -o rick2 ../src/main.cpp world.o keyboard.o camera.o sprite.o animation.o character.o timer.o pugixml.o -O3 -std=c++11 -static -lallegro-5.0.10-monolith-md

world.o: ../src/world.cpp
	g++ -c ../src/world.cpp

keyboard.o: ../src/keyboard.cpp
	g++ -c ../src/keyboard.cpp

camera.o: ../src/camera.cpp
	g++ -c ../src/camera.cpp

sprite.o: ../src/sprite.cpp
	g++ -c ../src/sprite.cpp

animation.o: ../src/animation.cpp
	g++ -c ../src/animation.cpp

character.o: ../src/character.cpp
	g++ -c ../src/character.cpp

timer.o: ../src/timer.cpp
	g++ -c ../src/timer.cpp

pugixml.o: ../src/pugixml.cpp
	g++ -c ../src/pugixml.cpp