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

void xgl::Initialize() {
  unsigned int core_major_version, core_minor_version, core_patch_version;
  const char* version = getenv("GL_FAKE_VERSION");
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
  const char* blacklist_list = getenv("GL_DISABLED_EXTENSIONS");
  if(blacklist_list != NULL)
    chunkify_extension_list(blacklisted_extension_list, blacklist_list);
  for(auto ext : blacklisted_extension_list) {
    if(chunky_extension_list.count(ext)) {
      fprintf(stderr, "xgl: Excommunicating ");
      output_token(stderr, ext);
      fprintf(stderr, " (due to GL_DISABLED_EXTENSIONS)\n");
      chunky_extension_list.erase(ext);
    }
    else {
      fprintf(stderr, "xgl: Extension ");
      output_token(stderr, ext);
      fprintf(stderr, " present in GL_DISABLED_EXTENSIONS,"
              " but absent from GL_EXTENSIONS\n");
    }
  }
  /* ARB_vertex_buffer_object */
  if(MIN_VERSION(1, 5)
     || chunky_extension_list.count("GL_ARB_vertex_buffer_object")) {
    if(blacklisted_extension_list.count("GL_ARB_vertex_buffer_object"))
      fprintf(stderr, "xgl: GL_ARB_vertex_buffer_object was in GL_DISABLED_EXTENSIONS, but is a core feature in the current OpenGL version. In order to disable vertex buffer objects, you must also set GL_FAKE_VERSION to 1.4 or earlier.\n");
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
    fprintf(stderr, "xgl: Missing optional GL extension"
            " ARB_vertex_buffer_object\n");
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
}
