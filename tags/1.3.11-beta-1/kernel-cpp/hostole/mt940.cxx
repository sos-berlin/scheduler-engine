// $Id$        Joacim Zschimmer

#include "precomp.h"
#include "../kram/olestd.h"
#include "../kram/sosopt.h"
#include "hostole2.h"

#include "../zschimmer/zschimmer.h"
using zschimmer::replace_regex_ref;


#ifdef SYSTEM_WIN

#   import <msxml3.dll> rename_namespace("xml")

    namespace xml 
    {
        typedef IXMLDOMElement          Element;
        typedef IXMLDOMElementPtr       Element_ptr;
        typedef IXMLDOMTextPtr          Text_ptr;
        typedef IXMLDOMCDATASectionPtr  Cdata_section_ptr;
        typedef IXMLDOMNode             Node;
        typedef IXMLDOMNodePtr          Node_ptr;
        typedef IXMLDOMNodeList         NodeList;
        typedef IXMLDOMNodeListPtr      NodeList_ptr;
        typedef IXMLDOMDocument         Document;
        typedef IXMLDOMDocumentPtr      Document_ptr;
        typedef IXMLDOMDocumentTypePtr  DocumentType_ptr;
    }
#endif



namespace sos {


typedef _bstr_t                 Dom_string;

template<typename T>
inline Dom_string               as_dom_string               ( const T& t )                          { return as_bstr_t( t ); }

//--------------------------------------------------------------------------------------------Mt940

struct Mt940
{
                                Mt940                       ( const string& options );

    void                        parse                       ( BSTR source );
    
    int                         eat_char                    ();
    void                        eat_char                    ( char );
    string                      eat_numeric_string          ();
    string                      eat_numeric_string          ( int fixed_length );
    string                      eat_string                  ( int fixed_length );
    string                      eat_string_until            ( string delimiter );
    string                      eat_date_string             ( const char* format );
    string                      eat_debit_credit_as_sign    ();                             // Liefert "" oder "-"
    string                      eat_amount                  ();
    string                      eat_rest_of_line            ();
    void                        eat_line_end                ();
    void                        optional_lineend            ();

    int                         next_next_char              ()                              { return _input_ptr < _input_end? (uint)_input_ptr[0] : EOF; }

    void                        parse_bloc                  ( const xml::Element_ptr& );
    void                        parse_msg                   ( const xml::Element_ptr& );
    void                        parse_unknown_bloc          ( const xml::Element_ptr& );

    void                        parse_field                 ( const xml::Element_ptr& );

