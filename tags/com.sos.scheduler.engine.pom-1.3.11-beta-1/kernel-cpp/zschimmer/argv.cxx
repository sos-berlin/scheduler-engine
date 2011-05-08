// zschimmer.cxx                                    © 2000 Joacim Zschimmer
// $Id$

// §1693

#include "argv.h"

using namespace std;


namespace zschimmer {

//----------------------------------------------------------------------------------get_argv_option

bool get_argv_option( const char* argv_i, const string& option )
{
    return argv_i == option;
}

//----------------------------------------------------------------------------------get_argv_option

bool get_argv_option( const char* argv_i, const string& prefix, string* result )
{
    if( string_begins_with( argv_i, prefix ) )
    {
        result->assign( argv_i + prefix.length() );
        return true;
    }
    else
        return false;
}

//----------------------------------------------------------------------------------get_argv_option

bool get_argv_option( const char* argv_i, const string& prefix, int* result )
{
    string result_string;

    if( get_argv_option( argv_i, prefix, &result_string ) )
    {
        try
        {
            *result = as_int( result_string );
        }
        catch( Xc& x )
        {
            x.append_text( prefix );
            throw;
        }

        return true;
    }
    else
        return false;
}

//----------------------------------------------------------------------------------get_argv_option

bool get_argv_option( const char* argv_i, const string& prefix, bool* result )
{
    string result_string;

    if( get_argv_option( argv_i, prefix, &result_string ) )
    {
        try
        {
            *result = as_bool( result_string );
        }
        catch( Xc& x )
        {
            x.append_text( prefix );
            throw;
        }

        return true;
    }
    else
        return false;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
