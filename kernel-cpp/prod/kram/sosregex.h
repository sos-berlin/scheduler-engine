// sosregex.h

#ifndef __SOSREGEX_H
#define __SOSREGEX_H

#include <sysdep.h>

#if defined SYSTEM_WIN
#include <gregex.h>
#endif

#if defined SYSTEM_SOLARIS
#include <rw/cstring.h>
#include <rw/regexp.h>
typedef RWCRegexp Regex;
#endif

// Sos_regex - Klasse
struct Sos_regex : Regex
{
    Sos_regex( const Sos_string& str );

    void match( const Sos_string& to_match );

 private:
    Sos_string _regex_string; // Für Fehlerausgaben
};

// Implementierung
Sos_regex::Sos_regex( const Sos_string& str )
:
    Regex( c_str( str ), 1 ),
    _regex_string( str )
{
}

void Sos_regex::match( const Sos_string& to_match )
{
    int len = length( to_match );
    unsigned int matched_length;

#if defined SYSTEM_WIN
    matched_length = Regex::match( c_str( to_match ), len, 0 );
#endif

#if defined SYSTEM_SOLARIS
    Regex::index( to_match, &matched_length, 0 );
#endif

    if ( matched_len != len )
    {
        Xc x( "SOS-1306", Msg_insertions( c_str(_regex_string), erg ) );
        throw x;
    }
}

#endif
