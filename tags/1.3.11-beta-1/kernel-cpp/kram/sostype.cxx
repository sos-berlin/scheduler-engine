#if 0

#define MODULE_NAME "sostype"
#define COPYRIGHT   "©1995 SOS GmbH Berlin"
#define AUTHOR      "Joacim Zschimmer"

#include <sos.h>
#include <sostype.h>

typedef unsigned long int ulong;

extern const Fundamental_type type_char     ( Fundamental_type::plain_char , sizeof (char)     );
extern const Fundamental_type type_int      ( Fundamental_type::plain_int  , sizeof (int)      );
extern const Fundamental_type type_int1     ( Fundamental_type::plain_int  , sizeof (int1)     );
extern const Fundamental_type type_int2     ( Fundamental_type::plain_int  , sizeof (int2)     );
extern const Fundamental_type type_int4     ( Fundamental_type::plain_int  , sizeof (int4)     );
extern const Fundamental_type type_long     ( Fundamental_type::plain_int  , sizeof (long)     );
extern const Fundamental_type type_uint     ( Fundamental_type::plain_int  , sizeof (uint)     );
extern const Fundamental_type type_uint1    ( Fundamental_type::plain_int  , sizeof (uint1)    );
extern const Fundamental_type type_uint2    ( Fundamental_type::plain_int  , sizeof (uint2)    );
extern const Fundamental_type type_uint4    ( Fundamental_type::plain_int  , sizeof (uint4)    );
extern const Fundamental_type type_ulong    ( Fundamental_type::plain_int  , sizeof (ulong)    );

///////////////////////////////////////////////////////////////////////////////////////////////

struct X : Sos_self_deleting
{
    const Sos_type&            _obj_type                () const;

  private:
    int                        _a;
    int                        _b;
};

struct Test : Sos_self_deleting
{
    const Sos_type&            _obj_type                () const;

  private:
    static const Struct_type   _type;

    int                        _integer;
    char                       _char;
    X                          _x;
};

const Struct_type Test::_type

void add_member( Struct_type*, const int* , const char* name, const char* ext_name = "" );
void add_member( Struct_type*, const char*, const char* name, const char* ext_name = "" );
void add_member( Struct_type*, const X*   , const char* name, const char* ext_name = "" );

template< class TYPE >
void add_member( Struct_type*, const char* name, const Sos_ptr<TYPE>* );

template< class TYPE >
void add_member( Struct_type*, const char* name, const TYPE* );  // NICHT DEFINIERT

#define TYPE_ADD_MEMBER    ( NAME )            add_member( t, &o->NAME, #NAME, #NAME )
#define TYPE_ADD_MEMBER_EXT( NAME, EXT_NAME )  add_member( t, &o->NAME, #NAME, #EXT_NAME )

void add_member( Struct_type* t, const int* offset, const char* name, const char* ext_name )
{
    t->add_member( Struct_member_decr( &type_int, (int)offset, name, ext_name ) );   // Record_type/Field_type?
}

void xx()
{
    Struct_type* t = &_type;
    const Test*  o = 0;

    TYPE_ADD_MEMBER_EXT( _integer, "xy_integer" );
    TYPE_ADD_MEMBER    ( _char   );
    TYPE_ADD_MEMBER    ( _x      );
}


const Sos_type& Test::_obj_type() const
{
    return _type;
}


