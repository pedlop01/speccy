CXX ?= g++
CXXFLAGS ?= -O2 -std=c++11
PACKAGES = allegro-5 allegro_audio-5 allegro_dialog-5 allegro_font-5 allegro_primitives-5
CPPFLAGS += $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell pkg-config --libs $(PACKAGES))
SOURCES = ../src/main.cpp ../src/z80.cpp ../src/ula.cpp
HEADERS = $(wildcard ../src/*.h ../src/*.hpp)

.PHONY: all clean test test-ui
all: gomas

gomas: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES) $(LDLIBS)

test: ula-tests floating-bus-tests machine-tests tape-tests tape-game-tests
	./ula-tests
	./floating-bus-tests
	./machine-tests
	./tape-tests
	./tape-game-tests

ula-tests: ../src/tests/ula_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/ula_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

floating-bus-tests: ../src/tests/floating_bus_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/floating_bus_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

machine-tests: ../src/tests/machine_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/machine_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

tape-tests: ../src/tests/tape_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

tape-game-tests: ../src/tests/tape_game_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_game_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

test-ui: tape-ui-tests
	./tape-ui-tests

tape-ui-tests: ../src/tests/tape_ui_test.cpp ../src/main.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/tape_ui_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS) $(shell pkg-config --libs x11 xtst) -pthread

clean:
	rm -f *.o gomas ula-tests floating-bus-tests machine-tests tape-tests tape-game-tests tape-ui-tests
