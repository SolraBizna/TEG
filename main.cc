#include "teg.hh"

int g_argc;
const char** g_argv;

/* this is used by io.cc, and is thanks to Windows */
#if !(__WIN32__ && _UNICODE)
typedef char TCHAR;
#endif
const TCHAR* g_argv0 = NULL;

#if __WIN32__ && _UNICODE
#include <windows.h>
/* Convert UTF-16 -> UTF-8 for the rest of the program */
extern "C" int wmain(int argc, WCHAR* w_argv[]) {
  g_argv0 = reinterpret_cast<const TCHAR*>(w_argv[0]);
  char** argv = new char*[argc];
  for(int n = 0; n < argc; ++n) {
    int len = WideCharToMultiByte(CP_UTF8, 0, w_argv[n], -1, NULL, 0, NULL, NULL);
    argv[n] = reinterpret_cast<char*>(safe_malloc(len));
    WideCharToMultiByte(CP_UTF8, 0, w_argv[n], -1, argv[n], len, NULL, NULL);
  }
  return teg_main(argc, argv);
}

/* When building with MinGW, wmain isn't called... */
extern "C" int main(int argc, char* argv[]) {
  (void)argc; (void)argv;
  int w_argc;
  LPWSTR* w_argv;
  LPWSTR command_line = GetCommandLineW();
  if(!command_line) die("No command line?!");
  w_argv = CommandLineToArgvW(command_line, &w_argc);
  return wmain(w_argc, w_argv);
}
#else
extern "C" int main(int argc, char* argv[]) {
  g_argv0 = argv[0];
  g_argc = argc;
  g_argv = const_cast<const char**>(argv);
  return teg_main(argc, argv);
}
#endif
