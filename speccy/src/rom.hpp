#ifndef ROM_H
#define ROM_H

#include <cstring>
#include "bus.hpp"

template<unsigned int B, unsigned int S>
class ROM : public BusComponent<B,S> {
  protected:
    unsigned char data[S];

  public:
    ROM() {
      memset(data, 0, S);
    };
    ~ROM() {};

    int Load(char *romName) {
      FILE *fd = NULL;
      fd = fopen(romName, "r");
      if (fd != NULL) {
          fread(data, 1, S, fd);
          fclose(fd);
          return 0;
      }
      return -1;
    };

    void Write(unsigned int address, unsigned char value) {
        // Do nothing. This is a ROM.
    };

    unsigned char Read(unsigned int address) {
      return (data[address-B]);
    };
};

#endif
