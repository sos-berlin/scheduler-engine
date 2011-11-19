// $Id: env.cxx 11818 2005-12-15 10:02:25Z jz $

#include "precomp.h"
#include "sos.h"
#include "../file/anyfile.h"
#include "env.h"

#include <set>

namespace sos {

typedef std::set<string> Env;

static bool set_environment_from_sos_ini_executed = false;
static Env  env;

//----------------------------------------------------------------set_environment_from_sos_ini_once

void set_environment_from_sos_ini_once()
{
    Z_MUTEX( hostware_mutex )
    {
        if( !set_environment_from_sos_ini_executed )  set_environment_from_sos_ini();
    }
}

//----------------------------------------------------------------set_environment_from_sos_ini_once

void clear_environment_from_sos_ini()
{
    Z_MUTEX( hostware_mutex )
    {
        if( !set_environment_from_sos_ini_executed )  return;

        set_environment_from_sos_ini_executed = false;

        if( env.size() > 0 )    // Falls bereits destruiert, ist size=0 ...
        {
            for( Env::iterator it = env.begin(); it != env.end(); it++ )
            {
                string n = *it;
                const char* p = strchr( n.c_str(), '=' );

                if( p ) 
                {
                    n.erase( p + 1 - n.c_str() );
                    LOG( "[env] " << n << '\n' );
                    putenv( (char*)n.c_str() );       // Verweis auf String wieder entfernen
                }
            }

            env.clear();
        }
    }
}

//-----------------------------------------------------------------substitute_environment_variables
/*
string substitute_environment_variables( const string& value )
{
    if( !strchr( value.c_str(), '$' ) )  return value;      // Abkürzung


    string result;
    const char* p = value.c_str();

    while( *p )
    {
        if( *p == '$' )
        {
            p++;
            string name;
            while( isalnum( *p )  ||  *p == '_' )  name += *p,  p++;
            result.append( getenv( name.c_str() ) );
        }
        else
            result += *p++;
    }

    return result;
}
*/
//---------------------------------------------------------------------set_environment_from_sos_ini

void set_environment_from_sos_ini()
{
    Z_MUTEX( hostware_mutex )
    {
        set_environment_from_sos_ini_executed = true;

        Any_file section;
        Dynamic_area entry;
        
        section.open( "-in profile -section=env" );

        while( !section.eof() )
        {
            section.get( &entry );

            string name  = section.record_type()->as_string( "entry", entry.byte_ptr() );
            string value = section.record_type()->as_string( "value", entry.byte_ptr() );

            value = z::subst_env( value );
            Env::iterator it = env.insert( name + "=" + value ).first;

            LOG( "[env] " << *it << '\n' );
            putenv( (char*)it->c_str() );
        }
    }
}

} //namespace sos
