/* All C++ headers or source files using TEG should include this file. C files
   should include "teg.h" instead. */
#ifndef TEG_HH
#define TEG_HH

#include <stdarg.h>
#include <stdio.h>
#include <string>

#include "teg.h"

namespace TEG {
  std::string format(const char* format, ...);
  std::string format(const std::string& format, ...);
  std::string vformat(const char* format, va_list arg);
  std::string vformat(const std::string& format, va_list arg);
}

#endif
