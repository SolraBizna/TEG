#include "io.hh"

#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <limits.h>
#include <fstream>
#if TEG_USE_SN && !defined(__WIN32__)
#include <dirent.h>
#endif

static std::unique_ptr<std::istream>
OpenDataFileForReadStupidWindowsHack(const std::string& path);

std::unique_ptr<std::istream>
IO::OpenDataFileForRead(const std::string& path) {
  std::string massaged_path(path);
  auto it = massaged_path.begin();
  while(it != massaged_path.end()) {
    if(*it == '.' && (it == path.begin() || it[-1] == '/'))
      die("Attempt to access an illegal datafile path: %s", path.c_str());
    else if(*DIR_SEP != '/') {
      if(*it == '/') *it = *DIR_SEP;
      else if(*it == *DIR_SEP) *it = '/';
    }
    ++it;
  }
  return OpenDataFileForReadStupidWindowsHack(path);
}

#ifdef __WIN32__
# include <windows.h>
# include <tchar.h>
# ifdef _UNICODE
#  define perror stupidperror
#  define fprintf stupidfprintf
#  include <stdarg.h>
static void stupidfprintf(FILE* f, const WCHAR* format, ...) {
  static WCHAR wide_buffer[1024];
  static char thin_buffer[1024];
  va_list arg;
  va_start(arg, format);
  _vsnwprintf(wide_buffer, sizeof(wide_buffer)/sizeof(WCHAR), format, arg);
  va_end(arg);
  WideCharToMultiByte(CP_UTF8, 0, wide_buffer, -1, thin_buffer,
                      sizeof(thin_buffer), NULL, NULL);
  fwrite(thin_buffer, strlen(thin_buffer), 1, f);
}
static void stupidperror(const WCHAR* wat) {
  stupidfprintf(stderr, L"%S: %s\n", wat, strerror(errno));
}
# endif
# define strlen _tcslen
# define strcpy _tcscpy
# define strchr _tcschr
# define strrchr _tcsrchr
# define snprintf _sntprintf
# define getenv _tgetenv
# define getcwd _tgetcwd
# define fopen _tfopen
# define freopen _tfreopen
# define rename _trename
# define remove _tremove
#else
typedef char TCHAR; //I love Windows!
# define _T(a) a
#endif

#ifdef __WIN32__
# define CONFIG_BASE_ENV "USERPROFILE"
# define CONFIG_BASE_ENV_DEFAULT "C:\\Documents and Settings\\User"
# define CONFIG_BASE_DIR "My Documents\\My Games\\" GAME_PRETTY_NAME
#else
# define CONFIG_BASE_ENV "HOME"
# ifdef MACOSX
#  define CONFIG_BASE_ENV_DEFAULT "/Users/Shared"
#  define CONFIG_BASE_DIR "Library/Preferences/" GAME_PRETTY_NAME
# else
#  define CONFIG_BASE_ENV_DEFAULT "/home"
#  define CONFIG_BASE_DIR ".local/share/" GAME_PRETTY_NAME
# endif
#endif
#define CONFIG_EXT ""

#define DATA_BASE_DIR "Data"
#define LANG_BASE_DIR "Lang"

extern "C" const TCHAR* g_argv0;

#if MACOSX
static TCHAR* ExecutablePathToContentsPath(TCHAR* in_path) {
  if(in_path && strlen(in_path) >= 6) {
    if(!strcmp(in_path + strlen(in_path) - strlen("/MacOS"), "/MacOS")) {
      in_path[strlen(in_path) - strlen("/MacOS")] = 0;
      in_path = (TCHAR*)safe_realloc(in_path, strlen(in_path));
    }
  }
  return in_path;
}
#else
#define ExecutablePathToContentsPath(x) x
#endif

