#include "precomp.h"
//#define MODULE_NAME "dynobj"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"


// Bei Objekten bis 8 Bytes eingebauten statt dynamisch angeforderten Speicher benutzen!
// Eigenschaft von Dynamic_area? Nein.

#include "../kram/sos.h"
#include "../kram/stdfield.h"
#include "dynobj.h"
#include "../kram/soslimtx.h"
#include "../kram/decimal.h"
#include "../kram/sosdate.h"
#include "../kram/log.h"

using namespace std;
namespace sos {

const Dyn_obj null_dyn_obj;


// Default-Typ, wenn Typ nur lesbare Werte beschreibt: (sollte dynamische Größe haben!)
static const int    std_size = 2048;  //Vergrößert für SAN 21.7.97   1024;
static String0_type std_dyn_obj_type ( std_size );   // Wenn !_field_copy_possible (rectab.cxx Field_type)

//---------------------------------------------------------------------------_Dyn_obj::_Dyn_obj

_Dyn_obj::_Dyn_obj()
:
    _ptr ( NULL )
{
}

//---------------------------------------------------------------------------_Dyn_obj::_Dyn_obj
/*
_Dyn_obj::_Dyn_obj( const _Dyn_obj& o )
:
    _ptr ( NULL )
{
}
*/
//--------------------------------------------------------------------------_Dyn_obj::~_Dyn_obj

_Dyn_obj::~_Dyn_obj()
{
    if( _ptr  &&  _ptr != (Byte*)&_small_buffer ) {
        sos_free( _ptr );
    }
    _ptr = NULL;
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj()
{
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const Dyn_obj& o )
{
    _assign( o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const Field_type* type )
{
    if( !type )  throw_xc( "SOS-1193", "Record" );

    _type = (Field_type*)type;
    construct();
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const Field_type* t, const Const_area& o )
{
    _assign( t, o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const Field_type* t, const void* o )
{
    _assign( t, o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const Field_descr* f, const void* o )
{
    _assign( f, o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( Big_int o )
{
    _assign( &big_int_type, &o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( int o )
{
    _assign( &int_type, &o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( double o )
{
    _assign( &double_type, &o );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj
#if defined SYSTEM_BOOL

Dyn_obj::Dyn_obj( Bool o )
{
    _assign( &bool_type, &o );
}

#endif
//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const char* o )
{
    //_assign( &const_string0_type, o );

    assign( o, strlen( o ) );
}

//-----------------------------------------------------------------------------Dyn_obj::Dyn_obj

Dyn_obj::Dyn_obj( const Sos_string& o )
{
    //_assign( &const_string0_type, c_str( o ) );

    Sos_ptr<String0_type> type = SOS_NEW( String0_type( length( o ) ) );
    _assign( type, c_str( o ) );
}

//----------------------------------------------------------------------------Dyn_obj::~Dyn_obj

Dyn_obj::~Dyn_obj()
{
    destruct();
}

//---------------------------------------------------------------------Dyn_obj::operator string

Dyn_obj::operator string() const
{
    Dynamic_area buffer;
    write_text( &buffer );
    return string( buffer.char_ptr(), length( buffer ) );
}

//-------------------------------------------------------------------------------Dyn_obj::alloc

void Dyn_obj::alloc( Field_type* type )
{
    _type = type;
    alloc();
}

//-------------------------------------------------------------------------------Dyn_obj::alloc

void Dyn_obj::alloc()
{
    uint size = _type->field_size();

    //if( size == 0 )  throw_xc( "Dyn_obj-size=0", _type );
    //SQLBindParameter setzt _field_size = cbValue, was 0 sein kann ???????????

    if( _ptr  &&  size > _size ) {
        if( _ptr != (Byte*)&_small_buffer )  sos_free( _ptr );
        _ptr = NULL;
    }

    _size = size;

    if( !_ptr ) {
        if( size <= sizeof _small_buffer )  {
            _ptr = (Byte*)&_small_buffer;
        } else {
            _ptr = (Byte*)sos_alloc( size, "Dyn_obj buffer" );
        }
    }
}

//---------------------------------------------------------------------------Dyn_obj::construct

void Dyn_obj::construct( Field_type* type )
{
    alloc( type );
    _type->construct( _ptr );
}

//---------------------------------------------------------------------------Dyn_obj::construct

void Dyn_obj::construct()
{
    alloc();
    _type->construct( _ptr );
}

//---------------------------------------------------------------------------Dyn_obj::_destruct

void Dyn_obj::_destruct()
{
    if( !_type )  return;

    _type->destruct( _ptr );
    _type = 0;
}

//--------------------------------------------------------------------------Dyn_obj::operator =

Dyn_obj& Dyn_obj::operator = ( const char* o )
{
    //_assign( &const_string0_type, o );

    Sos_ptr<String0_type> type = SOS_NEW( String0_type( length( o ) ) );
    _assign( type, c_str( o ) );

    return *this;
}

//--------------------------------------------------------------------------Dyn_obj::operator =

Dyn_obj& Dyn_obj::operator = ( const Sos_string& o )
{
    //_assign( &const_string0_type, c_str( o ) );

    Sos_ptr<String0_type> type = SOS_NEW( String0_type( length( o ) ) );
    _assign( type, c_str( o ) );

    return *this;
}

//-----------------------------------------------------------------------------Dyn_obj::_assign

void Dyn_obj::_assign( const Dyn_obj& o )
{
    _assign( o._type, o._ptr );
}

//------------------------------------------------------------------------------Dyn_obj::assign

void Dyn_obj::assign( const char* text, int len )
{
    Sos_ptr<String0_type> type = SOS_NEW( String0_type( len ) );
    //_assign( type, Const_area( text, len ) );
    _type = type;
    alloc();
    memcpy( _ptr, text, len );
    _ptr[ len ] = '\0';
}

//-----------------------------------------------------------------------------Dyn_obj::_assign

void Dyn_obj::_assign( const Field_type* type, const Const_area& value )
{
    //jz 15.4.97 destruct();

    if( type ) {
        if( value.length() != type->field_size() )  throw_xc( "SOS-DYN-OBJ-LENGTH" );
        _assign( type, value.byte_ptr() );
    }
}

//-----------------------------------------------------------------------------Dyn_obj::_assign

void Dyn_obj::_assign( const Field_type* type, const void* p )
{
    //jz 15.4.97  destruct();

    if( !type ) 
    {
        destruct();
    }
    else
    {
        if( !type->info()->_field_copy_possible )  
        {
            // Wir nehmen String0_type:
            if( +_type != &std_dyn_obj_type ) {
                destruct();
                _type = &std_dyn_obj_type;
                construct();
            }

            Area area ( _ptr, _type->field_size() - 1 );  // nicht vergrößerbar
            type->write_text( (const Byte*)p, &area );
            _ptr[ area.length() ] = '\0';
        } 
        else 
        {
            if( _type != type ) {
                destruct();
                _type = (Field_type*)type;
                construct();
            }

            type->field_copy( _ptr, (const Byte*)p );
        }
    }
}

//-----------------------------------------------------------------------------Dyn_obj::_assign

void Dyn_obj::_assign( const Field_descr* f, const void* p )
{
    if( !f  ||  f->has_null_flag()  &&  f->null_flag( (const Byte*)p ) ) {
        destruct();
        //null
    } else {
        _assign( f->type_ptr(), f->const_ptr( (const Byte*)p ) );
    }
}

//--------------------------------------------------------------------------Dyn_obj::write_text

void Dyn_obj::write_text( Area* result_buffer ) const
{
    result_buffer->length( 0 );

    if( !_type )  return;       // null

    _type->write_text( _ptr, result_buffer );
}

//---------------------------------------------------------------------------Dyn_obj::read_text

void Dyn_obj::read_text( const char* text )
{
    if( !_type )  throw_xc( "Dyn_obj::read_text", "NULL" );

    _type->read_text( _ptr, text );
}

//--------------------------------------------------------------------Dyn_obj::throw_null_error

void Dyn_obj::throw_null_error() const
{
    throw_xc( "Dyn_obj-NULL" );
}

//-----------------------------------------------------------------------------Dyn_obj::compare

int Dyn_obj::compare( const Dyn_obj& o ) const
{
    //if( _type == o._type  &&  _type.compare_implemented )  return _type->compare( o.ptr() );

    //LOG( "Dyn_obj::compare std_type=" << _type->info()->_std_type << ", " << o._type->info()->_std_type << "\n" );
                                                                                                                   
/*jz 16.4.97
    Type_param a_par;  _type->get_param( &a_par );
    Type_param b_par;  o._type->get_param( &b_par );

    if( ((    a_par._std_type == std_type_integer
           || a_par._std_type == std_type_decimal ) && a_par._scale == 0 )
     && ((    b_par._std_type == std_type_integer
           || b_par._std_type == std_type_decimal ) && b_par._scale == 0 ) )
*/
    if( (   (    _type->info()->_std_type == std_type_integer
              || _type->info()->_std_type == std_type_decimal ) 
         && _type->info()->_min_scale == 0 
         && _type->info()->_max_scale == 0 
        )
     && (   (    o._type->info()->_std_type == std_type_integer
              || o._type->info()->_std_type == std_type_decimal ) 
         && o._type->info()->_min_scale == 0 
         && o._type->info()->_max_scale == 0 
        ) )
    {
        Big_int a = as_big_int( *this );
        Big_int b = as_big_int( o );

        return a < b? -1  :  a > b? 1  :  0;
    }
    else
    if( _type->is_numeric()  ||  o._type->is_numeric() )        // jz 27.5.01 Fehrmann, Dia-Nielsen: Bei Record_type wird is_numeric() auf _group_type angewendet.
    {
        // Wenn ein Parameter numerisch ist, muss numerisch verglichen werden,
        // weil sonst "00" != "0" und "2" > "10".
        // Wenn ein Parameter nicht numerisch ist und auch keine Zahl enthält,
        // soll dann als Text verglichen werden? Hier gibt es einen Fehler und NULL ist das Ergebnis.
        double a = as_double( *this );
        double b = as_double( o );

        return a < b? -1  :  a > b? 1  :  0;
    }
    else
    if( (    _type->info()->_std_type == std_type_date
          || _type->info()->_std_type == std_type_date_time ) 
     && (    o._type->info()->_std_type == std_type_date
          || o._type->info()->_std_type == std_type_date_time ) )
    {
        // date und date_time sind nicht als Text miteinander vergleichbar.

        Sos_limited_text<100>   buffer;    
        Sos_optional_date_time  a, b;

        write_text( &buffer );     
        a.read_text(  c_str( buffer ) );

        o.write_text( &buffer );   
        b.read_text( c_str( buffer ) );

        return a.compare( b );
    }
    else
    {
        Dynamic_area    a_buffer;    
        Area*           a = NULL;  
        Area            a_area;     // Nicht Const_area, weil length verändert wird

        Dynamic_area    b_buffer;    
        Area*           b = NULL;  
        Area            b_area;     // Nicht Const_area, weil length verändert wird

        // Bei String0_type und Text_type ist der Wert bereits als Text gespeiochert und kann direkt verglichen werden:

        if( _type->info() == &String0_type::_type_info ) { 
            a_area = Area( (Byte*)ptr(), ((String0_type*)+_type)->length( ptr() ) ); 
            a = &a_area; 
        }
        
        if( _type->info() == &Text_type::_type_info ) { 
            a_area = Area( (Byte*)ptr(), length_without_trailing_spaces( (const char*)ptr(), _type->field_size() ) ); 
            a = &a_area; 
        }

        if( o._type->info() == &String0_type::_type_info ) { 
            b_area = Area( (Byte*)o.ptr(), ((String0_type*)+o._type)->length( o.ptr() ) ); 
            b = &b_area; 
        }
        
        if( o._type->info() == &Text_type::_type_info ) { 
            b_area = Area( (Byte*)o.ptr(), length_without_trailing_spaces( (const char*)o.ptr(), o._type->field_size() ) ); 
            b = &b_area; 
        }

        // Alle anderen Typen werden hier nach Text konvertiert:

        if( !a ) {
            a = &a_buffer;
            write_text( a );
        }

        if( !b ) {
            b = &b_buffer;
            o.write_text( b );
        }

        // sosodbc liefert bei SQLGetData (nicht bei SQlBindColumn/SQLFetch) statt Leerstring
        // einen String mit einem Blank, weil sonst
        // MS-ACCESS 2.0 durcheinanderkommt: ein Leerstring ist dort beim Aufbau der
        // UPDATE-Anweisung ein Leerstring, beim SQLBindParameter aber NULL.

        // wenn std_type_char mit std_type_varchar verglichen wird, dann werden vom
        // std_type_varchar die hängenden Blanks ignoriert (std_type_varchar wird zu std_type_char).

        // Wenn ein Feld CHAR ist, Länge der Felder durch Verkürzen einander anpassen:

        if( _type->info()->_std_type == std_type_char ) {
            if( a->length() > b->length()  /*&&  _type->info()->_std_type == std_type_char jz 15.4.97 ??*/ ) {
                a->length( b->length() +
                          length_without_trailing_spaces( a->char_ptr() + b->length(),
                                                          a->length() - b->length() ) );
            }
            else
            if( b->length() > a->length() ) {
                b->length( a->length() +
                           length_without_trailing_spaces( b->char_ptr() + a->length(),
                                                          b->length() - a->length() ) );
            }
        }

        return *a < *b? -1  :  *a > *b? 1  :  0;
    }
}

//-----------------------------------------------------------------------------calculate_double

void calculate_double( double a, double b, Dyn_obj::Operator op, double* result )
{
    double r;

    // double-Fehler abfangen! ==> Exception
    switch( op ) {
        case Dyn_obj::op_add     : r = a + b; break;
        case Dyn_obj::op_subtract: r = a - b; break;
        case Dyn_obj::op_multiply: r = a * b; break;
        case Dyn_obj::op_divide  : if( b == 0 )  if( a == 0 )  throw_null_error( "SOS-1330" );
                                                         else  throw_overflow_error();  // throw_divide_by_zero_error();
                                   r = a / b; break;
                         default : throw_xc( "Dyn_obj::calculate" );
    }

    *result = r;
}

//----------------------------------------------------------------------------calculate_decimal

void calculate_decimal( const Decimal& a, const Decimal& b, Dyn_obj::Operator op, Decimal* result )
{
    Decimal r;

    // Überlauf prüfen!!
    switch( op ) {
        case Dyn_obj::op_add     : r = a + b; break;
        case Dyn_obj::op_subtract: r = a - b; break;
        case Dyn_obj::op_multiply: r = a * b; break;
      //case Dyn_obj::op_divide  : r = a / b; break;
                         default : throw_xc( "Dyn_obj::calculate" );
    }

    *result = r;
}

//----------------------------------------------------------------------------calculate_big_int

void calculate_big_int( Big_int a, Big_int b, Dyn_obj::Operator op, Big_int* result )
{
    Big_int r;

    // Überlauf prüfen!!
    switch( op ) {
        case Dyn_obj::op_add     : r = a + b; break;
        case Dyn_obj::op_subtract: r = a - b; break;
        case Dyn_obj::op_multiply: r = a * b; break;
        case Dyn_obj::op_divide  : r = a / b; break;
                         default : throw_xc( "Dyn_obj::calculate" );
    }

    *result = r;
}

//---------------------------------------------------------------------------Dyn_obj::calculate

Dyn_obj Dyn_obj::calculate( const Dyn_obj& o, Operator op ) const
{
    ZERO_RETURN_VALUE( Dyn_obj );

    Dyn_obj result;
    calculate( o, op, &result );
    return result;
}

//---------------------------------------------------------------------------Dyn_obj::calculate

void Dyn_obj::calculate( const Dyn_obj& o, Operator op, Dyn_obj* result ) const
{
    if( null()  ||  o.null() )  { *result = null_dyn_obj; return; }

    Std_type a_std_type = _type->info()->_std_type;
    Std_type b_std_type = o._type->info()->_std_type;

    //if( is_numeric( a_std_type )  &&  is_numeric( b_std_type ) )
    {
        //jz 16.4.97 Type_param a_par;  _type->get_param( &a_par );
        //jz 16.4.97 Type_param b_par;  o._type->get_param( &b_par );

        try {
            if( a_std_type == std_type_float || b_std_type == std_type_float )
            {
                double r, a, b;

                try {
                    a = as_double( *this );
                }
                catch( const Null_error& )
                {
                    as_double( o );             // Löst Exception aus, wenn nicht konvertierbar
                    *result = null_dyn_obj;
                    return;
                }

                b = as_double( o );

                calculate_double( a, b, op, &r );
                *result = r;
            }
            else
            if( (    ( a_std_type == std_type_integer || a_std_type == std_type_decimal )
                  && _type->info()->_min_scale == 0 
                  && _type->info()->_max_scale == 0 
                )
            &&  (    ( b_std_type == std_type_integer || b_std_type == std_type_decimal )
                  && o._type->info()->_min_scale == 0 
                  && o._type->info()->_max_scale == 0 
              )  ) 

            {
                Big_int r, a, b;

                try {
                    a = as_big_int( *this );
                }
                catch( const Null_error& )
                {
                    as_big_int( o );          // Löst Exception aus, wenn nicht konvertierbar
                    *result = null_dyn_obj;
                    return;
                }

                b = as_big_int( o );

                if( op == op_divide  &&  b == 0 )  *result = null_dyn_obj;
                else {
                    calculate_big_int( a, b, op, &r );
                    *result = r;
                }
            }
            else
            if( ( a_std_type == std_type_date || a_std_type == std_type_date_time )
             && is_numeric( b_std_type )  
             &&  ( op == op_add || op == op_subtract ) )
            {
                Sos_limited_text<100> buffer;
                write_text( &buffer );
                Sos_optional_date_time dt = c_str( buffer );
                double b = as_double(o);
                dt.add_days_and_time( op == op_add? b : -b );
                *result = dt;
            }
            else
            {
                double r, a, b;

                try {
                    a = as_double( *this );
                }
                catch( const Null_error& )
                {
                    as_double( o );             // Löst Exception aus, wenn nicht konvertierbar
                    *result = null_dyn_obj;
                    return;
                }

                b = as_double( o );
                calculate_double( a, b, op, &r );
                *result = r;
            }
        }
        catch( const Null_error& )
        {
            *result = null_dyn_obj;
        }
    }
    //else
    //{
    //    //throw_xc( "Dyn_obj::calculate" );
    //    *result = null_dyn_obj;
    //}
}

//------------------------------------------------------------------------------Dyn_obj::negate

void Dyn_obj::negate()
{
    if( null() )  return;

    Std_type std_type = _type->info()->_std_type;

    if( std_type == std_type_integer 
     || std_type == std_type_decimal 
        && _type->info()->_min_scale == 0 
        && _type->info()->_max_scale == 0 )
    {
        *this = -as_big_int( *this );
    }
    else
    {
        *this = -as_double( *this );
    }
}

//--------------------------------------------------------------------------Dyn_obj::_obj_print

void Dyn_obj::print( ostream* s, char quote, char quote_quote ) const
{
    if( !_type  ||  _type->null( _ptr ) )  *s << "«NULL»";
                                     else  _type->print( _ptr, s, std_text_format, quote, quote_quote );
}

//--------------------------------------------------------------------------Dyn_obj::_obj_print

void Dyn_obj::_obj_print( ostream* s ) const
{
    print( s );
/*
    if( !_type  ||  _type->null( _ptr ) )  *s << "«NULL»";
  //else  _type->print( _ptr, s, std_text_format );
    else {
        Dynamic_area buffer ( max_display_field_size );
        buffer.length( 0 );
        _type->write_text( _ptr, &buffer, std_text_format );
        if( _type->info()->_quote ) {
            buffer += '\0';
            *s << quoted_string( buffer.char_ptr() );
        } else {
            *s << buffer;
        }
    }
*/
}

//----------------------------------------------------------------------------------operator <<

ostream& operator << ( ostream& s, const Dyn_obj& o )
{
    //o.type()->print( (const Byte*)o.ptr(), &s, raw_text_format );
    o._obj_print( &s );
    return s;
}

} //namespace sos
