#ifndef IO_HH
#define IO_HH

#include "teg.hh"

#include <stdio.h>
#include <string.h>
#include <errno.h>

namespace IO {
  FILE* OpenDataFileForRead(const char* format, ...)
  __attribute__((format(printf,1,2)));
  FILE* OpenConfigFileForRead(const char* filename);
  FILE* OpenConfigFileForWrite(const char* filename);
  void UpdateConfigFile(const char* filename);
  // sequence for writing a config file:
  // OpenConfigFileForWrite, (write stuff), fclose, UpdateConfigFile
#if __WIN32__
  void DoRedirectOutput();
#endif
  template<class A> union bytey {A a; uint8_t bytes[sizeof(A)];};
  template<class A, class B> union punny {A a; B b; uint8_t bytes[sizeof(A)];};
  static inline void ReadData(FILE* f, void* target, size_t size) {
    if(fread(target, size, 1, f) < 1)
      throw (const char*)(feof(f) ? "Unexpected EOF" : strerror(errno));
  }
  static inline uint16_t Read16LE(FILE* f) {
    bytey<uint16_t> u;
    ReadData(f, u.bytes, sizeof(u.bytes));
    return SDL_SwapLE16(u.a);
  }
  static inline uint32_t Read32LE(FILE* f) {
    bytey<uint32_t> u;
    ReadData(f, u.bytes, sizeof(u.bytes));
    return SDL_SwapLE32(u.a);
  }
  static inline float ReadFloatLE(FILE* f) {
    punny<uint32_t, float> u;
    ReadData(f, u.bytes, sizeof(u.bytes));
    u.a = SDL_SwapLE32(u.a);
    return u.b;
  }
};

#endif
