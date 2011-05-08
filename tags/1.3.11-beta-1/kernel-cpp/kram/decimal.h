// decimal.h

#ifndef __DECIMAL_H
#define __DECIMAL_H

//#pragma interface

#ifndef __SOSFIELD_H
#   include "../kram/sosfield.h"        // Text_format
#endif

namespace sos
{

#ifdef sign
#undef sign
#endif

//template <int digits>
//struct Decimal {
struct Text_format;
const extern Text_format std_text_format;

const int decimal_digits = 31;

struct Decimal
{
	                            Decimal                 ( Big_int value = 0, int length = decimal_digits);

    Byte                        length                  () const                { return _length;     }
    void                        length                  ( int i)                { _length = i;        }
    Byte                        digit                   ( int i ) const         { return _digits [i]; }
    int                         sign                    () const                { return _sign;       }
    Bool                        zero                    () const;
    void                        digit                   ( int i, Byte digit )   { _digits[i] = digit; }
    void                        sign                    ( int sign )            { _sign = sign;       }
    DECLARE_PRIVATE_MEMBER( int, scale )

    Decimal                     operator -              () const                { Decimal d = *this; d.neg(); return d; }
    Decimal                     operator +              ( const Decimal& b ) const { Decimal a = *this; return a += b; }
    Decimal                     operator -              ( const Decimal& b ) const { Decimal a = *this; return a -= b; }
    Decimal                     operator *              ( const Decimal& b ) const { Decimal a = *this; return a *= b; }
 //!Decimal                     operator /              ( const Decimal& b ) const { Decimal a = *this; return a /= b; }
  //Decimal                     operator >>             ( int n ) const;
  //Decimal                     operator <<             ( int n ) const;

    Bool                        operator <              ( const Decimal& p ) const;
    Bool                        operator <=             ( const Decimal& p ) const;
    Bool                        operator ==             ( const Decimal& p ) const;
    Bool                        operator !=             ( const Decimal& p ) const              { return ! ( p == *this ); }
    Bool                        operator >=             ( const Decimal& p ) const              { return p <= *this ; }
    Bool                        operator >              ( const Decimal& p ) const              { return p < *this ; }

//jz 17.9.96                    operator Big_int        ()         const;
    Big_int                     as_big_int              ()         const;

    Decimal&                    operator +=             ( const Decimal& p );
    Decimal&                    operator -=             ( const Decimal& p ) { return *this += -p; }
    Decimal&                    operator *=             ( const Decimal& p );
 //!Decimal&                    operator /=             ( const Decimal& p );
  //Decimal&                    operator >>=            ( int n ) const;
    Decimal&                    divide                  ( const Decimal& divisor, Decimal* rest_ptr );

    int                         no_of_significant_digits() const;  // 0..length()
    void                        exp10                   ( int n, Byte round = 0 );
    void                        add                     ( const Decimal& p );
    void                        sub                     ( const Decimal& p );
    void                        neg                     ()                                  { _sign = -_sign; }

    Bool                        null                    () const                            { return _sign == 99; }
    void                        set_null                ()                                  { _sign = 99; }
    Bool                        empty                   () const                            { return null(); }
    void                        make_empty              ()                                  { set_null(); }

    void                        write_text              ( Area*, const Text_format& = std_text_format ) const;
    void                        read_text               ( const char*, const Text_format& = std_text_format, Bool adjust_scale = false );

  private:
    Byte                       _digits [ decimal_digits ];
    signed char                _sign;          // -1, 0, +1
    Byte                       _length;        // Bytes
    Byte                       _overflow;
};

typedef Decimal Decimal_31;


struct Sos_binary_istream;
struct Sos_binary_ostream;

void read_ibm370_packed ( Sos_binary_istream*, Decimal*       , uint field_size );
void write_ibm370_packed( Sos_binary_ostream*, const Decimal& , uint field_size, Byte positive_sign = 0x0C );

Decimal as_decimal( const char* );

//---------------------------------------------------------------------------------Decimal_type
#if defined __SOSFIELD_H

struct Decimal_type : Field_type
{
                                Decimal_type            ()                      : Field_type( &_type_info, sizeof (Decimal) ) {}

    virtual Bool                nullable                () const                { return true; }
    virtual Bool                null                    ( const Byte* ) const;
    virtual void                set_null                ( Byte* ) const;

    void                        write_text              ( const Byte*, Area*, const Text_format& ) const;
    void                        read_text               ( Byte*, const char*, const Text_format& ) const;

    static Type_info           _type_info;
};

extern Decimal_type decimal_type;

DEFINE_ADD_FIELD( Decimal, decimal_type );

#endif
//==========================================================================================inlines

} //namespace sos

//#include <decimal.inl>
#endif
