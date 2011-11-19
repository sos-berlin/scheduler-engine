// rtf.cxx                                      © 2000 Joacim Zschimmer
// $Id: rtf.cxx 12891 2007-06-29 12:29:34Z jz $

// §1677  

//#include "headers.h"

#include "../zschimmer.h"
#include "../log.h"
#include <assert.h>
#include <sstream>
#include <iomanip>

#include "rtf.h"


using namespace zschimmer::file;


namespace zschimmer {
namespace rtf {

extern const char rtf_copyright[] = "\n$Id: rtf.cxx 12891 2007-06-29 12:29:34Z jz $ (C) 2001 Zschimmer GmbH\n";

//-------------------------------------------------------------------------------------------static

int                         Simple_entity::static_entity_count         = 0;
int                         Simple_entity::static_entity_count_high    = 0;
size_t                      Simple_entity::static_allocated_bytes      = 0;
size_t                      Simple_entity::static_allocated_bytes_high = 0;

//--------------------------------------------------------------------------------------error_codes

static Message_code_text error_codes[] =
{
    { "Z-RTF-001", "Datenstruktur $2 ist zu klein für den RTF-Code $1 (new)" },
    { "Z-RTF-002", "Der RTF-Code $1 hat nicht die Datenstruktur $2 (cast)" },
    { "Z-RTF-003", "Zuviele verschiedene RTF-Codes" },
    { "Z-RTF-004", "RTF-Code $1 ist keine Destination" },
    {}
};

//--------------------------------------------------------------------------------------------const

const char hex[] = "0123456789ABCDEF";

//--------------------------------------------------------------------------------const known_array

const extern Known_descr const_new_descr = 
    { code_none              ,0 , "<new>"            , kind_none             ,flag_none                           , 0  };     // Default-Werte für unbekannte RTF-Codes

const Known_descr known_descr_array[ code__predefined_count ] =
{
    { code_none              ,6 , "<none>"           , kind_none             ,flag_none                           , 0  },
    { code_eof               ,6 , "<eof>"            , kind_none             ,flag_none                           , 0  },
    { code_text              ,6 , "<text>"           , kind_none             ,flag_unknown                        , 0  },
    { code_group             ,6 , "<group>"          , kind_destination      ,flag_unknown | flag_print_newline   , 0  },  // Gruppe {..} (nicht Text), z.B. in {\stylesheet {\s1 ...}...}  
    { code_any               ,6 , "<any>"            , kind_none             ,flag_none                           , 0  },
  //{ "<*any>"               ,kind_destination, flag_none                           , 0  },

#   include "rtf_code.cxx"   // Generiert von generate_rtf.h aus rtf.xml
};

//-------------------------------------------------------------------------------------------------
/*
const Code border_properties[] =    // Bei \brdrnone und in Tabellendefiniton zurücksetzen
{                                   // In RTF-Doku: Grammatikelement <brdr>
    code_brdrw, 
    code_brsp, 
    code_brdrcf, 
    code_brdrs, 
    code_brdrth, 
    code_brdrsh, 
    code_brdrdb, 
    code_brdrdot, 
    code_brdrdash, 
    code_brdrhair,
    code_brdrinset, 
    code_brdrdashsm, 
    code_brdrdashd, 
    code_brdrdashdd, 
    code_brdrtriple, 
    code_brdrtnthsg, 
    code_brdrthtnsg, 
    code_brdrtnthtnsg, 
    code_brdrtnthmg, 
    code_brdrthtnmg, 
    code_brdrtnthtnmg, 
    code_brdrtnthlg, 
    code_brdrthtnlg, 
    code_brdrtnthtnlg, 
    code_brdrwavy, 
    code_brdrwavydb, 
    code_brdrdashdotstr,
    code_brdremboss,
    code_brdrengrave,
    code_brdroutset,
    code_none
};
*/
/*
    code_box,
    code_brdrb,   
    code_brdrbar,   
    code_brdrbtw,   
    code_brdrl,   
    code_brdrr,   
    code_brdrt,   
    code_brdrw,
    code_none       // Ende
};
*/

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_rtf )
{
    add_message_code_texts( error_codes ); 
}

//----------------------------------------------------------------------------------print_rtf_table

void print_rtf_table( ostream* s )
{
    *s << "name\t"
          "code\t" 
          "kind\t" 
          "prop_char\t"
          "prop_para\t"
          "prop_sect\t"
          "prop_doc\t"
          "prop_row\t"
          "prop_tab\t"
          "prop_cell\t"
          "prop_dest\t"
          "prop_border\t"
          "header\t"
          "special_char\t"
          "has_text\t"
          "own_prop\t"
          "star\t"
          "print_newline\t"
          "deflt\n";

    for( Code c = (Code)0; c < (Code)NO_OF( known_descr_array ); c++ )
    {
        const Known_descr& k = known_descr_array[ c ];

        *s << '\\' << k._name << '\t';
        *s << "code_" << k._name << '\t';

        switch( k._kind )
        {
            case kind_none:                 *s << "none";               break;
            case kind_symbol:               *s << "symbol";             break;
            case kind_toggle:               *s << "toggle";             break;
            case kind_flag:                 *s << "flag";               break;
            case kind_value:                *s << "value";              break;
            case kind_destination:          *s << "destination";        break;
            case kind_ignore_destination:   *s << "ignore_destination"; break;
            case kind_ignore:               *s << "ignore";             break;
        }
        *s << '\t';

        *s << ( ( k._flags & flag_prop_char     ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_para     ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_sect     ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_doc      ) != 0 ) << '\t';

        *s << ( ( k._flags & flag_prop_row      ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_tab      ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_cell     ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_dest     ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_prop_border   ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_header        ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_special_char  ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_has_text      ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_own_prop      ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_star          ) != 0 ) << '\t';
        *s << ( ( k._flags & flag_print_newline ) != 0 ) << '\t';

        *s << k._default;
        
        *s << '\n';
    }
}

//-------------------------------------------------------------------------------Known_descr::type
// s.a. Descr::compute_type() !!

Entity_type Known_descr::type( Code code ) const
{ 
    return code == code_text? text_entity        :
           is_destination()  ? destination_entity :
           has_param()       ? param_entity       
                             : simple_entity; 
}

//------------------------------------------------------------------------------Descr::compute_type
// s.a. Known_descr::type() !!

void Descr::compute_type() const
{ 
    _type = _code == code_text? text_entity        :
            is_destination()  ? destination_entity :
            has_param()       ? param_entity       
                              : simple_entity; 
}

//-------------------------------------------------------------------------Rtf_context::Rtf_context

Rtf_context::Rtf_context()
:
    Has_rtf_context(this),
    _descr_array( code__predefined_count )
{
    for( int i = code_none; i < code__predefined_count; i++ )
    {
        Descr&             d = _descr_array[ i ];
        const Known_descr& k = known_descr_array[ i ];

        assert( (Code)i == k._code );       // Ordnungen von enum Code und known_descr_array müssen übereinstimmen.

        d._code    = k._code;
        d._name    = k._name;
        d._kind    = k._kind;
        d._flags   = k._flags;
        d._default = k._default;

        d.compute_type();

        add_to_descr_map( d._code, false );
    }


    _doc_to_section_property_map[ code_paperw    ] = code_pgwsxn;
    _doc_to_section_property_map[ code_paperh    ] = code_pghsxn;    
    _doc_to_section_property_map[ code_margl     ] = code_marglsxn; 
    _doc_to_section_property_map[ code_margr     ] = code_margrsxn; 
    _doc_to_section_property_map[ code_margt     ] = code_margtsxn; 
    _doc_to_section_property_map[ code_margb     ] = code_margbsxn; 
    _doc_to_section_property_map[ code_gutter    ] = code_guttersxn;
  //_doc_to_section_property_map[ code_?         ] = code_margmirsxn; // Switches margin definitions on left and right pages. Used in conjunction with \facingp. 
    _doc_to_section_property_map[ code_landscape ] = code_lndscpsxn;  
    // Page orientation is in landscape format. To mix portrait and landscape sections within a document,
    // the \landscape control should not be used so that the default for a section is portrait, 
    // which may be overridden by the \lndscpsxn control.
}

//---------------------------------------------------------------------Rtf_context::ignore_pgdsctbl
    
void Rtf_context::ignore_pgdsctbl( bool b )
{ 
    _descr_array[ code_pgdsctbl ]._kind = b? kind_ignore_destination : kind_destination; 
    _descr_array[ code_pgdsctbl ].compute_type();
}

//--------------------------------------------------------------------Rtf_context::add_to_descr_map

void Rtf_context::add_to_descr_map( Code code, bool is_new )
{
    Descr& d = _descr_array[ code ];
    string name = d._name;

    if( d._flags & flag_star  &&  !string_begins_with( name, "*\\" ) )  name = "*\\" + name;

    _descr_map[ name ] = code;

    if( is_new )  
        Z_LOG2( "rtf.learn", "\\" + name + " ist neu\n" );
}

//-------------------------------------------------------------------------Rtf_context::add_rtf_descr

Code Rtf_context::add_rtf_descr( const string& name, Kind kind, Flags flags )
{
    Descr new_descr;
    
    new_descr._code    = (Code)_descr_array.size();
    new_descr._name    = name;
    new_descr._kind    = kind;
    new_descr._flags   = flags;
    new_descr._default = 0;
    new_descr.compute_type();

    _descr_array.push_back( new_descr );

    add_to_descr_map( new_descr._code, true );

    return new_descr._code;
}

//------------------------------------------------------Rtf_context::set_rtf_descr_no_default_param

void Rtf_context::set_rtf_descr_no_default_param( Code code )
{
    if( code >= code__predefined_count )
    {
        if( _descr_array[ code ]._default != invalid_param )
        {
            Z_LOG2( "rtf.learn", "\\" << _descr_array[ code ]._name << "0 mit ausdrücklichem Parameter 0\n" );
            _descr_array[ code ]._default = invalid_param;       // Neuer RTF-Code mit explizitem Parameter 0
        }
    }
}

//----------------------------------------------------------------------Rtf_context::code_from_name

Code Rtf_context::code_from_name( const string& control_word )
{
    Descr_map::iterator pos = _descr_map.find( control_word );

    if( pos != _descr_map.end() )  return pos->second;


    if( string_begins_with( control_word, "*\\" ) )
    {
        // Wenn Control Word mit \* beginnt (eine Destination) und vordefiniert ist (code < code__predefined_count),
        // beachten wird \* nicht.

        pos = _descr_map.find( control_word.substr(2) );
        if( pos != _descr_map.end() )
        {
            Code code = pos->second;
            if( code < code__predefined_count  &&  _descr_array[ code ]._kind == kind_destination )  return code;

            // Dasselbe Controlword gibt es mal ohne und mal mit \*. Das unterscheiden wir für OpenOffice
            // {\*flyvertN ...} und \flyvertN.
            Z_LOG2( "rtf", "\\" << control_word << " gibt es auch als Destination\n" );   
        }
    }

    return code_any;
}

//-------------------------------------------------------------------Rtf_context::print_as_rtf_text
// s.a. print_as_rtf_text(File_base)

void Rtf_context::print_as_rtf_text( ostream* s, const char* text, size_t len )
{
    // 0-Bytes sind möglich!

    //int         col   = 0;
    const char* p     = text;
    const char* p_end = p + len;
    char        hex_rtf[4];
    
    hex_rtf[0] = '\\';
    hex_rtf[1] = '\'';

    while( p < p_end ) 
    {
        //if( col >= 100 ) 
        //{
        //    *s << "\r\n";       // Datei soll binär geöffnet sein
        //    col = 0;
        //}

        if( p[0] == '\\' )  *s << "\\\\"; 
        else
        if( p[0] == '{'  )  *s << "\\{";
        else
        if( p[0] == '}'  )  *s << "\\}";
        else
        if( (signed char)p[0] < ' ' )
        {
            // Das ist kein guter Platz, denn hier wird auch Text aus der Vorlage ersetzt. Es sollen aber nur die Zeichen in Variablen ersetzt werden.
            //*s << "{\\uc1\\u"             // 2006-09-11  Für Open Office auch als Unicode, wegen {\fN\fcharset128...} (siehe eMail von Andreas Püschel)
            //   << (signed char)p[0];     // Ab 0x8000 negativ

            hex_rtf[2] = hex[ (unsigned char)p[0] >> 4 ];
            hex_rtf[3] = hex[ (unsigned char)p[0] & 0x0F ];
            s->write( hex_rtf, 4 );
            //col += 4;

            //*s << '}';      // Ende Unicode
        }
        else
        {
            *s << p[0]; 
            //col++;
        }
        p++;
    }
}

//-------------------------------------------------------------------Rtf_context::print_as_rtf_text
// s.a. print_as_rtf_text(ostream)

void Rtf_context::print_as_rtf_text( File_base* file, const char* text, size_t len )
{
    // 0-Bytes sind möglich!

    int         col   = 0;
    const char* p     = text;
    const char* p_end = p + len;
    char        hex_rtf[4];
    
    hex_rtf[0] = '\\';
    hex_rtf[1] = '\'';

    while( p < p_end ) 
    {
        //if( col >= 100 ) 
        //{
        //    *s << "\r\n";       // Datei soll binär geöffnet sein
        //    col = 0;
        //}

        if( p[0] == '\\' )  file->print( "\\\\" ),  col += 2; 
        else
        if( p[0] == '{'  )  file->print( "\\{"  ),  col += 2;
        else
        if( p[0] == '}'  )  file->print( "\\}"  ),  col += 2;  
        else
        if( (signed char)p[0] < ' ' )
        {
            hex_rtf[2] = hex[ (unsigned char)p[0] >> 4 ];
            hex_rtf[3] = hex[ (unsigned char)p[0] & 0x0F ];
            file->write( hex_rtf, 4 );
            col += 4;
        }
        else
        {
            file->print( p[0] ); 
            col++;
        }
        p++;
    }
}

//---------------------------------------------------------------------------Properties::Properties

Properties::Properties( Has_rtf_context* c )
: 
    Has_rtf_context(c),
    _zero_(this+1),
    _properties( after_last_property+1 ),
    _tabs(c,code_group)
{ 
    _properties.at( before_first_property )._next = (Code)after_last_property;
    _properties.at( after_last_property   )._prev = (Code)before_first_property;

    reset_properties( flag_prop );
}

//--------------------------------------------------------------------------------Properties::clear
/*
void Properties::clear()
{
    for( Code code = code_none; code < code__properties_max; code++ )
    {
        if( descr_array[code]._flags & flag )  _properties[code]._value = descr_array[code]._default;
    }

    if( flag & ( flag_prop_para | flag_prop_tab ) )  _tabs.set_head(NULL);

    _prop_flags = Flags( _prop_flags & ~flag );
}
*/
//---------------------------------------------------------------------Properties::reset_properties

void Properties::reset_properties( Flags flag )
{
    for( Code code = code__properties_min; code < (int)_properties.size(); code++ )
    {
        if( descr(code)->_flags & flag )  reset_property( code, descr(code)->_default );
    }

    if( flag & flag_prop_sect  &&  !_rtf_context->_change_doc_to_section_properties )
    {
        reset_property( code_pgwsxn   , _properties[ code_paperw    ]._value );
        reset_property( code_pghsxn   , _properties[ code_paperh    ]._value );
        reset_property( code_marglsxn , _properties[ code_margl     ]._value );
        reset_property( code_margrsxn , _properties[ code_margr     ]._value );
        reset_property( code_margtsxn , _properties[ code_margt     ]._value );
        reset_property( code_margbsxn , _properties[ code_margb     ]._value );
        reset_property( code_guttersxn, _properties[ code_gutter    ]._value );
        reset_property( code_lndscpsxn, _properties[ code_landscape ]._value );
    }

    if( flag & ( flag_prop_para | flag_prop_tab ) )  _tabs.set_head(NULL);

    _prop_flags = Flags( _prop_flags & ~flag );
}

//-------------------------------------------------------------------------Properties::set_property

void Properties::set_property( Code code, int param, Simple_entity* e )
{ 
    if( !is_property( code ) )  return;

    assert( !e  ||  e->code() == code  &&  e->param() == param );
    assert( !known_descr(code)->is_destination()  ||  e );


    _prop_flags = Flags( _prop_flags | known_descr(code)->_flags );

    switch( code )
    {
        case code_plain:        reset_properties( flag_prop_char );
                                set_property2( code_langfe   , _properties[ code_deflangfe ]._value, NULL );
                                set_property2( code_langfenp , _properties[ code_deflangfe ]._value, NULL );
                                set_property2( code_lang     , _properties[ code_deflang   ]._value, NULL );
                                set_property2( code_langnp   , _properties[ code_deflang   ]._value, NULL );
                                return;

        case code_pard:         reset_properties( flag_prop_para_complete );    return;

        case code_sectd:        reset_properties( flag_prop_sect );             
/*                              nach reset_properties() verschoben. Für code_plain sollte das auch gelten?
                                set_property2( code_pgwsxn    , _properties[ code_paperw    ]._value, NULL );
                                set_property2( code_pghsxn    , _properties[ code_paperh    ]._value, NULL );
                                set_property2( code_marglsxn  , _properties[ code_margl     ]._value, NULL );
                                set_property2( code_margrsxn  , _properties[ code_margr     ]._value, NULL );
                                set_property2( code_margtsxn  , _properties[ code_margt     ]._value, NULL );
                                set_property2( code_margbsxn  , _properties[ code_margb     ]._value, NULL );
                                set_property2( code_guttersxn , _properties[ code_gutter    ]._value, NULL );
                                set_property2( code_lndscpsxn , _properties[ code_landscape ]._value, NULL );
                                set_property2( code_header    , 0                                   , NULL );
                                set_property2( code_footer    , 0                                   , NULL );
*/
                                return;

        case code_brdrnone:     reset_properties( flag_prop_para_border );      return;
        case code_trowd:        reset_properties( flag_prop_row         );      return;
        case code_tcelld:       reset_properties( flag_prop_cell        );      return;

        case code_ul:           _properties[ code_uld ]._value = 0;
                                break;

        case code_uld:          _properties[ code_ul  ]._value = 0; 
                                break;

        case code_ulnone:       _properties[ code_ul  ]._value = 0;
                                _properties[ code_uld ]._value = 0;
                                return;

        case code_super:        reset_property( code_sub       , 0 );   //_properties[ code_sub        ]._value = 0;
                                reset_property( code_nosupersub, 0 );   //_properties[ code_nosupersub ]._value = 0;
                                break;

        case code_sub:          reset_property( code_super     , 0 );   //_properties[ code_super      ]._value = 0;
                                reset_property( code_nosupersub, 0 );   //_properties[ code_nosupersub ]._value = 0;
                                break;

        case code_nosupersub:   reset_property( code_super     , 0 );   //_properties[ code_super ]._value = 0;
                                reset_property( code_sub       , 0 );   //_properties[ code_sub   ]._value = 0;
                                return;

        case code_trbrdrt:      // Tabellendefintion
        case code_trbrdrb:
        case code_trbrdrl: 
        case code_trbrdrr: 
        case code_trbrdrh: 
        case code_trbrdrv:
        
        case code_cldglu:
        case code_clbrdrt:
        case code_clbrdrb:
        case code_clbrdrl:
        case code_clbrdrr:      reset_properties( flag_prop_para_border );  
                                break;


        case code_pgbrdrt:      // Offenbar setzten diese Code die Rahmeneinstellungen zurück
        case code_pgbrdrb:
        case code_pgbrdrl:
        case code_pgbrdrr:      reset_properties( flag_prop_para_border );
                                break;

      //case code_rtlch:        _properties[ code_ltrch ] = 0;    \rtlch sehen wir nicht als Eigenschaft an.
      //                        break;
      
      //case code_ltrch:        _properties[ code_rtlch ] = 0;
      //                        break;

        case code_wrapdefault:
        case code_wraparound:   
        case code_wraptight:   
        case code_wrapthrough:  reset_property( code_wrapdefault, 0 );
                                reset_property( code_wraparound , 0 );
                                reset_property( code_wraptight  , 0 );
                                reset_property( code_wrapthrough, 0 );
                                break;


        default: 
        {
            if( is_tab_property( code ) )
            {
                if( _handle_complex_properties )  _tabs.append_to_destination( e? new_entity( *e ) : new_entity( code, param ) );
                return;
            }
            else
            if( known_descr(code)->_flags & flag_prop_row )  return;        // NICHT BEHANDELT!
            else
            if( known_descr(code)->_flags & flag_prop_cell )  return;       // NICHT BEHANDELT!
            else
            if( known_descr(code)->_flags & flag_prop_dest )  return;       // NICHT BEHANDELT!
        }
    }
    

    set_property2( code, param, e );
}

//-------------------------------------------------------------------------Properties::set_property

void Properties::set_property( Simple_entity* e )
{ 
    set_property( e->code(), e->param(), e );
}

//------------------------------------------------------------------------Properties::set_property2

void Properties::set_property2( Code code, int value, Simple_entity* entity )
{ 
    // Um die ursprüngliche Reihenfolge der Eigenschaften einzuhalten (sie ist für OpenOffice wichtig 
    // und vielleicht auf für Microsoft Word), werden die Eigenschaften verkettet. Jede gesetzte
    // Eigenschaft wird ans Ende der Liste gehängt.
    // write() gibt die Eigenschaften dann entlang der Liste aus.
    // Der Anfang der Liste ist im Dummy-Eintrag _properties[before_first_property]._next.
    // Das Ende der Liste ist im Dummy-Eintrag _properties[after_last_property]._prev
    // Nebeneffekt ist, das write() wenige Schleifendurchläufe hat, denn von den Hunderten
    // Eigenschaften werden nur wenige verwendet.

    if( code <= (Code)before_first_property  ||  code >= (Code)after_last_property )  throw_xc( "Properties::set_property2", descr(code)->_name );


    Entry* e = &_properties.at( code );

    e->_value = value;
    e->_entity = entity;

    if( e->_next != e->_prev )
    {
        // Aus der bisherigen Verkettung lösen und Kette schließen:
        _properties.at( e->_prev )._next = e->_next;
        _properties.at( e->_next )._prev = e->_prev;
    }

    // Ans Ende der Kette hängen:
    e->_prev = _properties.at( after_last_property )._prev;
    e->_next = (Code)after_last_property;
    _properties.at( e->_prev            )._next = code;
    _properties.at( after_last_property )._prev = code;
}

//-----------------------------------------------------------------------Properties::reset_property

void Properties::reset_property( Code code, int value )
{ 
    Entry* e = &_properties.at( code );

    e->_value  = value;
    e->_entity = NULL;

    if( e->_next != e->_prev )
    {
        // Aus der bisherigen Verkettung lösen und Kette schließen:
        _properties.at( e->_prev )._next = e->_next;
        _properties.at( e->_next )._prev = e->_prev;
        e->_next = code_none;
        e->_prev = code_none;
    }
}

//--------------------------------------------------------------------------------Properties::write

void Properties::write( void* context, Write_property_callback callback, Flags allowed_properties ) const
{
    if( _prop_flags & flag_prop_para )  callback( context, Param_entity( code_pard  ) );
    if( _prop_flags & flag_prop_char )  callback( context, Param_entity( code_plain ) );


    for( Code code = _properties.at( before_first_property )._next; 
              code != (Code)after_last_property; 
              code = _properties[ code ]._next )
    {
        if( known_descr( code )->_flags & allowed_properties )
        {
            int value = _properties[ code ]._value;

            if( value != known_descr( code )->_default  
             && !is_destination( code ) )           // Also nicht {\*\pnseclvlN ...}
            {
                if( descr( code )->_flags & ( flag_prop_sect | flag_prop_para | flag_prop_char ) )
                {
                    callback( context, Param_entity( code, value ) );
                }
            }
        }
    }


    if( allowed_properties & ( flag_prop_para | flag_prop_tab ) )
    {
        Simple_entity* e = _tabs.head();
        while( e )  callback( context, *e ),  e = e->next();
    }
}

//--------------------------------------------------------------------------------Properties::print

void Properties::print( ostream* s ) const
{
    for( Code code = code_none; code < (int)_properties.size(); code++ )
    {
        if( _properties[code]._value != descr(code)->_default ) {
            *s << '\\' << descr(code)->_name << _properties[code]._value << ' ';
        }
    }
}

//---------------------------------------------------------------------------------entity_type_name

const char* entity_type_name( Entity_type type )
{
    switch( type )
    {
        case simple_entity      : return "Simple_entity";
        case param_entity       : return "Param_entity";
        case destination_entity : return "Destination_entity";
        case text_entity        : return "Text_entity";
        default                 : return "?";
    }
}

//--------------------------------------------------------------Simple_entity::Iterator::operator++
/*
Simple_entity::Iterator& Simple_entity::Iterator::operator++()
{
    if( rtf::is_destination( _ptr->code() ) )
    {
        Simple_entity* branch = _ptr->head();
        if( branch )
        {
            _stack.push( _ptr );
            _ptr = branch;
            return *this;
        }
    }

    _ptr = _ptr->_next;
    while( !_ptr  &&  !_stack.empty() )  _ptr = _stack.top()->_next,  _stack.pop();

    return *this;
}
*/
//-----------------------------------------------------------Simple_entity::Iterator::operator bool
/*
bool Simple_entity::Iterator::operator bool()
{ 
}
*/
//--------------------------------------------------------------------Simple_entity::~Simple_entity
/*
Simple_entity::~Simple_entity()
{
    if( _code != code_none )
    {
        --static_entity_count;

        _code = code_none;

        switch( descr()->type() )
        {
            case simple_entity      : break;
            case param_entity       : ((Param_entity      *)this) -> ~Param_entity();
            case destination_entity : ((Destination_entity*)this) -> ~Destination_entity();
            case text_entity        : ((Text_entity       *)this) -> ~Text_entity();
        }
    }
}
*/
//-----------------------------------------------------------------------Simple_entity::change_code

void Simple_entity::change_code( Code new_code )
{
    if( rtf::known_descr( new_code )->type(new_code) != known_descr_type() )  throw_xc( "Simple_entity::change_code", new_code );

    _code = new_code;
}

//-------------------------------------------------------------Simple_entity::assert_matching_class

void Simple_entity::assert_matching_class( Entity_type type ) const
{ 
    // Prüfen, ob die Datenstruktur (Simple_entity, Param_entity, Destination_entity, Text_entity) für den RTF-Code passt
    if( known_descr_type() > type )  throw_xc( "Z-RTF-001", known_descr()->_name, entity_type_name(type) );
}

//----------------------------------------------------------------Simple_entity::assert_entity_type
/*
void Simple_entity::assert_cast( Entity_type iface ) const
{ 
    // Prüfen, ob die Datenstruktur (Simple_entity, Param_entity, Destination_entity, Text_entity) für den RTF-Code passt
    //if( ( descr()->type() & ~type ) != 0 )  throw_xc( "Z-RTF-ENTITY-TYPE" );
    if( descr()->type() < iface )  throw_xc( "Z-RTF-002", descr()->_name, entity_type_name(iface) );
}
*/
//----------------------------------------------------------------------Has_rtf_context::new_entity

Simple_entity* Has_rtf_context::new_entity( Code code, int param )
{
    switch( descr(code)->type() )
    {
        case simple_entity      : return new Simple_entity     (       code );
        case param_entity       : return new Param_entity      (       code, param );
        case destination_entity : return new Destination_entity( this, code, param );
        case text_entity        : return new Text_entity       ( this );
        default                 : throw_xc( "Has_rtf_context::new_entity" );
    }
}

//--------------------------------------------------------------------------Rtf_context::new_entity

Simple_entity* Has_rtf_context::new_entity( const Simple_entity& e )
{
    switch( descr( e.code() )->type() )
    {
        case simple_entity      : return new Simple_entity     (                                            e   );
        case param_entity       : return new Param_entity      ( *static_cast<const Param_entity*>      ( &e ) );
        case destination_entity : return new Destination_entity( *static_cast<const Destination_entity*>( &e ) );
        case text_entity        : return new Text_entity       ( *static_cast<const Text_entity*>       ( &e ) );
        default                 : throw_xc( "Simple_entity::new_entity" );
    }
}

//-----------------------------------------------------------Rtf_context::section_from_doc_property

Code Rtf_context::section_from_doc_property( Code doc_property )
{ 
    Doc_to_section_property_map::iterator it = _doc_to_section_property_map.find( doc_property );

    return it == _doc_to_section_property_map.end()? code_none 
                                                   : it->second; 
}

//----------------------------------------------------------------------Simple_entity::operator new

void* Simple_entity::operator new( size_t size )
{
    static_allocated_bytes += size;
    if( static_allocated_bytes_high < static_allocated_bytes )  static_allocated_bytes_high = static_allocated_bytes;

    return malloc( size );
}

//--------------------------------------------------------------------Simple_entity::operator delete

void  Simple_entity::operator delete( void* p, size_t size )
{
    static_allocated_bytes -= size;
    free( p );
}

//-------------------------------------------------------------------------Simple_entity::set_param
/*
void Simple_entity::set_param( int param )
{ 
    if( descr()->type() >= param_entity)  ((Param_entity*)this)->_param = param;
    else  
    if( param != 0 )  throw_xc( "Z-RTF-001", descr()->_name, entity_type_name(param_entity) );
}
*/
//-----------------------------------------------------------------------------Simple_entity::print
// s.a. print( File_base )

void Simple_entity::print( Rtf_context* rtf_context, ostream* s ) const
{
    *s << '\\';
    const Descr* d = rtf_context->descr( _code );
    
    if( d->_name[0] == '~'  &&  d->_name[1] != '\0' )  *s << d->_name.c_str() + 1;
                                                 else  *s << d->_name;
}

//-----------------------------------------------------------------------------Simple_entity::print
// s.a. print( ostream )

void Simple_entity::print( Rtf_context* rtf_context, File_base* file ) const
{
    file->print( '\\' );
    const Descr* d = rtf_context->descr( _code );
    
    if( d->_name[0] == '~'  &&  d->_name[1] != '\0' )  file->print( d->_name.c_str() + 1 );
                                                 else  file->print( d->_name );
}

//------------------------------------------------------------------------------Param_entity::print
// s.a. print( File_base )

void Param_entity::print( Rtf_context* rtf_context, ostream* s ) const
{
    Simple_entity::print( rtf_context, s );

    switch( rtf_context->descr( _code )->_kind ) 
    {
        case kind_toggle:       if( param() == 0 )  *s << '0';  break;
        case kind_flag:         break;  //jz 26.10.04
        case kind_symbol:       break;  //jz 26.10.04
/*
#     ifdef _DEBUG
        case kind_flag:         throw_xc( "Param_entity::print" ); break;      // Sollte nicht vorkommen (ist Simple_entity)
        case kind_symbol:       throw_xc( "Param_entity::print" ); break;      // Sollte nicht vorkommen (ist Simple_entity)
#     endif
*/
        default:                *s << param();
    }
}

//------------------------------------------------------------------------------Param_entity::print
// s.a. print( ostream )

void Param_entity::print( Rtf_context* rtf_context, File_base* file ) const
{
    Simple_entity::print( rtf_context, file );

    switch( rtf_context->descr( _code )->_kind ) 
    {
        case kind_toggle:       if( param() == 0 )  file->print( '0' );  break;
        case kind_flag:         break;  //jz 26.10.04
        case kind_symbol:       break;  //jz 26.10.04
/*
#     ifdef _DEBUG
        case kind_flag:         throw_xc( "Param_entity::print" ); break;      // Sollte nicht vorkommen (ist Simple_entity)
        case kind_symbol:       throw_xc( "Param_entity::print" ); break;      // Sollte nicht vorkommen (ist Simple_entity)
#     endif
*/
        default:                file->print( param() );
    }
}

//-----------------------------------------------------------Destination_entity::Destination_entity

Destination_entity::Destination_entity( const Destination_entity& e )
:
    Has_rtf_context(e)
{
    _head = NULL;
    _last = NULL;

    deep_copy( e );
}

//----------------------------------------------------------Destination_entity::~Destination_entity

Destination_entity::~Destination_entity()
{
    set_head( NULL );
}

//-------------------------------------------------------------------Destination_entity::operator =

Destination_entity& Destination_entity::operator = ( const Destination_entity& e )
{
    deep_copy( e );
    return *this;
}

//-----------------------------------------------------------------Destination_entity::shallow_copy

void Destination_entity::shallow_copy( const Destination_entity& e )
{
    _code  = e._code;
    _param = e._param;
  //_text  = e._text;
    set_head( NULL );
}

//--------------------------------------------------------------------Destination_entity::deep_copy

void Destination_entity::deep_copy( const Destination_entity& e )
{
    shallow_copy( e );

    if( e._head )
    {
        Simple_entity*  f = e.head();
        Simple_entity** E = &_head;
        
        while( f ) 
        {
            _last = new_entity( *f );
            *E = _last;
            E = &(*E)->_next;
            f = f->_next;
        }
    }
}
    
//---------------------------------------------------------------------Destination_entity::set_head

void Destination_entity::set_head( Simple_entity* new_head )
{
    if( _head )
    {
        Simple_entity* e = _head;
    
        while( e ) 
        {
            Simple_entity* next = e->next();
            delete e;
            e = next;
        }
    }

    _head = new_head;
    _last = NULL;
}

//-----------------------------------------------------------------Destination_entity::move_head_to

void Destination_entity::move_head_to( Destination_entity* other )
{
    other->set_head( _head );
    
    _head = NULL;
    _last = NULL;
}

//-----------------------------------------------------------------Destination_entity::insert_after

void Destination_entity::insert_after( Simple_entity* entity_before, Simple_entity* new_entity )
{
    assert( !new_entity->_next );

    if( entity_before )
    {
        new_entity->_next = entity_before->_next;
        entity_before->_next = new_entity;
    }
    else
    {
        new_entity->_next = _head;
        _head = new_entity;
    }
    
    if( _last == entity_before )  _last = new_entity;
}

//--------------------------------------------------------Destination_entity::append_to_destination

void Destination_entity::append_to_destination( Simple_entity* new_entity )
{
    if( _head )
    {
        if( !_last )
        {
            Simple_entity* e = _head;
            while( e->next() )  e = e->next();
            _last = e;
        }

        _last->append( new_entity );
    }
    else
    {
        _head = new_entity;
    }

    _last = new_entity;
}

//---------------------------------------------------------------------Destination_entity::truncate

void Destination_entity::truncate( Simple_entity* previous_element )
{
    if( previous_element == NULL )
    {
        set_head( NULL );
    }
    else
    {
#       ifdef _DEBUG
        {
            Simple_entity* e = _head;
            while( e  &&  e != previous_element )  e = e->next();
            if( e != previous_element )  throw_xc( "Destination_entity::truncate" );
        }
#       endif

        while( previous_element->_next )  previous_element->remove_successor();

        _last = previous_element;
    }
}

//------------------------------------------------------------------------Destination_entity::print
// s.a. print( File_base )

void Destination_entity::print( Rtf_context* rtf_context, ostream* s ) const
{
    const Descr* d = rtf_context->descr( code() );

    if( d->star() )  *s << "\\*"; 
    *s << '\\'; 
    *s << d->_name; 
    if( param() != d->_default )  *s << param();
}

//------------------------------------------------------------------------Destination_entity::print
// s.a. print( ostream )

void Destination_entity::print( Rtf_context* rtf_context, File_base* file ) const
{
    const Descr* d = rtf_context->descr( code() );

    if( d->star() )  file->print( "\\*" ); 
    file->print( '\\' ); 
    file->print( d->_name ); 
    if( param() != d->_default )  file->print( param() );
}

//--------------------------------------------------------------------Destination_entity::as_string
/*
string Destination_entity::as_string( int max_size ) const
{
    char* p = new char[ max_size + 1 ];
    
    try {
        ostrstream s ( p, max_size + 1 );

        *s << *this << '\0';
        return p;
    }
    catch(...)
    {
        delete [] p;
        throw;
    }
}
*/

//-------------------------------------------------------------------------------Text_entity::print
// s.a. print( File_base )

void Text_entity::print( Rtf_context* rtf_context, ostream* s ) const
{
    if( code() == code_text )
    {
        rtf_context->print_as_rtf_text( s, c_str( _text ), _text.length() );
    }
    else
    {
        Destination_entity::print( rtf_context, s );
    }
}

//-------------------------------------------------------------------------------Text_entity::print
// s.a. print( ostream )

void Text_entity::print( Rtf_context* rtf_context, File_base* file ) const
{
    if( code() == code_text )
    {
        rtf_context->print_as_rtf_text( file, c_str( _text ), _text.length() );
    }
    else
    {
        Destination_entity::print( rtf_context, file );
    }
}

//------------------------------------------------------------------Simple_entity::remove_successor

void Simple_entity::remove_successor()
{
    Simple_entity* next_next = _next->_next;
    delete _next;
    _next = next_next;
}

//-----------------------------------------------------------------Simple_entity::special_as_string
/*
string Simple_entity::special_as_string( int len, bool fill )
{
    // Schreibt die nächsten RTF-Codes, außer Eigenschaften. Destination werden abgekürzt

    std::ostringstream s;
    Entity_ptr p = this;

    while( !p.eof()  &&  s.tellp() < len ) 
    {
        if( !p->is_property_or_header() ) 
        {
            if( p->is_destination() )  s << '{';
            s << p;
            if( p->is_destination() )  s << "..}";
        }

        p.to_next_entity();
    }

    if( p.eof() ) {
#       ifdef __GNUC__
            if( fill )  s <<              std::setw( len - s.tellp() );
#        else
            if( fill )  s << std::left << std::setw( len - s.tellp() );
#       endif
        s << "\\EOF";
    }

    return s.str().substr( 0, min( (int)s.tellp(), len ) );
}
*/
//------------------------------------------------------------------------------Properties::insert

Simple_entity* Properties::insert( Simple_entity* e, const Properties& base )
{
    for( Code code = code_none; code < code__properties_max; code++ )
    {
        if( (*this)[code]._value != base[code]._value )
        {
            e->insert_successor( new Param_entity( code, (*this)[code] ) );
            e = e->next();
        }
    }

    return e;
}

//-----------------------------------------------------------------------------Entity_ptr::char_ptr

const char* Entity_ptr::char_ptr() const
{ 
    //assert( dynamic_cast<Text_entity*>( _next_entity ) );

    if( _next_entity->code() == code_text )  return static_cast<Text_entity*>( _next_entity ) -> text().c_str() + _char_pos; 
                                       else  return "";
}

//--------------------------------------------------------------------------Entity_ptr::rest_length

size_t Entity_ptr::rest_length() const
{ 
    //assert( dynamic_cast<Text_entity*>( _next_entity ) );

    if( _next_entity->code() == code_text )  return static_cast<Text_entity*>( _next_entity ) -> text().length() - _char_pos; 
                                       else  return 0;
}

//-----------------------------------------------------------------------Entity_ptr::paragraph_text

string Entity_ptr::paragraph_text() const
{
    // Gegenstück zu Processor::skip_parapraph_chars( int n )

    // Schnelle Implementierung: paragraph_text im RTF-Baum ständig vorhalten!

    string result;

    const Simple_entity* e = _next_entity;

    if( e  &&  e->code() == code_text ) 
    {
        if( _char_pos == 0 )  result = cast_text_entity(_next_entity)->text();
                        else  result = string( char_ptr(), rest_length() );
        e = e->_next;
    }
    
    while( e )
    {
        while( e  &&  e->is_property() )  e = e->_next;
        if( !e )  break;

        if( e->code() == code_text )  result.append( static_cast<const Text_entity*>( e )->text() );
        else
        if( e->code() == code_tilde )  result += ' ';
        else
            break;

        e = e->_next;
    }

/*
    if( e ) 
    {
        if( e->code() == code_text ) 
        {
            result = char_ptr();
            e = e->_next;
        }

        while( e )
        {
            if( e->code() == code_line )  break;
            else
            if( e->code() == code_page )  break;
            else
            if( e->code() == code_par  )  break;
            else
            if( e->code() == code_sect )  break;
            else
            if( e->code() == code_text )  result.append( e->text() );
            else
            if( e->code() == code_tilde )  result += ' ';
            else
            if( e->code() == code_tab   )  result += ' ';
            e = e->_next;
        }
    }
*/

    return result;
}

//----------------------------------------------------------------------------Entity_ptr::peek_char

int Entity_ptr::peek_char()
{
    while( !eof() )
    {
        int c = peek_char_until_2( NULL );
        if( c != EOF )  return c;
    }

    return EOF;
}

//----------------------------------------------------------------------Entity_ptr::peek_char_until

int Entity_ptr::peek_char_until( const Entity_ptr& end )
{
    while( !eof() )
    {
        if( end == *this )  return EOF;

        int c = peek_char_until_2( end._next_entity );
        if( c != EOF )  return c;
    }

    return EOF;
}

//--------------------------------------------------------------------Entity_ptr::peek_char_until_2

int Entity_ptr::peek_char_until_2( const Simple_entity* end )
{
    switch( _next_entity->code() )
    {
        case code_text: 
        {
            char c = *char_ptr();  

            switch( c )
            {
              //Parameter bool for_script: Nur im Skript diese Zeichen ersetzen? case '\x84':
                case '\x91': 
                case '\x92': return '\'';
                case '\x93':
                case '\x94': return '"';  // Deutsche Anführungszeichen?
                default:     return (Byte)c;
            }
        }

        case code_tab       : return '\t';
        case code_cell      : return '\t';
        case code_line      : return '\n';
        case code_par       : return '\n';
        case code_sect      : return '\n';
        case code_page      : return '\f';
        case code_tilde     : return ' ';
        case code_lquote    : return '\'';
        case code_rquote    : return '\'';
        case code_ldblquote : return '"';
        case code_rdblquote : return '"';
        default             : if( _next_entity->is_property() )  process_properties_until( end );
                                                           else  to_next_entity();
    }

    return EOF;
}

//----------------------------------------------------------------------------Entity_ptr::skip_char

void Entity_ptr::skip_char()
{
    if( _next_entity->code() == code_text )  to_next_char();
                                       else  to_next_entity();
}

//----------------------------------------------------------------Entity_ptr::skip_and_ignore_props

void Entity_ptr::skip_and_ignore_props()
{
    if( _next_entity->is_property() )  process_properties();
                                 else  to_next_entity();
}

//---------------------------------------------------------------------------Entity_ptr::split_text

void Entity_ptr::split_text()
{
    // code_text an der Stelle _char_pos aufspalten in zwei Entities
    // Das braucht die Factory, um "abc<%script" in zwei Entities zu trennen.

    // *** ES DARF KEINEN WEITERES Entity_ptr IN DIESES Entity ZEIGEN ***

    if( !_next_entity                     )  return;
    if( _next_entity->code() != code_text )  return;
    if( _char_pos == 0                    )  return;

    assert( *char_ptr() != '\0' ); 

    Text_entity* e = new Text_entity( has_rtf_context(), char_ptr() );   // Der rechte Teil
    
    Text_entity* e0 = cast_text_entity(_next_entity);
    e0->set_text( e0->text().substr( 0, _char_pos ) );

    e->_next = _next_entity->_next;             // Einflechten
    _next_entity->_next = e;

    _next_entity = e;                           // Auf den rechten Teil stellen
    _char_pos = 0;   
}

//---------------------------------------------------------Entity_ptr::correct_char_ptr_after_split

void Entity_ptr::correct_char_ptr_after_split()
{
    if( _next_entity )
    {
        if( _next_entity->code() == code_text )  
        {
            Text_entity* e = static_cast<Text_entity*>( +_next_entity );
            size_t diff = _char_pos - e->_text.length();
            if( diff >= 0 )
            {
                _char_pos = e->_text.length();
                to_next_char( diff );
            }
        }
    }
}

//----------------------------------------------------------------------------------------read_text

string read_text( const Entity_ptr& entity_ptr )
{
    string     result;
    Entity_ptr e = entity_ptr;
    
    while( e )
    {
        int c = e.peek_char();
        if( c == EOF )  break;
        result += (char)c;
        e.skip_char();
    }

    return result;
}

//------------------------------------------------------------------------Entity_ptr::set_property2
/*
bool Entity_ptr::set_property2( Code code, int param )
{
    bool ok = true;

    if( _processed_properties )
    {
        if( _processed_properties[ code ] == proc_processed )  // Eigenschaft durch eine Konstruktionsregel berücksichtigt?
        {
            if( descr_array[ code ].is_on_off() )  
            {
                if( _properties[ code ] == 1  && param == 0 )  // Und diese Eigenschaft wird jetzt ausgeschaltet? (z.B. \b0)
                {
                    ok = false;   // XML-Verschachtelung beenden! (für rtf2xml)
                                  // Beispiel: \b fett \i fett kursiv \b0        nur kursiv\i0
                                  // ==>       <b>fett <i>fett kursiv </i></b><i>nur kursiv</i>
                }
            }
        }
    }

    _properties.set_property( code, param );
    return ok;
}

//------------------------------------------------------------------Entity_ptr::reset_properties

bool Entity_ptr::reset_properties( Flags flag )
{
    for( Code code = code_none; code < code__count; code++ )
    {
        if( descr_array[code]._flags & flag ) 
        {
            int deflt = descr_array[code]._default;

            if( _properties[ code ] != deflt ) 
            {
                bool ok = set_rtf_property( code, deflt );
                if( !ok )  return false;
            }
        }
    }

    return true;
}
*/

//-------------------------------------------------------------------------------Entity_ptr::print

void Entity_ptr::print( ostream* s ) const
{
    if( _next_entity->code() == code_text )
    {
        _rtf_context->print_as_rtf_text( s, char_ptr(), cast_text_entity(_next_entity)->text().length() - _char_pos );
    }
    else
    {
        //*s << *_next_entity;
        _next_entity->print( _rtf_context, s );
    }
}

//-------------------------------------------------------------Entity_ptr::process_properties_until

bool Entity_ptr::process_properties_until( const Simple_entity* until )
{
    while( !eof()  &&  _next_entity != until  &&  _next_entity->is_property() )  to_next_entity();
    return true;
}

//---------------------------------------------------------------Entity_ptr_with_prop::set_property
/*
inline bool Entity_ptr_with_prop::set_property( Code code, int param )        
{ 
    _properties.set_property( code, param ); 
    return true; 
}
*/
//---------------------------------------------------------------Entity_ptr_with_prop::set_property

bool Entity_ptr_with_prop::set_property()
{
    if( !eof()  &&  _next_entity->is_property() )
    {
        _properties.set_property( _next_entity );
    }

    return true;
}

//---------------------------------------------------Entity_ptr_with_prop::process_properties_until

bool Entity_ptr_with_prop::process_properties_until( const Simple_entity* until )
{
    while( !eof()  &&  _next_entity != until  &&  _next_entity->is_property() )
    {
        bool ok = set_property();
        if( !ok )  break;

        to_next_entity();
    }

    return true;
}

//-----------------------------------------------------------------Entity_ptr_with_prop::forward_to

void Entity_ptr_with_prop::forward_to( const Simple_entity* until )
{
    while( _next_entity != until )
    {
        set_property();
        to_next_entity();
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace rtf
} //namespace zschimmer
