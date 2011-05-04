#include "precomp.h"
//#define MODULE_NAME "typereg"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/sosstat.h"
#include "../kram/sosfield.h"
#include "../kram/stdfield.h"
#include "../kram/delphi.h"
#include "../kram/typereg.h"

using namespace std;
namespace sos {


Delphi_string_type delphi_string_type_30  ( 30 );
Delphi_string_type delphi_string_type_100 ( 100 );

DEFINE_SOS_STATIC_PTR( Type_register )

//----------------------------------------------------------------------------type_register_ptr

Type_register* type_register_ptr()
{
    Sos_static* s = sos_static_ptr();
    if( !s->_type_register )  {
        Sos_ptr<Type_register> tr = SOS_NEW_PTR( Type_register );
        s->_type_register = +tr;
    }

    return s->_type_register;
}

//------------------------------------------------------------------Type_register::Entry::Entry

Type_register::Entry::Entry()
{
}

//-----------------------------------------------------------------Type_register::Entry::~Entry

Type_register::Entry::~Entry()
{
}

//-----------------------------------------------------------------Type_register::Type_register

Type_register::Type_register()
:
    _zero_(this+1)
{
    _array.obj_const_name( "Type_register::_array" );

    add( &long_type             , "Long" );
    add( &int_type              , "Integer" );
    add( &int_type              , "Int" );
    add( &char_type             , "Character" );
    add( &char_type             , "Char" );
    add( &area_type             , "Area" );
    add( &delphi_string_type    , "Delphi_string" );
    add( &delphi_string_type_30 , "Delphi_string(30)" );
    add( &delphi_string_type_100, "Delphi_string(100)" );
  //add( &delphi_tdatetime_type , "Delphi_TDateTime" );
}

//----------------------------------------------------------------Type_register::~Type_register

Type_register::~Type_register()
{
}

//---------------------------------------------------------------------------Type_register::add

void Type_register::add( const Sos_ptr<Field_type>& field_type, const char* name )
{
    add( field_type, Sos_string( name ) );
}

//---------------------------------------------------------------------------Type_register::add

void Type_register::add( const Sos_ptr<Field_type>& field_type, const Sos_string& name )
{
    if( _type( c_str( name ) ) )  throw_xc( "SOS-1208", c_str( name ) );

    Entry* e = _array.add_empty();
    e->_name = name;
    e->_type = field_type;
}

//---------------------------------------------------------------------------Type_register::add

void Type_register::add( const Sos_ptr<Record_type>& type )
{
    add( +type, type->name() );
}

//--------------------------------------------------------------------------Type_register::type

Sos_ptr<Field_type> Type_register::type( const char* name )
{
    Sos_ptr<Field_type> t = _type( name );
    if( t )  return t;

    throw_xc( "SOS-1209", name );
    return t;
}

//-------------------------------------------------------------------------Type_register::_type

Sos_ptr<Field_type> Type_register::_type( const char* name )
{
    if( _last_used_index >= _array.first_index()  &&  _last_used_index <= _array.last_index() ) {
        if( field_names_are_equal( c_str( _array[ _last_used_index ]._name ), name ) )  return _array[ _last_used_index ]._type;
    }

    int i;
    for( i = _array.first_index(); i <= _array.last_index(); i++ ) {
        if( field_names_are_equal( name, c_str( _array[ i ]._name ) ) )  break;
    }

    if( i > _array.last_index() )  return 0;

    _last_used_index = i;
    return _array[ i ]._type;
}

//-------------------------------------------------------------------Type_register::record_type

Sos_ptr<Record_type> Type_register::record_type( const char* name )
{
    Sos_ptr<Field_type> t = type( name );
    //if( t->field_count() == 0 )  throw_xc( "SOS-1210", name );
    //return (Record_type*)+t;
    return SOS_CAST( Record_type, t );
}

} //namespace sos
