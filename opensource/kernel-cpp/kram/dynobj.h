// dynobj.h                                             ©1996 SOS GmbH Berlin, Joacim Zschimmer

#ifndef __DYNOBJ_H
#define __DYNOBJ_H

#include "sosdate.h"

#ifndef __STDFIELD_H
#   include "../kram/stdfield.h"
#endif

namespace sos
{

struct _Dyn_obj : Sos_self_deleting  // zur Code-Ersparnis bei den vielen Dyn_obj-Konstruktoren
{                                    // möglicherweise auf Kosten der Laufzeit (bei Borland)
                                _Dyn_obj                ();
                               ~_Dyn_obj                ();

//protected:
    Sos_ptr<Field_type>        _type;
  //Dynamic_area               _buffer;
    Byte*                      _ptr;                    // wenn _ptr != &_small_buffer: sos_alloc
    uint                       _size;                   // wenn _ptr != &_small_buffer
    double                     _small_buffer;

  private:
                                _Dyn_obj                ( const _Dyn_obj& );
    void                        operator =              ( const _Dyn_obj& );
};


struct Dyn_obj : /*private*/ _Dyn_obj
{

    enum Operator
    {
        op_none,
        op_add,
        op_subtract,
        op_multiply,
        op_divide
    };

                                Dyn_obj                 ();
                                Dyn_obj                 ( const Dyn_obj& );
                                Dyn_obj                 ( const Field_type* );
                                Dyn_obj                 ( const Field_type*, const Const_area& );
                                Dyn_obj                 ( const Field_type*, const void* );
                                Dyn_obj                 ( const Field_descr*, const void* );
                                Dyn_obj                 ( int );
#   if defined SYSTEM_BOOL
                                Dyn_obj                 ( Bool );
#   endif
                                Dyn_obj                 ( Big_int );
                                Dyn_obj                 ( double );
                                Dyn_obj                 ( const char* );
                                Dyn_obj                 ( const Sos_string& );
                               ~Dyn_obj                 ();

    Dyn_obj&                    operator =              ( const Dyn_obj& o )                    { _assign( o ); return *this; }

#   if defined SYSTEM_BOOL
        Dyn_obj&                operator =              ( Bool o )                              { _assign( &bool_type, &o ); return *this; }
#   endif

#   if defined SYSTEM_BIG_INT
        Dyn_obj&                operator =              ( Big_int o )                           { _assign( &big_int_type, &o ); return *this; }
#   endif

    Dyn_obj&                    operator =              ( long o )                              { _assign( &long_type, &o ); return *this; }
    Dyn_obj&                    operator =              ( int o )                               { _assign( &int_type, &o ); return *this; }
    Dyn_obj&                    operator =              ( double o )                            { _assign( &double_type, &o ); return *this; }
    Dyn_obj&                    operator =              ( const char* );
    Dyn_obj&                    operator =              ( const Sos_string& );
    Dyn_obj&                    operator =              ( const Sos_optional_date_time& dt )    { _assign( &sos_optional_date_time_type, &dt ); return *this; }

                                operator string         () const;
    Const_area                  const_area              () const                                { return Const_area( _ptr, _size ); }

    void                        alloc                   ( Field_type* );
    void                        alloc                   ();
    void                        construct               ( Field_type* );
    void                        construct               ();
    void                        destruct                ()                                      { if( _type )  _destruct(); }

    Bool                        operator <              ( const Dyn_obj& o ) const              { return compare( o ) <  0; }
    Bool                        operator <=             ( const Dyn_obj& o ) const              { return compare( o ) <= 0; }
    Bool                        operator ==             ( const Dyn_obj& o ) const              { return compare( o ) == 0; }
    Bool                        operator !=             ( const Dyn_obj& o ) const              { return compare( o ) != 0; }
    Bool                        operator >=             ( const Dyn_obj& o ) const              { return compare( o ) >= 0; }
    Bool                        operator >              ( const Dyn_obj& o ) const              { return compare( o ) >  0; }
    int                         compare                 ( const Dyn_obj& o ) const;

