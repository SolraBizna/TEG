#ifndef XGLHH
#define XGLHH

#include "teg.hh"

#ifdef WE_ARE_XGL
#define EXTERN 
#else
#define EXTERN extern
#endif

namespace xgl {
  void Initialize();
  /* ARB_vertex_buffer_object */
  EXTERN bool have_ARB_vertex_buffer_object;
  EXTERN void(*BindBuffer)(GLenum target, GLuint buffer);
  EXTERN void(*DeleteBuffers)(GLsizei n, const GLuint* buffers);
  EXTERN void(*GenBuffers)(GLsizei n, GLuint* buffers);
  EXTERN GLboolean(*IsBuffer)(GLuint buffer);
  EXTERN void(*BufferData)(GLenum target, GLsizei* size,
                           const void* data, GLenum usage);
  EXTERN void(*BufferSubData)(GLenum target, GLint* offset, GLsizei* size,
                              const void* data);
  EXTERN void(*GetBufferSubData)(GLenum target, GLint* offset, GLsizei* size,
                                 void* data);
  EXTERN void*(*MapBuffer)(GLenum target, GLenum access);
  EXTERN GLboolean(*UnmapBuffer)(GLenum target);
  EXTERN void(*GetBufferParameteriv)(GLenum target, GLenum pname, GLint* v);
  EXTERN void(*GetBufferPointerv)(GLenum target, GLenum pname, void** v);
  /* EXT_texture_rectangle */
  EXTERN bool have_EXT_texture_rectangle;
};

#undef EXTERN

#endif
