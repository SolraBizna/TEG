#define WE_ARE_XGL 1
#include "xgl.hh"

#include <unordered_set>

static struct extension_element {
  unsigned int min_major_version, min_minor_version, min_patch_version;
  const char* name;
  const char** aliases;
  bool& presence_flag, **deps;
  const struct proc_element {
    const char** names;
    void** addr;
  } *procs;
} extensions[] = {
#define BEGIN_PROCS (const extension_element::proc_element[]){
#define ARB_PROC(name) {(const char*[]){"gl" #name "ARB", "gl" #name, NULL}, (void**)&xgl::name},
#define END_PROCS {NULL, NULL}}
#define NO_PROCS BEGIN_PROCS END_PROCS
  {
    2, 1, 0,
    "GL_ARB_texture_rectangle", (const char*[]){"GL_EXT_texture_rectangle","GL_NV_texture_rectangle",NULL},
    xgl::have_ARB_texture_rectangle, NULL,
    NO_PROCS
  },
  {
    1, 5, 0,
    "GL_ARB_vertex_buffer_object", NULL,
    xgl::have_ARB_vertex_buffer_object, NULL,
    BEGIN_PROCS
    ARB_PROC(BindBuffer)
    ARB_PROC(DeleteBuffers)
    ARB_PROC(GenBuffers)
    ARB_PROC(IsBuffer)
    ARB_PROC(BufferData)
    ARB_PROC(BufferSubData)
    ARB_PROC(GetBufferSubData)
    ARB_PROC(MapBuffer)
    ARB_PROC(UnmapBuffer)
    ARB_PROC(GetBufferParameteriv)
    ARB_PROC(GetBufferPointerv)
    END_PROCS
  },
  {
    2, 0, 0,
    "GL_ARB_shader_objects", NULL,
    xgl::have_ARB_shader_objects, NULL,
    BEGIN_PROCS
    ARB_PROC(DeleteObject)
    ARB_PROC(GetHandle)
    ARB_PROC(DetachObject)
    ARB_PROC(CreateShaderObject)
    ARB_PROC(ShaderSource)
    ARB_PROC(CompileShader)
    ARB_PROC(CreateProgramObject)
    ARB_PROC(AttachObject)
    ARB_PROC(LinkProgram)
    ARB_PROC(UseProgramObject)
    ARB_PROC(ValidateProgram)
    ARB_PROC(Uniform1f)
    ARB_PROC(Uniform2f)
    ARB_PROC(Uniform3f)
    ARB_PROC(Uniform4f)
    ARB_PROC(Uniform1i)
    ARB_PROC(Uniform2i)
    ARB_PROC(Uniform3i)
    ARB_PROC(Uniform4i)
    ARB_PROC(Uniform1fv)
    ARB_PROC(Uniform2fv)
    ARB_PROC(Uniform3fv)
    ARB_PROC(Uniform4fv)
    ARB_PROC(Uniform1iv)
    ARB_PROC(Uniform2iv)
    ARB_PROC(Uniform3iv)
    ARB_PROC(Uniform4iv)
    ARB_PROC(UniformMatrix2fv)
    ARB_PROC(UniformMatrix3fv)
    ARB_PROC(UniformMatrix4fv)
    ARB_PROC(GetObjectParameterfv)
    ARB_PROC(GetObjectParameteriv)
    ARB_PROC(GetInfoLog)
    ARB_PROC(GetAttachedObjects)
    ARB_PROC(GetUniformLocation)
    ARB_PROC(GetActiveUniform)
    ARB_PROC(GetUniformfv)
    ARB_PROC(GetUniformiv)
    ARB_PROC(GetShaderSource)
    END_PROCS
  },
  {
    2, 0, 0,
    "GL_ARB_vertex_shader", NULL,
    xgl::have_ARB_vertex_shader, (bool*[]){&xgl::have_ARB_shader_objects, NULL},
    BEGIN_PROCS
    ARB_PROC(VertexAttrib1f)
    ARB_PROC(VertexAttrib1s)
    ARB_PROC(VertexAttrib1d)
    ARB_PROC(VertexAttrib2f)
    ARB_PROC(VertexAttrib2s)
    ARB_PROC(VertexAttrib2d)
    ARB_PROC(VertexAttrib3f)
    ARB_PROC(VertexAttrib3s)
    ARB_PROC(VertexAttrib3d)
    ARB_PROC(VertexAttrib4f)
    ARB_PROC(VertexAttrib4s)
    ARB_PROC(VertexAttrib4d)
    ARB_PROC(VertexAttrib4Nub)
    ARB_PROC(VertexAttrib1fv)
    ARB_PROC(VertexAttrib1sv)
    ARB_PROC(VertexAttrib1dv)
    ARB_PROC(VertexAttrib2fv)
    ARB_PROC(VertexAttrib2sv)
    ARB_PROC(VertexAttrib2dv)
    ARB_PROC(VertexAttrib3fv)
    ARB_PROC(VertexAttrib3sv)
    ARB_PROC(VertexAttrib3dv)
    ARB_PROC(VertexAttrib4fv)
    ARB_PROC(VertexAttrib4sv)
    ARB_PROC(VertexAttrib4dv)
    ARB_PROC(VertexAttrib4iv)
    ARB_PROC(VertexAttrib4bv)
    ARB_PROC(VertexAttrib4ubv)
    ARB_PROC(VertexAttrib4usv)
    ARB_PROC(VertexAttrib4uiv)
    ARB_PROC(VertexAttrib4Nbv)
    ARB_PROC(VertexAttrib4Nsv)
    ARB_PROC(VertexAttrib4Niv)
    ARB_PROC(VertexAttrib4Nubv)
    ARB_PROC(VertexAttrib4Nusv)
    ARB_PROC(VertexAttrib4Nuiv)
    ARB_PROC(VertexAttribPointer)
    ARB_PROC(EnableVertexAttribArray)
    ARB_PROC(DisableVertexAttribArray)
    ARB_PROC(BindAttribLocation)
    ARB_PROC(GetActiveAttrib)
    ARB_PROC(GetAttribLocation)
    ARB_PROC(GetVertexAttribdv)
    ARB_PROC(GetVertexAttribfv)
    ARB_PROC(GetVertexAttribiv)
    ARB_PROC(GetVertexAttribPointerv)
    END_PROCS
  },
  {
    2, 0, 0,
    "GL_ARB_fragment_shader", NULL,
    xgl::have_ARB_fragment_shader, (bool*[]){&xgl::have_ARB_shader_objects, NULL},
    NO_PROCS
  },
  {
    2, 0, 0,
    "GL_ARB_draw_buffers", NULL,
    xgl::have_ARB_draw_buffers, NULL,
    BEGIN_PROCS
    ARB_PROC(DrawBuffers)
    END_PROCS
  },
  {
    3, 0, 0,
    "GL_ARB_framebuffer_object", NULL,
    xgl::have_ARB_framebuffer_object, NULL,
    BEGIN_PROCS
    ARB_PROC(IsRenderbuffer)
    ARB_PROC(BindRenderbuffer)
    ARB_PROC(DeleteRenderbuffers)
    ARB_PROC(GenRenderbuffers)
    ARB_PROC(RenderbufferStorage)
    ARB_PROC(RenderbufferStorageMultisample)
    ARB_PROC(GetRenderbufferParameteriv)
    ARB_PROC(IsFramebuffer)
    ARB_PROC(BindFramebuffer)
    ARB_PROC(DeleteFramebuffers)
    ARB_PROC(GenFramebuffers)
    ARB_PROC(CheckFramebufferStatus)
    ARB_PROC(FramebufferTexture1D)
    ARB_PROC(FramebufferTexture2D)
    ARB_PROC(FramebufferTexture3D)
    ARB_PROC(FramebufferTextureLayer)
    ARB_PROC(FramebufferRenderbuffer)
    ARB_PROC(GetFramebufferAttachmentParameteriv)
    ARB_PROC(BlitFramebuffer)
    ARB_PROC(GenerateMipmap)
    END_PROCS
  },
};

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

