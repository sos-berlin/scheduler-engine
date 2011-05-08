// $Id$

#ifndef __ZSCHIMMER_UTF8
#define __ZSCHIMMER_UTF8

namespace zschimmer {

//-------------------------------------------------------------------------------------------------

int                             utf8_byte_count_from_iso_8859_1( const char* iso_8859_1, int length );
int                             utf8_byte_count_from_iso_8859_1( const char* iso_8859_1 );
inline int                      utf8_byte_count_from_iso_8859_1( const string& iso_8859_1 )             { return utf8_byte_count_from_iso_8859_1( iso_8859_1.data(), iso_8859_1.length() ); }

int                             write_iso_8859_1_as_utf8    ( const char* iso_8859_1, int length, char* utf8_buffer, int buffer_size );
inline int                      write_iso_8859_1_as_utf8    ( const string& iso_8859_1, char* utf8_buffer, int buffer_size ) { return write_iso_8859_1_as_utf8( iso_8859_1.data(), iso_8859_1.length(), utf8_buffer, buffer_size ); }

int                             write_utf8_as_unicode       ( const char* utf8, int utf8_length, OLECHAR* unicode, int unicode_size );
int                             write_unicode_as_utf8       ( const OLECHAR* unicode, int length, char* utf8_buffer, int buffer_size );

string                          iso_8859_1_from_utf8        ( const char* utf8, int utf8_length );
inline string                   iso_8859_1_from_utf8        ( const char* utf8 )                        { return iso_8859_1_from_utf8( utf8, strlen( utf8 ) ); }
inline string                   iso_8859_1_from_utf8        ( const string& utf8 )                      { return iso_8859_1_from_utf8( utf8.data(), utf8.length() ); }


string                          utf8_from_iso_8859_1        ( const char* iso_8859_1, int length );
inline string                   utf8_from_iso_8859_1        ( const char* iso_8859_1 )                  { return utf8_from_iso_8859_1( iso_8859_1, strlen( iso_8859_1  ) ); }
inline string                   utf8_from_iso_8859_1        ( const string& iso_8859_1 )                { return utf8_from_iso_8859_1( iso_8859_1.data(), iso_8859_1.length() ); }

int                             utf8_length                 ( const char* utf8, int byte_count );
int                             utf8_length                 ( const char* utf8 );

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif