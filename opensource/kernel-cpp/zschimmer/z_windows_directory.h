// $Id: z_windows_directory.h 12116 2006-06-06 18:03:24Z jz $

#ifndef __Z_WINDOWS_DIRECTORY_H
#define __Z_WINDOWS_DIRECTORY_H

#include <io.h>
#include "regex_class.h"

namespace zschimmer {
namespace windows {

//--------------------------------------------------------------------------Simple_directory_reader

struct Simple_directory_reader : Simple_directory_reader_base
{
                                    Simple_directory_reader ()                                      : _handle(-1), _zero_(this+1) {}
                                    Simple_directory_reader ( const std::string& path, Flags flags,
                                                              const std::string& pattern = "*" )    : _handle(-1), _zero_(this+1) { open( path, flags, pattern ); }
    virtual                        ~Simple_directory_reader ()                                      { close();  }

    void                            open                    ( const std::string& path, Flags flags, const std::string& pattern );
    void                            close                   ();
    string                          get                     ();

  private:
    Fill_zero                      _zero_;
    int                            _handle;
    Flags                          _flags;
    Regex                          _regex;
    bool                           _is_first_entry;
    _finddatai64_t                 _entry;
};

//-------------------------------------------------------------------------------------------------

} //namespace windows
} //namespace zschimmer

#endif
