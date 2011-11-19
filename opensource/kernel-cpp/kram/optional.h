/* optional.h                                               (c) SOS GmbH Berlin
                                                            Joacim Zschimmer

    Opt< T > , T sollte Unterklasse von Storable_as< Sos_string > sein,
    ergibt einen von T abgeleiteten Typ, der leere String-Eingaben an ein Objekt vom Typ T erlaubt.

    Für T müssen folgende Funktionen definiert sein:

    void empty( T* )
    void object_load( const Sos_string& )

    Für Opt<T> werden die Funktionenen

    void convert_from_string( const Sos_string&, Opt<T>* )
    void Opt<T>::object_load( const Sos_string& )

    definiert, die bei einem leeren String (Blanks und Tabs sind erlaubt) T::make_empty() ruft,
    sonst T::object_load( Sos_string, T* ).
*/

#ifndef __OPTIONAL_H
#define __OPTIONAL_H

namespace sos
{

//#pragma implementation

#if !defined __LIMITS_H
//#   include <limits.h>
#endif

//typedef Sos_string;


struct Has_empty
{
    virtual Bool                empty                   () const            = 0;
    virtual void                make_empty              ()                  = 0;
};

template< class T >    // T mit Methoden empty() und make_empty()
struct Opt : T  //, Has_empty
{
                                Opt<T>                  ()                  {}
                                Opt<T>                  ( const T& o )      { *(T*)this = o; }

  //Bool                        empty                   () const            { return ::empty( *(T*)this ); } //{ return T::empty(); }
  //void                        make_empty              ()                  { ::empty( (T*)this );         }  //{ T::make_empty();   }

    void                        object_load             ( const Sos_string& );  // Falls T: Storable_as<Sos_string>
                              //operator Sos_date       () const;
};

template< class T >
void convert_from_string( const Sos_string& str, Opt<T>* object_ptr );

//template< class T >  Bool empty( T );    // Nicht allgemein implementiert, jeder Typ muß
//template< class T >  void empty( T* );   // besonders implementiert werden. Einige unten.

// empty() für ein paar Standard-Datentypen:

//Bool empty( const Sos_string& );
//void empty( Sos_string* );

/*
#define IMPLEMENT_SIMPLE_EMPTY( Type, empty_value )                                             \
    inline Bool empty( Type o )     { return o == empty_value; }                                \
    inline void empty( Type* p )    { *p = empty_value;        }

IMPLEMENT_SIMPLE_EMPTY( char , ' '      );
IMPLEMENT_SIMPLE_EMPTY( short, SHRT_MIN );
IMPLEMENT_SIMPLE_EMPTY( int  , INT_MIN  );
IMPLEMENT_SIMPLE_EMPTY( long , LONG_MIN );
*/
//==========================================================================================inlines

template< class T >
inline void convert_from_string( const Sos_string& str, Opt<T>* object_ptr )
{
    object_ptr->object_load( str );
}


template< class T >
void Opt<T>::object_load( const Sos_string& str )
{
    if( ::empty( str )) {
        ::empty( (T*)this );
    } else {
        T::object_load( str );
        //convert_from_string( str, (T*) this );   ruft sich selbst auf.
    }
}

inline Bool empty( const Has_empty& object )
{
    return object.empty();
}


inline void empty( Has_empty* object_ptr )
{
    object_ptr->make_empty();
}


} //namespace sos

#endif
