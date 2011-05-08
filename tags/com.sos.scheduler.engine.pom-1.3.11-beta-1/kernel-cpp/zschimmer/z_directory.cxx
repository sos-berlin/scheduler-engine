// $Id$

#include "zschimmer.h"
#include "z_directory.h"

namespace zschimmer {


//--------------------------------------------------------------------list_directory_with_wildcards
/*    
std::list<string> list_directory_with_wildcards( const string& pattern, Simple_directory_reader::Flags flags )
{
    std::list<string>       result;
    Simple_directory_reader dir;

    dir.open( pattern, flags );

    while(1)
    {
        string filename = dir.get();
        if( filename == "" )  break;
        result.push_back( filename );
    }

    dir.close();

    return result;
}
*/
//--------------------------------------------------------------------list_directory_with_wildcards

//static int glob_errfunc( const char* path, int errn ){}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