    void                        operator +=             ( const Dyn_obj& o )                    { calculate( o, op_add     , this ); }  // *this = *this + o; }
    void                        operator -=             ( const Dyn_obj& o )                    { calculate( o, op_subtract, this ); }  // *this = *this - o; }
    void                        operator *=             ( const Dyn_obj& o )                    { calculate( o, op_multiply, this ); }  // *this = *this * o; }
    void                        operator /=             ( const Dyn_obj& o )                    { calculate( o, op_divide  , this ); }  // *this = *this / o; }

    Dyn_obj                     operator +              ( const Dyn_obj& o ) const              { return calculate( o, op_add      ); }
    Dyn_obj                     operator -              ( const Dyn_obj& o ) const              { return calculate( o, op_subtract ); }
    Dyn_obj                     operator *              ( const Dyn_obj& o ) const              { return calculate( o, op_multiply ); }
    Dyn_obj                     operator /              ( const Dyn_obj& o ) const              { return calculate( o, op_divide   ); }

    Dyn_obj                     operator -              () const                                { Dyn_obj o = *this; o.negate(); return o; }
    void                        negate                  ();

    Dyn_obj                     calculate               ( const Dyn_obj&, Operator ) const;
    void                        calculate               ( const Dyn_obj&, Operator op, Dyn_obj* result ) const;

    Bool                        null                    () const                                { return _type? _type->null( _ptr ) : true; }
    void                        set_null                ()                                      { _type = NULL; }
    void                        write_text              ( Area* ) const;
    void                        read_text               ( const char* );

    const Sos_ptr<Field_type>&  type                    () const                                { return _type; }
    const Byte*                 ptr                     () const                                { return _ptr; }

    void                        throw_null_error        () const;
    void                        check_not_null          () const;

    void                        assign                  ( const char*, int );
    void                        assign                  ( const Field_type* t, const Const_area& o ) { _assign( t, o ); }
    void                        assign                  ( const Field_type* t, const void* o )       { _assign( t, o ); }
    void                        assign                  ( const Field_descr* f, const void* o )      { _assign( f, o ); }

 //?Record_field&               operator []             ( const string& field )                 

    bool                        has_field               ( const string& field ) const               { return _type->field_descr_by_name_or_0( field ) != NULL; }
    Bool                        null                    ( int field ) const                         { return _type->field_descr_ptr(field)->null(_ptr); }
    Bool                        null                    ( const char* field ) const                 { return _type->field_descr_ptr(field)->null(_ptr); }

    int                         as_int                  ( const int     field ) const               { return _type->field_descr_ptr(field)->as_int(_ptr); }
    int                         as_int                  ( const char*   field ) const               { return _type->field_descr_ptr(field)->as_int(_ptr); }
    int                         as_int                  ( const string& field ) const               { return _type->field_descr_ptr(field)->as_int(_ptr); }

    int64                       as_int64                ( const int     field ) const               { return _type->field_descr_ptr(field)->as_int64(_ptr); }
    int64                       as_int64                ( const char*   field ) const               { return _type->field_descr_ptr(field)->as_int64(_ptr); }
    int64                       as_int64                ( const string& field ) const               { return _type->field_descr_ptr(field)->as_int64(_ptr); }

    double                      as_double               ( const int     field ) const               { return _type->field_descr_ptr(field)->as_double(_ptr); }
    double                      as_double               ( const string& field ) const               { return _type->field_descr_ptr(field)->as_double(_ptr); }

    string                      as_string               ( const int     field ) const               { return _type->field_descr_ptr(field)->as_string(_ptr); }
    string                      as_string               ( const char*   field ) const               { return _type->field_descr_ptr(field)->as_string(_ptr); }
    string                      as_string               ( const string& field ) const               { return _type->field_descr_ptr(field)->as_string(_ptr); }