static const TCHAR* GetSelfPath() {
  static TCHAR* self_path = NULL;
  if(self_path == NULL) {
#ifdef __WIN32__
    /* Windows */
	self_path = (TCHAR*)safe_malloc(sizeof(TCHAR) * 1024);
	DWORD r = GetModuleFileName(NULL, self_path, 1024);
	if(r == 0)
	  /* Is this the best error message? */
	  fprintf(stderr, _T("GetModuleFileName() failed, error code %i (We're going to need a bigger buffer?"), GetLastError());
	else if(r > 1) {
	  TCHAR* p = strrchr(self_path, *_T(DIR_SEP));
	  if(p) {
	    *p = 0;
      self_path = (TCHAR*)safe_realloc(self_path, sizeof(TCHAR) * (strlen(self_path)+1));
      self_path = ExecutablePathToContentsPath(self_path);
      return self_path;
    }
	}
	/* let the more portable code handle it */
	if(self_path != NULL) {
	  safe_free(self_path);
	  self_path = NULL;
	}
	/*
    DWORD req_buf = GetCurrentDirectory(0, NULL);
    self_path = (TCHAR*)safe_malloc(sizeof(TCHAR) * req_buf);
    DWORD dir_return = GetCurrentDirectory(req_buf, self_path);
    if(dir_return == 0) {
      fprintf(stderr, _T("GetCurrentDirectory failed, error code %i"), GetLastError());
    } else if(dir_return > req_buf) {
      fprintf(stderr, _T("Buffer not large enough to store directory path!\n")); // need win equiv of PROC_SELF_EXE
    } else {
      return self_path;
    }
	
    // copying verbatim from below
    if(self_path != NULL) {
      safe_free(self_path);
      self_path = NULL;
    }
	*/
#else
    /* UNIX-like */
#if MACOSX || EMSCRIPTEN
#define DISABLE_PROC_SELF_EXE 1
#endif
# ifndef DISABLE_PROC_SELF_EXE
    /* first, try /proc/self/exe */
#  define PROC_SELF_EXE DIR_SEP "proc" DIR_SEP "self" DIR_SEP "exe"
    self_path = (TCHAR*)safe_malloc(sizeof(TCHAR) * (PATH_MAX+1));
    ssize_t r = readlink(PROC_SELF_EXE, self_path, PATH_MAX);
    if(r < 0)
      perror(_T(PROC_SELF_EXE));
    else {
      self_path[r] = 0;
      TCHAR* p = strrchr(self_path, *DIR_SEP);
      if(p) {
        *p = 0;
        if(*self_path == 0) strcpy(self_path, DIR_SEP);
        self_path = (TCHAR*)safe_realloc(self_path, strlen(self_path)+1);
	self_path = ExecutablePathToContentsPath(self_path);
        return self_path;
      }
    }
    /* let the more portable code handle it */
    if(self_path != NULL) {
      safe_free(self_path);
      self_path = NULL;
    }
# endif
#endif
#ifndef DISABLE_ARGV0_PATH_EXTRACTION
    if(!g_argv0)
      die("g_argv0 was not set!");
    else if(g_argv0) {
      if(g_argv0[0] == *DIR_SEP) {
        /* Absolute path. */
        self_path = (TCHAR*)safe_malloc(sizeof(TCHAR) * (strlen(g_argv0)+1));
        strcpy(self_path, g_argv0);
        TCHAR* p = strrchr(self_path, *DIR_SEP);
        if(p) {
          *p = 0;
          if(*self_path == 0) strcpy(self_path, _T("/"));
          self_path = (TCHAR*)safe_realloc(self_path, strlen(self_path)+1);
	  self_path = ExecutablePathToContentsPath(self_path);
	  return self_path;
        }
      }
      else if(strchr(g_argv0, *DIR_SEP)) {
        /* Relative path. */
        size_t path_len = 32;
        while(1) {
          self_path = (TCHAR*)safe_realloc(self_path, path_len);
          if(getcwd(self_path, path_len) != NULL) break;
          path_len *= 2;
          if(path_len > 65536) die("Absurdly long working directory.");
        }
        path_len = strlen(self_path);
        self_path = (TCHAR*)safe_realloc(self_path, path_len + strlen(g_argv0) + 2);
        self_path[path_len] = '/';
        strcpy(self_path + path_len + 1, g_argv0);
        TCHAR* p = strrchr(self_path, *DIR_SEP);
        if(p) {
          *p = 0;
          if(*self_path == 0) strcpy(self_path, _T("/"));
          self_path = (TCHAR*)safe_realloc(self_path, strlen(self_path)+1);
	  self_path = ExecutablePathToContentsPath(self_path);
	  return self_path;
        }
      }
      else {
        /* Path in $PATH. */
        /* We're not smart, let the user help. Fall through... */
      }
    }
    /* give up */
    if(self_path != NULL) {
      safe_free(self_path);
      self_path = NULL;
    }
#endif
#if EMSCRIPTEN
    self_path = const_cast<TCHAR*>(_T("."));
#endif
    if(self_path == NULL) {
      fprintf(stderr, _T("Unable to find the path to ourselves, using the working directory!\n"));
      // won't actually be modified, so discarding const is okay
      self_path = const_cast<TCHAR*>(_T("."));
    }
  }
  return self_path;
}

