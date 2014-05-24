#define WE_ARE_XGL 1
#include "xgl.hh"

#include <unordered_set>

static size_t bad_fast_token_hash(const char* src) {
  size_t result = 0;
  while(*src && *src != ' ')
    result = ((result << 25) | (result >> (8*sizeof(size_t)-25))) ^ *src++;
  return result;
}

static bool tokens_are_equivalent(const char* a, const char* b) {
  /* Ugh, did I write this? */
  while(*a == *b) {
    if(*a == ' ' || *a == 0) return true;
    ++a; ++b;
  }
  return (*a == ' ' || *a == 0) && (*b == ' ' || *b == 0);
}

static void output_token(FILE* f, const char* p) {
  while(*p && *p != ' ') fputc(*p++, f);
}

static void chunkify_extension_list(std::unordered_set<const char*,
                                    typeof(&bad_fast_token_hash),
                                    typeof(&tokens_are_equivalent)>&
                                    chunky_extension_list,
                                    const char* extension_list) {
  const char* p = extension_list;
  while(*p) {
    chunky_extension_list.insert(p);
    while(*p && *p != ' ')
      ++p;
    while(*p == ' ')
      ++p;
  }
}

static void* get_proc_address(const char* canon_name, ...) {
  va_list arg;
  va_start(arg, canon_name);
  void* ret = NULL;
  const char* next = canon_name;
  do {
    if(!ret) ret = SDL_GL_GetProcAddress(next);
    next = va_arg(arg, const char*);
  } while(next);
  va_end(arg);
  if(!ret)
    fprintf(stderr, "xgl: Unable to find any implementation of %s\n",
            canon_name);
  return ret;
}

static void extension_stub() {
  die("Called an OpenGL function that was part of an unsupported or disabled extension.");
}

#define stub_proc_address(out) \
xgl::out = reinterpret_cast<typeof(xgl::out)>(extension_stub)

#define set_proc_address(wat, out, ...) \
xgl::out = reinterpret_cast<typeof(xgl::out)> \
  (get_proc_address(__VA_ARGS__, NULL));      \
