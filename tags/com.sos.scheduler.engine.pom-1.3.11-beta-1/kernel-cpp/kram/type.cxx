#if 0

#define MODULE_NAME "type"
// type.cpp

#include <sysdep.h>
#if !defined SYSTEM_SOLARIS

#if defined SYSTEM_STARVIEW
#   if !defined( _SV_HXX )
#      include <sv.hxx>
#   endif
#endif

#include <assert.h>
#include <string.h>

#include <sos.h>
#include <xception.h>

#include <type.h>


typedef unsigned long int ulong;

extern const Type_fundamental type_char     ( Type_fundamental::plain_char , sizeof (char)     );
extern const Type_fundamental type_int      ( Type_fundamental::plain_int  , sizeof (int)      );
extern const Type_fundamental type_int1     ( Type_fundamental::plain_int  , sizeof (int1)     );
extern const Type_fundamental type_int2     ( Type_fundamental::plain_int  , sizeof (int2)     );
extern const Type_fundamental type_int4     ( Type_fundamental::plain_int  , sizeof (int4)     );
extern const Type_fundamental type_long     ( Type_fundamental::plain_int  , sizeof (long)     );
extern const Type_fundamental type_uint     ( Type_fundamental::plain_int  , sizeof (uint)     );
extern const Type_fundamental type_uint1    ( Type_fundamental::plain_int  , sizeof (uint1)    );
extern const Type_fundamental type_uint2    ( Type_fundamental::plain_int  , sizeof (uint2)    );
extern const Type_fundamental type_uint4    ( Type_fundamental::plain_int  , sizeof (uint4)    );
extern const Type_fundamental type_ulong    ( Type_fundamental::plain_int  , sizeof (ulong)    );
//extern const Type_fundamental type_float    ( Type_fundamental::plain_float, sizeof (float)    );


const int max_typed_objects = 8000;

struct Typed_object_descr
{
    Typed_object_descr()                               : _object_ptr( 0 ), _type_ptr( 0 )  {}
    Typed_object_descr( const void* o, const Type* t ) : _object_ptr( o ), _type_ptr( t )  {}

    const Type& type() const { return *_type_ptr; }
    const Type* type_ptr() const { return _type_ptr; }
    const void* object_ptr() const { return _object_ptr; }

  //private:
    const void*  _object_ptr;
    const Type*  _type_ptr;
};

#if defined SYSTEM_STARVIEW

struct Object_window_impl : Object_window, WorkWindow
{
                 Object_window_impl( Window*, const Typed_object_descr& object_descr );
                ~Object_window_impl();
            void refresh      ();

  protected:
    virtual void Paint        ( const Rectangle& );
    virtual void Resize       ();
//          Size max_output_area_size();

  private:
    Typed_object_descr    _object_descr;
    Bool                  _resized;
};

#endif

//---------------------------------------------------------------------------------------------Type

Type::Type( const char* name_ptr, Kind kind ) : _name_ptr( name_ptr ), _kind( kind ) {}

//---------------------------------------------------------------Type_fundamental::Type_fundamental

Type_fundamental::Type_fundamental( Plain_type plain_type, int size, Bool is_signed )
  : Type        ( "", kind_fundamental ),
    _plain_type ( plain_type           ),
    _size       ( size                 ),
    _signed     ( is_signed            )
{}

//---------------------------------------------------------------------------Type_fundamental::size

int Type_fundamental::size() const
{
    return _size;
}

//--------------------------------------------------------------------Type_fundamental::print_value

void Type_fundamental::print_value( ostream& s, const void* object_ptr ) const
{
    s.setf( ios::uppercase );
    s << hex;
    switch( _plain_type ) {
        case plain_char : s << *(char *) object_ptr;  break;
        case plain_int  : switch( size() ) {
                              case 1: if( is_signed() )  s << (int) *( int1*) object_ptr;
                                                   else  s << (uint) *(uint1*) object_ptr;  break;
                              case 2: if( is_signed() )  s << *( int2*) object_ptr;
                                                   else  s << *(uint2*) object_ptr;  break;
                              case 4: if( is_signed() )  s << *( int4*) object_ptr;
                                                   else  s << *(uint4*) object_ptr;  break;
                            //case 8: if( is_signed() )  s << *( int8*) object_ptr;
                            //                     else  s << *(uint8*) object_ptr;  break;
                              default: s << "<unknown size " << size() << '>';
                          }
      //case plain_float: s << *(float*) object_ptr;  break;
    }
}
     
