// profile.h                                            (c) SOS GmbH Berlin
//                                                      Jörg Schwiemann
//                                                      Joacim Zschimmer

#ifndef __PROFILE_H
#define __PROFILE_H


//#if !defined( __STORABLE_H )
//    #include <storable.h>
//#endif

#ifndef __TYPE_H
//    #include <type.h>
#endif


struct string;
//template< class T >  struct Storable_as<T>;

/*
const int max_profile_name_length                 = 100;
const int max_profile_section_name_length         = 50;
*/
const int max_profile_section_entry_value_length  = 200;

struct Profile
{
    struct Section
    {
        struct Entry
        {
                                Entry                   ( const string& entry_name,
                                                          const string& default_value, Section* );
                                Entry                   ( const string& entry_name,
                                                          const string& default_value, const Section& );
                               ~Entry                   ();

//          void                operator=               ( const Storable_as<string>& );
            void                operator=               ( const string& );
                                operator string         () const;
          //void                get                     ( Typed* ) const;

          private:
            Section* const     _section_ptr;
            Bool               _my_section;
            string             _entry_name;
            string             _default_value;
        };


                                Section                 ( const string& section_name,
                                                          const string& ini_filename = "sos.ini" );

        void                    flush                   ();

        const string&           filename                () const;
        const string&           section_name            () const;


      private:
        string                 _filename;
        string                 _section_name;
    };

};

typedef Profile::Section::Entry  Profile_entry;

#if 0   // templates machen in Borland C++ 3.1 nur Ärger!
template< class Type, const char* name_string >
struct Profile__Section__Typed_entry : Profile::Section::Entry
{
    Profile__Section__Typed_entry( Profile::Section* section_ptr )
      : Profile::Section::Entry( name_string, section_ptr )
    {}

    operator Type();
    Profile__Section__Typed_entry& operator= ( const Type& );
};
#endif


#include <profile.inl>

#endif

/*
E370_section e370_section;



struct Representable
{
    virtual void set( Const_string_area );
    Representable& operator= ( Const_string_area area )
    {
        set( area );
        return *this;
    }

    virtual void get( Area& area )
    {
        strstream( area, area.size() ) << *this;    // Exceptions?  ios::fail?
    }

  //virtual String_area repr();         // Achtung dynamischer Speicher
    virtual void print( ostream& s )
    {
        Fixed_string string < 100 >;    // Evtl. Dyn_string
        get( string );
        s << string;
    }
};

struct Tolerant_representable   // ????
{
    //farbe = "mist";
    //f << farbe;

    Tolerant_representable( Representable* repr_ptr );
    void take_input_value();          // Repr <- Eingabe
    void take_program_value();        // Repr -> Eingabe
  private:
    Fixed_string<100> eingabe;
}

inline ostream& operator<< ( ostream& s, Representable r )
{
    r.print( s );
    return s;
}


struct Persistent       // Bin„re Darstellung
{
    virtual void set( Const_area );
    virtual void get( Area& );
};


Int : Representable
{
    void set ( Const_area area )  { _value = a2i( area ); }
    void get ( Area& area      )  { area = i2a( _value ); }
  private:
    int _value;
};


struct Type
{
    enum Kind ( enum, pointer, array );
};

struct Array_type : Type
{
    Array_type() : Type ( array );
    int count();
    Type member_type;
};

struct Int_type : Type {
    Int_type() : Type( "Int", int_type );
    void set( Typed* objekt_ptr, const char* value ) {
       check(value);xc;
       *objekt_ptr = a2i( value );
    }
};

struct Typed
{
    Typed( Type* );

    void show();
    void set(..) { _type_ptr->set( this, value ); }
    void get(..);
  private:
    Type* _type_ptr;
};

struct Int : Typed
{
    Int() : Typed( new Type( "Int", enum ) );
}


struct Dumpable
{
     Dumpable( const char* variable_name, Type* type_ptr )
     {
        Dump->declare( variable_name, type_ptr, this );
     }
};

struct Type
{
};


template<Type,int size>
struct Fixed_area : Area
{
    Type _value [ size ];
}


struct String_area : Area
{
    String_area( char* ptr, int size ) : Area( ptr, size ) { length( 0 ); }

    operator const char* () const  { return _string; }
    String_area& operator= ( const char* string )
    {
        _set( string );
    }

    String_area& operator+= ( String string );
    String_area& operator+ ( String string );
    String_area& truncate( int new_length );
    // Starview: operator String();

  protected:
    String_area( char* ptr, int size, const char* string ) : Area( ptr, size )
    {
        _set( string );
    }

  private:
    void _set( const char* string )
    {
        int len = strlen( string );
        if( len + 1 > size() ) {
            _string[ 0 ] = 0;
            length( 0 );
            raise ("STRING","STRING");
        }
        length( len );
        memcpy( char_ptr(), string, len + 1 );
    }

};


template <int size >
struct Fixed_string : String_area
{
    Fixed_string() : String_area( _string, sizeof _string );
    Fixed_string( const char* string ) : String_area( _string, sizeof _string, string )  {}

    Fixed_string& operator= ( const char* string ) const
    {
        (Fixed_string)*this = string;
        return *this;
    }

  private:
    char _string [ size + 1 ];
};

void set ( Const_area s );
*/