if(xgl::out == NULL) goto wat
#define set_proc_address_ARB(wat, out) \
set_proc_address(wat, out, "gl" #out, "gl" #out "ARB")

static void blacklisted_core_extension_warning(const char* ext,
                                               int major, int minor) {
  fprintf(stderr, "xgl: %s was in XGL_DISABLED_EXTENSIONS, but is a core feature in the current OpenGL version. In order to disable this feature, you must also set XGL_FAKE_VERSION to a version earlier than %i.%i.\n", ext, major, minor);
}

static void blacklisted_aliased_extension_warning(const char* ext,
                                                  const char* alt) {
  fprintf(stderr, "xgl: %s was in XGL_DISABLED_EXTENSIONS, but the presence of %s in XGL_DISABLED_EXTENSIONS is enough to blacklist all related extensions. %s has no effect.\n", ext, alt, ext);
}

void xgl::Initialize() {
  unsigned int core_major_version, core_minor_version, core_patch_version;
  const char* version = getenv("XGL_FAKE_VERSION");
  if(version == NULL)
    version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  if(version == NULL) {
    fprintf(stderr, "xgl: Could not retrieve version number\n");
    die("Your OpenGL implementation does not know what version it is."
        " We give up.");
  }
  if(sscanf(version, "%u.%u.%u", &core_major_version, &core_minor_version,
            &core_patch_version)!=3) {
    core_patch_version = 0;
    if(sscanf(version, "%u.%u", &core_major_version, &core_minor_version)!=2) {
      die("Unable to determine your OpenGL version.");
    }
  }
#define MIN_MAJOR_VERSION(major) \
  (core_major_version > (major))
#define MIN_VERSION(major,minor) \
  (core_major_version > (major) \
   || (core_major_version == (major) && core_minor_version >= (minor)))
#define MIN_VERSION_2(major,minor,patch) \
  (core_major_version > (major) \
   || (core_major_version == (major) \
       && (core_minor_version > (minor) \
           || (core_minor_version == (minor) && core_patch_version >= (patch))
  if(!MIN_VERSION(1, 2)) {
    die("We require OpenGL 1.2 or later. Your OpenGL appears to be version"
        " %u.%u.%u. We are frankly amazed that you managed to obtain such an"
        " ancient OpenGL version. We might be willing to try to make this"
        " work on your OpenGL version if you get in touch with us.",
        core_major_version, core_minor_version, core_patch_version);
  }
  std::unordered_set<const char*, typeof(&bad_fast_token_hash),
                     typeof(&tokens_are_equivalent)>
    chunky_extension_list(20, bad_fast_token_hash, tokens_are_equivalent),
    blacklisted_extension_list(5, bad_fast_token_hash, tokens_are_equivalent);
  const char* extension_list
    = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
  if(extension_list == NULL) {
    fprintf(stderr, "xgl: Could not retrieve extensions list\n");
    extension_list = "";
  }
  chunkify_extension_list(chunky_extension_list, extension_list);
  if(chunky_extension_list.count("GL_EXT_texture_rectangle"))
    chunky_extension_list.insert("GL_ARB_texture_rectangle");
  if(chunky_extension_list.count("GL_NV_texture_rectangle"))
    chunky_extension_list.insert("GL_ARB_texture_rectangle");
  const char* blacklist_list = getenv("XGL_DISABLED_EXTENSIONS");
  if(blacklist_list != NULL)
    chunkify_extension_list(blacklisted_extension_list, blacklist_list);
  if(blacklisted_extension_list.count("GL_ARB_texture_rectangle"))
    blacklisted_aliased_extension_warning("GL_EXT_texture_rectangle",
                                          "GL_ARB_texture_rectangle");
  if(blacklisted_extension_list.count("GL_NV_texture_rectangle"))
    blacklisted_aliased_extension_warning("GL_NV_texture_rectangle",
                                          "GL_ARB_texture_rectangle");
  for(auto ext : blacklisted_extension_list) {
    if(chunky_extension_list.count(ext)) {
      fprintf(stderr, "xgl: Excommunicating ");
      output_token(stderr, ext);
      fprintf(stderr, " (due to XGL_DISABLED_EXTENSIONS)\n");
      chunky_extension_list.erase(ext);
    }
    else {
      fprintf(stderr, "xgl: Extension ");
      output_token(stderr, ext);
      fprintf(stderr, " present in XGL_DISABLED_EXTENSIONS,"
              " but absent from GL_EXTENSIONS\n");
    }
  }
  /* ARB_vertex_buffer_object */
  if(MIN_VERSION(1, 5)
     || chunky_extension_list.count("GL_ARB_vertex_buffer_object")) {
    if(blacklisted_extension_list.count("GL_ARB_vertex_buffer_object"))
      blacklisted_core_extension_warning("GL_ARB_vertex_buffer_object",1,5);
    have_ARB_vertex_buffer_object = true;
    set_proc_address_ARB(no_ARB_vertex_buffer_object, BindBuffer);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, DeleteBuffers);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, GenBuffers);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, IsBuffer);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, BufferData);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, BufferSubData);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, GetBufferSubData);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, MapBuffer);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, UnmapBuffer);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, GetBufferParameteriv);
    set_proc_address_ARB(no_ARB_vertex_buffer_object, GetBufferPointerv);
  }
  else {
  no_ARB_vertex_buffer_object:
    have_ARB_vertex_buffer_object = false;
  }
  if(!have_ARB_vertex_buffer_object) {
    stub_proc_address(BindBuffer);
    stub_proc_address(DeleteBuffers);
    stub_proc_address(GenBuffers);
    stub_proc_address(IsBuffer);
    stub_proc_address(BufferData);
    stub_proc_address(BufferSubData);
    stub_proc_address(GetBufferSubData);
    stub_proc_address(MapBuffer);
    stub_proc_address(UnmapBuffer);
    stub_proc_address(GetBufferParameteriv);
    stub_proc_address(GetBufferPointerv);
  }
  /* ARB_texture_rectangle */
  if(MIN_VERSION(2, 1)
     || chunky_extension_list.count("GL_ARB_texture_rectangle")) {
    if(blacklisted_extension_list.count("GL_ARB_texture_rectangle"))
      blacklisted_core_extension_warning("GL_ARB_texture_rectangle",2,1);
    have_ARB_texture_rectangle = true;
  } else have_ARB_texture_rectangle = false;
  /* ARB_shader_objects */
  if(MIN_MAJOR_VERSION(2)
     || chunky_extension_list.count("GL_ARB_shader_objects")) {
    if(blacklisted_extension_list.count("GL_ARB_shader_objects"))
      blacklisted_core_extension_warning("GL_ARB_shader_objects",2,0);
    have_ARB_shader_objects = true;
    set_proc_address_ARB(no_ARB_shader_objects, DeleteObject);
    set_proc_address_ARB(no_ARB_shader_objects, GetHandle);
    set_proc_address_ARB(no_ARB_shader_objects, DetachObject);
    set_proc_address_ARB(no_ARB_shader_objects, CreateShaderObject);
    set_proc_address_ARB(no_ARB_shader_objects, ShaderSource);
    set_proc_address_ARB(no_ARB_shader_objects, CompileShader);
    set_proc_address_ARB(no_ARB_shader_objects, CreateProgramObject);
    set_proc_address_ARB(no_ARB_shader_objects, AttachObject);
    set_proc_address_ARB(no_ARB_shader_objects, LinkProgram);
    set_proc_address_ARB(no_ARB_shader_objects, UseProgramObject);
    set_proc_address_ARB(no_ARB_shader_objects, ValidateProgram);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform1f);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform2f);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform3f);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform4f);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform1i);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform2i);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform3i);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform4i);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform1fv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform2fv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform3fv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform4fv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform1iv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform2iv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform3iv);
    set_proc_address_ARB(no_ARB_shader_objects, Uniform4iv);
    set_proc_address_ARB(no_ARB_shader_objects, UniformMatrix2fv);
    set_proc_address_ARB(no_ARB_shader_objects, UniformMatrix3fv);
    set_proc_address_ARB(no_ARB_shader_objects, UniformMatrix4fv);
    set_proc_address_ARB(no_ARB_shader_objects, GetObjectParameterfv);
    set_proc_address_ARB(no_ARB_shader_objects, GetObjectParameteriv);
    set_proc_address_ARB(no_ARB_shader_objects, GetInfoLog);
    set_proc_address_ARB(no_ARB_shader_objects, GetAttachedObjects);
    set_proc_address_ARB(no_ARB_shader_objects, GetUniformLocation);
    set_proc_address_ARB(no_ARB_shader_objects, GetActiveUniform);
    set_proc_address_ARB(no_ARB_shader_objects, GetUniformfv);
    set_proc_address_ARB(no_ARB_shader_objects, GetUniformiv);
    set_proc_address_ARB(no_ARB_shader_objects, GetShaderSource);
    /* ARB_vertex_shader */
    if(chunky_extension_list.count("GL_ARB_vertex_shader")) {
      if(blacklisted_extension_list.count("GL_ARB_vertex_shader"))
        blacklisted_core_extension_warning("GL_ARB_vertex_shader",2,0);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib1f);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib1s);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib1d);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib2f);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib2s);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib2d);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib3f);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib3s);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib3d);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4f);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4s);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4d);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Nub);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib1fv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib1sv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib1dv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib2fv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib2sv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib2dv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib3fv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib3sv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib3dv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4fv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4sv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4dv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4iv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4bv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4ubv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4usv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4uiv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Nbv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Nsv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Niv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Nubv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Nusv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttrib4Nuiv);
      set_proc_address_ARB(no_ARB_vertex_shader, VertexAttribPointer);
      set_proc_address_ARB(no_ARB_vertex_shader, EnableVertexAttribArray);
      set_proc_address_ARB(no_ARB_vertex_shader, DisableVertexAttribArray);
      set_proc_address_ARB(no_ARB_vertex_shader, BindAttribLocation);
      set_proc_address_ARB(no_ARB_vertex_shader, GetActiveAttrib);
      set_proc_address_ARB(no_ARB_vertex_shader, GetAttribLocation);
      set_proc_address_ARB(no_ARB_vertex_shader, GetVertexAttribdv);
      set_proc_address_ARB(no_ARB_vertex_shader, GetVertexAttribfv);
      set_proc_address_ARB(no_ARB_vertex_shader, GetVertexAttribiv);
      set_proc_address_ARB(no_ARB_vertex_shader, GetVertexAttribPointerv);
    }
    else {
    no_ARB_vertex_shader:
      have_ARB_vertex_shader = false;
    }
    /* ARB_fragment_shader */
    if(chunky_extension_list.count("GL_ARB_fragment_shader")) {
      if(blacklisted_extension_list.count("GL_ARB_fragment_shader"))
        blacklisted_core_extension_warning("GL_ARB_fragment_shader",2,0);
      have_ARB_fragment_shader = true;
    } else have_ARB_fragment_shader = false;
  }
  else {
  no_ARB_shader_objects:
    have_ARB_shader_objects = false;
    have_ARB_vertex_shader = false;
    have_ARB_fragment_shader = false;
  }
  if(!have_ARB_shader_objects) {
    stub_proc_address(DeleteObject);
    stub_proc_address(GetHandle);
    stub_proc_address(DetachObject);
    stub_proc_address(CreateShaderObject);
    stub_proc_address(ShaderSource);
    stub_proc_address(CompileShader);
    stub_proc_address(CreateProgramObject);
    stub_proc_address(AttachObject);
    stub_proc_address(LinkProgram);
    stub_proc_address(UseProgramObject);
    stub_proc_address(ValidateProgram);
    stub_proc_address(Uniform1f);
    stub_proc_address(Uniform2f);
    stub_proc_address(Uniform3f);
    stub_proc_address(Uniform4f);
    stub_proc_address(Uniform1i);
    stub_proc_address(Uniform2i);
    stub_proc_address(Uniform3i);
    stub_proc_address(Uniform4i);
    stub_proc_address(Uniform1fv);
    stub_proc_address(Uniform2fv);
    stub_proc_address(Uniform3fv);
    stub_proc_address(Uniform4fv);
    stub_proc_address(Uniform1iv);
    stub_proc_address(Uniform2iv);
    stub_proc_address(Uniform3iv);
    stub_proc_address(Uniform4iv);
    stub_proc_address(UniformMatrix2fv);
    stub_proc_address(UniformMatrix3fv);
    stub_proc_address(UniformMatrix4fv);
    stub_proc_address(GetObjectParameterfv);
    stub_proc_address(GetObjectParameteriv);
    stub_proc_address(GetInfoLog);
    stub_proc_address(GetAttachedObjects);
    stub_proc_address(GetUniformLocation);
    stub_proc_address(GetActiveUniform);
    stub_proc_address(GetUniformfv);
    stub_proc_address(GetUniformiv);
    stub_proc_address(GetShaderSource);
    stub_proc_address(BindBuffer);
    stub_proc_address(DeleteBuffers);
    stub_proc_address(GenBuffers);
    stub_proc_address(IsBuffer);
    stub_proc_address(BufferData);
    stub_proc_address(BufferSubData);
    stub_proc_address(GetBufferSubData);
    stub_proc_address(MapBuffer);
    stub_proc_address(UnmapBuffer);
    stub_proc_address(GetBufferParameteriv);
    stub_proc_address(GetBufferPointerv);
  }
  if(!have_ARB_vertex_shader) {
    stub_proc_address(VertexAttrib1f);
    stub_proc_address(VertexAttrib1s);
    stub_proc_address(VertexAttrib1d);
    stub_proc_address(VertexAttrib2f);
    stub_proc_address(VertexAttrib2s);
    stub_proc_address(VertexAttrib2d);
    stub_proc_address(VertexAttrib3f);
    stub_proc_address(VertexAttrib3s);
    stub_proc_address(VertexAttrib3d);
    stub_proc_address(VertexAttrib4f);
    stub_proc_address(VertexAttrib4s);
    stub_proc_address(VertexAttrib4d);                               
    stub_proc_address(VertexAttrib4Nub);
    stub_proc_address(VertexAttrib1fv);
    stub_proc_address(VertexAttrib1sv);
    stub_proc_address(VertexAttrib1dv);
    stub_proc_address(VertexAttrib2fv);
    stub_proc_address(VertexAttrib2sv);
    stub_proc_address(VertexAttrib2dv);
    stub_proc_address(VertexAttrib3fv);
    stub_proc_address(VertexAttrib3sv);
    stub_proc_address(VertexAttrib3dv);
    stub_proc_address(VertexAttrib4fv);
    stub_proc_address(VertexAttrib4sv);
    stub_proc_address(VertexAttrib4dv);
    stub_proc_address(VertexAttrib4iv);
    stub_proc_address(VertexAttrib4bv);
    stub_proc_address(VertexAttrib4ubv);
    stub_proc_address(VertexAttrib4usv);
    stub_proc_address(VertexAttrib4uiv);
    stub_proc_address(VertexAttrib4Nbv);
    stub_proc_address(VertexAttrib4Nsv);
    stub_proc_address(VertexAttrib4Niv);
    stub_proc_address(VertexAttrib4Nubv);
    stub_proc_address(VertexAttrib4Nusv);
    stub_proc_address(VertexAttrib4Nuiv);
    stub_proc_address(VertexAttribPointer);
    stub_proc_address(EnableVertexAttribArray);
    stub_proc_address(DisableVertexAttribArray);
    stub_proc_address(BindAttribLocation);
    stub_proc_address(GetActiveAttrib);
    stub_proc_address(GetAttribLocation);
    stub_proc_address(GetVertexAttribdv);
    stub_proc_address(GetVertexAttribfv);
    stub_proc_address(GetVertexAttribiv);
    stub_proc_address(GetVertexAttribPointerv);
  }
}
