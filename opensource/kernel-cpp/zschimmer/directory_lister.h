// $Id: directory_lister.h 13199 2007-12-06 14:15:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_DIRECTORY_LISTER_H
#define __ZSCHIMMER_DIRECTORY_LISTER_H

#include <stdio.h>

#ifdef Z_WINDOWS
#   include <io.h>              // _finddata_t()
#else
#   include <dirent.h>
#endif

#include "base.h"
#include "file_path.h"


namespace zschimmer {
namespace file {

//---------------------------------------------------------------------------------Directory_lister

struct Directory_lister
{
                                Directory_lister            ();
                                Directory_lister            ( const File_path& path );
                               ~Directory_lister            ();

    void                        open                        ( const File_path& );
    void                        close                       ();
    ptr<File_info>              get                         ();
    bool                        is_opened                   () const;

  private:
    void                        init                        ();
    ptr<File_info>              read                        ();

    Fill_zero                  _zero_;
    string                     _directory_path;

#   ifdef Z_WINDOWS
        int                    _handle;
        _finddata_t            _finddata;
#    else
        DIR*                   _handle;
#   endif
};

//-------------------------------------------------------------------------------------------------

} //namespace file
} //namespace zschimmer

#endif