static void clean_dirseps(TCHAR* p) {
  TCHAR* q = p;
  while(*p == *DIR_SEP) {
    /* account for UNC paths */
    *q++ = *p++;
  }
  while(*p) {
    if(*p == *DIR_SEP) {
      *q++ = *DIR_SEP;
      while(*++p == *DIR_SEP)
        ;
    }
    else *q++ = *p++;
  }
  *q++ = 0;
}

/* return true if the attempt succeeded */
static bool try_recursive_mkdir(TCHAR* path) {
  TCHAR* p = strrchr(path, *DIR_SEP);
  if(!p || p == path) return false;
  *p = 0;
  bool ret = false;
#ifdef __WIN32__
	if(CreateDirectory(path, NULL)) ret = true;
	else if(GetLastError() == ERROR_PATH_NOT_FOUND && try_recursive_mkdir(path)
			&& CreateDirectory(path, NULL)) ret = true;
/* # error Implement for Windows */
#else
  if(mkdir(path, 0755) == 0) ret = true;
  else if(errno == ENOENT && try_recursive_mkdir(path)
          && mkdir(path, 0755) == 0) ret = true;
#endif
  *p = *DIR_SEP;
  return ret;
}

static TCHAR* get_data_path(const char* in_filename) {
  TCHAR* filename;
#if __WIN32__ && _UNICODE
  int string_length = MultiByteToWideChar(CP_UTF8, 0, in_filename, -1, NULL, 0);
  filename = reinterpret_cast<TCHAR*>(safe_malloc(string_length * sizeof(TCHAR)));
  MultiByteToWideChar(CP_UTF8, 0, in_filename, -1, filename, string_length);
#else
  /* won't actually be modified... thank you so much, Windows~ */
  filename = const_cast<char*>(in_filename);
#endif
  const TCHAR* self_path = GetSelfPath();
  size_t len = strlen(self_path) + strlen(_T(DATA_BASE_DIR)) + strlen(filename) + 3;
  TCHAR* path = (TCHAR*)safe_malloc(len * sizeof(TCHAR));
  snprintf(path, len, _T("%s" DIR_SEP DATA_BASE_DIR DIR_SEP "%s"),
           self_path, filename);
  clean_dirseps(path);
#if __WIN32__ && _UNICODE
  safe_free(filename);
#endif
  return path;
}

std::unique_ptr<std::istream>
OpenDataFileForReadStupidWindowsHack(const std::string& filename) {
  TCHAR* path = get_data_path(filename.c_str());
  std::unique_ptr<std::ifstream> ret(new std::ifstream());
  ret->open(path, std::ios::binary | std::ios::in);
  if(!ret->good()) {
    perror(path);
    ret.reset();
  }
  safe_free(path);
  return std::move(ret);
}

static TCHAR* get_raw_path(const char* in_path) {
  TCHAR* path;
#if __WIN32__ && _UNICODE
  int string_length = MultiByteToWideChar(CP_UTF8, 0, in_path, -1, NULL, 0);
  path = reinterpret_cast<TCHAR*>(safe_malloc(string_length * sizeof(TCHAR)));
  MultiByteToWideChar(CP_UTF8, 0, in_path, -1, path, string_length);
#else
  path = teg_strdup(in_path);
#endif
  clean_dirseps(path);
  return path;
}

std::unique_ptr<std::istream>
IO::OpenRawPathForRead(const std::string& filename, bool log_error) {
  TCHAR* path = get_raw_path(filename.c_str());
  std::unique_ptr<std::ifstream> ret(new std::ifstream());
  ret->open(path, std::ios::binary | std::ios::in);
  if(!ret->good()) {
    if(log_error) perror(path);
    ret.reset();
  }
  safe_free(path);
  return std::move(ret);
}

std::unique_ptr<std::ostream>
IO::OpenRawPathForWrite(const std::string& filename, bool log_error) {
  TCHAR* path = get_raw_path(filename.c_str());
  std::unique_ptr<std::ofstream> ret(new std::ofstream());
  ret->open(path, std::ios::binary | std::ios::out);
  if(!ret->good()) {
    if(log_error) perror(path);
    ret.reset();
  }
  safe_free(path);
  return std::move(ret);
}

