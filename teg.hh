/* All C++ headers or source files using TEG should include this file. C files
   should include "teg.h" instead. */
#ifndef TEG_HH
#define TEG_HH

#include <stdarg.h>
#include <stdio.h>

#include "teg.h"

namespace teg {
  /* Pass a char* in and it takes possession of it. Pass an auto_cstring or a
     const char* in and it duplicates it. */
  class auto_cstring {
    char* p;
  public:
    auto_cstring() noexcept : p(NULL) {}
    auto_cstring(char* p) noexcept : p(p) {}
    auto_cstring(const char* p) : p(NULL) {
      if(p != NULL)
        this->p = teg_strdup(p);
    }
    auto_cstring(const auto_cstring& o) : p(NULL) {
      if(o.p != NULL)
        p = teg_strdup(o.p);
    }
    auto_cstring(auto_cstring&& o) noexcept : p(NULL) {
      p = o.p;
      o.p = NULL;
    }
    ~auto_cstring() { Free(); }
    void Free() {
      if(p != NULL) safe_free(p);
      p = NULL;
    }
    auto_cstring& operator=(char* p) {
      Free();
      this->p = p;
      return *this;
    }
    auto_cstring& operator=(const char* p) {
      Free();
      if(p != NULL)
        this->p = teg_strdup(p);
      return *this;
    }
    auto_cstring& operator=(const auto_cstring& o) {
      Free();
      if(o.p != NULL)
        this->p = teg_strdup(o.p);
      return *this;
    }
    auto_cstring& operator=(auto_cstring&& o) {
      if(&o != this) {
        Free();
        p = o.p;
        o.p = NULL;
      }
      return *this;
    }
    char* operator*() const noexcept { return p; }
    operator char*() const noexcept { return p; }
    void format(const char* format, ...) {
      va_list arg;
      va_start(arg, format);
      vformat(format, arg);
      va_end(arg);
    }
    void vformat(const char* format, va_list arg) {
      char* new_p;
      asprintf(&new_p, format, arg);
      assert(new_p);
      *this = new_p;
    }
  };
}

#endif
