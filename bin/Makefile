CXX ?= g++
CXXFLAGS ?= -O2 -std=c++11
PACKAGES = allegro-5 allegro_audio-5 allegro_dialog-5
CPPFLAGS += $(shell pkg-config --cflags $(PACKAGES))
LDLIBS += $(shell pkg-config --libs $(PACKAGES))
SOURCES = ../src/main.cpp ../src/z80.cpp ../src/ula.cpp
HEADERS = $(wildcard ../src/*.h ../src/*.hpp)

.PHONY: all clean test
all: gomas

gomas: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $(SOURCES) $(LDLIBS)

test: ula-tests floating-bus-tests machine-tests
	./ula-tests
	./floating-bus-tests
	./machine-tests

ula-tests: ../src/tests/ula_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/ula_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

floating-bus-tests: ../src/tests/floating_bus_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/floating_bus_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

machine-tests: ../src/tests/machine_test.cpp ../src/ula.cpp ../src/z80.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ ../src/tests/machine_test.cpp ../src/ula.cpp ../src/z80.cpp $(LDLIBS)

clean:
	rm -f *.o gomas ula-tests floating-bus-tests machine-tests
