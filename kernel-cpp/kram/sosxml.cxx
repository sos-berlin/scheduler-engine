// sosxml.cxx, Joacim Zschimmer
// $Id$

#include "precomp.h"

#include "sosstrng.h"
#include "sos.h"
#include "log.h"
#include "sosfield.h"
#include "soslimtx.h"
#include "sosctype.h"
#include "sosprof.h"
#include "sosopt.h"
#include "../file/anyfile.h"

#include "sosxml.h"

using namespace std;
namespace sos {

//----------------------------------------------------------------------------------------const

const int max_xml_field_length   = 1024;
const int max_entity_name_length = 1024;


// S.a. xml_file.cxx und hostole.cxx

/*---------------------------------------------------------------------------------------------

Aus http://www.pemberley.com/janeinfo/latin1.html :

Latin 1 Characters

This chart shows the effects of numeric ampersand entities on your browser. To use these 
characters in your own HTML files, put the appropriate number into &#__; (e.g. "&#163;" 
for the British pound (currency) sign), or, for the 8-bit alphabetic characters, use the 
alternative standard HTML 2.0 entity in parentheses on the right. (These are the only
non-numeric character entities defined in HTML 2.0, except for "&amp;", "&lt;", and "&gt;", 
which should be used to escape the characters & < > in an HTML file, and "&quot;" to escape
a double-quote character in an attribute value.)

If the right column looks the same as the left column, you're losing the eighth bit somewhere.
If the characters in the right column don't match their descriptions, then your browser is 
translating incorrectly between ISO 8859-1 Latin 1 and your platform's native character set. 
(You can refer to a .gif image of the character glyphs that should be displayed in 8859-1 
Latin 1.)

Finally, note that positions 127-159 are not displayable characters in ISO 8859-1 Latin 1, 
and are not part of any HTML standard, so that HTML code such as "™" is incorrect, and will
be displayed differently in browsers on different platforms (probably often in ways that
you did not intend). See the next chart below for the (future) correct way of displaying 
characters which are in positions 130-159 in Microsoft Windows -- including such typographical 
niceties as "curly" quotes, dashes, ellipses, and the trademark symbol.

The following chart only tests the ISO 8859-1 compliance of your browser's non-proportional
font; to test the proportional font see the alternative Latin 1 chart using HTML Tables.

 

 32             160         Non-breaking space
 33    !        161    ¡    Inverted exclamation
 34    "        162    ¢    Cent sign
 35    #        163    £    Pound sterling
 36    $        164    ¤    General currency sign
 37    %        165    ¥    Yen sign
 38    &        166    ¦    Broken vertical bar
 39    '        167    §    Section sign
 40    (        168    ¨    Umlaut (dieresis)
 41    )        169    ©    Copyright
 42    *        170    ª    Feminine ordinal
 43    +        171    «    Left angle quote, guillemotleft
 44    ,        172    ¬    Not sign
 45    -        173    ­    Soft hyphen
 46    .        174    ®    Registered trademark
 47    /        175    ¯    Macron accent
 48    0        176    °    Degree sign
 49    1        177    ±    Plus or minus
 50    2        178    ²    Superscript two
 51    3        179    ³    Superscript three
 52    4        180    ´    Acute accent
 53    5        181    µ    Micro sign
 54    6        182    ¶    Paragraph sign
 55    7        183    ·    Middle dot
 56    8        184    ¸    Cedilla
 57    9        185    ¹    Superscript one
 58    :        186    º    Masculine ordinal
 59    ;        187    »    Right angle quote, guillemotright
 60    <        188    ¼    Fraction one-fourth
 61    =        189    ½    Fraction one-half
 62    >        190    ¾    Fraction three-fourths
 63    ?        191    ¿    Inverted question mark
 65    A        193    Á    Capital A, acute accent ("&Aacute;")
 66    B        194    Â    Capital A, circumflex accent ("&Acirc;")
 67    C        195    Ã    Capital A, tilde ("&Atilde;")
 68    D        196    Ä    Capital A, dieresis or umlaut mark ("&Auml;")
 69    E        197    Å    Capital A, ring ("&Aring;")
 70    F        198    Æ    Capital AE dipthong (ligature) ("&AElig;")
 71    G        199    Ç    Capital C, cedilla ("&Ccedil;")
 72    H        200    È    Capital E, grave accent ("&Egrave;")
 73    I        201    É    Capital E, acute accent ("&Eacute;")
 74    J        202    Ê    Capital E, circumflex accent ("&Ecirc;")
 75    K        203    Ë    Capital E, dieresis or umlaut mark ("&Euml;")
 76    L        204    Ì    Capital I, grave accent ("&Igrave;")
 77    M        205    Í    Capital I, acute accent ("&Iacute;")
 78    N        206    Î    Capital I, circumflex accent ("&Icirc;")
 79    O        207    Ï    Capital I, dieresis or umlaut mark ("&Iuml;")
 80    P        208    Ð    Capital Eth, Icelandic ("&ETH;")
 81    Q        209    Ñ    Capital N, tilde ("&Ntilde;")
 82    R        210    Ò    Capital O, grave accent ("&Ograve;")
 83    S        211    Ó    Capital O, acute accent ("&Oacute;")
 84    T        212    Ô    Capital O, circumflex accent ("&Ocirc;")
 85    U        213    Õ    Capital O, tilde ("&Otilde;")
 86    V        214    Ö    Capital O, dieresis or umlaut mark ("&Ouml;")
 87    W        215    ×    Multiply sign
 88    X        216    Ø    Capital O, slash ("&Oslash;")
 89    Y        217    Ù    Capital U, grave accent ("&Ugrave;")
 90    Z        218    Ú    Capital U, acute accent ("&Uacute;")
 91    [        219    Û    Capital U, circumflex accent ("&Ucirc;")
 92    \        220    Ü    Capital U, dieresis or umlaut mark ("&Uuml;")
 93    ]        221    Ý    Capital Y, acute accent ("&Yacute;")
 94    ^        222    Þ    Capital THORN, Icelandic ("&THORN;")
 95    _        223    ß    Small sharp s, German (sz ligature) ("&szlig;")
 96    `        224    à    Small a, grave accent ("&agrave;")
 97    a        225    á    Small a, acute accent ("&aacute;")
 98    b        226    â    Small a, circumflex accent ("&acirc;")
 99    c        227    ã    Small a, tilde ("&atilde;")
100    d        228    ä    Small a, dieresis or umlaut mark ("&auml;")
101    e        229    å    Small a, ring ("&aring;")
102    f        230    æ    Small ae dipthong (ligature) ("&aelig;")
103    g        231    ç    Small c, cedilla ("&ccedil;")
104    h        232    è    Small e, grave accent ("&egrave;")
105    i        233    é    Small e, acute accent ("&eacute;")
106    j        234    ê    Small e, circumflex accent ("&ecirc;")
107    k        235    ë    Small e, dieresis or umlaut mark ("&euml;")
108    l        236    ì    Small i, grave accent ("&igrave;")
109    m        237    í    Small i, acute accent ("&iacute;")
110    n        238    î    Small i, circumflex accent ("&icirc;")
111    o        239    ï    Small i, dieresis or umlaut mark ("&iuml;")
112    p        240    ð    Small eth, Icelandic ("&eth;")
113    q        241    ñ    Small n, tilde ("&ntilde;")
114    r        242    ò    Small o, grave accent ("&ograve;")
115    s        243    ó    Small o, acute accent ("&oacute;")
116    t        244    ô    Small o, circumflex accent ("&ocirc;")
117    u        245    õ    Small o, tilde ("&otilde;")
118    v        246    ö    Small o, dieresis or umlaut mark ("&ouml;")
119    w        247    ÷    Division sign
120    x        248    ø    Small o, slash ("&oslash;")
121    y        249    ù    Small u, grave accent ("&ugrave;")
122    z        250    ú    Small u, acute accent ("&uacute;")
123    {        251    û    Small u, circumflex accent ("&ucirc;")
124    |        252    ü    Small u, dieresis or umlaut mark ("&uuml;")
125    }        253    ý    Small y, acute accent ("&yacute;")
126    ~        254    þ    Small thorn, Icelandic ("&thorn;")
                255    ÿ    Small y, dieresis

------------------------------------------------------------------

From: Markus Kuhn <kuhn@cs.purdue.edu>
Newsgroups: comp.text.sgml, comp.std.internat, comp.infosystems.www.authoring.html
Date: Thu, 24 Apr 1997 23:57:52 -0500
Message-ID: <336039D0.FD4@cs.purdue.edu>
[Question: &#146; valid HTML or no?]

The characters 128-159 are not used in ISO 8859-1 and Unicode, the character sets of HTML. 
MS-Windows uses a superset of ANSI/ISO 8859-1, known to experts as "Code Page 1252 (CP1252)", 
a Microsoft-specific character set with additional characters in the 128-159 range (also 
known as the "C1" range).

All the CP1252 characters are also available in Unicode. For example the CP1252 character 146 
that you mentioned (RIGHT SINGLE QUOTATION MARK) has the Unicode number 8217, therefore you 
should use this number in order to conform to the HTML standard. Modern HTML browsers like 
Netscape 4.0 understand Unicode, and will automatically convert the Unicode character &#8217; 
back into the character 146 on MS-Windows machines, and into the appropriate character on other 
systems.

The official CP1252<->Unicode conversion table is printed in the Unicode 2.0 standard for instance, 
and is available on <ftp://ftp.informatik.uni-erlangen.de/pub/doc/ISO/charsets/> in the file 
ucs-map-cp1252. [See also the file 
ftp://ftp.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/CP1252.TXT at the official Unicode site.]

MS-Windows HTML-authoring software definitely should implement the conversion table below! Please 
forward this mail to the developers of your HTML authoring tool if this is currently done wrong.

The CP1252 characters that are not part of ANSI/ISO 8859-1, and that should therefore always be 
encoded as Unicode characters greater than 255, are the following:

 Windows   Unicode    Char.
  char.   HTML code   test         Description of Character
  -----     -----     ---          ------------------------
ALT-0130   &#8218;   ‚    Single Low-9 Quotation Mark
ALT-0131   &#402;    ƒ    Latin Small Letter F With Hook
ALT-0132   &#8222;   „    Double Low-9 Quotation Mark
ALT-0133   &#8230;   …    Horizontal Ellipsis
ALT-0134   &#8224;   †    Dagger
ALT-0135   &#8225;   ‡    Double Dagger
ALT-0136   &#710;    ˆ    Modifier Letter Circumflex Accent
ALT-0137   &#8240;   ‰    Per Mille Sign
ALT-0138   &#352;    Š    Latin Capital Letter S With Caron
ALT-0139   &#8249;   ‹    Single Left-Pointing Angle Quotation Mark
ALT-0140   &#338;    Œ    Latin Capital Ligature OE
ALT-0145   &#8216;   ‘    Left Single Quotation Mark
ALT-0146   &#8217;   ’    Right Single Quotation Mark
ALT-0147   &#8220;   “    Left Double Quotation Mark
ALT-0148   &#8221;   ”    Right Double Quotation Mark
ALT-0149   &#8226;   •    Bullet
ALT-0150   &#8211;   –    En Dash
ALT-0151   &#8212;   —    Em Dash
ALT-0152   &#732;    ˜    Small Tilde
ALT-0153   &#8482;   ™    Trade Mark Sign
ALT-0154   &#353;    š    Latin Small Letter S With Caron
ALT-0155   &#8250;   ›    Single Right-Pointing Angle Quotation Mark
ALT-0156   &#339;    œ    Latin Small Ligature OE
ALT-0159   &#376;    Ÿ    Latin Capital Letter Y With Diaeresis

-- 
Markus Kuhn, Computer Science grad student, Purdue
University, Indiana, US, email: kuhn@cs.purdue.edu

*/

static const char* symbol_names[ Xml_processor::sym__count ] =
{
    "{none}",
    "{dateiende}",
    "+",
    "=",
    "#CDATA",
    "<tag>",
    "</endtag>",
    "<?processinginstruction?>",
    "<!dtd!>",
    ">",
    "/>",
    "?>",
    "{\"string\"}",
    "{name}",
};



struct Predefined_entity
{ 
    const char* _name; 
    const char* _value;
};

const static Predefined_entity predefined_entities[] =
{
    { "nbsp"    , " " },
    { "Agrave"  , "À" },
    { "Aring"   , "Å" },
    { "Auml"    , "Ä" },
    { "Auml"    , "Ä" },
    { "Ouml"    , "Ö" },
    { "Uuml"    , "Ü" },
    { "aring"   , "å" },
    { "auml"    , "ä" },
    { "ouml"    , "ö" },
    { "uuml"    , "ü" },
    { "amp"     , "&" }, 
    { "lt"      , "<" }, 
    { "gt"      , ">" }, 
    { ""        , ""  }
};

//-------------------------------------------------------Xml_processor::Xml_processor

Xml_processor::Xml_processor()
:
    _zero_(this+1)
{
    _buffer.allocate_min( 10*1024 );

    const Predefined_entity* e = predefined_entities;
    while( e->_name[0] != '\0' )  { _entity_array.add( Entity( e->_name, e->_value ) );  e++; }
}

//-----------------------------------------------------------Xml_processor::init_read

void Xml_processor::init_write()
{
    if( length( _enclosing_tag ) == 0 )  _enclosing_tag  = "hostxml";
    if( length( _record_tag    ) == 0 ) {
        if( empty( _record_tag ) )  _record_tag = "record";
    }
}

//-----------------------------------------------------------Xml_processor::init_read

void Xml_processor::init_read()
{
    if( _line.size() == 0 ) {
        _line.allocate_min( 1024 );
        _line.char_ptr()[0] = '\0';
    }

    _ptr = _line.char_ptr();

    eat_char();
    _char_pos._line = 0;
    _char_pos._col = 0;
    _next_token._text.allocate_min( 2*max_xml_field_length );
    get_next_token();
}

//-----------------------------------------------------------Xml_processor::init_read

void Xml_processor::init_read( const char* text, int len )
{ 
    _line.allocate_min( len + 1 );
    _line = text; 
    _line.char_ptr()[ len ] = '\0';

    init_read(); 
}

//--------------------------------------------------------Xml_processor::get_encoding

void Xml_processor::get_encoding( const Sos_string& encoding )
{
    _iso_8859_1 = false;
    _windows_1252 = false;

    if( encoding == "windows-1252" ) {
        _windows_1252 = true;
    }
    else
    if( encoding == "iso-8859-1" ) {
        _iso_8859_1 = true;
    }
    else
    if( encoding == "" ) {
        // Default
    }
    else throw_xc( "SOS-1399" );
}

//--------------------------------------------------------Xml_processor::put_xml_line

void Xml_processor::put_xml_line( const Const_area& )
{
}

//----------------------------------------------------------Xml_processor::start_line

void Xml_processor::start_line()
{
    if( length( _indent_string ) > 0 ) 
    {
        for( int i = 1; i <= _nesting; i++ ) 
        {
            _buffer.append( _indent_string );
        }
    }
}

//--------------------------------------------------------Xml_processor::put_start_tag

void Xml_processor::put_start_tag( const char* tag )
{
    start_line();
    _buffer += "<";
    append_field_name( tag );
    _buffer += '>';
    put_xml_line( _buffer );
}

//----------------------------------------------------------Xml_processor::put_end_tag

void Xml_processor::put_end_tag( const char* tag )
{
    start_line();
    _buffer += "</";
    append_field_name( tag );
    _buffer += '>';
    put_xml_line( _buffer );
}

//----------------------------------------------------------Xml_processor::append_cdata

void Xml_processor::append_cdata( const Byte* start, int len )
{
    const Byte* p     = start;
    const Byte* p_end = start + len;

    while( p < p_end ) 
    {
        if( _iso_8859_1  ? *p >= 0x20  &&  *p <= 0x7E  ||  *p >= 0xA0 :
            _windows_1252? *p >= 0x20  &&  *p <= 0x7E  ||  *p >= 0x80
                         : *p >= 0x20  &&  *p <= 0x7E )
        {
            switch( *p ) 
            {
                case '<': _buffer.append( "&lt;"  );  p++;  break;
                case '>': _buffer.append( "&gt;"  );  p++;  break;
                case '&': _buffer.append( "&amp;" );  p++;  break;
                case ' ': if( p == start  ||  p[-1] == ' ' )  { _buffer.append( "&#32;" ); p++; break; }  // Mehrere Leerzeichen
                default:  _buffer.append( *p++ );
            }
        }
        else
        {
            switch( (char)*p ) 
            {
                default:  
                {
                    _buffer.resize_min( _buffer.length() + 6 );     // "&#...;"
                    char* b = _buffer.char_ptr() + _buffer.length();
                    int n = (Byte)*p;
                    *b++ = '&';
                    *b++ = '#';
                    if( n >= 100 )  *b++ = '0' + n / 100;
                    if( n >=  10 )  *b++ = '0' + ( n / 10 ) % 10;
                    *b++ = '0' + n % 10;
                    *b++ = ';';
                    _buffer.length( b - _buffer.char_ptr() );

                    if( p[0] == '\n' )  // Damit die Zeile nicht zu lang wird, eine Auflockerung:
                    {
                        if( p > start  &&  p[-1] == '\r' )  _buffer.append( '\r' );
                        _buffer.append( '\n' );
                    }
                    p++;
                }
            }
        }
    }
}

//--------------------------------------------------Xml_processor::append_cdata_section

void Xml_processor::append_cdata_section( const Sos_string& value )
{
    static const Sos_string end = "]]>";

    int pos = position( value, end );
    if( pos >= 0  &&  pos < (int)length(value) ) 
    {
        append_cdata( value );
    }
    else
    {
        _buffer.resize_min( _buffer.size() + 9 + length( value ) + 3 );

        _buffer.append( "<![CDATA[" );
        _buffer.append( value );
        _buffer.append( end );
    }
}

//-----------------------------------------------------------Xml_processor::append_field_name

void Xml_processor::append_field_name( const char* name )
{
    _buffer.append( name );

    if( _ucase )  Area( _buffer.char_ptr() + _buffer.length() - length( name ), length( name ) ).upper_case();
    else
    if( _lcase )  Area( _buffer.char_ptr() + _buffer.length() - length( name ), length( name ) ).lower_case();
}

//-----------------------------------------------------------Xml_processor::write_field

void Xml_processor::write_data( const char* name, const Field_type* type, const Byte* p, int rest )
{
    if( rest < (int)type->field_size() )  return;    // Sollte nicht vorkommen
    if( _suppress_null  &&  type->null( p ) )  return;

    _buffer.resize_min( 3 * type->field_size() );

    type->write_text( p, &_field_buffer, _text_format );
    _field_buffer.length( length_without_trailing_spaces( _field_buffer.char_ptr(), _field_buffer.length() ) );

    if( _suppress_empty  &&  _field_buffer.length() == 0 )  return;

    start_line();
    _buffer += "<";
    append_field_name( name );

    if( _field_buffer.length() == 0 )
    {
        _buffer += "/>";            // Empty tag
        put_xml_line( _buffer );
    }
    else
    {
        _buffer += '>';

        append_cdata( _field_buffer );

        _buffer += "</";
        append_field_name( name );
        _buffer += '>';

        put_xml_line( _buffer );
    }
}

//-----------------------------------------------------------Xml_processor::write_field

void Xml_processor::write_field( const Field_descr* field, const Byte* p, int rest )
{
    if( rest < (int)field->type_ptr()->field_size() )  return;   // Sollte nicht vorkommen
    if( _suppress_null  &&  field->null( p ) )  return;

    write_data( field->name(), field->type_ptr(), p + field->offset(), rest );
}

//-------------------------------------------------------Xml_processor::write_array_elem

void Xml_processor::write_array_elem( const Array_field_descr* field, int index, const Byte* p, int rest )
{
    const Field_type*   type    = field->type_ptr();
    int                 offset  = field->elem_offset( index, _array_level - 1 );

    if( rest < offset + (int)field->type_ptr()->field_size() )  return;  // Sollte nicht vorkommen
    if( _suppress_null && type->null( p + offset ) )  return;

    start_line();
    _buffer += "<";
    append_field_name( field->name() );
    _buffer += " index=\"";
    _buffer += as_string( index );
    _buffer += "\">";

    if( type->obj_is_type( tc_Record_type ) )
    {
        put_xml_line( _buffer );

        const Record_type* t = (const Record_type*)type;
        if( !t->flat_scope() )  throw_xc( "SOS-1408", c_str( t->name() ) );
        write_record_fields( t, p + offset - field->offset(), rest );

        start_line();
    }
    else
    {
        type->write_text( p + offset, &_field_buffer );
        append_cdata( _field_buffer );
    }

    _buffer += "</";
    append_field_name( field->name() );
    _buffer += '>';

    put_xml_line( _buffer );
}

//------------------------------------------------------------Xml_processor::write_array

void Xml_processor::write_array( const Array_field_descr* field, const Byte* p, int rest )
{
    const Array_field_descr::Dim* dim = &field->_dim[ _array_level - 1 ];
    
    int end = dim->_first_index + dim->_elem_count;

    for( int i = dim->_first_index; i < end; i++ )
    {
        write_array_elem( field, i, p, rest );
    }
}

//---------------------------------------------------Xml_processor::write_record_fields

void Xml_processor::write_record_fields( const Record_type* record_type, const Byte* p, int rest )
{
    _nesting++;

    for( int i = 0; i < record_type->field_count(); i++ ) 
    {
        Field_descr* field = record_type->field_descr_ptr( i );
        if( field ) 
        {
            const Field_type*   type        = field->type_ptr();

            if( type ) 
            {
                if( field->obj_is_type( tc_Array_field_descr ) 
                 && ((Array_field_descr*)field)->_level == _array_level + 1 ) 
                {
                    _array_level++;
                    write_array( (Array_field_descr*)field, p, rest );
                    _array_level--;
                }
                else
                if( type->obj_is_type( tc_Record_type ) )
                { 
                    put_start_tag( field->name() );
                    const Record_type* t = (const Record_type*)type;
                    if( !t->flat_scope() )  throw_xc( "SOS-1408", c_str( t->name() ) );
                    write_record_fields( t, p, rest );
                    put_end_tag( field->name() );
                }
                else
                {
                    write_field( field, p, rest );
                }
            }
        }
    }

    _nesting--;
}

//------------------------------------------------------------------------fields_as_xml

Sos_string fields_as_xml( const Record_type* type, const Const_area& record )
{
    Xml_processor processor;

    processor.write_record_fields( type, record.byte_ptr(), record.length() );

    return as_string( processor._buffer );
}

//------------------------------------------------------------Xml_processor::eat_char

int Xml_processor::eat_char()
{
    int current_char = _next_char;
    _eaten_char_pos._line = _char_pos._line;
    _eaten_char_pos._col  = _char_pos._col;

    if( _next_char != EOF ) 
    {
        _next_char = (Byte)*_ptr++;

        while( _next_char == '\0' ) 
        {
            try {
                get_xml_line( &_line );
                _line.append( '\0' );
                //_next_char = (Byte)*_ptr++;
                _next_char = '\n';
                _char_pos._line++;
            }
            catch( const Eof_error& )
            {
                _next_char = EOF;
                _line.char_ptr()[0] = '\0';
            }

            _ptr = _line.char_ptr();
        }

    }

    _char_pos._col = _ptr - _line.char_ptr();

    return current_char;
}

//----------------------------------------------------------Xml_processor::get_next_token

Xml_processor::Token* Xml_processor::get_next_token()
{
    if( _next_token._symbol == sym_eof )  throw_xc( "SOS-1402" );  // get_next_token() trotz eof gerufen?

    _next_token._pos = _char_pos;
    _next_token._text.length( 0 );

NEXT_TRY:
    if( _in_tag )
    {
        // Zwischen "<" und ">"

        switch( _next_char ) 
        {
            case '>':   eat_char();
                        _next_token._symbol = sym_gt; 
                        _in_tag = false;
                        break;

            case '/':   eat_char();
                        if( _next_char == '>' ) 
                        {
                            _next_token._symbol = sym_slash_gt;          // />
                            eat_char();
                            _in_tag = false;
                        } 
                        else goto TOKEN_ERROR;
                        break;
                    
            case '?':   eat_char();
                        if( _next_char == '>' ) 
                        {
                            _next_token._symbol = sym_question_gt;       // ?>
                            eat_char();
                            _in_tag = false;
                        } 
                        else goto TOKEN_ERROR;
                        break;

            case '=':   eat_char();
                        _next_token._symbol = sym_equal; 
                        break;

            case '"':   eat_char();
                        _next_token._symbol = sym_string;

                        while(1) 
                        {
                            if( _next_char == '\n' )  goto TOKEN_ERROR;
                            if( _next_char == EOF  )  goto TOKEN_ERROR;
                            if( _next_char == '"' ) {
                                eat_char();
                                if( _next_char != '"' )  break;
                            }
                            _next_token._text.append( (char)eat_char() );
                        }
                        break;

            case ' ':   
            case '\t':  
            case '\r':  eat_char();  goto NEXT_TRY;  

            case '\n':  eat_char();  _char_pos._line++;  _char_pos._col = 0;  goto NEXT_TRY;

            case EOF:   _next_token._symbol = sym_eof; break;

            default:    _next_token._symbol = sym_name;

                        while( _next_char != '?'  
                           &&  _next_char != '/'  
                           &&  _next_char != '>' 
                           &&  _next_char != '=' 
                           &&  _next_char != EOF )  _next_token._text.append( (char)eat_char() );
        }
    }
    else
    {
        while( sos_isspace( _next_char ) )  eat_char();     // Leerraum am Anfang wegwerfen

        if( _next_char == '<' )
        {
            eat_char();
            switch( _next_char )
            {
                case '!':   eat_char();
                            if( _next_char == '-'  )                    // <!--...--> verschlucken
                            {  
                                eat_char();
                                if( eat_char() != '-' )  goto TOKEN_ERROR;

                                while(1) {
                                    if( eat_char() == '-'  &&  eat_char() == '-'  && eat_char() == '>' )  break;
                                    if( _next_char == EOF )  goto TOKEN_ERROR;
                                }

                                goto NEXT_TRY;
                            }
                            else
                            if( _next_char == '['  )                    // <![CDATA[...]]>
                            {  
                                eat_char();
                                if( eat_char() != 'C' )  goto TOKEN_ERROR;
                                if( eat_char() != 'D' )  goto TOKEN_ERROR;
                                if( eat_char() != 'A' )  goto TOKEN_ERROR;
                                if( eat_char() != 'T' )  goto TOKEN_ERROR;
                                if( eat_char() != 'A' )  goto TOKEN_ERROR;
                                if( eat_char() != '[' )  goto TOKEN_ERROR;

                                while(1) 
                                {
                                    if( _next_char == EOF )  goto TOKEN_ERROR;
                                    
                                    if( _next_char == ']' ) 
                                    {
                                        eat_char();
                                        
                                        if( _next_char == ']' ) {
                                            eat_char();
                                            if( _next_char == '>' )  { eat_char(); break; }
                                            _next_token._text.append( ']' );
                                        } 

                                        _next_token._text.append( ']' );
                                    }

                                    if( _next_token._text.length() == _next_token._text.size() ) {
                                        int new_size = _next_token._text.length() + _next_token._text.length() / 2 + 100000;
                                        _next_token._text.resize_min( new_size );
                                    }
                                    _next_token._text.append( (char)eat_char() );
                                }

                                _next_token._symbol = sym_text;
                                goto FERTIG;  
                                break;
                            }

                            _next_token._symbol = sym_dtd;
                            _in_tag = true;
                            break;

                case '/':   _next_token._symbol = sym_end_tag;           _in_tag = true;  eat_char();  break;
                case '?':   _next_token._symbol = sym_processing_instr;  _in_tag = true;  eat_char();  break;
                default:    _next_token._symbol = sym_standard_tag;      _in_tag = true;  
            }

            if( _in_tag ) 
            {
                while( sos_isalnum( _next_char )  ||  _next_char == '_'  ||  _next_char == '-' ) {
                    _next_token._text.append( (char)eat_char() );
                }
            }
        }
        else
        {
            // Text zwischen zwei Tags
            _next_token._symbol = sym_text;

            while( _next_char != '<'  &&  _next_char != EOF ) 
            {
                switch( _next_char )
                {
                    case ' ':   
                    case '\t':  // Leerraum auf ein Blank reduzieren
                                eat_char();
                                _next_token._text.append( ' ' );
                                while( sos_isspace( _next_char ) )  eat_char(); 
                                break;

                    case '\r':
                    case '\n':  while( sos_isspace( _next_char ) )  eat_char();     // Leerraum am Zeilenanfang wegwerfen
                                break;

                    case '&':   eat_char();
                                
                                if( _next_char == '#' ) 
                                {
                                    eat_char();
                                    char c = 0;
                                    while( sos_isdigit( _next_char ) )  c = 10*c + eat_char() - '0';
                                    if( eat_char() != ';' )  throw_syntax_error( "SOS-PARSER-202", _eaten_char_pos );
                                    _next_token._text.append( (char)c );
                                }
                                else
                                {
                                    Sos_limited_text<max_entity_name_length> name;
                                    Source_pos pos = _char_pos;
                    
                                    while( sos_isalnum( _next_char ) ) 
                                    {
                                        if( name.length() == name.size() )  throw_syntax_error( "SOS-PARSER-203", c_str( name ), pos );
                                        name.append( (char)eat_char() );
                                    }
                                    if( eat_char() != ';' )  throw_syntax_error( "SOS-PARSER-202", _char_pos );

                                    Entity* entity = NULL;
                                    for( int i = _entity_array.first_index(); i <= _entity_array.last_index(); i++ ) 
                                    {
                                        if( strcmp( c_str( _entity_array[i]._name ), c_str( name ) ) == 0 ) 
                                        {
                                            entity = &_entity_array[ i ];  
                                            break; 
                                        }
                                    }

                                    if( !entity )  throw_syntax_error( "SOS-PARSER-204", c_str( name ), pos );
                                    _next_token._text.append( entity->_value );
                                }
                                break;
        
                    default:    _next_token._text.append( (char)eat_char() );
                }
            }
        }

        // Leerraum am Ende entfernen
        _next_token._text.length( length_without_trailing_spaces( _next_token._text.char_ptr(), 
                                                                  _next_token._text.length() ) );
        if( _next_char == EOF  &&  _next_token._text.length() == 0 )  _next_token._symbol = sym_eof;
    }

  FERTIG:
    _next_token._text.resize_min( _next_token._text.length() + 1 );
    _next_token._text.char_ptr()[ _next_token._text.length() ] = '\0';
    return &_next_token;
       
  TOKEN_ERROR: 
    throw_syntax_error( "SOS-1402", _next_token._pos );
    return NULL;
}

//----------------------------------------------------------------------Xml_processor::expect

void Xml_processor::expect( Symbol expected_symbol, const char* name )
{
    if( _next_token._symbol != expected_symbol ) {
        throw_syntax_error( "SOS-1403", symbol_names[expected_symbol], _next_token._pos );
    }

    if( name  &&  name[0] != '\0'  &&  _next_token._text != name ) 
    {
        Sos_string text = name;
        if( expected_symbol == sym_standard_tag     )  text = '<' + text + '>';
        else
        if( expected_symbol == sym_processing_instr )  text = "<?" + text + "?>";
        else                                                              
        if( expected_symbol == sym_dtd              )  text = "<!" + text + "!>";

        throw_syntax_error( "SOS-1403", c_str( text ), _next_token._pos );
    }
}

//------------------------------------------------------------------Xml_processor::eat_token

void Xml_processor::eat_token( Symbol expected_symbol, const char* name )
{
    if( expected_symbol != sym_none )  expect( expected_symbol, name );
    get_next_token();
}

//--------------------------------------------------------Xml_processor::eat_token_as_string

Sos_string Xml_processor::eat_token_as_string( Symbol expected_symbol, const char* name )
{
    Sos_string result = _next_token._text.char_ptr();
    eat_token( expected_symbol, name );
    return result;
}

//--------------------------------------------------------------Xml_processor::parse_end_tag

void Xml_processor::parse_end_tag( const char* name )
{
    eat_token( sym_end_tag, name );
    eat_token( sym_gt );
}

//------------------------------------------------------------Xml_processor::parse_field_tag

Xml_processor::Field_tag Xml_processor::parse_field_tag()
{
    Field_tag xml_field;

    xml_field._pos = _next_token._pos;
    xml_field._name = eat_token_as_string( sym_standard_tag );

    if( _next_token._symbol == sym_name )
    {
        eat_token( sym_name, "index" );
        eat_token( sym_equal );
        xml_field._index = as_int( eat_token_as_string( sym_string ) );
        xml_field._has_index = true;
    }
 
    if( _next_token._symbol == sym_slash_gt )
    {
        xml_field._empty = true;
        eat_token();
    }
    else eat_token( sym_gt );

    return xml_field;
}

//--------------------------------------------------------Xml_processor::parse_record_fields

void Xml_processor::parse_record_fields( Record_type* record_type, Byte* p )
{
    if( _next_token._symbol == sym_text )   // Der Inhalt der ganzen Gruppe im XML-Dokument?
    {
        if( record_type->_group_type ) {
            record_type->_group_type->read_text( p, _next_token._text.char_ptr() );
            eat_token();
        }
        else throw_xc( "SOS-1406", c_str( record_type->name() ), _next_token._pos );
    }
    else
    {
        // Im XML-Dokument sind die einzelnen Felder der Gruppe aufgeführt.

        while( _next_token._symbol == sym_standard_tag  || _next_token._symbol == sym_text ) 
        {
            Field_tag    field_tag  = parse_field_tag();
            Field_descr* field      = record_type->field_descr_by_name_or_0( c_str( field_tag._name ) );
            if( !field ) 
            {
                if( !_ignore_unknown_fields )  throw_syntax_error( "SOS-1405", c_str( field_tag._name ), field_tag._pos );                  
                if( _next_token._symbol == sym_text )  eat_token();
                parse_end_tag( field_tag._name );
            }
            else
            {
                if( field->obj_is_type( tc_Array_field_descr ) 
                 && ((Array_field_descr*)field)->_level == _array_level + 1 ) 
                {
                    if( !field_tag._has_index )  throw_syntax_error( "SOS-1409", c_str( field_tag._name ), field_tag._pos );
                
                    _array_level++;

                    int offset = ((Array_field_descr*)field)->elem_offset( field_tag._index, _array_level - 1 );

                    if( field_tag._empty ) 
                    {
                        field->type_ptr()->read_text( p + offset, "" );
                    }
                    else
                    {
                        if( field->type_ptr()->obj_is_type( tc_Record_type ) )
                        {
                            Record_type* t = (Record_type*)field->type_ptr();
                            if( !t->flat_scope() )  throw_xc( "SOS-1408", c_str( t->name() ) );
                            parse_record_fields( t, p + offset - field->offset() );
                        }
                        else
                        {
                            field->type_ptr()->read_text( p + offset, _next_token._text.char_ptr() );
                            eat_token();
                        }

                        parse_end_tag( field_tag._name );
                    }

                    _array_level--;
                }
                else
                {
                    if( field_tag._has_index )  throw_syntax_error( "SOS-1410", c_str( field_tag._name ), field_tag._pos );

                    if( field_tag._empty ) 
                    {
                        field->read_text( p, "" );
                    }
                    else
                    {
                        if( field->type_ptr()->obj_is_type( tc_Record_type ) )
                        {
                            Record_type* t = (Record_type*)field->type_ptr();
                            if( !t->flat_scope() )  throw_xc( "SOS-1408", c_str( t->name() ) );
                            parse_record_fields( t, p );
                        }
                        else
                        {
                            field->read_text( p, _next_token._text.char_ptr() );
                            eat_token();
                        }

                        parse_end_tag( field_tag._name );
                    }
                }
            }
        }
    }
}

//----------------------------------------------------Xml_processor::parse_processing_instr

void Xml_processor::parse_processing_instr()
{
    eat_token( sym_processing_instr, "xml" );

    while( _next_token._symbol == sym_name )
    {
        if( _next_token._text == "encoding" ) 
        {
            eat_token();
            eat_token( sym_equal );
            get_encoding( eat_token_as_string( sym_string ) );
        }
        else
        {
            eat_token();
            if( _next_token._symbol == sym_equal )
            {
                eat_token();
                eat_token();
            }
        }
    }

    eat_token( sym_question_gt );
}

//---------------------------------------------------------Xml_processor::parse_dtd_element

void Xml_processor::parse_dtd_element()
{
    while( _next_token._symbol != sym_gt )  eat_token();
}

//---------------------------------------------------------------Xml_processor::parse_header

void Xml_processor::parse_header()
{
    Bool weiter = true;

    while( weiter ) 
    {
        switch( _next_token._symbol )
        {
            case sym_processing_instr:  parse_processing_instr();   break;
            case sym_dtd:               parse_dtd_element();        break;
            default:                    weiter = false;
        }
    }

    _enclosing_tag = eat_token_as_string( sym_standard_tag, c_str( _enclosing_tag ) );
    eat_token( sym_gt );

    if( _next_token._symbol == sym_standard_tag
     && _next_token._text == _date_tag )        // <date>...</date>
    {
        eat_token();
        eat_token( sym_gt );
        eat_token( sym_text );
        parse_end_tag( _date_tag );
    }
}

//---------------------------------------------------------------Xml_processor::get_xml_line

void Xml_processor::get_xml_line( Area* )
{
    throw_eof_error();
}

//----------------------------------------------------------------------------xml_as_string

string xml_as_string( const string& text )
{
    Xml_processor xml_processor;
    
    xml_processor.init_read( text.c_str(), text.length() );
    xml_processor.expect( Xml_processor::sym_text );

    string result ( xml_processor.text().char_ptr(), xml_processor.text().length() );

    xml_processor.get_next_token();
    xml_processor.expect( Xml_processor::sym_eof );

    return result;
}

//------------------------------------------------------------------------------------as_xml

string as_xml( const string& text, const string& options )
{
    bool cdata = false;

    for( Sos_option_iterator opt = options; !opt.end(); opt.next() )
    {
        if( opt.flag( "cdata"         ) )  cdata = opt.set(); 
        else
            throw_sos_option_error( opt.value() );
    }



    Xml_processor xml_processor;
    
    xml_processor.init_write();
    xml_processor.allocate( 100*1024 );

    if( cdata )  xml_processor.append_cdata_section( text );   // <![CDATA[...]]>
           else  xml_processor.append_cdata( text );           // ...&amp;...

    return string( xml_processor._buffer.char_ptr(), xml_processor._buffer.length() );
}

//-------------------------------------------------------------------------------------as_xml

string as_xml( Record_type* type, const Const_area& record )
{
    Xml_processor xml_processor;

    xml_processor.init_write();
    xml_processor.allocate( 100*1024 );
    xml_processor.write_record_fields( type, record );

    return string( xml_processor._buffer.char_ptr(), xml_processor._buffer.length() );
}


} //namespace sos
