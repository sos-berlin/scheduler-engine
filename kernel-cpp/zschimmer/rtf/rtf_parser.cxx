// rtf_parser.cxx                                   © 2000 Joacim Zschimmer
// $Id: rtf_parser.cxx 12234 2006-09-04 09:50:11Z jz $

//#include "headers.h"
#include "../zschimmer.h"
#include "../log.h"
#include <assert.h>
#include "rtf.h"
#include "rtf_doc.h"
#include "rtf_parser.h"

namespace zschimmer {
namespace rtf {

//--------------------------------------------------------------------------------------error_codes

static Message_code_text error_codes[] =
{
    { "Z-RTF-PARSER-100", "Datei ist kein RTF-Dokument" },
    { "Z-RTF-PARSER-101", "RTF-Dokument ist abgeschnitten?" },
    { "Z-RTF-PARSER-102", "Dateiende ist falsch" },
    {}
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_rtf_parser )
{
    add_message_code_texts( error_codes ); 
}

//-----------------------------------------------------------------------------------Parser::Parser

Parser::Parser( const ptr<Doc>& doc )         
: 
    Has_rtf_context(doc->has_rtf_context()), 
    _zero_(this+1), 
    _token(doc->has_rtf_context()),
    _doc(doc) 
{
}

//----------------------------------------------------------------------------------Parser::~Parser

Parser::~Parser()
{
    delete [] _hilfspuffer;
}

//---------------------------------------------------------------------------Parser::init

void Parser::init()
{
}

//-----------------------------------------------------------------------Parser::eat_char

int Parser::eat_char()
{
    int result = _next_char;

    if( _next_char != EOF )
    {
        _eaten_char_pos._line = _char_pos._line;
        _eaten_char_pos._col  = _char_pos._col;
   
        errno = 0;

        if( _input )  _next_char = getc( _input );      // Datei
                else  _next_char = *_input_ptr++;       // Speicher

        if( _next_char == 0 )  _next_char = EOF;    // Warum liefert getc() 0 statt EOF?
        if( _next_char == EOF  &&  errno != 0 )  throw_errno( errno );

        if( _next_char == '\n' ) {
            _char_pos._line++;
            _char_pos._col = 0;
        } else {
            _char_pos._col++;
        }
    }

    return result;
}

//------------------------------------------------------------------Parser::get_next_token

void Parser::get_next_token()
{
    bool star = false;

    _token._rtf.clear();
    _token._pos = _char_pos;

  AGAIN:                              // Nach \*

    switch( _next_char ) 
    {
        case '\\':  eat_char();

                    switch( _next_char )
                    {
                        case '\\':  // Normaler Text
                        case '{':
                        case '}':   {
                                        _token._rtf.set_code( code_text );
                                        char c[2];
                                        c[0] = (char)eat_char();
                                        c[1] = '\0';
                                        _token._rtf.set_text( c );
                                        break;
                                    }

                        case '\'':  // Hexadezimales
                                    {
                                        eat_char();
                                        int h1 = eat_char();
                                        int h2 = eat_char();
                                        if( !isxdigit( h1 )  ||  !isxdigit( h2 ) )  throw_xc( "RTF-Token \'xx falsch", "", _token._pos );
                                        _token._rtf.set_code( code_text );
                                        char c = (char)( hex_to_digit( (char)h1 ) * 16 + hex_to_digit( (char)h2 ) );
                                        _token._rtf.set_text( string( &c, 1 ) );
                                        break;
                                    }
      
                        case '-':   eat_char();  _token._rtf.set_code( code_dash  );  break;
                        case ':':   eat_char();  _token._rtf.set_code( code_colon );  break;
                        case '|':   eat_char();  _token._rtf.set_code( code_bar   );  break;
                        case '~':   eat_char();  _token._rtf.set_code( code_tilde );  break;

                        case '*':   eat_char();  star = true;  goto AGAIN;   // Nach "\*" kann ein Zeilenwechsel kommen, also geh über Los

                        default:    {
                                        string name;

                                        while( isalpha( _next_char ) )  name += (char)eat_char();

                                        bool has_param = false;
                                        int  sign      = _next_char == '-'? ( eat_char(), -1 ) : +1;
                                        int  param     = 0;
                                        while( isdigit( _next_char ) )  { param *= 10; param += eat_char() - '0'; has_param = true; }
                                        param *= sign;


                                        if( star )  _token._rtf.set_code( _rtf_context->code_from_name( "*\\" + name ) );
                                              else  _token._rtf.set_code( _rtf_context->code_from_name(         name ) );

                                        if( _token._rtf.code() == code_any )
                                        {
                                            _token._rtf.set_code( 
                                                _rtf_context->add_rtf_descr( name,
                                                                             star     ? kind_destination :
                                                                             has_param? kind_value 
                                                                                      : kind_flag,
                                                                             star     ? flag_star 
                                                                                      : flag_none ) );

                                            if( !_non_doc_property_discovered  &&  _group_nesting == 1 )
                                            {
                                                // Alle unbekannten, hinzugefügten Codes vor \fonttbl etc. sind gelernte Dokumenteigenschaften (für factory_rtf.cxx)

                                                Descr* d = &_rtf_context->_descr_array[ _rtf_context->_descr_array.size() - 1 ];
                                                Z_LOG2( "rtf.learn", "\\" << d->_name << " ist anscheinend eine Dokumenteigenschaft\n" );
                                                d->_flags |= flag_learned_prop_doc;
                                            }
                                        }

                                        if( _token._rtf.code() >= code__predefined_count  &&  has_param  &&  param == 0 )  _rtf_context->set_rtf_descr_no_default_param( _token._rtf.code() );  // Parameter 0 soll ausgegeben werden.

                                        _token._rtf.set_param( param );

                                        if( is_on_off( _token._rtf.code() ) ) {
                                            if( !has_param  ||  _token._rtf.param() != 0 )  _token._rtf.set_param( 1 );
                                        }

                                        if( _next_char == ' ' )  eat_char();

                                        if( _token._rtf.code() == code_any )  goto AGAIN;

                                        if( !_non_doc_property_discovered  &&  _group_nesting == 1  &&  !_token._rtf.is_doc_property()  
                                         &&  _token._rtf.code() != code_rtf   &&  _token._rtf.code() != code_pwd )
                                        {
                                            _non_doc_property_discovered = true;
                                        }
                                    }
                    }
                    break;

        case '{':   _token._rtf.set_code( code_klammer_auf );  eat_char();  _group_nesting++;  break;
        case '}':   _token._rtf.set_code( code_klammer_zu  );  eat_char();  _group_nesting--;  break;

        case EOF:   _token._rtf.set_code( code_eof );
                    delete [] _hilfspuffer;     // Speicher schon mal freigeben
                    _hilfspuffer = NULL; 
                    break;

        default:    // Normaler Text
                    {
                        _token._rtf.set_code( code_text );
                        _token._rtf._text.erase();

                        const int size = 100000;
                        if( !_hilfspuffer )  _hilfspuffer = new char [size];
                        char* h = _hilfspuffer;
                        
                        while(1)
                        {
                            while( _next_char != EOF  
                               &&  _next_char != '{'  
                               &&  _next_char != '}'  
                               &&  _next_char != '\\'  
                               &&  _next_char != '\r'  
                               &&  _next_char != '\n' ) 
                            {
                                if( h == _hilfspuffer + size )  _token._rtf._text.append( _hilfspuffer, h - _hilfspuffer ),  h = _hilfspuffer;
                                *h++ = (char)eat_char();
                            }


                            if( _next_char != '\r'  &&  _next_char != '\n' )  break;

                            // Text in der folgenden Zeile hinzunehmen
                            while( _next_char == '\r'  ||  _next_char == '\n' )  eat_char();
                        }

                        _token._rtf._text.append( _hilfspuffer, h - _hilfspuffer );

                        if( _token._rtf._text.empty() ) {
                            if( _next_char == EOF )  _token._rtf.set_code( code_eof );
                            goto AGAIN;
                        }
                    }
    }
}

//------------------------------------------------------------------------Parser::ignore_group

void Parser::ignore_group( int start_nesting )
{
    // "{ ... }" verwerfen
    
    //std::cerr << "ignore_group: " << _token._rtf << '\n';
    
    Source_pos pos = _token._pos;

    int nesting = start_nesting;

    while(1)
    {
        switch( _token._rtf.code() )
        {
            case code_klammer_auf:
                nesting++;
                break;

            case code_klammer_zu:
                nesting--;
                break;

            case code_text:
                //? parse_text();    // Liest evtl. weitere Tokens, s.a. parse_inline_expr()
                break;

            case code_eof:
                throw_xc( "Z-RTF-PARSER-101" );

            default:
                break;
        }

        get_next_token();

        if( nesting == 0 )  break;
    }
}

//-----------------------------------------------------------------------------Parser::eat

void Parser::eat( Code code )
{
    if( _token._rtf.code() != code )  throw_xc( "Anderer RTF-Code erwartet", "", _token._pos );
    get_next_token();
}

//-----------------------------------------------------------Parser::parse_any_destination

void Parser::parse_any_destination( Code code, int param )
{
    Destination_entity* e = _doc->append_destination( code, param );

    parse_rest_of_group( code );

    _doc->end_destination( e );
}

//---------------------------------------------------------------Parser::parse_destination

void Parser::parse_destination()
{
    Code code  = _token._rtf.code();
    int  param = _token._rtf.param();
    get_next_token();
    parse_any_destination( code, param );
}

//-----------------------------------------------------------Parser::parse_non_destination

void Parser::parse_non_destination()
{
    switch( _token._rtf.code() )
    {
        case code_s:
        {
            Doc::Style* style = _doc->_header.style( _token._rtf.param() );
            if( style )  _token._rtf.set_text( style->_name );              // Für rtf2xml
            _doc->append_entity( _token._rtf );
            break;
        }

        case code_eof:
            throw_xc( "Z-RTF-PARSER-101" );


        // Diese Code für nach links laufende Schrift und assozierte Eigenschaften ignorieren wir, denn deren Behandlung ist unklar.
        // Werden sie durchgelassen, ist das RTF-Dokument nicht mehr korrekt.
        // Anscheinend bilden \rtfch und \ltrch eine Klammer mit den assozierten Eigenschaften darin. Diese Klammer müsste erhalten bleiben.
        // Die Anwendung der nach links laufenden Schrift ist unklar.
        // Joacim 9.1.03
        case code_rtlch:
        case code_ltrch:
        case code_ab:
        case code_acaps:
        case code_acf:
        case code_adn:
        case code_aexpnd:
        case code_af:
        case code_afs:
        case code_ai:
        case code_alang:
        case code_aoutl:
        case code_ascaps:
        case code_ashad:
        case code_astrike:
        case code_aul:
        case code_auld:
        case code_auldb:
        case code_aulnone:
        case code_aulw:
        case code_aup:

        case code_loch:         // 20.9.04 Hinzugefügt (nur \loch)
        case code_hich:
        case code_dbch:

        // RTF 1.8: RSIDs (Revision Save IDs) indicate when text or a property was changed.
        case code_charrsid:     
        case code_delrsid:
        case code_insrsid:
        case code_pararsid:
        case code_rsid:
        case code_rsidroot:
      //case code_rsidtbl:  // s. parse_destination()
        case code_sectrsid:
        case code_tblrsid:      
            break;

        default: 
            _doc->append_entity( _token._rtf );
    }

    get_next_token();
}

//-------------------------------------------------------------------Parser::restore_state

void Parser::restore_state()
{
    //! Optimierungspotenzial: Die Schleife wird 750mal durchlaufen, bei (fast) jedem "}".

    std::vector< Properties >::iterator to_be_restored = _properties_stack.end() - 1;

    for( Code c = code__properties_min; c < code__properties_max; c++ ) 
    {
        if( descr(c)->has_param() )
        {
            int property_value = to_be_restored->property( c );

            //jz 2006-09-04  Nachfolgendes if() ist neu
            // Bei ignore_pgdsctbl(false): {\*\pgdsctbl ... {\header {\b fett}..}}: bei "fett}" nur \b0 ausgeben.
            // set_property() gibt alles aus wegen _is_in_doc_prop_destination (dabei ist \header keine Dokumenteigenschaft)
            // Andere Lösung: _is_in_doc_prop_destination bei \header vorübergehend auf 0 setzen
            // Die tiefere Bedeutung von _is_in_doc_prop_destination sollte geklärt werden.
            // Erstmal nur bei !_ignore_pgdsctbl
            if( descr( code_pgdsctbl )->_kind == kind_ignore_destination  ||  _doc->_properties[c] != property_value )        
            {
                _doc->set_property( c, property_value );
            }
        }
    }

    // 18.1.06 \super wird jetzt zurückgesetzt
    if( to_be_restored->is_set( code_super ) != _doc->_properties.is_set( code_super )
     || to_be_restored->is_set( code_sub   ) != _doc->_properties.is_set( code_sub   ) )
    {
        if( to_be_restored->is_set( code_super ) )  _doc->append_entity( Simple_entity( code_super ) );
        else
        if( to_be_restored->is_set( code_sub   ) )  _doc->append_entity( Simple_entity( code_sub ) );
        else
                                                    _doc->append_entity( Simple_entity( code_nosupersub ) );
    }

    _properties_stack.pop_back();
}

//------------------------------------------------------------Parser::parse_rest_of_group

void Parser::parse_rest_of_group( Code outer_destination_code )
{
    size_t intial_properties_stack_size = _properties_stack.size();

    while(1)
    {
        if( _token._rtf.code() == code_klammer_auf )
        {
            get_next_token();

            if( is_destination( _token._rtf.code() ) )
            {
                _properties_stack.push_back( _doc->_properties );
                if( has_own_properties( _token._rtf.code() ) )  _doc->_properties.reset_properties( Flags( flag_prop_char | flag_prop_para_complete ) );
                parse_destination();

                _doc->_properties = *last( _properties_stack );
                _properties_stack.pop_back();
            }
            else
            if( !has_text( outer_destination_code ) )
            {
                _properties_stack.push_back( _doc->_properties );

                // Im Kopf werden manche RTF-Codes in anderer Bedeutung verwendet. Das wird nun unterschieden:

                if( outer_destination_code == code_fonttbl  &&  _token._rtf.code() == code_f ) {
                    _token._rtf.set_code( code_f_fonttbl ); // In \fonttbl ist \f was anderes
                }
                else               
                if( outer_destination_code == code_stylesheet ) {
                    if( _token._rtf.code() == code_s  )  _token._rtf.set_code( code_s_stylesheet );
                    if( _token._rtf.code() == code_cs )  _token._rtf.set_code( code_cs_stylesheet );
                }

                parse_any_destination( code_group );

                _doc->_properties = *last( _properties_stack );
                _properties_stack.pop_back();
            }
            else
            if( descr( _token._rtf.code() )->_kind == kind_ignore_destination )
              //&& ( !_dont_ignore_pgdsctbl || _token._rtf.code() != code_code_pgdsctbl ) )
            {
                ignore_group( 1 );
            }
            else
            {
                //std::cerr << "push properties " << *last( _properties_stack ) << '\n'; 
                _properties_stack.push_back( _doc->_properties );
            }
        }
        else
        if( _token._rtf.code() == code_klammer_zu )
        {
            get_next_token();
            if( _properties_stack.size() == intial_properties_stack_size )  break;
            restore_state();
        }
        else
        {
            parse_non_destination();
            //throw_xc( "RTF2XML-103", "RTF-Kopf nicht erkennbar" );
        }
    }
}

//---------------------------------------------------------------------------Parser::parse

void Parser::parse()
{
    init();

    if( !_doc )  _doc = Z_NEW( Doc( has_rtf_context() ) );
    
    //_properties_stack.push_back( _doc->_properties );

    eat_char();
    get_next_token();       // Erstes Token

    if( _token._rtf.code() != code_klammer_auf )  
        throw_xc( "Z-RTF-PARSER-100" );

    get_next_token();

    if( _token._rtf.code() != code_rtf  ||  _token._rtf.param() != 1 )  
        throw_xc( "Z-RTF-PARSER-100" );

    get_next_token();

    parse_rest_of_group( code_rtf );

    if( _token._rtf.code() != code_eof )  throw_xc( "Z-RTF-PARSER-102", _token._rtf.text() );

    _doc->optimize();
}

//---------------------------------------------------------------------------------------------

} //namespace rtf
} //namespace zschimmer
