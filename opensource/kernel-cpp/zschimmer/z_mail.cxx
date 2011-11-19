// $Id: z_mail.cxx 13199 2007-12-06 14:15:42Z jz $

#include "zschimmer.h"
#include "z_mail.h"
#include "log.h"


namespace zschimmer {

//----------------------------------------------------------------------------Email_address::assign
    
void Email_address::assign( const string& name_and_address )
{
    _name    = "";
    _address = "";


    const char* p    = name_and_address.c_str();

    while( p[0] == ' ' )  p++;

    

    if( p[0] == '"' )
    {
        p++;

        while( p[0]  &&  p[0] != '"' )
        {
            if( p[0] == '\\' )  p++;
            _name += p++[0];
        }

        if( p[0] == '"' )  p++;
        while( p[0] == ' ' )  p++;
    }
    else
    {
        while( p[0]  &&  p[0] != '<' )  _name += p++[0];
        _name = rtrim( _name );

        if( !p[0] )  _address = _name,  _name = "";
    }


    if( p[0] == '<' )
    {
        p++;
        while( p[0]  &&  p[0] != '>' )  _address += p++[0];
        if( p[0] == '>' )  p++;
    }

    while( p[0] == ' ' )  p++;

    Z_LOG2( "mail", Z_FUNCTION << "(\"" << name_and_address << "\") ==> \"" << _name << "\", \"" << _address << "\"\n" );

    if( !_suppress_exceptions  &&  p[0] )  throw_xc( "Z-MAIL-001", name_and_address );
}

//---------------------------------------------------------------------Email_address::email_address

string Email_address::email_address() const
{
    if( _name == "" )
    {
        return _address;
    }
    else
    {
        S result;

        result << '"' << _name << '"';     // Fehlt: '"' im Namen berücksichtigen!
        result << " <" << _address << '>';

        return result;
    }
}

//-------------------------------------------------------------------------------------------------


} //namespace zschimmer
