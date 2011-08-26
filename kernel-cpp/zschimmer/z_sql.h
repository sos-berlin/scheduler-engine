// $Id: z_sql.h 13707 2008-10-17 10:41:41Z jz $

#ifndef __ZSCHIMMER_Z_SQL_H
#define __ZSCHIMMER_Z_SQL_H

#include "z_com.h"

namespace zschimmer {
namespace sql {

/*
    struct Transaction;   Im Thread Local Storage ablegen (Thread_data)
    Für jede Datenbankverbindung eine eigene Transaktion: map<Db*, ptr<Transaction> >
    Alle Datenbankzugriffe über die Transaktion, so dass sie weiß, ob ein Rollback oder Commit nötig ist.
    Verschachtelte Transaktionen (die inneren ignorieren Commit, Rollback ist nicht möglich, außer am Anfang)
    Unabhängig von der Hostware (ist über allgemeines Interface ankoppelbar)

    Ohne TLS: DB-Objekt wird mit Transaction verbunden. Operation ohne eröffnete Transaction ist unzulässig.
*/

//-------------------------------------------------------------------------------------------------

struct Value;

//-------------------------------------------------------------------------------------------------

inline string                   quoted                      ( const string& value )                 { return quoted_string( value, '\'', '\'' ); }
inline string                   quoted_name                 ( const string& name )                  { return quoted_string( name, '"', '"' ); }
inline string                   uquoted_name                ( const string& name )                  { return quoted_string( ucase( name ), '"', '"' ); }

string                          null_string_equation        ( const string& value );                // "='...'" oder "is null", wenn value==""
string                          without_comments_regardless_quotes( const string& sql );

//--------------------------------------------------------------------------------------------
/*
enum Column_attribute
{
    is_nullable,
    is_not_nullable,
    is_primary_key
};
*/
//--------------------------------------------------------------------------------------------
/*
struct Column_declaration
{
                                Column_declaration          ( const string& name, const string& type, Column_attribute a ) : _name(name), _type(type), _attribute(a) {}

    string                     _name;
    string                     _type;
    Column_attribute           _attribute;
};
*/
//--------------------------------------------------------------------------------------------
/*
struct Table
{
    void                        set_name                    ( const string& name )                  { _name = name; }
    void                        add_column                  ( const string& name, const string& type, Column_attribute a ) { _column_declarations.push_back( Column_declaration( name, type, a ) ); }

    std::vector<Column_declaration> _column_declarations;
    string                     _name;
};
*/
//--------------------------------------------------------------------------------------------Flags

enum Flags
{
    flag_none                   = 0,
    flag_uppercase_names        = 0x01,
    flag_quote_names            = 0x02,
    flag_write_empty_as_null    = 0x04,
    flag_dont_quote_table_names = 0x08,
    flag_use_simple_iso_datetime_string = 0x10,
};

inline Flags                    operator|                   ( Flags a, Flags b )                    { return (Flags)( (int)a | (int)b ); }

//------------------------------------------------------------------------------Database_descriptor

struct Database_descriptor
{
                                Database_descriptor         ( Flags flags = flag_none );


    string                      make_name                   ( const string& name )  const;
    string                      make_table_name             ( const string& name ) const;

    string                      equal_expr                  ( const string& name, const string&  value ) const;
    string                      equal_expr                  ( const string& name, int64          value ) const;
    string                      equal_expr                  ( const string& name, const VARIANT& value ) const;
    string                      equal_expr                  ( const string& name, const Value&   value ) const;
    string                      timestamp_string            ( const string& datetime ) const;
    string                      operand                     ( const string& value ) const           { return quoted(value); }
    string                      operand                     ( int64 value ) const                   { return as_string(value); }
    string                      operand                     ( const VARIANT& value ) const;



    Fill_zero                  _zero_;
  //Transaction*               _transaction;
    bool const                 _uppercase_names;
    bool const                 _quote_names; 
/*
    bool                       _quote_table_names; 
    bool                       _quote_field_names;
    bool                       _uppercase_table_names;
    bool                       _uppercase_field_names;
*/
    bool const                 _write_empty_as_null;
    bool const                 _dont_quote_table_names;
    bool                       _use_simple_iso_datetime_string;     // 'yyyy-mm-dd' statt {ts'yyyy-mm-dd'}
};

//---------------------------------------------------------------------------------Table_descriptor

struct Table_descriptor
{
                                Table_descriptor            ( const Database_descriptor* p, const string& table_name, const string& key_names );