    Fill_zero                  _zero_;
    xml::Document_ptr          _doc;
    xml::Element_ptr           _mt940_element;
    Source_pos                 _pos;
    wchar_t*                   _input_ptr;
    wchar_t*                   _input_end;
    int                        _next_char;
    bool                       _schweizer_postfinance;
    bool                       _bank_austria;
};

//----------------------------------------------------------------------------------dom_append_text

void dom_append_text( const xml::Element_ptr& element, const string& text )
{
    xml::Document_ptr doc       = element->ownerDocument;
    xml::Node_ptr     text_node = doc->createTextNode( as_dom_string( text ) );
    
    element->appendChild( text_node );
}

//--------------------------------------------------------------------------dom_append_text_element

void dom_append_text_element( const xml::Element_ptr& element, const char* element_name, const string& text )
{
    xml::Element_ptr e = element->ownerDocument->createElement( element_name );

    dom_append_text( e, text );
    element->appendChild( e );
}

//------------------------------------------------------------------------------------set_attribute

void set_attribute( const xml::Element_ptr& element, const char* name, const char* value )
{
    element->setAttribute( name, value );
}

//------------------------------------------------------------------------------------set_attribute

void set_attribute( const xml::Element_ptr& element, const char* name, const string& value )
{
    element->setAttribute( name, as_dom_string(value) );
}

//------------------------------------------------------------------------------------set_attribute

void set_attribute( const xml::Element_ptr& element, const char* name, char value )
{
    char str[2] = "x";
    str[0] = value;

    set_attribute( element, name, str );
}

//-------------------------------------------------------------------------------------Mt940::Mt940

Mt940::Mt940( const string& options )
:
    _zero_(this+1)
{
    for( Sos_option_iterator opt = options; !opt.end(); opt.next() )
    {
        if( opt.with_value( "filename" ) )  _pos.filename( opt.value().c_str() );
        else
            throw_sos_option_error(opt);
    }

    _doc = xml::Document_ptr( __uuidof(xml::DOMDocument30), NULL );
    _doc->appendChild( _doc->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
}

//----------------------------------------------------------------------------------Mt940::eat_char

int Mt940::eat_char()
{
    if( _next_char == '\n' )  _pos._col = 0, _pos._line++;
                        else  _pos._col++;

    int result = _next_char;
    if( result == EOF )  return EOF;

    if( *_input_ptr == 0 )  _next_char = EOF;
    else  
    {
        _next_char = (uint)*_input_ptr++;
        
        if( _bank_austria )
        {
            switch( _next_char )
            {
                case  381: _next_char = (uint)'Ä';  break;
             //?case     : _next_char = (uint)'Ö';  break;
                case  353: _next_char = (uint)'Ü';  break;
                case 8222: _next_char = (uint)'ä';  break;
             //?case     : _next_char = (uint)'ö';  break;
                case  129: _next_char = (uint)'ü';  break;
                case  225: _next_char = (uint)'ß';  break;
                default  : break;    
            }
        }
    }

    return result;
}

//----------------------------------------------------------------------------------Mt940::eat_char

void Mt940::eat_char( char c )
{
    if( eat_char() != c )  
    {
        string str = as_string(c);
        throw_syntax_error( "MT940-001", str.c_str(), _pos );
    }
}

//------------------------------------------------------------------------------Mt940::eat_line_end

void Mt940::eat_line_end()
{
    if( _next_char == EOF  )  return;
    if( _next_char == '}'  )  return;
    if( _next_char == '\r' )  eat_char();
    if( _next_char != '\n' )  throw_syntax_error( "MT940-001", "Zeilenende", _pos );
    eat_char();
}

//--------------------------------------------------------------------------Mt940::optional_lineend

void Mt940::optional_lineend()
{
    if( _next_char == '\r'  ||  _next_char == '\n' )  eat_line_end();
}

//------------------------------------------------------------------------Mt940::eat_numeric_string

string Mt940::eat_numeric_string()
{
    string result;

    optional_lineend();

    if( !isdigit( _next_char ) )  throw_syntax_error( "MT940-001", "Ziffer", _pos );
    while( isdigit( _next_char ) )  result += eat_char();

    return result;
}

//------------------------------------------------------------------------Mt940::eat_numeric_string

string Mt940::eat_numeric_string( int length )
{
    string result;
    
    optional_lineend();

    for( int i = 0; i < length; i++ )  
    {
        if( !isdigit( _next_char ) )  throw_syntax_error( "MT940-001", "Ziffer", _pos );
        result += eat_char();
    }

    return result;
}

//--------------------------------------------------------------------------------Mt940::eat_string

string Mt940::eat_string( int length )
{
    string result;

    optional_lineend();

    Source_pos orig_pos = _pos;
    
    for( int i = 0; i < length; i++ )  
    {
        if( _next_char < ' '  ||  _next_char == '}' )  { string t = as_string(length); throw_syntax_error( "MT940-005", t.c_str(), orig_pos ); }
        result += eat_char();
    }

    return string( result.c_str(), length_without_trailing_spaces( result ) );
}

//--------------------------------------------------------------------------Mt940::eat_string_until

string Mt940::eat_string_until( string delimiter )
{
    string result;
    
    assert( delimiter.length() == 2 );

    while(1)
    {
        if( _next_char == EOF )  break;
        if( _next_char == '}' )  break;
        if( _next_char == '\r' )  break;
        if( _next_char == '\n' )  break;

        if( _next_char == delimiter[0] )
        {
            eat_char();
            if( _next_char == delimiter[1] )  { eat_char(); break; }
            result += delimiter[0];
        }
        result += eat_char();
    }

    return result;
}

//---------------------------------------------------------------------------Mt940::eat_date_string

string Mt940::eat_date_string( const char* format )
{
    Sos_optional_date_time  date;

    date.read_text( eat_numeric_string( strlen(format) ), format );
    return date.formatted( "yyyy-mm-dd" );
}

//------------------------------------------------------------------Mt940::eat_debit_credit_as_sign

string Mt940::eat_debit_credit_as_sign()
{
    char c = eat_char();

    if( c == 'C' )  return "";      // credit
    if( c == 'D' )  return "-";     // debit

    throw_syntax_error( "MT940-001", "C oder D", _pos );
    return "";
}

//--------------------------------------------------------------------------------Mt940::eat_amount

string Mt940::eat_amount()
{
    string amount;

    while( isdigit(_next_char) || _next_char == ',' )  amount += _next_char == ','? '.' : _next_char,  eat_char();

    return amount;
}

//--------------------------------------------------------------------------Mt940::eat_rest_of_line

string Mt940::eat_rest_of_line()
{
    string result;
    while( _next_char != '\r'  &&  _next_char != '\n'  &&  _next_char != '}'  &&  _next_char != EOF )  result += eat_char();
    eat_line_end();
    return result;
}

//-------------------------------------------------------------------------------Mt940::parse_field

void Mt940::parse_field( const xml::Element_ptr& element )
{
    string field_name;
    string text;

    if( _next_char == '-' )     // Schweizer Postfinance
    {
        eat_char();
        return;
    }

    if( _next_char == '\r' || _next_char == '\n' )    // Bank Austria
    {
        eat_line_end();
        return;
    }

    eat_char( ':' );
    while( isalnum( _next_char ) )  field_name += eat_char();
    eat_char( ':' );

    string   element_name      = "f" + field_name;
    _bstr_t  element_name_bstr = element_name.c_str();
    string   field_name2       = field_name.substr(0,2);

    //append_comment( element, "Field " + field_name );

    if( field_name == "20" )    // Laufnummer der Meldung (Schweizer Postfinance)
    {
        dom_append_text_element( element, element_name_bstr, eat_rest_of_line() );
/*
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //"transaction_reference_nr" );

            //set_attribute( e, "nr", eat_numeric_string(16) );
            set_attribute( e, "nr", eat_numeric_string() );
            eat_line_end();

        element->appendChild( e );
*/
    }
    else
    if( field_name == "21" )    // Bezugsreferenznummer (Commerzbank)
    {
        dom_append_text_element( element, element_name_bstr, eat_rest_of_line() );
/*
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //( "bezugsreferenznr" );

            set_attribute( e, "nr", eat_numeric_string() );
            eat_line_end();

        element->appendChild( e );
*/
    }
    else
    if( field_name == "25" )    // Kontobezeichnung, account identification
    {
        dom_append_text_element( element, element_name_bstr, eat_rest_of_line() );
/*
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //( "account" );

            string      text = eat_rest_of_line();
            const char* nr   = text.c_str();

            int pos = text.find( '/' );
            if( pos != string::npos )
            {
                set_attribute( e, "bank_code", text.substr( 0, pos ) );
                nr = text.c_str() + pos + 1;
            }

            while( *nr == '0' )  nr++;
            set_attribute( e, "nr", nr );

        element->appendChild( e );
*/
    }
    else
    if( field_name == "28"      // Auszugsnummer (veraltet, für Postbank)
     || field_name == "28C" )   // Auszugsnummer
    {
        dom_append_text_element( element, element_name_bstr, eat_rest_of_line() );
/*
        xml::Element_ptr e = _doc->createElement( element_name_bstr );   //"statement_nr" );

            set_attribute( e, "nr", eat_numeric_string() );     // statement number
        
            if( _next_char == '/' )
            {
                eat_char();
                set_attribute( e, "page", eat_numeric_string() );
            }

            eat_line_end();

        element->appendChild( e);
*/
    }
    else
    if( field_name2 == "60" || field_name2 == "62" )  // Saldo
    {
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //"balance" );
        
          //set_attribute( e, "qualifier", field_name.substr(2) );
          //set_attribute( e, "which"    , field_name2 == "60"? "opening" : "closing" );

            string sign = eat_debit_credit_as_sign();

            set_attribute( e, "date"     , eat_date_string( "yymmdd" ) );
            set_attribute( e, "currency" , eat_string(3) );
            set_attribute( e, "amount"   , sign + eat_amount() );
            eat_line_end();

        element->appendChild( e );
    }
    else
    if( field_name == "61" )    
    {
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //"statement_line" );

            string date1 = eat_date_string( "yymmdd" );
            string date2 = isdigit(_next_char)? replace_regex_ref( eat_string(4), "(..)(..)", "\1-\2" ) : "";

            if( _schweizer_postfinance )  set_attribute( e, "entry_date"   , date1 ),  set_attribute( e, "validity_date", date2 );
                                    else  set_attribute( e, "validity_date", date1 ),  set_attribute( e, "entry_date"   , date2 );  // Nur Bank Austria

            bool storno = false;
            if( _next_char == 'R' )  storno = true, eat_char();
            string sign = eat_debit_credit_as_sign();
            if( storno )  sign = sign == "-"? "" : "-",  set_attribute( e, "storno", "yes" );

            // Commerzbank: Hier ein optionales Zeichen für Währungsart: Die dritte Stelle der Währungsbezeichnung, wenn sie zur Unterscheidung wichtig ist.

            set_attribute( e, "amount"                        , sign + eat_amount() );
            set_attribute( e, "transaction_type"              , eat_string(4) );            // Commerzbank: Buchungsschlüssel
            
            if( _schweizer_postfinance )  // Auch Commerzbank?
            {
                set_attribute( e, "customer_reference"            , eat_string(5) );            
                set_attribute( e, "accounting_servicing_reference", eat_string(8) );        // Beginnt immer mit Trenner "//"?
                set_attribute( e, "supplementary_details"         , eat_string(32) );
                eat_line_end();
            } 
            else  // Bank Austria
            {  
                set_attribute( e, "customer_reference"            , eat_string_until("//") );       // "NONREF": Keine Referenz
                set_attribute( e, "institutions_reference"        , eat_rest_of_line() );
                if( _next_char != EOF  &&  _next_char != ':' )
                {
                    set_attribute( e, "belegref"                  , eat_string(8) );
                    set_attribute( e, "supplementary_details"     , eat_rest_of_line() );           // Zusatzinfo
                }
            }

        element->appendChild( e );
    }
    else
    if( field_name == "61R" )    // Bank Austria, Swift-Erweiterung für Belege, Referenz zu 61
    {
        xml::Element_ptr e = _doc->createElement( element_name_bstr );

            set_attribute( e, "entry_date"   , eat_date_string( "yymmdd" ) );
            set_attribute( e, "belegref"     , eat_string(8) );
            set_attribute( e, "type"         , eat_string(3) );                 // "TXT"

        element->appendChild( e );
    }
    else
/* Das Format stimmt nicht mit dem Beispiel überein
    if( field_name == "61" )    
    {
        xml::Element_ptr e = _doc->createElement( "umsatz" );

        Sos_optional_date_time valutadatum;
        valutadatum.read_text( eat_numeric_string(6), "yymmdd" );
        set_attribute( e, "valutadatum", as_dom_string( valutadatum.formatted( "yyyy-mm-dd" ) ) );

        Sos_optional_date_time buchungsdatum;
        int month = as_int( eat_numeric_string(2) );
        buchungsdatum.assign_date( buchungsdatum.month() == 1 && valutadatum.month() == 12? valutadatum.year() + 1 : valutadatum.year(),
                                   month,
                                   as_int( eat_numeric_string(2) ) );
        set_attribute( e, "buchungsdatum", as_dom_string( buchungsdatum.formatted( "yyyy-mm-dd" ) ) );

        if( _next_char == 'R' )  set_attribute( e, "storno", "yes" ), eat_char();
        
        if( _next_char == 'C' )  set_attribute( e, "art", "credit" ), eat_char();
        else
        if( _next_char == 'D' )  set_attribute( e, "art", "debit" ), eat_char();
                           else  throw_syntax_error( "MT940-004" );

        eat_char();  // Dritte Stelle der Währungsbezeichnung

        string betrag;
        while( isdigit(_next_char) || _next_char == ',' )  betrag += eat_char();
        set_attribute( e, "betrag", as_dom_string( betrag ) );

        set_attribute( e, "buchungsschlüssel", as_dom_string( eat_string(4) ) );

        set_attribute( e, "kundenreferenz", as_dom_string( eat_rest_of_line() ) );

        element->appendChild( e );
    }
    else
*/
    if( field_name == "86"  
     && ( _schweizer_postfinance || !isdigit(_next_char ) ) )       // Information to Account Order (Schweizer Postfinance)
    {                                                               // Bank Austria liefert in :86: manchmal unstruktierten Text (nicht dokumentiert)
        string text;                                                // Das versuchen wir an !isdigit() zu erkennen.

        while(1)
        {
            text += eat_rest_of_line();
            if( _next_char == EOF )  break;
            if( _next_char == '}' )  break;
            if( _next_char == ':' )  break;
            if( _next_char == '-' && next_next_char() == '}' )  break;
            //text += "\r\n";
        }

        dom_append_text_element( element, element_name_bstr, text );   // "buchungstext"
    }
    else
    if( field_name == "86" )        // Commerzbank, Bank Austria (außer manchmal unstruktierter Text, s.o.)
    {
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //"buchung" );

            set_attribute( e, "geschäftsvorfall", eat_string(3) );

            string verwendungszweck;
        
            if( _next_char == ' ' )  
            {
                verwendungszweck = eat_rest_of_line();
            }
            else
            if( _next_char != '\n'  &&  _next_char != EOF )
            {
                char trennzeichen = _next_char;

                while( _next_char == trennzeichen )
                {
                    eat_char();

                    string subnr = eat_numeric_string(2);
                    string text;

                    while(1)
                    {
                        if( _next_char == '\r' )  eat_char();
                        if( _next_char == '\n' )
                        {
                            eat_char();
                            if( _next_char == ':' )  break;
                        }

                        if( _next_char == EOF )  break;
                        if( _next_char == '}' )  break;
                        if( _next_char == trennzeichen )  break;
                        text += eat_char();
                    }

                    if( subnr[0] == '2' ) 
                    {
                        verwendungszweck += text;
                        if( text.length() < 27 )  verwendungszweck += ' ';
                    }
/*
                    if( subnr == "00" )  set_attribute( e, "buchungstext"        , text );
                    else
                    if( subnr == "10" )  set_attribute( e, "primanotennummer"    , text );
                    else
                    if( subnr[0] == '2' )  dom_append_text_element( e, "verwendungszweck", text );
                    else
                    if( subnr == "30" )  set_attribute( e, "blz"                 , text );
                    else
                    if( subnr == "31" )  set_attribute( e, "konto"               , text );
                    else
                    if( subnr == "32" )  set_attribute( e, "name1"               , text );
                    else
                    if( subnr == "33" )  set_attribute( e, "name2"               , text );
                    else
                    if( subnr == "34" )  set_attribute( e, "textschlüsselergänzung", text );
                    else
                    if( subnr == "60" )  set_attribute( e, "verwendungszweck1"   , text );
                    else
                    if( subnr == "61" )  set_attribute( e, "verwendungszweck2"   , text );
                    else
*/
                    {
                        string attr = "sub" + subnr;
                        e->setAttribute( as_dom_string( attr ), as_dom_string( text ) );
                    }
                }
            }
 
            if( !verwendungszweck.empty() )  dom_append_text( e, verwendungszweck );

        element->appendChild( e );
    }
    else
    if( field_name == "86E" )       // Bank Austria, Swift-Erweiterung für den Beleg
    {
        string text;
        int    next_linenr = 1;

        while( isdigit(_next_char) )
        {
            int linenr = as_uint( eat_numeric_string(2) );
            while( next_linenr < linenr )  text += "\n",  next_linenr++;
            text += eat_rest_of_line();

            if( _next_char == EOF )  break;
            if( _next_char == '}' )  break;
            if( _next_char == ':' )  break;
            if( _next_char == '-' )  break;
        }

        dom_append_text_element( element, element_name_bstr, text );   // "buchungstext"
    }
    else
    if( field_name == "NS"  )   // Auszugsnummer (veraltet, für Postbank)
    {
        xml::Element_ptr e = _doc->createElement( element_name_bstr );  //"ns" );

            while( _next_char != EOF  &&  _next_char != '}'  &&  _next_char != ':' )
            {
                string subnr = eat_numeric_string(2);
  /*
                if( subnr == "22" )
                {
                    set_attribute( e, "inhaber", eat_rest_of_line() );
                }
                else
                if( subnr == "23" )
                {
                    set_attribute( e, "sonderbezeichnung", eat_rest_of_line() );
                }
                else
                if( subnr == "24" )
                {
                    set_attribute( e, "zinssatz", eat_rest_of_line() );
                }
                else
                if( subnr == "32" )
                {
                    string text;
                    string art = eat_rest_of_line();
                    set_attribute( e, "kontoart", art );
                
                    if( art == "010" )  text = "Kontokorrent";
                    if( art == "020" )  text = "Tagesgeld";
                
                    if( !text.empty() )  set_attribute( e, "kontoart_text", text );
                }
                else
*/
                {
                    string attr = "sub" + subnr;
                    e->setAttribute( as_dom_string( attr ), as_dom_string( eat_rest_of_line() ) );
                }
            }

        element->appendChild( e );
    }
    else
    {
        string xml_name = "mt940." + field_name;
        xml::Element_ptr field_element = _doc->createElement( as_dom_string( xml_name ) );
        field_element->appendChild( _doc->createTextNode( as_dom_string( eat_rest_of_line() ) ) );
        element->appendChild( field_element );
    }
}

//------------------------------------------------------------------------Mt940::parse_unknown_bloc

void Mt940::parse_unknown_bloc( const xml::Element_ptr& element )
{
    string text;

    while( _next_char != '}'  &&  _next_char != EOF )  
    {
        if( _next_char == '{' )
        {
            if( !text.empty() )  dom_append_text( element, text ),  text = "";
            parse_bloc( element );
        }
        else
            text += eat_char();
    }

    if( !text.empty() )  dom_append_text( element, text );
}

//--------------------------------------------------------------------------------Mt940::parse_bloc

void Mt940::parse_bloc( const xml::Element_ptr& element )
{
    eat_char( '{' );
    
    string bloc_type;                                                           // Bloc identifier
    while( isalnum(_next_char) )  bloc_type += eat_char();               
    eat_char( ':' );

    if( bloc_type == "1" )
    {
        xml::Element_ptr e = _doc->createElement( "basic_header_bloc" );

            set_attribute( e, "application"              , eat_string( 1) );    // "F"
            set_attribute( e, "service"                  , eat_string( 2) );    // "01"
            set_attribute( e, "lt_adresse"               , eat_string(12) );    // "POFICHBEAXXX"
            set_attribute( e, "session_nr"               , eat_string( 4) );    // "0000"
            set_attribute( e, "sequence_nr"              , eat_string( 6) );    // "000000"

        element->appendChild( e );
    }
    else
    if( bloc_type == "2" ) 
    {
        xml::Element_ptr e = _doc->createElement( "application_header_bloc" );

            set_attribute( e, "io"                       , eat_string( 1) );    // "I"
            set_attribute( e, "message_type"             , eat_string( 3) );    // "940" oder "950"
            set_attribute( e, "receiver_address"         , eat_string(12) );
            set_attribute( e, "message_priority"         , eat_string( 1) );
          //set_attribute( e, "delivery_monitoring"      , eat_string( 1) );    // Nicht mehr seit 4.5.2004
          //set_attribute( e, "obsolescence_period"      , eat_string( 3) );    // Nicht mehr seit 4.5.2004

        element->appendChild( e );
    }
    else
    if( bloc_type == "3" ) 
    {
        xml::Element_ptr e = _doc->createElement( "user_header_bloc" );

            parse_bloc( e );

        element->appendChild( e );
    }
    else
    if( bloc_type == "4" ) 
    {
        xml::Element_ptr e = _doc->createElement( "text_bloc" );

            eat_line_end();
            while( _next_char != '}' )  parse_field( e ); 

        element->appendChild( e );
    }
    else 
    if( bloc_type == "5" ) 
    {
        xml::Element_ptr e = _doc->createElement( "trailer_bloc" );

            while( _next_char == '{' )  parse_bloc(e);

        element->appendChild( e );
    }
    else
    if( bloc_type == "108" )       // Verschachtelt im user_header_block "3"
    {
        xml::Element_ptr e = _doc->createElement( "bloc_108" );

        set_attribute( e, "datum"                    , eat_date_string("yymmdd") );
        set_attribute( e, "empfaengerland"           , eat_string(2) );    // "CH"
        set_attribute( e, "file_nr"                  , eat_string(4) );    // "0000"
        set_attribute( e, "zusatz_nr"                , eat_string(4) );    // "0000"

        element->appendChild( e );
    }
    else
    {
        //throw_syntax_error( "MT940-004", bloc_type );

        xml::Element_ptr e = _doc->createElement( L"bloc" );

            set_attribute( e, "bloc", as_dom_string( bloc_type ) );

            string text;

            while( _next_char != '}'  &&  _next_char != EOF )  
            {
                if( _next_char == '{' )
                {
                    if( !text.empty() )  dom_append_text( e, text ),  text = "";
                    parse_bloc( e );
                }
                else
                    text += eat_char();
            }

            if( !text.empty() )  dom_append_text( e, text );

        element->appendChild( e );
    }

    eat_char( '}' );
}

//---------------------------------------------------------------------------------Mt940::parse_msg

void Mt940::parse_msg( const xml::Element_ptr& element )
{
    xml::Element_ptr e = _doc->createElement( "msg" );

        eat_char( '\x01' );             // ASCII SOH (Start of Header)
        while( _next_char == '{' )  parse_bloc( e );
        eat_char( '\x03' );             // ASCII ETX (End of Text)

    element->appendChild( e );
}

//-------------------------------------------------------------------------------------Mt940::parse

void Mt940::parse( BSTR source )
{
    xml::Element_ptr element = _doc->createElement( "mt940" );
    
    _doc->appendChild( element );

    _input_ptr = source;
    _input_end = source + SysStringLen( source );

    eat_char();
    _pos._col = 0, _pos._line = 0;

    if( _next_char == '\x01' )          // Schweizer Postfinance
    {
        _schweizer_postfinance = true;

        while( _next_char == '\x01' )  parse_msg( element );
    }
    else                                
    {
        _bank_austria = true;  // Und vielleicht Commerzbank
        //while( _next_char != '-' )  parse_field( element );
        while( _next_char == ':' )  
        {
            parse_field( element );
            while( _next_char == '\r' || _next_char == '\n' )  eat_char();
        }
    }

    while( isspace( _next_char ) )  eat_char();
    if( _next_char != EOF  )  throw_syntax_error( "MT940-003", _pos );
}

//----------------------------------------------------------------------------Hostware::parse_mt940

STDMETHODIMP Hostware::Parse_mt940( BSTR source, BSTR options, IDispatch** result )
{
    HRESULT hr = NOERROR;

    LOGI( "Hostware::parse_mt940\n" );

    try
    {
        Mt940 mt940 ( bstr_as_string(options) );

        mt940.parse( source );

        *result = mt940._doc;
        (*result)->AddRef();
    }
    catch( const exception & x )  { hr = _set_excepinfo( x, "hostWare.parse_mt940" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.parse_mt940" ); }

    LOG( "Hostware::parse_mt940 fertig\n" );

    return hr;
}

//-------------------------------------------------------------------------------------------------

} //namespace sos