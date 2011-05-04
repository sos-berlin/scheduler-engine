// sossql.h                             ©1996 SOS GmbH Berlin

#ifndef __SOSSQL_H
#define __SOSSQL_H

namespace sos
{

const int  max_orderby_params            = 1000; //20;     // Sollte für alle Fälle reichen
const int  max_groupby_params            = 1000; //20;     // Sollte für alle Fälle reichen
const int  sql_max_tables_per_select     = 100;
const char sql_identifier_quote_char     = '"';  //'`';
const char sql_identifier_quote_string[] = "\""; //"`";   // sql_identifier_quote_char als String


struct Sql_token_table_entry 
{
    int                     _kind;
    const char*             _name;
};

extern const Sql_token_table_entry sql_token_table [];
extern const int                   sql_token_table_size;

} //namespace sos

#endif

