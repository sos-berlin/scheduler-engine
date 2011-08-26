// $Id: z_directory.h 12090 2006-05-30 07:00:24Z jz $

#ifndef __Z_DIRECTORY_H
#define __Z_DIRECTORY_H


namespace zschimmer {

//---------------------------------------------------------------------Simple_directory_reader_base
    
struct Simple_directory_reader_base
{
    enum Flags
    {
        no_flag,
        no_subdirectory,
    };
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#ifdef Z_WINDOWS
#   include "z_windows_directory.h"
#else
#   include "z_posix_directory.h"
#endif

namespace zschimmer {

//--------------------------------------------------------------------------Simple_directory_reader

//std::list<string>                   list_directory              ( const string& path, const string& pattern, 
//                                                                  Simple_directory_reader::Flags flags = Simple_directory_reader::no_flag );

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer


#endif