enum path_type {
  NORMAL, BACKUP, EDIT
};
static TCHAR* get_config_path(const char* in_filename, path_type wat=NORMAL) {
  TCHAR* filename;
#if __WIN32__ && _UNICODE
  int string_length = MultiByteToWideChar(CP_UTF8, 0, in_filename, -1, NULL, 0);
  filename = reinterpret_cast<TCHAR*>(safe_malloc(string_length * sizeof(TCHAR)));
  MultiByteToWideChar(CP_UTF8, 0, in_filename, -1, filename, string_length);
#else
  /* won't actually be modified... thank you so much, Windows~ */
  filename = const_cast<char*>(in_filename);
#endif
  const TCHAR* home = getenv(_T(CONFIG_BASE_ENV));
  if(!home) home = _T(CONFIG_BASE_ENV_DEFAULT);
  size_t len = strlen(home) + strlen(_T(CONFIG_BASE_DIR)) + strlen(_T(CONFIG_EXT)) + strlen(filename) + (wat == NORMAL ? 0 : 1) + 3;
  TCHAR* path = (TCHAR*)safe_malloc(len * sizeof(TCHAR));
  snprintf(path, len, _T("%s" DIR_SEP CONFIG_BASE_DIR DIR_SEP "%s" CONFIG_EXT "%s"),
           home, filename, wat == BACKUP ? _T("~") : wat == EDIT ? _T("^") : _T(""));
  clean_dirseps(path);
#if __WIN32__ && _UNICODE
  safe_free(filename);
#endif
  return path;
}

std::unique_ptr<std::istream>
IO::OpenConfigFileForRead(const std::string& filename) {
  TCHAR* path = get_config_path(filename.c_str());
  std::unique_ptr<std::ifstream> ret(new std::ifstream());
  ret->open(path, std::ios::binary | std::ios::in);
  if(!ret->good() && errno == ENOENT) {
    safe_free(path);
    path = get_config_path(filename.c_str(), BACKUP);
    ret->clear();
    ret->open(path, std::ios::binary | std::ios::in);
  }
  if(!ret->good()) {
    if(errno != ENOENT) perror(path);
    ret.reset();
  }
  safe_free(path);
  return std::move(ret);
}

std::unique_ptr<std::ostream>
IO::OpenConfigFileForWrite(const std::string& filename) {
  TCHAR* path = get_config_path(filename.c_str(), EDIT);
  std::unique_ptr<std::ofstream> ret(new std::ofstream());
  ret->open(path, std::ios::binary | std::ios::out);
  if(!ret->good() && errno == ENOENT && try_recursive_mkdir(path)) {
    ret->clear();
    ret->open(path, std::ios::binary | std::ios::out);
  }
  if(!ret->good()) {
    perror(path);
    ret.reset();
  }
  safe_free(path);
  return std::move(ret);
}

std::string IO::GetConfigFilePath(const std::string& filename) {
#if __WIN32__ && _UNICODE
  TCHAR* tpath = get_config_path(filename.c_str());
  char* path;
  int string_length = WideCharToMultiByte(CP_UTF8, 0, tpath, -1, NULL, 0,
                                          NULL, NULL);
  path = reinterpret_cast<char*>(safe_malloc(string_length));
  WideCharToMultiByte(CP_UTF8, 0, tpath, -1, path, string_length,
                      NULL, NULL);
  safe_free(tpath);
#else
  /* won't actually be modified... thank you so much, Windows~ */
  char* path = get_config_path(filename.c_str());
#endif
  std::string ret(path);
  safe_free(path);
  return ret;
}

void IO::TryCreateConfigDirectory() {
  TCHAR* path = get_config_path("Missingfi");
  try_recursive_mkdir(path);
  safe_free(path);
}

void IO::UpdateConfigFile(const std::string& filename) {
  TCHAR* path_normal = get_config_path(filename.c_str(), NORMAL);
  TCHAR* path_backup = get_config_path(filename.c_str(), BACKUP);
  TCHAR* path_edit = get_config_path(filename.c_str(), EDIT);
  /* optimistically assume success on all the below operations */
  /* TODO: fsync/_commit this file */
  remove(path_backup);
  rename(path_normal, path_backup);
  rename(path_edit, path_normal);
  safe_free(path_normal);
  safe_free(path_backup);
  safe_free(path_edit);
}