    const string&               key_name                    ( int index ) const                     { return _key_names[index]; }
    bool                        is_key                      ( const string& name ) const;
    void                    set_name                        ( const string& );
    string                      name                        () const                                { return _table_name; }
    string                      sql_name                    () const;
    const Database_descriptor*  database_descriptor         () const                                { return _database_descriptor; }


    Fill_zero                  _zero_;
    const Database_descriptor* _database_descriptor;
    string                     _table_name;
  //string const               _key_names;                  // "name1,name2,..."

    typedef std::vector<string> Key_names;
    Key_names                  _key_names;
};

//--------------------------------------------------------------------------------------------Value

struct Value
{
    enum Kind
    {
        k_null,
        k_string,
        k_numeric,
        k_datetime,
        k_direct
    };

  //enum Direct { direct };


                                Value                       ()                                      : _kind(k_null) {}
                                Value                       ( const string& value )                 { set( value ); }
                                Value                       ( int64 value )                         { set( value ); }
                                Value                       ( const VARIANT& value )                { set( value ); }
                              //Value                       ( const string& value, Direct )         { set_direct( value ); }
                              //Value                       ( const char* value  , Direct )         { set_direct( value ); }

    string                      sql_operand                 ( const Database_descriptor& ) const;

    Value&                      operator =                  ( const string& value )                 { set( value );  return *this; }
    Value&                      operator =                  ( int64 value )                         { set( value );  return *this; }
    Value&                      operator =                  ( const VARIANT& value )                { set( value );  return *this; }

    void                        set                         ( const string& value )                 { _value = value;  _kind = k_string; }
    void                        set                         ( int64 value )                         { _value = as_string(value);  _kind = k_numeric; }
    void                        set                         ( const VARIANT& );
    void                        set_direct                  ( const string& value )                 { _value = value;  _kind = k_direct; }
    void                        set_datetime                ( const string& value )                 { _value = value;  _kind = k_datetime; }


    string                     _value;
    Kind                       _kind;


    //friend ostream&             operator <<                 ( ostream& s, const Value& v )          { s << v.sql_operand( false );  return s; }
};

extern const Value              null_value;

//-------------------------------------------------------------------------------------Direct_value

struct Direct_value : Value
{
                                Direct_value                ( const string& value )                 { set_direct( value ); }
                                Direct_value                ( const char* value   )                 { set_direct( value ); }
};

//---------------------------------------------------------------------------------------------Stmt

struct Stmt
{
                              //Stmt                        ()                                          : _database_descriptor(NULL) {}
                                Stmt                        ( const Database_descriptor* p, const string& table_name    ) : _database_descriptor(p), _table_name(table_name), _table_descriptor(NULL) {}
                                Stmt                        ( const Database_descriptor* p, const char* table_name = "" ) : _database_descriptor(p), _table_name(table_name), _table_descriptor(NULL) {}
                                Stmt                        ( const Table_descriptor* p )               : _database_descriptor(p->_database_descriptor), _table_name(p->_table_name), _table_descriptor(p) {}
    virtual                    ~Stmt                        ()                                          {}


    void                    set_table_name                  ( const string& name )                      { _table_name = name; }
    string                      make_name                   ( const string& name ) const                { return _database_descriptor->make_name( name ); }
    string                      make_table_name             ( const string& name ) const                { return _database_descriptor->make_table_name( name ); }


                                operator string             ()                                          { return make_stmt(); }
    virtual const string        make_stmt                   () = 0;

    string                     _table_name;
    const Database_descriptor* _database_descriptor;
    const Table_descriptor*    _table_descriptor;
};

//-------------------------------------------------------------------------------------------------

struct Where_clause
{
                                Where_clause                ( const Database_descriptor* p )            : _database_descriptor(p) {}
                                Where_clause                ( const Table_descriptor* p )               : _database_descriptor(p->_database_descriptor) {}

    template<class T>
    void                        and_where_condition         ( const string& name, const T& value )      { _where_conditions[ name ] = value; }

    void                        remove_where_condition      ( const string& name )                      { _where_conditions.erase( name ); }
    void                        add_where                   ( const string& text )                      { _where_tail += text; }
    bool                        is_where_empty              ()                                          { return _where_conditions.empty()  &&  _where_tail.empty(); }
    string                      where_string                ();

