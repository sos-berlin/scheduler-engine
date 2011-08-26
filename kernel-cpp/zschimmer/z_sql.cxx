// $Id: z_sql.cxx 13707 2008-10-17 10:41:41Z jz $

#include "zschimmer.h"
#include "z_sql.h"

namespace zschimmer {
namespace sql {

//-------------------------------------------------------------------------------------------static

static Message_code_text error_codes[] =
{
    { "Z-SQL-001", "Wert für Schlüssel '$1' für UPDATE-Statement nicht angegeben" },
    { "Z-SQL-002", "SQL-Operand lässt sich nicht als Zeichenkette darstellen" },
    {}
};

//--------------------------------------------------------------------------------------------const

extern const Value  null_value = Value();

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_sql )
{
    add_message_code_texts( error_codes );
}

//-----------------------------------------------------------------------------null_string_equation

string null_string_equation( const string& value )
{ 
    return value == ""? " is null" 
                      : "=" + quoted( value ); 
}

//---------------------------------------------------------------without_comments_regardless_quotes

string without_comments_regardless_quotes( const string& sql )
// "/*" in Strings wird nicht beachtet. 
// Wenn Strings beachtet werden: MySQL kennt /, die anderen, ANSI-konformen Datenbank nicht.
{
    string result;
    result.reserve( sql.size() );

    const char* p     = sql.c_str();
    const char* p_end = p + sql.length();

    assert( *p_end == '\0' );

    while( p < p_end )
    {
        const char* p0 = p;
        while( p < p_end  &&  ( p[0] != '/'  ||  p[1] != '*' ) )  p++;  // Bis "/*"
        assert( p > p_end - 2  ||  p[0] == '/' && p[1] == '*' );
        
        const char* q = p0;
        while( isspace( (unsigned char)*q ) )  q++;   
        if( q < p )                                   // Nur ausgeben, wenn nicht nur weißer Zwischenraum
            result.append( p0, p - p0 );       

        p += 2;
        while( p < p_end  &&  ( p[0] != '*'  ||  p[1] != '/' ) )  p++;  // Bis "*/"
        
        if( p <= p_end - 2 )
        {
            assert( p[0] == '*'  &&  p[1] == '/' );
            p += 2;
        }
    }

    assert( result.length() <= sql.length() );
    return result;
}

//---------------------------------------------------------------------------------------Value::set

void Value::set( const VARIANT& value )
{ 
    try
    {
        _value = com::string_from_variant( value );

        _kind = com::variant_is_numeric(value)? k_numeric :
                value.vt == VT_NULL           ? k_null :
                value.vt == VT_DATE           ? k_datetime :
                                                k_string;
    }
    catch( exception& x )
    {
        throw_xc( "Z-SQL-002", x.what() );
    }
}

//-------------------------------------------------------------------------------Value::sql_operand

string Value::sql_operand( const Database_descriptor& db_descr ) const
{ 
    switch( _kind )
    {
        case k_null:        return "NULL";
        case k_numeric:     return _value;
        case k_direct:      return _value;
        case k_datetime:    return _value == ""? "NULL" : db_descr.timestamp_string( _value );
        default:            return db_descr._write_empty_as_null && _value.empty()? "NULL" : quoted( _value );
    }
}

//----------------------------------------------------------atabase_descriptor::Database_descriptor

Database_descriptor::Database_descriptor( Flags flags )
: 
    _zero_(this+1),
    _uppercase_names               ( ( flags & flag_uppercase_names                ) != 0 ),
    _quote_names                   ( ( flags & flag_quote_names                    ) != 0 ),
    _write_empty_as_null           ( ( flags & flag_write_empty_as_null            ) != 0 ),
    _dont_quote_table_names        ( ( flags & flag_dont_quote_table_names         ) != 0 ),
    _use_simple_iso_datetime_string( ( flags & flag_use_simple_iso_datetime_string ) != 0 )
{
}

//-------------------------------------------------------------------Database_descriptor::make_name

string Database_descriptor::make_name( const string& name ) const
{
    string my_name = _uppercase_names? ucase( name ) : name;
    return _quote_names? quoted_name( my_name ) : my_name;
}

//-------------------------------------------------------------Database_descriptor::make_table_name

string Database_descriptor::make_table_name( const string& name ) const
{
    string my_name = _uppercase_names? ucase( name ) : name;
    return _quote_names  &&  !_dont_quote_table_names? quoted_name( my_name ) : my_name;
}

//------------------------------------------------------------------Database_descriptor::equal_expr

string Database_descriptor::equal_expr( const string& name, const string& value ) const
{
    return S() << make_name(name) << "=" << operand(value);
}

//------------------------------------------------------------------Database_descriptor::equal_expr

string Database_descriptor::equal_expr( const string& name, int64 value ) const
{
    return S() << make_name(name) << "=" << operand(value);
}

//------------------------------------------------------------------Database_descriptor::equal_expr

string Database_descriptor::equal_expr( const string& name, const VARIANT& value ) const
{
    if( value.vt == VT_NULL )  return make_name(name) + " IS NULL";

    return S() << make_name(name) << "=" << operand(value);
}

//------------------------------------------------------------------Database_descriptor::equal_expr

string Database_descriptor::equal_expr( const string& name, const Value& value ) const
{
    switch( value._kind )
    {
        case Value::k_null     : return make_name( name ) + " IS NULL";
        case Value::k_string   : return equal_expr( name, value._value );
        case Value::k_numeric  : return S() << make_name( name ) << "=" << value._value;
        case Value::k_direct   : return S() << make_name( name ) << "=" << value._value;
        case Value::k_datetime : return S() << make_name( name ) << "=" << timestamp_string( value._value );
        default                : throw_xc( Z_FUNCTION );
    }
}

//------------------------------------------------------------Database_descriptor::timestamp_string

string Database_descriptor::timestamp_string( const string& datetime ) const
{
    string result;

    if( _use_simple_iso_datetime_string )  result = quoted( datetime );
                                     else  result = "{ts" + quoted( datetime ) +  "}";

    return result;
}

//---------------------------------------------------------------------Database_descriptor::operand

string Database_descriptor::operand( const VARIANT& value ) const
{ 
    try
    {
        return com::variant_is_numeric(value)? com::string_from_variant(value) : 
               value.vt == VT_DATE           ? timestamp_string( com::string_from_variant(value) ) :
               value.vt == VT_NULL           ? "NULL"
                                             : quoted( com::string_from_variant(value) );
    }
    catch( exception& x )
    {
        throw_xc( "Z-SQL-002", x.what() );
    }
}

//---------------------------------------------------------------Table_descriptor::Table_descriptor

Table_descriptor::Table_descriptor( const Database_descriptor* p, const string& table_name, const string& key_names ) 
: 
    _zero_(this+1), 
    _database_descriptor(p)
{
    set_name( table_name );
    vector_split( ",", p->_uppercase_names? ucase(key_names) : key_names, 0, &_key_names );
}

//-----------------------------------------------------------------------Table_descriptor::set_name
    
void Table_descriptor::set_name( const string& name )
{
    _table_name = _database_descriptor->_uppercase_names? ucase( name ) : name;
}

//-----------------------------------------------------------------------Table_descriptor::sql_name
    
string Table_descriptor::sql_name() const
{
    string result = _table_name;
    
    if( !_database_descriptor->_dont_quote_table_names )  result = quoted_name( result );

    return result;
}

//-------------------------------------------------------------------------Table_descriptor::is_key

bool Table_descriptor::is_key( const string& name ) const
{
    string my_name = _database_descriptor->_uppercase_names? ucase(name) : name;

    Z_FOR_EACH_CONST( Key_names, _key_names, k )
    {
        if( *k == my_name )  return true;
    }

    return false;
}

//-----------------------------------------------------------------------Where_clause::where_string

string Where_clause::where_string()
{
    bool is_empty = true;
    S    result;

    result <<  "  where ";

    Z_FOR_EACH( Where_conditions, _where_conditions, it )  
    {
        if( !is_empty )  result << " and ";
        result << _database_descriptor->equal_expr( it->first, it->second );
        is_empty = false;
    }

    if( _where_tail != "" )  result << ' ' << _where_tail, is_empty = false;

  //if( !_not_empty_where_clause_allowed )
        if( is_empty )  z::throw_xc( Z_FUNCTION, "Empty where clause" );
    
    return result;
}

//----------------------------------------------------------------------------------Stmt::make_name
/*
string Stmt::make_name( const string& name ) const
{
    string my_name = _uppercase_names? ucase( name ) : name;
    return _quote_names? quoted_name( my_name ) : my_name;
}
*/
//---------------------------------------------------------------------------Write_stmt::operator[]

Value& Write_stmt::operator[] ( const string& name )
{ 
    return _database_descriptor->_uppercase_names? _data[ ucase(name) ]
                                      : _data[ name ]; 
}

//-------------------------------------------------------------------------Write_stmt::set_datetime

void Write_stmt::set_datetime( const string& name, const string& value ) 
{ 
    _data[ _database_descriptor->_uppercase_names? ucase(name) : name ].set_datetime( value ); 
}

//---------------------------------------------------------------------Write_stmt::make_insert_stmt

const string Write_stmt::make_insert_stmt() const
{
    S names;
    S values;

    Z_FOR_EACH_CONST( Map, _data, d )
    {
        string operand = d->second.sql_operand( *_database_descriptor );

        if( operand != "NULL" )
        {
            if( !names.empty() )  names << ',',  values << ',';
            names << make_name( d->first );
            values << operand;
        }
    }

    return S() << "INSERT into " << make_table_name( _table_name ) << " (" << names << ") values (" << values << ")";
}

//-------------------------------------------------------------------------Update_stmt::Update_stmt

Update_stmt::Update_stmt( const Table_descriptor* p )
: 
    Write_stmt( p ),
    Where_clause(p)
{
    //Z_FOR_EACH_CONST( Table_descriptor::Key_names, p->_key_names, k )  _keys.insert( *k );
}

//---------------------------------------------------------------------------Update_stmt::make_stmt

const string Update_stmt::make_update_stmt() 
{
    string settings;

    Z_FOR_EACH_CONST( Map, _data, d )
    {
        if( !_table_descriptor  ||  !_table_descriptor->is_key( d->first ) )
        {
            if( !settings.empty() )  settings += ",";
            
            settings += make_name( d->first );
            settings += "=";
            settings += d->second.sql_operand( *Stmt::_database_descriptor );
        }
    }

    if( is_where_empty() )
    {
        Z_FOR_EACH_CONST( Table_descriptor::Key_names, _table_descriptor->_key_names, k )  
        {
            Map::const_iterator it = _data.find( *k );
            if( it == _data.end() )  throw_xc( "Z-SQL-001", *k );
            and_where_condition( *k, it->second );
        }
    }

    return S() << "UPDATE " << make_table_name( _table_name ) << "  set " << settings << where_string();
}

//--------------------------------------------------------------------Update_stmt::make_delete_stmt

const string Update_stmt::make_delete_stmt()
{
    return S() << "DELETE from " << make_table_name( _table_name ) << where_string();
}

//---------------------------------------------------------------------------Delete_stmt::make_stmt

const string Delete_stmt::make_stmt()
{
    return S() << "DELETE from " << make_table_name( _table_name ) << where_string();
}

//-------------------------------------------------------------------------------------------------

} //namespace sql
} //namespace zschimmer