static bool set_proc_address(const extension_element& ext,
                             const extension_element::proc_element& proc) {
  *proc.addr = NULL;
  const char** p = proc.names;
  while(*p) {
    void* addr = SDL_GL_GetProcAddress(*p);
    if(addr != NULL) {
      *proc.addr = addr;
      return true;
    }
  }
  fprintf(stderr, "xgl: Unable to find any implementation of %s\n",
          ext.name);
  return false;
}

static void extension_stub() {
  die("Called an OpenGL function that was part of an unsupported or disabled extension.");
}

static void blacklisted_core_extension_warning(const char* ext,
                                               int major, int minor, int patch) {
  fprintf(stderr, "xgl: %s was in XGL_DISABLED_EXTENSIONS, but is a core feature in the current OpenGL version. In order to disable this feature, you must also set XGL_FAKE_VERSION to a version earlier than %i.%i.%i.\n", ext, major, minor, patch);
}

static void blacklisted_aliased_extension_warning(const char* ext,
                                                  const char* alt) {
  fprintf(stderr, "xgl: %s was in XGL_DISABLED_EXTENSIONS, but the presence of %s in XGL_DISABLED_EXTENSIONS also blacklists other, similar extensions. %s has no effect.\n", ext, alt, ext);
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
#define MIN_FULL_VERSION(major,minor,patch) \
  (core_major_version > (major) \
   || (core_major_version == (major) \
       && (core_minor_version > (minor) \
           || (core_minor_version == (minor) && core_patch_version >= (patch)))))
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
  for(auto ext : extensions) {
    if(ext.aliases != NULL) {
      for(auto p = ext.aliases; *p; ++p) {
        if(chunky_extension_list.count(*p))
          chunky_extension_list.insert(ext.name);
      }
    }
  }
  const char* blacklist_list = getenv("XGL_DISABLED_EXTENSIONS");
  if(blacklist_list != NULL)
    chunkify_extension_list(blacklisted_extension_list, blacklist_list);
  for(auto ext : extensions) {
    if(ext.aliases != NULL) {
      for(auto p = ext.aliases; *p; ++p) {
        if(blacklisted_extension_list.count(*p))
          blacklisted_aliased_extension_warning(*p, ext.name);
      }
    }
  }
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
  for(auto ext : extensions) {
    ext.presence_flag = false;
    if(ext.deps != NULL) {
      for(bool** p = ext.deps; *p != NULL; ++p) {
        if(!**p) {
          dprintf("Extension %s is missing a dependency.\n", ext.name);
          goto extension_is_dead;
        }
      }
    }
    if(MIN_FULL_VERSION(ext.min_major_version,
                        ext.min_minor_version,
                        ext.min_patch_version)
       || chunky_extension_list.count(ext.name)) {
      if(blacklisted_extension_list.count(ext.name))
        blacklisted_core_extension_warning(ext.name,
                                           ext.min_major_version,
                                           ext.min_minor_version,
                                           ext.min_patch_version);
      ext.presence_flag = true;
      for(auto proc = ext.procs; proc->names != NULL; ++proc) {
        if(!set_proc_address(ext, *proc)) {
          ext.presence_flag = false;
          goto extension_is_dead;
        }
      }
      dprintf("Found procs for %s.\n", ext.name);
    }
    extension_is_dead:
    if(!ext.presence_flag) {
      dprintf("%s is not present.\n", ext.name);
      for(auto proc = ext.procs; proc->names != NULL; ++proc) {
        *proc->addr = (void*)extension_stub;
      }
    }
  }
}
