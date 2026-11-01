#ifndef ROM_H
#define ROM_H

#include <cstring>
#include <cstdio>
#include <vector>
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
      FILE *fd = fopen(romName, "rb");
      if (!fd) return -1;
      std::vector<unsigned char> candidate(S + 1);
      size_t bytes = fread(candidate.data(), 1, candidate.size(), fd);
      bool failed = ferror(fd);
      fclose(fd);
      if (failed || bytes != S) return -1;
      memcpy(data, candidate.data(), S);
      return 0;
    };

    bool LoadBytes(const std::vector<unsigned char> &bytes) {
      if (bytes.size() != S) return false;
      memcpy(data, bytes.data(), S);
      return true;
    }

    void Write(unsigned int address, unsigned char value) {
        // Do nothing. This is a ROM.
    };

    unsigned char Read(unsigned int address) {
      return (data[address-B]);
    };
};

#endif
