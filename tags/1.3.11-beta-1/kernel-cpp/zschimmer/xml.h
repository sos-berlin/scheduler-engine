// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_XML_H
#define __ZSCHIMMER_XML_H

#include "z_io.h"


#define DOM_FOR_EACH_ELEMENT( PARENT, ITERATOR )                                                \
                                                                                                \
    for( ::zschimmer::xml::libxml2::Simple_node_ptr ITERATOR##_node = (PARENT).firstChild();    \
         ITERATOR##_node;                                                                       \
         ITERATOR##_node = ITERATOR##_node.nextSibling() )                                      \
                                                                                                \
    if( ::zschimmer::xml::libxml2::Element_ptr ITERATOR = xml::Element_ptr( ITERATOR##_node, ::zschimmer::xml::libxml2::Element_ptr::no_xc ) )


namespace zschimmer {
namespace xml {


namespace libxml2
{
    struct Element_ptr;
}


//--------------------------------------------------------------------------------------------const

extern const char               unicode_substition_character ;
extern const char               allowed_xml_ascii_characters [ 256 ];
extern const char               allowed_xml_latin1_characters[ 256 ];

//-------------------------------------------------------------------------------------------------

inline bool                     is_valid_xml_ascii_character ( char c )                             { return allowed_xml_ascii_characters[ (unsigned char) c ] != 0; }
inline bool                     is_valid_xml_latin1_character( char c )                             { return allowed_xml_ascii_characters[ (unsigned char) c ] != 0; }

string                          encode_text                 ( const string&, bool quote_too = false );
inline string                   encode_attribute_value      ( const string& value )                 { return encode_text( value, true ); }
bool                            is_valid_xml_ascii_character( char );
bool                            is_valid_xml_latin1_character( char );
extern string                   non_xml_latin1_characters_substituted( const io::Char_sequence& );

//---------------------------------------------------------------------------------------Xml_writer

struct Xml_writer : io::Filter_writer
{
                                Xml_writer                  ( Writer* );

    void                    set_encoding                    ( const string& encoding )              { _encoding = encoding; }                                   
    void                        write_prolog                ();

    void                        begin_element               ( const string& element_name );
    void                        set_attribute               ( const string& name, const string& );
    void                        set_attribute               ( const string& name, int64 );
    void                        set_attribute_optional      ( const string& name, const string& );
    void                        end_element                 ( const string& element_name );

    void                        write                       ( const io::Char_sequence& utf8_text );
    void                        write_through               ( const io::Char_sequence& text );
    void                        flush                       ();

    void                        write_element               ( const libxml2::Element_ptr& );

  protected:
                                Xml_writer                  ();

    void                        close_tag                   ();

  private:
    bool                       _is_tag_open;
    string                     _encoding;
};

//--------------------------------------------------------------------------------Xml_string_writer

struct Xml_string_writer : Xml_writer
{
                                Xml_string_writer           ();

    void                        flush                       ();
    string                      to_string                   ();

  private:
    ptr<io::String_writer>     _string_writer;
};

//-------------------------------------------------------------------------------------------------

} //namespace xml
} //namespace zschimmer

#endif
