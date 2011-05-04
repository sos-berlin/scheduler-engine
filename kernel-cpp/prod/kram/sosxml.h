// sosxml.h, Joacim Zschimmer
// $Id$

#ifndef __SOSXML_H
#define __SOSXML_H

namespace sos {


struct Xml_processor
{
    struct Entity
    {
                                Entity                  ()  {}
                                Entity                  ( const Sos_string& name, const Sos_string& value );
                               ~Entity                  ();

        Sos_string             _name;
        Sos_string             _value;
    };

    enum Symbol
    {                           // Reihenfolge muss mit symbol_names[] übereinstimmen!
        sym_none,
        sym_eof,
        sym_plus,
        sym_equal,
        sym_text,               // Text, parsiert
        sym_standard_tag,       // <tag
        sym_end_tag,            // </tag
        sym_processing_instr,   // <?name
        sym_dtd,                // <!name
        sym_gt,                 // >  
        sym_slash_gt,           // />  
        sym_question_gt,        // ?>
        sym_string,             // "string"
        sym_name,
        sym__count
    };

    struct Token
    {
        Symbol                 _symbol;
        Dynamic_area           _text;                   // Name des Tags oder der Text
        Source_pos             _pos;
    };


    struct Field_tag            // Ein Feld-Tag  <feldname index="...">
    {
                                Field_tag               ()                  : _index(-1), _has_index(false), _empty(false) {}

        Source_pos             _pos;                    // Position in XML-Dokument
        Sos_string             _name;    
        int                    _index;                  // Attribut index="...", Index eines Array-Elements
        Bool                   _has_index;              // Attribut index="..." angegeben?
        Bool                   _empty;
    };


    
                                Xml_processor           ();
    virtual                    ~Xml_processor           ()                          {}

    void                        get_encoding            ( const Sos_string& );


    // SCHREIBEN
    void                        init_write              ();
    void                        allocate                ( int size )                { _buffer.allocate( size ); }
    void                        reset_buffer            ()                          { _buffer.length( 0 ); }

    void                        append_field_name       ( const char* name );
    void                        append_cdata            ( const Byte*, int );
    void                        append_cdata            ( const char* p, int l )    { append_cdata( (const Byte*)p, l ); }
    void                        append_cdata            ( const Sos_string& value ) { append_cdata( c_str( value ), length( value ) ); }
    void                        append_cdata            ( const Const_area& value ) { append_cdata( value.byte_ptr(), length( value ) ); }
    void                        append_cdata_section    ( const Sos_string& value );
    void                        write_data              ( const char* name, const Field_type* t, const Const_area& record )  { write_data( name, t, record.byte_ptr(), record.length() ); }
    void                        write_data              ( const char* name, const Field_type*, const Byte*, int rest );
    void                        write_field             ( const Field_descr*, const Byte*, int rest );
    void                        write_array_elem        ( const Array_field_descr*, int index, const Byte*, int rest );
    void                        write_array             ( const Array_field_descr*, const Byte*, int rest  );
    void                        write_record_fields     ( const Record_type*, const Byte* p, int rest );
    void                        write_record_fields     ( const Record_type* t, const Const_area& record )  { write_record_fields( t, record.byte_ptr(), record.length() ); }
    void                        put_start_tag           ( const char* tag );
    void                        put_end_tag             ( const char* tag );
    void                        start_line              ();
    
    virtual void                put_xml_line            ( const Const_area& line );


    // LESEN
    void                        init_read               ();
    void                        init_read               ( const char* xml_text, int len );
    void                        parse_header            ();
    void                        parse_processing_instr  ();
    void                        parse_dtd_element       ();
    Sos_string                  parse_simple_tag        ();                             // Tag ohne Attribute
    void                        parse_end_tag           ( const char* name = NULL );
    void                        parse_end_tag           ( const Sos_string& name )      { parse_end_tag( c_str( name ) ); }
    Sos_string                  parse_indexed_tag       ();                             // Tag mit Attribut index=".."
    void                        parse_record_fields     ( Record_type*, Byte* );
    Field_tag                   parse_field_tag         ();
    void                        eat_token               ( Symbol = sym_none, const char* text = NULL );
    Sos_string                  eat_token_as_string     ( Symbol = sym_none, const char* text = NULL );
    void                        expect                  ( Symbol expected_symbol, const char* text = NULL );
    Const_area                  text                    ()                              { return _next_token._text; }
    Token*                      get_next_token          ();
    int                         eat_char                ();         // Liefert _next_char und frisst es

    virtual void                get_xml_line            ( Area* line );


    Bool                       _iso_8859_1;             // -encoding=iso-8859-1
    Bool                       _windows_1252;           // -encoding=windows-1252
    Sos_string                 _enclosing_tag;
    Sos_string                 _record_tag;
    Sos_string                 _date_tag;

    // SCHREIBEN
    Fill_zero                  _zero_;
    Dynamic_area               _buffer;
    Dynamic_area               _field_buffer;
    Text_format                _text_format;
    Bool                       _suppress_null;          // Nichtige Werte nicht in das XML-Dokument schreiben
    Bool                       _suppress_empty;         // Leere Werte nicht in das XML-Dokument schreiben
    Bool                       _ucase;                  // Tags in großen Buchstaben
    Bool                       _lcase;                  // Tags in kleinen Buchstaben
    Sos_string                 _indent_string;          // Text einrücken
    int                        _nesting;
    int                        _array_level;            // Stufe für OCCURS-Klause (0,1,2,3)


    // LESEN
    Sos_simple_array<Entity>   _entity_array;

    // XML lesen:
    Dynamic_area               _line;                   // Eingabezeile (XML)
    char*                      _ptr;                    // Zeiger in _line
    Source_pos                 _char_pos;               // Position von _next_char für Fehlermeldungen
    Source_pos                 _eaten_char_pos;
    int                        _next_char;              // Nächstes von eat_char() gelesenes Zeichen
    Token                      _next_token;
    Bool                       _in_tag;                 // Zwischen < und >
    Bool                       _ignore_unknown_fields;
};



Sos_string                      xml_as_string           ( const Sos_string& text );    
Sos_string                      as_xml                  ( const string& text, const string& options = empty_string );   // Liefert Text als #PCDATA oder als <![CDATA[...]]>
Sos_string                      as_xml                  ( Record_type*, const Const_area& record );


} //namespace sos

#endif
