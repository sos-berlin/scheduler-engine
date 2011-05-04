// ini.h                                                (c) SOS GmbH Berlin
//                                                      J”rg Schwiemann
//                                                      Joacim Zschimmer

const int max_profile_section_name_length  = 50;
const int max_profile_section_entry_length = 200;


struct Profile
{

struct Section
{
    Section ( const char* section_name, const char* ini_filename = "sos.ini" );

    void flush();
    const char* filename() { return _filename; }
    const char* section_name() { return _section_name; }

    struct Entry
    {
        Entry( const char* entry_name, Section* section_ptr ) : _section_ptr ( section_ptr ) {}
        ~Entry();

        void operator= ( Representable value ) {
           Fixed_string< max_profile_section_entry_length > value_string;
           value.get( &value_string );
           WritePrivateProfileString(_section_ptr->section_name(),
                                     entry_name,
                                     value_string,
                                     _section_ptr->filename() ); 
        }

        operator Representable()
        {
            Representable _value;
            GetPrivateProfileString(entry_name,.....); }
            return _value;
        }
    };

  private:
    Section* const _section_ptr;
    char*          _filename     [ _MAX_FILENAME ];
    char*          _section_name [ _MAX_SECTION  ];
};

};


struct E370_section : Profile::Section
{
    E370_section : Section ( "e370" ) {}

    Entry< String, "trace_file"    > trace_filename ( this );
    Entry< Bool  , "trace_flush"   > trace_flush    ( this );
    Entry< int   , "terminal_type" > terminal_type  ( this );
};



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
