// sosnum.h                                        (c) SOS GmbH Berlin
//                                                  Joacim Zschimmer
#if 0
#ifndef __SOSNUM_H
#define __SOSNUM_H

#pragma interface

#if 1 //defined SYSTEM_INT8
#   define SOS_NUMBER_BIGINT
#   define SOS_NUMBER_BIG_INT
# else
#   define SOS_NUMBER_DECIMAL
#endif

#if defined __SOSFIELD_H  &&  !defined __STDFIELD_H
#   include "stdfield.h"
#endif

#if defined SOS_NUMBER_DECIMAL  &&  !defined __DECIMAL_H
#   include "decimal.h"
#endif

#if defined SOS_NUMBER_BIG_INT  &&  !defined SYSTEM_INT64
    const int SOS_NUMBER_NUR_NEUNSTELLIG = 0;
#endif


struct Sos_number
{
                                Sos_number              () : _number(0), _scale(0) {}

    int                         scale                   ()              { return _scale; }
    void                        scale                   ( int s )       { _scale = s; }
    void                        nks                     ( int s )       { _scale = s; }
    int4                        integer4                () const        { return _number; }
    Big_int                     big_int                 () const        { return _number; }
    Sos_number&                 operator =              ( Big_int n );

  //void                        scale                   ( int n )               { _number.scale( n ); }
    void                        object_load             ( const Sos_string& );
    void                        object_store            ( Sos_string* ) const;
  //Bool                        empty                   () const                { return _number.empty(); }
  //void                        make_empty              ()                      { _number.make_empty();   }

/*
    // Komma wie in Sos_number:
    friend inline void          read_ibm370_packed      ( Sos_binary_istream*, Sos_number*,
                                                          uint field_size );
    friend inline void          write_ibm370_packed     ( Sos_binary_ostream*, const Sos_number&,
                                                          uint field_size );

    // Komma wie angegeben: (Anzahl der Nachkommastellen in Sos_number wird nicht verändert)
    friend void                 read_ibm370_packed      ( Sos_binary_istream*, Sos_number*,
                                                          uint field_size, int nachkommastellen );
    friend void                 write_ibm370_packed     ( Sos_binary_ostream*, const Sos_number&,
                                                          uint field_size, int nachkommastellen );
*/
  protected:
                                Sos_number              ( Big_int value, int scale = 0 ) : _scale( scale ), _number( value ) {}

//private:
    friend                      class Sos_number_type;

#   if defined SOS_NUMBER_BIGINT
        Big_int                _number;
#    else
        Decimal                _number;  // Zwischenlösung, int4 ist viel zu klein. 999.999.999,999
#    endif
    int                        _scale;   // Exponent, Zahl = _number * 10**_scale
};

//inline Bool negative( const Sos_number& number );
//inline void mult_10( Sos_number* number_ptr )           { number_ptr->scale( 1 ); }

template< int SCALE >
struct Sos_number_as : Sos_number
{
                                Sos_number_as           ( Big_int vorkommawert = 0 ) : Sos_number( vorkommawert, SCALE ) {}

# if defined SOS_NUMBER_BIGINT
    Sos_number_as<SCALE>        operator -              () const                                 { Sos_number_as<SCALE> m ( *this ); m._number = -m._number; return m; }

    Sos_number_as<SCALE>        operator *              ( Big_int n                     ) const  { Sos_number_as<SCALE> m ( *this ); m *= n; return m; }
    Sos_number_as<SCALE>        operator /              ( Big_int n                     ) const  { Sos_number_as<SCALE> m ( *this ); m /= n; return m; }
    Sos_number_as<SCALE>        operator +              ( const Sos_number_as<SCALE>& n ) const  { Sos_number_as<SCALE> m ( *this ); m += n; return m; }
    Sos_number_as<SCALE>        operator -              ( const Sos_number_as<SCALE>& n ) const  { Sos_number_as<SCALE> m ( *this ); m -= n; return m; }

    Sos_number_as<SCALE>&       operator *=             ( Big_int n                     )        { _number *= n; return *this; }
    Sos_number_as<SCALE>&       operator /=             ( Big_int n                     )        { _number /= n; return *this; }
    Sos_number_as<SCALE>&       operator +=             ( const Sos_number_as<SCALE>& n )        { _number += n._number; return *this; }
    Sos_number_as<SCALE>&       operator -=             ( const Sos_number_as<SCALE>& n )        { _number -= n._number; return *this; }

    int                         operator <              ( const Sos_number_as<SCALE>& n ) const  { return _number <  n._number; }
    int                         operator <=             ( const Sos_number_as<SCALE>& n ) const  { return _number <= n._number; }
    int                         operator ==             ( const Sos_number_as<SCALE>& n ) const  { return _number == n._number; }
    int                         operator !=             ( const Sos_number_as<SCALE>& n ) const  { return _number != n._number; }
    int                         operator >=             ( const Sos_number_as<SCALE>& n ) const  { return _number >= n._number; }
    int                         operator >              ( const Sos_number_as<SCALE>& n ) const  { return _number >  n._number; }
#  endif
};

//template< int SCALE >
//inline Sos_number_as<SCALE> operator* ( Big_int a, const Sos_number_as<SCALE>& b )   { return b * a; }
//Solaris C++ 4.0.1: Error: The template argument SCALE is not a type.

//-----------------------------------------------------------------------------Sos_number_type
#if defined __SOSFIELD_H  && defined SOS_NUMBER_BIGINT

struct Sos_number_type;
struct Sos_number_type : Field_type,
                         Std_field_access_as<Sos_number_type,Big_int>
{
    virtual void                clear                   ( Byte* p ) const                       { ((Sos_number*)p)->_number = 0; }
    void                        print                   ( const Byte*, ostream*, const Text_format& ) const;
    void                        input                   (       Byte*, istream*, const Text_format& ) const;

    void                        check_type              ( const Sos_number* ) {}

  protected:
    uint                       _v_field_size            () const { return sizeof (Sos_number); }
    void                       _obj_print               ( ostream* s ) const            { *s << "Sos_number"; }
};

inline void read_field ( const Sos_number_type&, const Byte* p, Big_int* o )  { *o = *(Big_int*)p; }
inline void write_field( const Sos_number_type&,       Byte* p, Big_int  o )  { *(Big_int*)p = o; }

extern Sos_number_type sos_number_type;

DEFINE_ADD_FIELD( Sos_number, sos_number_type );

#endif

//==========================================================================================inlines
/*
inline Bool negative( const Sos_number& number )
{
    return number._number.sign() == -1;
}
*/
/*
inline void read_ibm370_packed( Sos_binary_istream* s, Sos_number* number_ptr, uint field_size )
{
    read_ibm370_packed( s, &number_ptr->_number, field_size );
}

inline void write_ibm370_packed( Sos_binary_ostream* s, const Sos_number& number, uint field_size )
{
    write_ibm370_packed( s, number._number, field_size );
}
*/
#endif
#endif
