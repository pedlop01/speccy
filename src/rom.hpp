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

    int Load(const char *romName) {
      FILE *fd = NULL;
      fd = fopen(romName, "rb");
      if (fd != NULL) {
          size_t bytes = fread(data, 1, S, fd);
          fclose(fd);
          return bytes == S ? 0 : -1;
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
