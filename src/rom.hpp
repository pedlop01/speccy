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
      FILE *fd = null;
      int err = fopen_s(&fd, romName, "rb");
      if ((err == 0) && (fd != NULL)) {
          fread(data, 1, S, fd);
          fclose(fd);
      }
      return err;
    };

    void Write(unsigned int address, unsigned char value) {
        // Do nothing. This is a ROM.
    };

    unsigned char Read(unsigned int address) {
      return (data[address-B]);
    };
}

#endif