//--------------------------------------------------------------------Type_fundamental::input_value
#ifndef NO_INPUT

void Type_fundamental::input_value( istream& s, void* object_ptr ) const
{
    switch( _plain_type ) {
        case plain_char : {
                              int c = s.get();
                              if( c == '\'' ) {
                                  c = s.get();
                                  if( s.get() != '\'' )  raise("SYNTAX","SYNTAX");
                              }
                              if( c == EOF )  raise("SYNTAX","syneof");
                              *(char*)object_ptr = (char)c;
                          }

        case plain_int  : {
                              uint4 n;

                              if( is_signed() ) {
                                  int  sign = 1;
                                  int4 number = 0;

                                  if( s.peek() == '+' ) { s.get(); }
											 else
                                  if( s.peek() == '-' ) { s.get(); sign = -1; }

                                  while( s.peek() >= '0'  &&  s.peek() <= '9' ) {
                                      number = 10*number + s.get() - '0';
                                  } 

                                  n = (uint4) (number * sign);
                              } else {
                                  n = 0;
                                  while( s.peek() >= '0'  && s.peek() <= '9' ) {
                                      n = 10*n + s.get() - '0';
                                  } 
                              }

                              switch( size() ) {
                                  case 1: *(uint1*)object_ptr = n; break;
                                  case 2: *(uint2*)object_ptr = n; break;
                                  case 4: *(uint4*)object_ptr = n; break;
                                //case 8: *(uint8*)object_ptr = n; break;
                              }
                          }
                          break;
	 }

  exceptions
}

#endif     
//---------------------------------------------------------------------------Type_array::Type_array

Type_array::Type_array( const Type* type_ptr, int no_of_elements, int element_distance )
  : Type( "", kind_array ),
    _type_ptr        ( type_ptr ),
    _no_of_elements  ( no_of_elements ),
    _element_distance( element_distance? element_distance : type_ptr->size() )
{
    assert( ("Nicht genug Speicher für Programmstart", _type_ptr) );
}

//--------------------------------------------------------------------------Type_array::print_value

void Type_array::print_value( ostream& s, const void* object_ptr ) const
{
    s << '{';

	 for( int i = 0; i < no_of_elements(); i++ )
    {
        if (i > 0) s << ',';
        _type_ptr->print_value( s, object_ptr );
        (char*) object_ptr += _element_distance;
    }

    s << '}';
}

//---------------------------------------------------------------------------------Type_array::size

int Type_array::size() const
{
    return _no_of_elements * _type_ptr->size();
}

//-----------------------------------------------------------------------Type_string0::Type_string0

Type_string0::Type_string0( int size )
 :  Type( "", kind_string0 ),
    _size( size )
{}

//---------------------------------------------------------------------------------Type_array::size

int Type_string0::size() const { return sizeof (void*); }

//--------------------------------------------------------------------------Type_array::print_value

void Type_string0::print_value( ostream& s, const void* object_ptr ) const
{
    s << '\"';
    for (int i = 0; i < _size; i++)
    {
        uchar c = *((char*) object_ptr)++;
        if (!c)  break;
        if( c >= ' ' ) {
            if( c == '"' ) {
                s << "\\\"";
            } else {
                s << c;
            }
        } else {
            switch( c ) {
                case '\a': s << "\\a"; break;
					 case '\b': s << "\\b"; break;
                case '\t': s << "\\t"; break;
                case '\r': s << "\\r"; break;
                case '\n': s << "\\n"; break;
                case '\f': s << "\\f"; break;
                case '\e': s << "\\e"; break;
                  default: const char hex [ 16+1 ] = "0123456789ABCDEF";
                           s << '\\' << 'x' << hex[ c >> 4 ] << hex [ c & 0x0F ];
                           "Hier fehlt noch ein Trenner zum nächsten Buchstaben / Oder eigene Syntax?";
            }
        }
    }
    s << '\"';
    if (i == _size)  s << "...";
}

