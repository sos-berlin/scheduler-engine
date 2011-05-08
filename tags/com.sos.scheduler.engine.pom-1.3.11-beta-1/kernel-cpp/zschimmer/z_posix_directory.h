// $Id$

#ifndef __Z_POSIX_DIRECTORY
#define __Z_POSIX_DIRECTORY

#include <glob.h>

namespace zschimmer {
namespace posix {

//-------------------------------------------------------------------------------------------------

/* Nicht geprüft
struct Simple_directory_reader
{
                                Simple_directory_reader     ()                                      : _handle(NULL) {}
                                Simple_directory_reader     ( const std::string& path, Flags flags ) : _handle(NULL) { open( path, pattern, flags ); }
    virtual                    ~Simple_directory_reader     ()                                      { close();  }

    void                        open                        ( const std::string& path, Flags );
    void                        close                       ();
    string                      get                         ();

  private:
    DIR*                       _handle;
    Flags                      _flags;
};
*/

//---------------------------------------------------------------------------------------------Glob
    
struct Glob : Non_cloneable
{
                                Glob                        ( const string& pattern, int glob_flags );
    virtual                    ~Glob                        ();
    
    const char* const*          files                       () const                                      { return const_cast<const char* const*>( _glob.gl_pathv ); }

    glob_t                     _glob;
};

//-------------------------------------------------------------------------------------------------

} //namespace posix
} //namespace zschimmer

#endif
