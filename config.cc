#include "config.hh"
#include "io.hh"

#include <iomanip>
#include <string.h>
#include <errno.h>

using namespace Config;
using namespace IO;

struct lazy_reader_param {
  std::unique_ptr<std::istream> file;
  char buffer[512];
  lazy_reader_param(std::unique_ptr<std::istream>&& file)
    : file(std::move(file)) {}
};
static const char* lazy_reader(lua_State* L, void* data, size_t* size) {
  (void)L; /* not used */
  lazy_reader_param& param = *reinterpret_cast<lazy_reader_param*>(data);
  param.file->read(param.buffer, sizeof(param.buffer));
  if(param.file->eof()) {
    *size = param.file->gcount();
    return param.buffer;
  }
  else if(param.file) {
    *size = sizeof(param.buffer);
    return param.buffer;
  }
  else {
    *size = 0;
    return nullptr;
  }
}

static int safely_read(lua_State* L) {
  const char* filename = (const char*)lua_topointer(L, 1);
  const Element* elements = (const Element*)lua_topointer(L, 2);
  size_t num_elements = lua_tonumber(L, 3);
  lua_settop(L, 0); // clear stack
  std::unique_ptr<std::istream> f = OpenConfigFileForRead(filename);
  if(!f || !*f) return 0;
  {
    struct lazy_reader_param param(std::move(f));
    switch(lua_load(L, lazy_reader, reinterpret_cast<void*>(&param), filename,
                    "t")) {
    case LUA_OK:
      /* Yay! */
      lua_call(L, 0, 0);
    default:
      /* No! */
      /* Wait, it makes no difference! */
      break;
    }
  }
  f.reset();
  for(size_t n = 0; n < num_elements; ++n) {
    lua_getglobal(L, elements[n].name);
    if(!lua_isnil(L, -1)) {
      switch(elements[n].type) {
      case String:
        {
          size_t length;
          const char* str = lua_tolstring(L, -1, &length);
          if(str != nullptr) {
            for(const char* p = str; p < str + length; ++p) {
              if(!*p) {
                fprintf(stderr, "config file %s: warning: string \"%s\" contained embedded NULs!\n", filename, elements[n].name);
                break;
              }
            }
            *(std::string*)elements[n].ptr = std::string(str, length);
          }
        } break;
      case Int32:
        {
          if(lua_isnumber(L, -1)) {
            lua_Number f = lua_tonumber(L, -1);
            if(f < -2147483648.0 || f > 2147483647 || floor(f) != f)
              fprintf(stderr, "config file %s: int32 \"%s\" out of range\n",
                      filename, elements[n].name);
            else
              *((int32_t*)elements[n].ptr) = (int32_t)f;
          } else fprintf(stderr, "config file %s: \"%s\" wrong type for int32\n",
                         filename, elements[n].name);
        } break;
      case Unsigned_Int32:
        {
          if(lua_isnumber(L, -1)) {
            lua_Number f = lua_tonumber(L, -1);
            if(f < 0 || f > 4294967295 || floor(f) != f)
              fprintf(stderr, "config file %s: uint32 \"%s\" out of range\n",
                      filename, elements[n].name);
            else
              *((uint32_t*)elements[n].ptr) = (uint32_t)f;
          } else fprintf(stderr, "config file %s: \"%s\" wrong type for uint32\n",
                         filename, elements[n].name);
        } break;
      case Float:
        {
          if(lua_isnumber(L, -1)) {
            lua_Number f = lua_tonumber(L, -1);
            *((float*)elements[n].ptr) = (float)f;
          } else fprintf(stderr, "config file %s: \"%s\" wrong type for float\n",
                         filename, elements[n].name);
        } break;
      case Double:
        {
          if(lua_isnumber(L, -1)) {
            lua_Number f = lua_tonumber(L, -1);
            *((double*)elements[n].ptr) = (double)f;
          } else fprintf(stderr, "config file %s: \"%s\" wrong type for double\n",
                         filename, elements[n].name);
        } break;
      case Bool:
        {
          if(lua_isboolean(L, -1))
            *((bool*)elements[n].ptr) = !!lua_toboolean(L, -1);
          else fprintf(stderr, "config file %s: \"%s\" wrong type for bool\n",
                       filename, elements[n].name);
        } break;
      default:
        /* NOTREACHED */
        die("Corrupted Config::Element passed to Config::Read");
      }
    }
    lua_pop(L, 1);
  }
  return 0;
}

void Config::Read(const char* filename,
                  const Element* elements, size_t num_elements) {
  lua_State* L = luaL_newstate();
  lua_pushcfunction(L, safely_read);
  lua_pushlightuserdata(L, (void*)filename);
  lua_pushlightuserdata(L, (void*)elements);
  lua_pushnumber(L, num_elements);
  int status = lua_pcall(L, 3, 0, 0);
  if(status == 0) {
    /* success! */
  }
  else {
    /* failure! */
    fprintf(stderr, "While processing config file: %s\n", filename);
    switch(status) {
    case LUA_ERRRUN: fprintf(stderr, "Lua runtime error\n"); break;
    case LUA_ERRMEM: fprintf(stderr, "Memory allocation error\n"); break;
    case LUA_ERRERR: fprintf(stderr, "Error inside message handler\n"); break;
    case LUA_ERRGCMM: fprintf(stderr, "Error inside __gc metamethod\n"); break;
    default: fprintf(stderr, "Unknown error code %i\n", status); break;
    }
    if(lua_gettop(L) > 0) {
      const char* err_str = lua_tostring(L, -1);
      if(err_str != nullptr) fprintf(stderr, "%s\n", err_str);
    }
  }
  lua_close(L);
}

void Config::Write(const char* filename,
                   const Element* elements, size_t num_elements) {
  std::unique_ptr<std::ostream> f = OpenConfigFileForWrite(filename);
  if(!f || !*f) return;
  for(size_t n = 0; n < num_elements; ++n) {
    const Element& element = elements[n];
    *f << element.name << " = ";
    switch(element.type) {
    case String:
      *f << '"';
      {
        const char* p = (const char*)element.ptr;
        while(*p) {
          if(*p >= ' ' && *p <= '~') {
            /* ASCII! */
            if(*p == '\\' || *p == '"') *f << '\\';
            *f << *p++;
          }
          else {
            *f << "\\" << std::setw(3) << std::setfill('0') << *p++;
          }
        }
      }
      *f << "\"\n";
      break;
    case Int32:
      *f << *((int32_t*)element.ptr) << '\n';
      break;
    case Unsigned_Int32:
      *f << *((uint32_t*)element.ptr) << '\n';
      break;
    case Float:
      *f << *((float*)element.ptr) << '\n';
      break;
    case Double:
      *f << *((double*)element.ptr) << '\n';
      break;
    case Bool:
      *f << (*((bool*)element.ptr) ? "true\n" : "false\n");
      break;
    default:
      die("Corrupted Config::Element passed to Config::Read");
    }
    if(!*f) {
      fprintf(stderr, "Couldn't write to config file %s.\n", filename);
      return;
    }
  }
  UpdateConfigFile(filename);
}