//--------------------------------------------------------------------------Type_array::input_value
#ifndef NO_INPUT

void Type_string0::input_value( istream& s, void* object_ptr ) const
{
    Bool  quoted = false;
    char* p      = (char*)object_ptr;

    if( s.peek() == '"' ) {
        s.get();
        quoted = true;
    }

    Bool  complete = false;

    for( int i = 0; i < _size - 1; i++ )
    {
        if( quoted ) {
            if( s.eof() )  raise( "SYNTAX", "syneof" );
            if( s.peek() == '"' ) {
                s.get();
                complete = true; break;
            }
        } else {
            if( s.eof()  ||  s.peek() <= ' '  ||  s.peek() == ';' )  { complete = true; break; }
        }

        int c = s.get();
        if( c == EOF )  raise( "SYNTAX", "syneof" );

		  *p++ = c;
    }

    *p = 0;     // Abschließendes null-byte

    if( !complete )  raise( "TRUNCATE", "syntrunc" );

  exceptions
    *p = 0;     // Abschließendes null-byte
}

#endif
//------------------------------------------------------------------------------------Type_function
//-----------------------------------------------------------------------------------Type_reference
//-------------------------------------------------------------------------------------Type_pointer

Type_pointer::Type_pointer( const Type* type_ptr )
  : Type( "", kind_array ),
    _type_ptr( type_ptr )
{
    assert( _type_ptr );
}

int Type_pointer::size() const { return sizeof (void*); }

void Type_pointer::print_value( ostream& s, const void* object_ptr ) const
{
    s << hex << (void*)*(const void**)object_ptr << dec << "->...";
    //_type_ptr->print_value( s, *(void**)object_ptr );
}

//---------------------------------------------------------------------------------------Type_const
//--------------------------------------------------------------------------------------Field_descr

Field_descr::Field_descr( const char* name, const Type* type_ptr )
 :
    _name( name ),
    _type_ptr( type_ptr )
{}

const char* Field_descr::name() const { return _name;      }
const Type& Field_descr::type() const { return *_type_ptr; }

//-------------------------------------------------------------------------------------Member_descr

Member_descr::Member_descr( const char* name, const Type* type_ptr, int offset )
 :
     Field_descr( name, type_ptr ),
     _offset( offset )
{}

//--------------------------------------------------------------------------------------Type_struct

Type_struct::Type_struct(
    const char* name_ptr,
    int size,
    const Member_descr member_descr[],
    int no_of_members
)
  : Type( name_ptr, kind_struct ),
    _size( size ),
    _no_of_members( no_of_members ),
    _member_descr( member_descr )  
{}

const Member_descr& Type_struct::member_descr( int i )  const
{
    assert( (uint) i < no_of_members() );
    return _member_descr[ i ];
}

int Type_struct::size() const
{
    return _size;
}

void Type_struct::print_value( ostream& s, const void* object_ptr ) const
{
    s << "{";
    for (int i = 0; i < no_of_members(); i++ )
    {
        if (i > 0)  s << ',';
        s << member_descr(i).name() << '=';
        member_descr(i).type().print_value( s, (char*)object_ptr + member_descr(i).offset() );
    }
    s << '}';
}

//-------------------------------------------------------------------------------------Type_pointer

Type_owner_pointer::Type_owner_pointer( const Type* type_ptr )
  : Type_pointer( type_ptr )
{}

int Type_owner_pointer::size() const { return sizeof (void*); }

void Type_owner_pointer::print_value( ostream& s, const void* object_ptr ) const
{
    s << hex << (void*) *(const void**)object_ptr << dec << "->";
    if( *(const void**)object_ptr ) {
        // _type_ptr->print_value( s, *(const void**)object_ptr );
    } else {
        s << '|';
    }
}

//--------------------------------------------------------------------Object_window::~Object_window
#if defined( SYSTEM_STARVIEW )

Object_window::~Object_window()
{}

//----------------------------------------------------------------------------Object_window::create

