// $Id$

#include "spooler.h"

namespace sos {
namespace scheduler {

using namespace zschimmer::file;

//--------------------------------------------------------------------------------------------const

//string file_path_from_include_element( const xml::Element_ptr& element )
//{
//    assert( element.nodeName_is( "include" ) );
//
//    File_path path      = element.getAttribute( "file"      );
//    File_path live_path = element.getAttribute( "live_path" );
//
//    if( path == ""  &&  live_path == "" )  z::throw_xc( XXX );
//    if( path != ""  &&  live_path != "" )  z::throw_xc( XXX );
//
//    if( live_path != "" )
//    {
//        file_path.assert_not_beyond_root();
//    }
//}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
