// sosprof.h                                                    (c) SOS GmbH Berlin

#ifndef __SOSPROF_H
#define __SOSPROF_H

namespace sos
{

void                            set_sos_ini_filename    ( const string& filename );

string                          read_profile_string     ( const char* filename, const char* group_name, const char* entry_name, const char* default_result = "" );
bool                            read_profile_bool       ( const char* filename, const char* group_name, const char* entry_name, bool def );
int                             read_profile_int        ( const char* filename, const char* group_name, const char* entry_name, int def = 0 );
uint                            read_profile_uint       ( const char* filename, const char* group_name, const char* entry_name, uint def = 0 );
double                          read_profile_double     ( const char* filename, const char* group_name, const char* entry_name, double def = 0.0 );
bool                            read_profile_entry      ( Area*, const char* filename, const char* group_name, const char* entry_name, const char* default_result = "" );
string                          read_existing_profile_string( const char* filename, const char* group_name, const char* entry_name );

inline string read_profile_string( const char* filename, const char* group_name, const char* entry_name, const string& default_result )
{
    return read_profile_string( filename, group_name, entry_name, default_result.c_str() );
}

inline string                   read_profile_string     ( const string& filename, const string& group_name, const string& entry_name, const string& default_result = "" )   { return read_profile_string( filename.c_str(), group_name.c_str(), entry_name.c_str(), default_result.c_str() ); }
inline bool                     read_profile_bool       ( const string& filename, const string& group_name, const string& entry_name, bool def )                            { return read_profile_bool  ( filename.c_str(), group_name.c_str(), entry_name.c_str(), def ); }
inline int                      read_profile_int        ( const string& filename, const string& group_name, const string& entry_name, int def = 0 )                         { return read_profile_int   ( filename.c_str(), group_name.c_str(), entry_name.c_str(), def ); }
inline uint                     read_profile_uint       ( const string& filename, const string& group_name, const string& entry_name, uint def = 0 )                        { return read_profile_uint  ( filename.c_str(), group_name.c_str(), entry_name.c_str(), def ); }
inline double                   read_profile_double     ( const string& filename, const string& group_name, const string& entry_name, double def = 0.0 )                    { return read_profile_double( filename.c_str(), group_name.c_str(), entry_name.c_str(), def ); }


void                            write_profile_bool      ( bool, const char* filename, const char* group_name, const char* entry_name );
void                            write_profile_uint      ( uint, const char* filename, const char* group_name, const char* entry_name, int def = 0 );
void                            write_profile_string    ( const string&, const char* filename, const char* group_name, const char* entry_name );
void                            write_profile_entry     ( const Const_area&, const char* filename, const char* group_name, const char* entry_name );
void                            write_profile           ( const char* filename, const char* section, const char* entry, const char* value );


} //namespace sos

#endif