    void                        set_field               ( const int     field, int v )              { _type->field_descr_ptr(field)->read_text( _ptr, sos::as_string(v).c_str() ); }
    void                        set_field               ( const char*   field, int v )              { _type->field_descr_ptr(field)->read_text( _ptr, sos::as_string(v).c_str() ); }
    void                        set_field               ( const int     field, const string& v )    { _type->field_descr_ptr(field)->read_text( _ptr, v.c_str() ); }
    void                        set_field               ( const char*   field, const string& v )    { _type->field_descr_ptr(field)->read_text( _ptr, v.c_str() ); }
    void                        set_field               ( const string& field, const string& v )    { _type->field_descr_ptr(field)->read_text( _ptr, v.c_str() ); }
    void                        set_field_null          ( const int     field )                     { _type->field_descr_ptr(field)->set_null ( _ptr ); }
    void                        set_field_null          ( const string& field )                     { _type->field_descr_ptr(field)->set_null ( _ptr ); }

/*
    template< class FIELD_NAME_TYPE, class VALUE_TYPE >
    void                        set_field               ( const FIELD_NAME_TYPE& field, const VALUE_TYPE& v )  { set_record_field( this, field, v ); }
  //void                        set_field               ( const FIELD_NAME_TYPE& field, const VALUE_TYPE& v )  { _type->field_descr_ptr(field)->read_text( _ptr, sos::as_string(v).c_str() ); }

    template< class FIELD_NAME_TYPE >
    void                        set_field               ( const FIELD_NAME_TYPE& field, const string& v )      { _type->field_descr_ptr(field)->read_text( _ptr, v.c_str() ); }
*/

    template< class FIELD_NAME_TYPE, class VALUE_TYPE >
    friend void                 set_record_field        ( Dyn_obj*, const FIELD_NAME_TYPE&, const VALUE_TYPE& );

    void                        print                   ( ::std::ostream*, char quote = '\0', char quote_quote = '\0' ) const;
    friend ::std::ostream&      operator <<             ( ::std::ostream&, const Dyn_obj& );

  protected:
    void                       _obj_print               ( ::std::ostream* ) const;

  private:
    void                       _destruct                ();
    void                       _assign                  ( const Field_type*, const Const_area& );
    void                       _assign                  ( const Field_type*, const void* );
    void                       _assign                  ( const Field_descr*, const void* );
    void                       _assign                  ( const Dyn_obj& );
};

typedef Dyn_obj Record;

/*
// Microsoft-Compiler verwechsel as_string() mit Dyn_obj::as_string(). Deshalb so:
template< class FIELD_NAME_TYPE, class VALUE_TYPE >
void set_record_field( Record* record, const FIELD_NAME_TYPE& field, const VALUE_TYPE& v )  
{ 
    record->_type->field_descr_ptr(field)->read_text( record->_ptr, as_string(v).c_str() ); 
}
*/

extern const Dyn_obj null_dyn_obj;




/*
inline Dyn_obj dyn_obj( int o )
{
    return Dyn_obj( int_type, &o );
}

inline Dyn_obj dyn_obj( const char* o )
{
    return Dyn_obj( const_string0_type, &o );
}
*/

inline void Dyn_obj::check_not_null() const
{
    if( !_type )  throw_null_error();
}

inline Bool              as_bool     ( const Dyn_obj& o ) { o.check_not_null(); return as_bool     ( o.type(), o.ptr() ); }
inline int               as_int      ( const Dyn_obj& o ) { o.check_not_null(); return as_int      ( o.type(), o.ptr() ); }
inline long              as_long     ( const Dyn_obj& o ) { o.check_not_null(); return as_long     ( o.type(), o.ptr() ); }
inline Big_int           as_big_int  ( const Dyn_obj& o ) { o.check_not_null(); return as_big_int  ( o.type(), o.ptr() ); }
inline double            as_double   ( const Dyn_obj& o ) { o.check_not_null(); return as_double   ( o.type(), o.ptr() ); }
inline char              as_char     ( const Dyn_obj& o ) { o.check_not_null(); return as_char     ( o.type(), o.ptr() ); }
inline Sos_string        as_string   ( const Dyn_obj& o ) { o.check_not_null(); return as_string   ( o.type(), o.ptr() ); }
inline Const_area_handle as_text_area( const Dyn_obj& o ) { o.check_not_null(); return as_text_area( o.type(), o.ptr() ); }


} //namespace sos

#endif
