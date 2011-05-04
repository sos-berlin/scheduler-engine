// Einzufügen in tabbed.cxx,          Joacim Zschimmer, 26.11.95

/* Eine Zeile mit Feldnamen und evtl. Feldtypen.
   Jedes Feld ist durch einen Separator, üblicherweise '\t', ' ', ';' oder ',',  getrennt.
   Typ und Name werden durch Doppelpunkt getrennt. Wenn der Separator nicht Blank ist,
   können Blanks verwendet werden und der Doppelpunkt wegfallen.
   Doppelpunkt ist nicht als Separator möglich.


   Typen: c[n]  (character)  Zeichenkette beliebiger oder der angegebenen Länge, mit Blanks aufgefüllt
          n[n[.m]] (numeric) Festpunktzahl mit n Ziffern, davon m Nachkommstellen
          f ??     (float)   -nicht implementiert-
          b        (bool)    0/1
          d        (date)    Alle Formate von Sos_date_type::input, Empf: ISO  yyyy-mm-dd
*/
/*
static Sos_ptr<Field_type> parse_type( Byte** pp )
{
    const char* p = *pp;

    if( *p == ':' ) {
        p++;
        while( *p == ' ' )  p++;
    }
    switch( *p )
    {
        case 'c':
        {
            size = get_number( &p );
            if( size ) {
                Sos_ptr<Sos_limited_text_type> t = SOS_NEW_PTR( Sos_limited_text_type( size ) );
                field_type = +t;
            } else {
                Sos_ptr<Sos_string_type> t = SOS_NEW_PTR( Sos_string_type );
                field_type = +t;
            }
            break;
        }
        case 'n':
        {
            size = get_number( &p );
            if( *p == '.' ) p++;
            scale = get_number( &p ); break;
            if( scale > size )  throw Xc( "record_type_of_name_list", "scale>size" );
            if( size > BIG_INT_PRECISION ) throw Xc( "record_type_of_name_list", "precision" );
            Sos_ptr<Big_int_type> t = SOS_NEW_PTR( Big_int_type( scale ) );s
            field_type = +t;
        }

      //case 'f':
        case 'b':
        {
            Sos_ptr<Bool_type> t = SOS_NEW_PTR( Bool_type );
            field_type = +t;
            break;
        }
        case 'd':
        {
            //while( *p == ' ' )  +p;
            //Format für tabulierte Datei lesen. Sos_date kann nicht zwei Formate unterscheiden
            Sos_ptr<Sos_optional_date_type> t = SOS_NEW_PTR( Sos_optional_date_type );
            field_type = +t;
            break;
        }

        default: error;
    }
    while( *p == ' ' )  break;

    *pp = p;
}
*/
//---------------------------------------------------------------------------------Tabbed_field

struct Tabbed_field
{
                                Tabbed_field            () : _offset(0),_length(0) {}

    const char*                 char_ptr                () const { return (const char*)this + _offset; }
    int                        _offset;
    int                        _length;
};

//----------------------------------------------------------------------------Tabbed_field_type

struct Tabbed_field_type : Field_type
{
  //virtual void                clear                   ( Byte* p ) const                       { ((Tabbed_field*)p)->_number = 0; }
    void                        print                   ( const Byte*, ostream*, const Text_format& ) const;
    void                        input                   (       Byte*, istream*, const Text_format& ) const;

  protected:
    uint                       _v_field_size            () const { return sizeof (Tabbed_field); }
    void                       _obj_print               ( ostream* s ) const                    { *s << "Tabbed_field"; }
};

extern Tabbed_field_type tabbed_field_type;

void Tabbed_field_type::print( const Byte* p, ostream* s, const Text_format& format ) const
{
    s->write( p + ((Tabbed_field*)p)->_offset, ((Tabbed_field*)p)->_length );
}

void Tabbed_field_type::input( Byte*, istream*, const Text_format& ) const
{
    throw Xc( "Tabbed_field_type::input" );
}

//---------------------------------------------------------------------------------------------

Sos_ptr<Record_type> record_type_of_name_list( const char* name_list, char separator )
{
    Sos_ptr<Dyn_record_type> t = SOS_NEW_PTR( Dyn_record_type );

    const char* p = name_list;

    while(1) {
        Sos_limited_text<max_name_length> name;
        Sos_ptr<Field_type>               field_type;

        while( *p  &&  *p != separator && *p != ' ' )  name += *p++;
        while( *p == ' ' )  p++;
        /*if( *p != separator ) { // Typ
            field_type = parse_type( &p );
        } else*/ {
            Sos_ptr<Sos_string_type> t = SOS_NEW_PTR( Tabbed_field_type );
            field_type = +t;
        }
        if( !*p )  break;

        field_type->add_to( t );
        field_type->add_null_flag_to( t );
    }
}


