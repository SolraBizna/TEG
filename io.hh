#ifndef IO_HH
#define IO_HH

#include "teg.hh"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <memory>
#ifdef TEG_USE_SN
#include "sn.hh"
#endif

namespace IO {
  /* Only use these two for tools! */
  std::unique_ptr<std::istream>
  OpenRawPathForRead(const std::string& path, bool log_error = true);
  std::unique_ptr<std::ostream>
  OpenRawPathForWrite(const std::string& path, bool log_error = true);
  /* Use this to read data files; FS virtualization may be in effect
     Always prints an error on failure */
  std::unique_ptr<std::istream> OpenDataFileForRead(const std::string& path);
  /* Use these to read/write configuration files
     Sequence for writing a config file:
     OpenConfigFileForWrite, (write stuff), fclose, UpdateConfigFile
     If you don't UpdateConfigFile, the configuration file will not be saved */
  std::unique_ptr<std::istream>
  OpenConfigFileForRead(const std::string& filename);
  std::unique_ptr<std::ostream>
  OpenConfigFileForWrite(const std::string& filename);
  void UpdateConfigFile(const std::string& filename);
  /* Use this for, say, an sqlite config database
     Returns a UTF-8 absolute path to a config file with the given name. */
  std::string GetConfigFilePath(const std::string& filename);
  /* Use this, say, if SQLITE_CANTOPEN is returned for a database */
  void TryCreateConfigDirectory();
#if __WIN32__
  void DoRedirectOutput();
#endif
#ifdef TEG_USE_SN
  std::shared_ptr<SN::CatSource> GetSNCatSource();
#endif
};

#endif