#if __WIN32__
TCHAR* get_relative_path(const char* in_filename) {
  TCHAR* filename;
#if _UNICODE
  int string_length = MultiByteToWideChar(CP_UTF8, 0, in_filename, -1, NULL, 0);
  filename = reinterpret_cast<TCHAR*>(safe_malloc(string_length * sizeof(TCHAR)));
  MultiByteToWideChar(CP_UTF8, 0, in_filename, -1, filename, string_length);
#else
  /* won't actually be modified... thank you so much, Windows~ */
  filename = const_cast<char*>(in_filename);
#endif
  const TCHAR* self_path = GetSelfPath();
  size_t len = strlen(self_path) + strlen(filename) + 2;
  TCHAR* path = (TCHAR*)safe_malloc(len * sizeof(TCHAR));
  snprintf(path, len, _T("%s" DIR_SEP "%s"),
           self_path, filename);
  clean_dirseps(path);
#if _UNICODE
  safe_free(filename);
#endif
  return path;
}

void IO::DoRedirectOutput() {
  /* lots of copy-pasting here... */
  TCHAR* path = get_relative_path("stdout.utxt");
  if(!freopen(path, _T("wb"), stdout)) {
    safe_free(path);
    path = get_config_path("stdout.utxt");
    if(!freopen(path, _T("wb"), stdout)) {
      if(errno == ENOENT) {
        if(!try_recursive_mkdir(path)) return; // If try_recursive_mkdir failed, attempting to redirect stderr is pointless
        freopen(path, _T("wb"), stdout);
      }
      /* if it failed, oh well */
    }
  }
  safe_free(path);
  path = get_relative_path("stderr.utxt");
  if(!freopen(path, _T("wb"), stderr)) {
    safe_free(path);
    path = get_config_path("stderr.utxt");
    freopen(path, _T("wb"), stderr);
    /* don't try_recursive_mkdir again because why would that even happen? */
  }
  safe_free(path);
}
#endif

#if TEG_USE_SN
namespace {
  class TegCatSource : public SN::CatSource {
    static const std::string SUFFIX;
    TCHAR* base_path;
  public:
    TegCatSource() {
      base_path = get_data_path(LANG_BASE_DIR DIR_SEP);
    }
    ~TegCatSource() {
      if(base_path != nullptr) safe_free(base_path);
    }
    void GetAvailableCats(std::function<void(std::string)> func) {
#ifdef __WIN32__
#error NIY
#else
      DIR* d = opendir(base_path);
      if(d) {
        struct dirent* ent;
        while((ent = readdir(d))) {
          if(ent->d_name[0] == '.'
#ifdef DT_REG
             || ent->d_type != DT_REG
#endif
             ) continue;
          std::string name(ent->d_name);
          if(name.compare(name.length()-SUFFIX.length(), name.length(),
                          SUFFIX) != 0)
            continue;
          std::string code(name.begin(),
                           name.begin()+(name.length()-SUFFIX.length()));
          for(auto& c : code) {
            if(c == '-') continue;
            else if(c == '_') c = '-';
          }
          if(SN::IsValidLanguageCode(code)) func(std::move(code));
        }
        closedir(d);
      }
#endif
    }
    std::shared_ptr<std::istream> OpenCat(const std::string& cat) {
      std::string path_string(LANG_BASE_DIR DIR_SEP + cat + SUFFIX);
      TCHAR* path = get_data_path(path_string.c_str());
      std::unique_ptr<std::ifstream> ret(new std::ifstream());
      ret->open(path, std::ios::binary | std::ios::in);
      if(!ret->good()) {
        perror(path);
        ret.reset();
      }
      safe_free(path);
      return std::move(ret);
    }
  };
#ifndef TEG_SN_CAT_EXTENSION
#define TEG_SN_CAT_EXTENSION ".utxt"
#endif
  const std::string TegCatSource::SUFFIX(TEG_SN_CAT_EXTENSION);
}

std::shared_ptr<SN::CatSource> IO::GetSNCatSource() {
  static std::shared_ptr<SN::CatSource> src
    = std::make_shared<TegCatSource>();
  return src;
}
#endif
