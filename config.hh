#ifndef CONFIG_HH
#define CONFIG_HH

#include "teg.hh"

namespace Config {
  enum Type {
    String, Int32, Unsigned_Int32, Float, Double, Bool
  };
  struct Element {
  private:
    Element() {}
  public:
    /* name ought to be a valid Lua identifier */
    inline Element(const char* name, std::string& ref)
      : name(name), type(String), ptr((void*)&ref) {}
    inline Element(const char* name, int32_t& ref)
      : name(name), type(Int32), ptr((void*)&ref) {}
    inline Element(const char* name, uint32_t& ref)
      : name(name), type(Unsigned_Int32), ptr((void*)&ref) {}
    inline Element(const char* name, float& ref)
      : name(name), type(Float), ptr((void*)&ref) {}
    inline Element(const char* name, double& ref)
      : name(name), type(Double), ptr((void*)&ref) {}
    inline Element(const char* name, bool& ref)
      : name(name), type(Bool), ptr((void*)&ref) {}
    const char* name;
    Type type;
    void* ptr;
  };
  extern void Read(const char* filename,
                   const Element* elements, size_t num_elements);
  extern void Write(const char* filename,
                    const Element* elements, size_t num_elements);
}

#endif
