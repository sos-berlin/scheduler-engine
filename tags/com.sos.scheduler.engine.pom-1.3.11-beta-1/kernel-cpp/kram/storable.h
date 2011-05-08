// storable.h                               (c) SOS GmbH Berlin

#ifndef __STORABLE_H
#define __STORABLE_H

#if !defined SYSTEM_WINDLL
//#pragma imlementation

#if !defined __XCEPTION_H
#   include <xception.h>
#endif

#if !defined __SOSSTRNG_H
    #error sosstrng.h muss vorher included werden.
#endif

#if !defined __AREA_H
#   include <area.h>
#endif

#if !defined __ABSFILE_H
#   include <absfile.h>
#endif

inline Sos_string as_string( const Sos_string& str )
{
    return str;
}

// nach sosstrng.h (jz 2.10.95): Sos_string as_string( int );

        int as_int ( const char* );
inline int  as_int ( const Sos_string& str );
inline long as_long( const Sos_string& str );


template< class T >
inline void convert_from_string( const Sos_string& str, T* object_ptr )
{
    object_ptr->object_load( str );
}

inline void convert_from_string( const Sos_string& str, int* object_ptr )
{
    *object_ptr = as_int( str );
}

inline void convert_from_string( const Sos_string& str, Sos_string* object_ptr )
{
    *object_ptr = str;
}

inline void convert_from_string( const Sos_string& str, Area* object_ptr )
{
    object_ptr->assign( str );
}



//=================================================================================================

// Abstrakte Name_file Klasse für Sos_list_control_as

struct Abs_name_file : Abs_file
{
                                Abs_name_file           ( Abs_file* file_ptr ) : _file_ptr(file_ptr) {}

    void                        rewind                  ( Key::Number = 0 );
	void                        set                     ( const Key& );
	void                        get_record              ( Area& area );
	void                        get_record_key          ( Area&, const Key& );

  protected:
    virtual void                format                  ( const Const_area&, ostream* ) = 0;

  private:
     Abs_file*                  _file_ptr;
};


struct Auto_storable
{
    virtual void                object_auto_load        ()                              = 0;
    virtual void                object_auto_store       () const                        = 0;
};

template< class T >
struct Storable_as
{
  //                            Storable_as             () {}
  //virtual                    ~Storable_as             () {}
        
    virtual void                object_check            ( const Sos_string& );
    virtual void                object_load             ( const T& )                    = 0;
    virtual void                object_store            ( T* ) const                    = 0;

/*
            void                object_convert          ()  { object_store( &_copy ); }

  private:
    Bool                       _copy_valid;
    T                          _copy;
*/
};

//----------------------------------------------------------------------Storable_as_string_type
#if defined __SOSFIELD_H

struct Storable_as_string_type : Field_type
{
    void                        print                   ( const Byte* p, ostream* s, const Text_format& f ) const;
    void                        input                   (       Byte* p, istream* s, const Text_format& f ) const;

    uint                       _v_field_size            () const                                { return sizeof (Bool); }
    void                       _obj_print               ( ostream* s ) const                    { *s << "Storable_as<Sos_string>"; }
};

extern Storable_as_string_type storable_as_string_type;

DEFINE_ADD_FIELD( Storable_as<Sos_string>, storable_as_string_type );


#endif
//--------------------------------------------------------------------------------------------
// Wird in winedit nicht verwendet:
template< class T >
struct Compatible_with : virtual Storable_as<T>
{
    virtual void                object_load             ( const T& )                    = 0;
            void                operator =              ( const T& );                       //???

    virtual void                object_store            ( T* ) const                    = 0;
                                operator T              () const;
};


template< class T >
struct Auto_storable_as : virtual Auto_storable,
                          virtual Storable_as< T >
{
                                Auto_storable_as        ( T* );

    virtual void                object_load             ( const T& )                    = 0;
    void                        object_auto_load        ();

    virtual void                object_store            ( T* ) const                    = 0;
    void                        object_auto_store       () const;

  private:
    T*                         _real_object_ptr;
};

/* Nicht gemeinsam mit dem nächsten template verwendbar. Leider, leider.
template< class T >
inline void object_check( const Sos_string& str, T* )
{
    convert_from_string( str, &T() );
}
*/
// IMPLEMENT_OBJECT_CHECK( non Storable_as<T> ):
// Nur für Typen, die nicht Storable_as<Sos_string> sind,
// dieses object_check() kann keine Unterklassen von T berücksichtigen!

#define IMPLEMENT_OBJECT_CHECK( T )                                                             \
    inline void object_check( const Sos_string& str, T* )                                       \
    {                                                                                           \
        T copy;                                                                                 \
        convert_from_string( str, &copy );                                                      \
    }

IMPLEMENT_OBJECT_CHECK( int )
IMPLEMENT_OBJECT_CHECK( Sos_string )

/*
template< class T >          // template macht hier kein Sinn.
inline void object_check( const Sos_string& str, Storable_as<T>* object_ptr )
{
    object_ptr->object_check( str );
}
*/

inline void object_check( const Sos_string& str, Storable_as< Sos_string >* object_ptr )
{
    object_ptr->object_check( str );
}


struct Abs_file;

struct Has_name_file
{
    virtual Abs_file*           name_file_ptr           () const    = 0;
};

inline Abs_file* name_file_ptr( const Has_name_file& object )
{
    return object.name_file_ptr();
}

//==========================================================================================inlines

// jz 5.7.95 s. sosstrg0.h  inline int as_int( const Sos_string& string )  { return as_int( c_str( string )); }

inline Sos_string as_string( const Storable_as<Sos_string>& s )
{
    Sos_string str;
    s.object_store( &str );
    return str;
}

//------------------------------------------------------------Auto_storable_as<T>::object_auto_load

template< class T >
inline Auto_storable_as<T>::Auto_storable_as( T* real_object_ptr )
 :  _real_object_ptr ( real_object_ptr )
{
}

//------------------------------------------------------------Auto_storable_as<T>::object_auto_load

template< class T >
inline void Auto_storable_as<T>::object_auto_load()
{
    object_load( *_real_object_ptr );
}

//-----------------------------------------------------------Auto_storable_as<T>::object_auto_store

template< class T >
inline void Auto_storable_as<T>::object_auto_store() const
{
    object_store( _real_object_ptr );
}

//---------------------------------------------------------------------Storable_as<T>::object_check
#if defined SYSTEM_EXCEPTIONS

template< class T >
//NON_INLINE_TEMPLATE
void Storable_as<T>::object_check( const Sos_string& str )
{
    Sos_string original_value;

    object_store( &original_value );

    try {
        object_load( str );
    }
    catch(...) {
        object_load( original_value );
        throw;
    }

    try {
        object_load( original_value );
    }
    catch(...) {
        //int object_check_aendert_ungueltigen_originalwert;
    }
}

#endif
//--------------------------------------------------------------------Compatible_with<T>::operator=

template< class T >
inline void Compatible_with<T>::operator= ( const T& object )
{
    object_load( object );
}

//-------------------------------------------------------------------Compatible_with<T>::operator T

template< class T >
inline Compatible_with<T>::operator T () const
{
    T object;
    object_store( &object );
    return object;
}

#endif

#endif