Object_window* Object_window::create( Window* window_ptr, void* object_ptr, Type* type_ptr )
{
    return new Object_window_impl( window_ptr, Typed_object_descr( object_ptr, type_ptr ) );
}

//-----------------------------------------------------------Object_window_impl::Object_window_impl

Object_window_impl::Object_window_impl( Window* parent_window_ptr, const Typed_object_descr& object_descr )
 :  WorkWindow( parent_window_ptr, WB_STDWORK ),
    _object_descr( object_descr ),
	 _resized     ( false     )
{
    Font font;
    font.ChangePitch( PITCH_FIXED );
    ChangeFont( font );

    ChangePosPixel( Point( 305,30 ) );
    Show();
}

//----------------------------------------------------------Object_window_impl::~Object_window_impl

Object_window_impl::~Object_window_impl()  {}

//-----------------------------------------------------------------------Object_window_impl::Resize

void Object_window_impl::Resize()
{
    _resized = true;
    WorkWindow::Resize();
}

//------------------------------------------------------------------------Object_window_impl::Paint

void Object_window_impl::Paint( const Rectangle& rectangle )
{
    Size        size;
    const int   window_width    = GetOutputSizePixel().Width();
    const int   pitch           = 8;
    const int   line_height     = 15;

    SetText( _object_descr.type().name() );

    switch( _object_descr.type().kind() ) 
    {
    case Type::kind_struct: {
        const Type_struct* t = (const Type_struct*) &_object_descr.type();
        int                max_width = 0;
        int                max_name_length = 0;
        int                i;

        for( i = 0; i < t->no_of_members(); i++ )
        {
            int l = strlen( t->member_descr( i ).name() );
            if( t->member_descr(i).type().kind() == Type::kind_array ) {
                char buffer [ 20 ];
					 ostrstream s ( buffer, sizeof buffer );
                s << ( ((const Type_array*)t)->no_of_elements() - 1 );
                l += 1 + s.pcount() + 1;
            }
            if( l > max_name_length )  max_name_length = l;
        }

        int line_no = 0;
        for( i = 0; i < t->no_of_members(); i++ )
        {
            if( t->member_descr(i).type().kind() == Type::kind_array ) {
                const Type_array* ta         = (const Type_array*) &t->member_descr(i).type();
                const Type*       te         = &ta->element_type();
                const void*       object_ptr = (char*)_object_descr.object_ptr() + t->member_descr(i).offset();

                for( int j = 0; j < ta->no_of_elements(); j++ ) {
                    char buffer [ 200 ];
                    ostrstream s ( buffer, sizeof buffer - 1 );
                    s << t->member_descr(i).name() << '[' << j << ']';
                    for( int k = s.pcount(); k < max_name_length; k++ )  s << ' ';
                    s << " = ";
                    te->print_value( s, object_ptr );
                    buffer[ s.pcount() ] = 0;
						  if( s.pcount() > max_width )  max_width = s.pcount();
                    while( s.pcount() < window_width ) s << ' ';
                    DrawText( Point( 0, line_no++ * line_height ), buffer );
                    (char*)object_ptr += ta->element_distance();
                }
            } else {
                char buffer [ 200 ];
                ostrstream s ( buffer, sizeof buffer - 1 );
                s << t->member_descr(i).name();
                for( int j = s.pcount(); j < max_name_length; j++ )  s << ' ';
                s << " = ";
                t->member_descr(i).type().print_value( s, (char*)_object_descr.object_ptr() + t->member_descr(i).offset() );
                buffer[ s.pcount() ] = 0;
                if( s.pcount() > max_width )  max_width = s.pcount();
                while( s.pcount() < window_width ) s << ' ';
                DrawText( Point( 0, line_no++ * line_height ), buffer );
            }
        }
        size = Size( max_width * pitch, line_no * line_height );
        break;
    }
    default: {
        char buffer [ 200 ];
		  ostrstream s ( buffer, sizeof buffer - 1 );
        _object_descr.type().print_value( s, _object_descr.object_ptr() );
        buffer[ s.pcount() ] = 0;
        while( s.pcount() < window_width ) s << ' ';
        DrawText( Point( 0, 0 ), buffer );
        size = Size( 1 * pitch, line_height );
    }}

    if( ! _resized ) {
		  ChangeOutputSizePixel( size );
    }
}