  private:
    const Database_descriptor* _database_descriptor;

    typedef stdext::hash_map<string,Value> Where_conditions;
    Where_conditions           _where_conditions;

    string                     _where_tail;
};

//--------------------------------------------------------------------------------Has_where_clause<>
/*
template< class BASECLASS >
struct has_where_clause : BASECLASS, Where_clause
{
    typedef has_where_clause<BASECLASS>  Has_where_clause;

                              //has_where_clause            ()                                          {}
                                has_where_clause            ( Database_descriptor* p, const string& table_name ) : BASECLASS(p,table_name) {}
                                has_where_clause            ( Database_descriptor* p, const char*   table_name ) : BASECLASS(p,table_name) {}
                                has_where_clause            ( const Table_descriptor* p )                        : BASECLASS(p) {}
};
*/
//---------------------------------------------------------------------------------------Write_stmt

struct Write_stmt : Stmt
{
                              //Write_stmt                  ()                                          {}
                                Write_stmt                  ( Database_descriptor* p, const string& table_name    ) : Stmt( p, table_name ) {}
                                Write_stmt                  ( Database_descriptor* p, const char* table_name = "" ) : Stmt( p, table_name ) {}
                                Write_stmt                  ( const Table_descriptor* p )                           : Stmt( p ) {}


  //void                    set_value                       ( const string& name, const string& value ) { string uname = ucase(name); _data[uname] = Name_value_pair(uname,value); }
  //void                    set_value                       ( const string& name, int64 value )         { string uname = ucase(name); _data[uname] = Name_value_pair(uname,value); }

    Value&                      operator []                 ( const string& name );
    Value&                      operator []                 ( const char* name )                        { return operator[]( string( name ) ); }

    void                        set_datetime                ( const string& name, const string& value );
    bool                        empty                       () const                                    { return _data.empty(); }
    const string                make_insert_stmt            () const;

    virtual bool                is_update                   () const                                    { return false; }


    typedef std::map<string,Value> Map;
    Map                        _data;
};

//--------------------------------------------------------------------------------------Insert_stmt

struct Insert_stmt : Write_stmt
{
                              //Insert_stmt                 ()                                          {}
                                Insert_stmt                 ( Database_descriptor* p, const string& table_name    ) : Write_stmt(p,table_name) {}
                                Insert_stmt                 ( Database_descriptor* p, const char* table_name = "" ) : Write_stmt(p,table_name) {}
                                Insert_stmt                 ( const Table_descriptor* p )                           : Write_stmt(p) {}
                              //Insert_stmt                 ( const string& table_name )                { set_table_name( table_name ); }

    const string                make_stmt                   ()                                          { return make_insert_stmt(); }
};

//--------------------------------------------------------------------------------------Update_stmt

struct Update_stmt : Write_stmt, Where_clause
{
                              //Update_stmt                 ()                                          {}
                                Update_stmt                 ( Database_descriptor* p, const string& table_name    ) : Write_stmt(p,table_name), Where_clause(p) {}
                                Update_stmt                 ( Database_descriptor* p, const char* table_name = "" ) : Write_stmt(p,table_name), Where_clause(p) {}
                                Update_stmt                 ( const Table_descriptor* );


  //void                        add_key                     ( const string& name )                      { _keys.insert( name ); }
  //void                        add_key                     ( const char* name )                        { _keys.insert( string(name) ); }

    const string                make_stmt                   ()                                          { return make_update_stmt(); }
    const string                make_update_stmt            ();
    const string                make_delete_stmt            ();

    virtual bool                is_update                   () const                                    { return true; }

  //typedef std::set<string>    Keys;
  //Keys                       _keys;
};

//--------------------------------------------------------------------------------------Delete_stmt

struct Delete_stmt : Stmt, Where_clause
{
                              //Delete_stmt                 ()                                          {}
                                Delete_stmt                 ( Database_descriptor* p, const string& table_name    ) : Stmt(p,table_name), Where_clause(p) {}
                                Delete_stmt                 ( Database_descriptor* p, const char* table_name = "" ) : Stmt(p,table_name), Where_clause(p) {}
                                Delete_stmt                 ( const Table_descriptor* p )                           : Stmt(p)           , Where_clause(p) {}


    const string                make_stmt                   ();
};

//-------------------------------------------------------------------------------------------------

} //namespace sql
} //namespace zschimmer

#endif
