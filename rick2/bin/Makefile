PLATFORM := $(shell uname -o)
ifeq  ($(PLATFORM),GNU/Linux)
rick2: ../src/main.cpp world.o keyboard.o camera.o sprite.o animation.o character.o colbox.o timer.o object.o platform.o item.o laser.o static_object.o pugixml.o
	g++ -o rick2 ../src/main.cpp world.o keyboard.o camera.o sprite.o animation.o character.o colbox.o timer.o object.o platform.o item.o laser.o static_object.o pugixml.o -O3 -std=c++11 `pkg-config --libs allegro-5.0 allegro_primitives-5.0 allegro_image-5`
else
rick2: ../src/main.cpp world.o keyboard.o camera.o sprite.o animation.o character.o colbox.o timer.o object.o platform.o item.o laser.o static_object.o pugixml.o
	g++ -o rick2 ../src/main.cpp world.o keyboard.o camera.o sprite.o animation.o character.o colbox.o timer.o object.o platform.o item.o laser.o static_object.o pugixml.o -O3 -std=c++11 -static -lallegro-5.0.10-monolith-md
endif

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

colbox.o: ../src/colbox.cpp
	g++ -c ../src/colbox.cpp

timer.o: ../src/timer.cpp
	g++ -c ../src/timer.cpp

object.o: ../src/object.cpp
	g++ -c ../src/object.cpp

platform.o: ../src/platform.cpp
	g++ -c ../src/platform.cpp

item.o: ../src/item.cpp
	g++ -c ../src/item.cpp

laser.o: ../src/laser.cpp
	g++ -c ../src/laser.cpp

static_object.o: ../src/static_object.cpp
	g++ -c ../src/static_object.cpp

pugixml.o: ../src/pugixml.cpp
	g++ -c ../src/pugixml.cpp
