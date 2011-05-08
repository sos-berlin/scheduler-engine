#include "precomp.h"
//#define MODULE_NAME "profile"
// profile.cpp                                          (c) SOS GmbH Berlin
//                                                          Joacim Zschimmer

#if 0

#include <cstring.h>
#include <windows.h>

#include <sos.h>
#include <xception.h>

#include <profile.h>

//-------------------------------------------Profile::Section::Section

Profile::Section::Section( const string& filename, const string& section_name )
:
    _filename     ( filename ),
    _section_name ( section_name )
{
}

//-------------------------------------------Profile::Section::Entry::operator=

Profile::Section::Entry::~Entry()
{
    if( _my_section ) {
        delete _section_ptr;
    }
}

//-------------------------------------------Profile::Section::Entry::operator=
/*
void Profile::Section::Entry::operator= ( const Storable_as<string>& value )
{
    string value_string;
    value.object_store( &value_string );

    WritePrivateProfileString( _section_ptr->section_name().c_str(),
                               _entry_name.c_str(),
                               value_string.c_str(),
                               _section_ptr->filename().c_str()
                             );
}
*/
//-------------------------------------------Profile::Section::Entry::operator=

void Profile::Section::Entry::operator= ( const string& value )
{
    WritePrivateProfileString( _section_ptr->section_name().c_str(),
                               _entry_name.c_str(),
                               value.c_str(),
                               _section_ptr->filename().c_str()
                             );
}

//------------------------Profile::Section::Entry::void operator string

Profile::Section::Entry::operator string() const
{
     char value [ max_profile_section_entry_value_length + 1 ];

     value[ 0 ] = '\0';

     GetPrivateProfileString( _section_ptr->section_name().c_str(),
                              _entry_name.c_str(),
                              _default_value.c_str(),
                              value, sizeof value,
                              _section_ptr->filename().c_str()
                            );

     return string( value );
}

//-------------------------------------------------Profile::Section::Entry::get

#if 0   // Ab Borland 4.0 Übersetzungsfehler
void Profile::Section::Entry::get( Typed* value_ptr ) const
{
    String value_string = *this;
    (istrstream) (Const_string_area) value_string >> *value_ptr;
}
#endif

#endif