//---------------------------------------------------------------------------Object_window_impl::refresh

void Object_window_impl::refresh()
{
    //Paint( Rectangle( Point( 0, 0 ), Point( 32767, 32767 ) ) );
    InvalidateForeground();
}

#endif
//--------------------------------------------------------------------------------Typed_not_virtual

Typed_object_descr* /*Typed_object_descr::*/_objects		  = 0;
int                 /*Typed_object_descr::*/_no_of_objects = 0;

//--------------------------------------------------------------------------Typed_not_virtual::dump

void Typed_not_virtual::dump_all_objects( ostream& s )
{
	 int i;

	 for( i = 0; i < _no_of_objects; i++ ) {
		  if( _objects[ i ].object_ptr() ) {
				Typed_object_descr* p = _objects + i;
				s << (void*)p->object_ptr();
				if( *p->type_ptr()->name() ) {
					 s << ": " << p->type_ptr()->name();
				}
				s << ' ';
				p->type_ptr()->print_value( s, p->object_ptr() );
				s << '\n';
		  }
	 }
}

//--------------------------------------------------------------------------------------------Typed
#if 0

Typed* Typed::_object_list = 0;

//-------------------------------------------------------------------------------------Typed::Typed

Typed::Typed()
{
    _next_typed_object = _object_list;
    _object_list = this;
}

//------------------------------------------------------------------------------------Typed::~Typed

Typed::~Typed()
{
    Typed* p    = _object_list;
    Typed* prev = 0;

	 while( p  &&  p != this ) {
         prev = p;
         p = p->_next_typed_object;
    }

    assert( p );

    if( prev ) {
        prev->_next_typed_object = p->_next_typed_object;
    } else {
        _object_list = _next_typed_object;
	 }
}

#endif

//-------------------------------------------------------------------------------Typed::object_show
#if defined SYSTEM_STARVIEW

Object_window* Typed::object_show( const void* object_ptr, const Type* type_ptr )
{
    return new Object_window_impl( NULL/*parent_window_ptr*/, Typed_object_descr( object_ptr, type_ptr ));
}

#endif
//-------------------------------------------------------------------------------Typed::object_show
#if defined SYSTEM_STARVIEW

Object_window* Typed::object_show() const
{
    return object_show( this, &this->object_type() );
}

#endif
//--------------------------------------------------------------------------Typed::dump_all_objects

void Typed::dump_all_objects( ostream& s )
{
#if 1
    Typed_not_virtual::dump_all_objects( s );
#else
    Typed* p = _object_list;

    while( p ) {
        s << (void*)p;
        if( *p->type().name() ) {
				s << ": " << p->type().name();
        } 
        s << ' ' << *p << '\n';
        p = p->_next_typed_object;
    }
#endif
}

//-----------------------------------------------------------------------------Typed::insert_object

void Typed::insert_object( const void* object_ptr, const Type* type_ptr )
{
	 int i = 0;

	 if( !_objects ) {
		  _objects = new Typed_object_descr [ max_typed_objects ];
		  if( !_objects )  return;
	 }

	 while ( i < max_typed_objects  &&  _objects[ i ].object_ptr() )  i++;

	 if( i >= max_typed_objects ) {
		  // Zu viele Objekte
	 } else {
		  _objects[ i ]._object_ptr = object_ptr;
		  _objects[ i ]._type_ptr   = type_ptr;

		  if( i + 1 > _no_of_objects ) {
				_no_of_objects = i + 1;
		  }
	 }
}

//-----------------------------------------------------------------------------Typed::remove_object

void Typed::remove_object( const void* object_ptr )
{
    int i = 0;

    while ( i < _no_of_objects  &&  _objects[ i ].object_ptr() != object_ptr )  i++;

    if( i >= _no_of_objects ) {
        // Objekt ist nicht in der Liste enthalten!?
    } else {
        _objects[ i ]._object_ptr = 0;
		  _objects[ i ]._type_ptr   = 0;
    }
}

#endif

#endif

