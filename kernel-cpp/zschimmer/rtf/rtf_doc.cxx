// rtf_doc.cxx                                  © 2000 Joacim Zschimmer
// $Id: rtf_doc.cxx 13691 2008-09-30 20:42:20Z jz $

//#include "headers.h"

#include "../zschimmer.h"
#include "../log.h"
#include "../file.h"
#include <assert.h>
#include "rtf.h"
#include "rtf_doc.h"
#include "rtf_parser.h"


using namespace zschimmer::file;


namespace zschimmer {
namespace rtf {

extern const char rtf_doc_copyright[] = "\n$Id: rtf_doc.cxx 13691 2008-09-30 20:42:20Z jz $ (C) 2004  Zschimmer GmbH\n";


//--------------------------------------------------------------------------------------error_codes

static Message_code_text error_codes[] =
{
    { "Z-RTF-DOC-001", "Dokument fehlt \\stylesheet" },
    { "Z-RTF-DOC-002", "Dokument fehlt \\colortbl" },
    { "Z-RTF-DOC-003", "Dokumenteigenschaft \\$1 steht nach Nicht-Dokumenteigenschaft \\$2 im Dokument" },
    { "Z-RTF-DOC-004", "Dokumenteigenschaft \\$1 steht nach unbekanntem RTF-Code \\$2 im Dokument" },
    {}
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_rtf_doc )
{
    add_message_code_texts( error_codes ); 
}

//---------------------------------------------------------------------------Doc::Header::style
/*
Style* Doc::Header::style( int nummer )
{
 Auf Existenz prüfen!
    return _style_map[ nummer ];
}
*/
//-------------------------------------------------------------------Doc::Header::entry_by_name

Doc::Header_entry* Doc::Header::entry_by_name( Code code, const string& name )
{
    Header_map& m = header_map( code );

    for( Header_map::const_iterator it = m.begin(); it != m.end(); it++ )
    {
        if( stricmp( c_str( it->second->_name ), c_str( name ) ) == 0 )   return it->second;
    }

    return NULL;
}

//-----------------------------------------------------------------Doc::Header::get_free_number

int Doc::Header::get_free_number( Code code, int wish )
{
    Header_map& m = header_map(code);

    if( m.find( wish ) == m.end() )  return wish;


    int highest_number = 0;

    Z_FOR_EACH( Header_map, m, it )
    {
        if( highest_number < it->first )   highest_number = it->first;
    }

    return highest_number + 1;
}

//--------------------------------------------------------------------------Doc::Header::remove

void Doc::Header::remove( Code code, Header_entry* entry )
{
    Header_map& m = header_map(code);

    Z_FOR_EACH( Header_map, m, it )
    {
        if( it->second == entry )  { m.erase( it ); return; }
    }
}

//---------------------------------------------------------------------------------------Doc::color

Doc::Color* Doc::color( int red, int green, int blue )
{
    Color my_color ( has_rtf_context(), red, green, blue );

    Z_FOR_EACH( Header_map, _header.header_map(code_colortbl), m )  if( m->second && *(Color*)+m->second == my_color )  return (Color*)+m->second;

    // Neue Farbe anlegen:
    ptr<Color> color = Z_NEW( Color( my_color ) );
    color->_number  = 1;

    Destination_entity* colortbl = cast_destination_entity( _doc_property_entities[ code_colortbl ] );
    if( !colortbl )  throw_xc( "Z-RTF-DOC-002" );

    Simple_entity* c = colortbl->head();
    if( c )  
    {
        while( c->_next )
        {
            if( c->code() == code_text )  color->_number++;       // ";"
            c = c->_next;
        }
    }

    c->insert_successor( new Param_entity( code_red  , red   ) );  c = c->_next;
    color->_entity = c;
    c->insert_successor( new Param_entity( code_green, green ) );  c = c->_next;
    c->insert_successor( new Param_entity( code_blue , blue  ) );  c = c->_next;
    c->insert_successor( new Text_entity( has_rtf_context(), ";" ) );

    _header.header_map(code_colortbl)[color->_number] = color;

    return color;
}

//---------------------------------------------------------------------------Doc::Header::style
/*
const Doc::Style* Doc::Header::style( const string& name ) const
{
    for( Doc::Style_map::const_iterator i = _style_map.begin(); i != _style_map.end(); i++ )
    {
        if( i->second._name == name )   return &i->second;
    }

    return NULL;
}

//---------------------------------------------------------------------------Doc::Header::font

const Doc::Font* Doc::Header::font( const string& name ) const
{
    for( Doc::Font_map::const_iterator i = _font_map.begin(); i != _font_map.end(); i++ )
    {
        if( i->second._name == name )   return &i->second;
    }

    return NULL;
}
*/

//----------------------------------------------------------------------Doc::Style::insert_into_doc

Simple_entity* Doc::Style::insert_into_doc( Simple_entity* after_this )
{
    Simple_entity* a = after_this;
    Simple_entity* e = _entity;

    if( !e )  return a;

    if( e->code() == code_group ) 
    {
        e = e->head();
        if( !e )  return a;
    }

    a->insert_successor( new_entity( _code, _number ) );
    a = a->next();

    while( e  &&  e->code() != code_text )
    {
        if( !e->is_header() ) {
            a->insert_successor( new_entity( *e ) );
            a = a->_next;
        }
        e = e->_next;
    }

    return a;   // Das zuletzt eingefügte Element
}

//-------------------------------------------------------------------------Doc::Style::set_property

void Doc::Style::set_property( Code code, int param )
{
    if( !_entity )  throw_xc( "Style::set_property" );   // Doc::add_style() nicht gerufen.

    Simple_entity* e = _entity->head();
    if( !e )  throw_xc( "Style::set_property" );   // Doc::add_style() nicht gerufen.

    while( e->_next  &&  e->_next->code() != code_text  &&  e->_next->code() != code )  e = e->_next;

    if( e->_next  &&  e->_next->code() == code )
    {
        cast_param_entity(e->_next)->_param = param;
    }
    else
    {
        e->insert_successor( new_entity( code, param ) );
    }
}

//-----------------------------------------------------------------------------------Doc::read_file

void Doc::read_file( const string& filename )
{

    if( filename == "-" )
    {
        read_file( stdin );
    }
    else
    {
        read_file( Stream_file( filename.c_str(), "r" ) );
    }
}

//-----------------------------------------------------------------------------------Doc::read_file

void Doc::read_file( FILE* file )
{
    Parser parser ( this );

  //parser._dont_ignore_pgdsctbl = _dont_ignore_pgdsctbl;
    parser._input = file;
    parser.parse();
}

//----------------------------------------------------------------------------------------Doc::read

void Doc::read( const char* rtf_text )
{
    Parser parser ( this );

  //parser._dont_ignore_pgdsctbl = _dont_ignore_pgdsctbl;
    parser._input_ptr = rtf_text;
    
    parser.parse();
}

//-----------------------------------------------------------------------------------------Doc::Doc

Doc::Doc( Has_rtf_context* c, const string& debug_name )
: 
    Has_rtf_context(c),
    _zero_(this+1),
    _debug_name(debug_name),
    _entity(c),
    _properties(c)
{
    _entity.set_code( code_group );
    //_last_entity_ptr = &_entity._head;
    _current_destination = &_entity;
    _optimize_properties = true;   // Factory: Sonst werden die Dokumenteigenschaften (\ansi etc.) wiederholt ausgeben. WARUM?
}

//------------------------------------------------------------------------------Doc::~Doc

Doc::~Doc()
{
    Z_LOG2( "rtf", "~Doc " << _debug_name << ": entities_high=" << Simple_entity::static_entity_count_high << ",  bytes=" << Simple_entity::static_allocated_bytes_high << "\n" );
}

//----------------------------------------------------------------------Doc::reset_borders
/*
void Doc::reset_borders()
{
    for( const Code* brdr = border_properties; *brdr != code_none; brdr++ )
    {
        _properties.set_property( *brdr, 0 );
    }
}
*/
//----------------------------------------------------------------------Doc::append_entity

Simple_entity* Doc::append_entity( const Simple_entity& entity )
{
    // Einige Codes werden umgesetzt, z.B. setzt \ulnone die Unterstreichung (\ul, \uld etc.) aus.

    switch( entity.code() )
    {
        case code_intbl:        _need_intbl = true;  break;
        case code_row:          _need_intbl = false;  break;

        case code_sbknone:   
        case code_sbkcol:    
        case code_sbkpage:   
        case code_sbkeven:   
        case code_sbkodd:       if( _section_break_set )  return NULL;      // Für Serienaufbereiter: \skbnone in Vorlage 
                                break;                                      // nach zwischen Dokumenten eingefügten 
                                                                            // Abschnittswechsel unterdrücken
        case code_sect:         _section_break_set = false;  break;

        case code_eof:          throw_xc( "RTFDOC-104", "EOF im RTF-Dokument" );

        default:;
    }


    if( _rtf_context->_change_doc_to_section_properties  &&  known_descr( entity.code() )->is_doc_property() )
    {
        // Einige Dokumenteigenschaften gibt es auch als Abschnittseigenschaften. Die konvertieren wir jetzt.

        Code c = _rtf_context->section_from_doc_property( entity.code() );
        if( c  &&  !_properties.is_set( c ) )
        {
            _properties.set_property( entity.code(), entity.param() );
            return append_entity( Param_entity( c, entity.param() ) );     // Noch mal von vorne mit Abschnittseigenschaft!
        }
    }


    // Eigenschaft merken, und wenn sie schon gesetzt, dann verwerfen
    if( is_property( entity.code() )  &&  !is_destination( entity.code() ) )
    {
        if( _optimize_properties  
        &&  !_in_doc_prop_destination
        &&  has_param( entity.code() )   // Damit werden auch \plain, \pard und \sectd nicht beachtet. Das ist gut.
        &&  descr( entity.code() )->_flags & ( flag_prop_char | flag_prop_para | flag_prop_sect ) )
        {
            if( _properties[ entity.code() ] == entity.param() )  
            {
                return NULL;
            }
        }
    }


    // Wenn noch kein return, dann Entity anhängen

    Simple_entity* e = new_entity( entity );
    _current_destination->append_to_destination( e );


    if( e->is_property() )
    {
        _properties.set_property( e );
        if( descr(e)->is_doc_property() )  handle_doc_property( e );       //  \stylesheet, \fonttbl etc.
    }

    //5.2001  Folgende Zeile muss für rtf2xml auskommentiert werden:
    if( entity.code() == code_pard  &&  _need_intbl )  
        append_entity( code_intbl, 1 );


    if( _rtf_context->_change_doc_to_section_properties  &&  entity.code() == code_sectd )
    {
        // Nach \sectd die unterdrückten Dokumenteigenschaften als Abschnittseigenschaften wiederholen.
        // Kann sein, dass eine Eigenschaft später mit einem anderen Wert neu gesetzt wird, dann ist sie hier überflüssig und sollte besser gelöscht werden. Aber wie?

        Z_FOR_EACH( Rtf_context::Doc_to_section_property_map, _rtf_context->_doc_to_section_property_map, it )
        {
            Code doc_property     = it->first;
            Code section_property = it->second;

            if( descr( doc_property )->has_param() )        // Jedenfalls nicht bei is_flag (z.B. \landscape) die Eigenschaft setzen,
            {                                               // denn diese Eigenschaften haben keinen Parameter, der 0 sein könnte.
                append_entity( Param_entity( section_property, _properties[ doc_property ] ) );
            }
        }
    }


    return e;
}

//----------------------------------------------------------------------Doc::append_entity

void Doc::append_entity( Code code, int param )
{
    append_entity( Param_entity( code, param ) );
}

//---------------------------------------------------------------------Doc::set_properties

void Doc::set_property( Code prop, int param )
{ 
    if( !_in_doc_prop_destination  &&  _properties[prop] == param )   return;

    append_entity( prop, param ); 
}

//-----------------------------------------------------------Doc::push_current_destination

void Doc::push_current_destination( bool has_own_properties )
{ 
    _current_destination_stack.push( Destination_entry( _current_destination, _need_intbl, has_own_properties ) ); 

    if( has_own_properties )
    {
        _need_intbl = false;
    }
}

//------------------------------------------------------------Doc::pop_current_destination

void Doc::pop_current_destination()
{ 
    const Destination_entry& stack_entry = _current_destination_stack.top();
    
    _current_destination = stack_entry._destination;

    if( stack_entry._restore_properties )
    {
      //_properties = stack_entry._properties;
        _need_intbl = stack_entry._need_intbl;
    }

    //_current_destination_stack.top(); 
    _current_destination_stack.pop(); 
}

//-----------------------------------------------------------------Doc::append_destination

Destination_entity* Doc::append_destination( Code code, int param )
{
    Destination_entity* e = new Destination_entity( has_rtf_context(), code, param );

    _current_destination->append_to_destination( e );

    push_current_destination( e->has_own_properties() );
    _current_destination = e;

    if( descr(e)->is_doc_property() )  _in_doc_prop_destination++;     // z.B. {\list, {\fonttbl etc.
    
    return e; // für end_destination()
}

//--------------------------------------------------------------------Doc::end_destination

void Doc::end_destination( Destination_entity* e )          
{ 
    if( is_doc_property( e->code() ) )  _in_doc_prop_destination--;     // z.B. {\list, {\fonttbl etc.

    pop_current_destination();

    handle_doc_property( e );
}

//---------------------------------------------------------------------Doc::set_properties

void Doc::set_properties( const Properties& props )
{
    for( Code c = code_none; c < code__properties_max; c++ ) 
    {
        set_property( c, props[c] );
    }
}

//-------------------------------------------------------------------Doc::handle_doc_property

void Doc::handle_doc_property( Simple_entity* e )
{
    if( is_doc_property( e->code() ) ) 
    {
        // \pnseclvl kommt mehrfach vor: if( _doc_property_entities[ e->code() ] )  throw_xc( "Dokumenteigenschaft doppelt: ", e->name() );

        _doc_property_entities[ e->code() ] = e;
        
        switch( e->code() )
        {
            case code_stylesheet:  
            case code_fonttbl:     
            case code_colortbl:     
            case code_listtable:     
            case code_listoverridetable:   build_table( e->code() );  break;
            default:;
        }
    }
}

//-----------------------------------------------------------------------------------Doc::print

void Doc::print( File_base* file ) const
{
/*
    *s << "{\\stylesheet\n";
    for( Style_map::const_iterator i = _header._style_map.begin(); i != _header._style_map.end(); i++ )
    {
        Style* style = i->second;
        *s << print_indent_text << "{\\s" << style->_number << ' ' << style->_name << "}\n";
    }
    *s << "}\n\n";
*/

    print_begin( file );
    print_entity_list( file, _entity.head() );
    print_end( file );
}

//-----------------------------------------------------------------------Doc::print_entity_list

void Doc::print_entity_list( File_base* file, const Simple_entity* entity_list, const Simple_entity* end, Flags allowed_properties, bool need_blank, int indent ) const
{
    bool doc_property_encountered = false;
    bool body_start_encountered   = false;


    for( const Simple_entity* e = entity_list;  e != end;  e = e->_next )
    {
        const Descr* d = descr( e->code() );

        if( !d->prop_flags()  ||  ( d->prop_flags() & allowed_properties ) )
        {
            if( !body_start_encountered )
            {
                if( d->is_doc_property() )  doc_property_encountered = true;
                else
                if( doc_property_encountered  &&  e->code() <= code__properties_max )
                {
                    body_start_encountered = true;
                    file->print( "\r\n\r\n" );   // Hinter dem Dokumentkopf eine Leerzeile
                    need_blank = false;
                }
            }

            if( _indent ) { for( int i = 0; i < indent; i++ )  file->write( "          ", _indent ); need_blank = false; }
            else
            if( !_single_lines  &&  d->_flags & flag_print_newline  &&  ( e->code() != code_par || e->_next ) )  file->print( "\r\n" ),  need_blank = false;


            if( d->_kind == kind_destination ) 
            {
                file->print( '{' );

                if( e->code() != code_group )   print_entity( file, *e );
     
                if( _indent )  file->print( "\r\n" ), need_blank = false;

                print_entity_list( file, e->head(), NULL, allowed_properties, true, indent + 1 );
                
                if( _indent ) { for( int i = 0; i < indent; i++ )  file->write( "          ", _indent ); need_blank = false; }
                file->print( '}' );
                
                if( _single_lines || _indent )  file->print( "\r\n" ),  need_blank = false;
                else
                if( d->_flags & flag_print_newline  &&  e->_next &&  !( descr( e->_next->code() )->_flags & flag_print_newline ) )  file->print( "\r\n" ),  need_blank = false;

                need_blank = false;
            }
            else
            {
                if( e->code() == code_text )  
                { 
                    const Text_entity* te = cast_text_entity( e );

                    if( need_blank )
                    {
                        Byte first_char = te->_text[0];
                        if( isalnum( first_char )  ||  first_char == '-'  ||  first_char == ' ' )  file->print( ' ' ); 
                        need_blank = false; 
                    }
                }
                else  
                {
                    const char* name = descr( e->code() )->_name.c_str();
                    if( !isalnum((Byte)name[0])  &&  name[1] == '\0' )  ;                         // Auf \-, \~ usw. folgt kein Blank
                                                                else  need_blank = true;
                }

                print_entity( file, *e );

                if( _single_lines || _indent )  
                {
                    if( _indent || e->code() != code_text )  file->print( "\r\n" ),  need_blank = false;   // Nach \'01 KEIN Zeilenwechsel, sonst zeigt Word 2000 den Aufzählungspunkt nicht an!
                                                                                                  // {\leveltext\leveltemplateid67567617 \'01\u-3913 ?;}
                                                                                                  //                                     ~~~~~~~~~~~>auf einer Zeile!
                }
            }
        }
    }
}

//---------------------------------------------------------------------Doc::find_last_header_entity

Simple_entity* Doc::find_last_header_entity( Entity_listelem* start ) const
{
    Entity_listelem* before_body = start;

    while( before_body->_next  
       &&  ( descr( before_body->_next->code() )->is_learned_doc_property()
             || is_doc_property( before_body->_next->code() ) ) )
    {
        before_body = before_body->_next;
    }

    //if(before_body->_next )  if( Log_ptr log = "zschimmer" )  log << __PRETTY_FUNCTION__ << " ", print_entity( log, *before_body->_next ), log << " is_doc=" << is_doc_property(before_body->_next->code()) << "\n";

    return before_body;
}

//------------------------------------------------------------------------------------Doc::truncate

void Doc::truncate( Entity_list* previous_element )
{
//#   ifdef _DEBUG
        if( Entity_listelem* first = previous_element? previous_element->_next : _current_destination->head() )
        {
            for( Entity_listelem* e = first; e->_next; e = e->_next )
            {
                if( Header::table_index( e->code() ) >= 0 )
                    throw_xc( "Z-RTF-DOC-003", descr(e)->_name, descr(first)->_name );

                if( e->is_doc_property()  &&  _doc_property_entities[ e->code() ] )
                {
                    //throw_xc( e->code() << code__predefined_count? "Z-RTF-DOC-003" : "Z-RTF-DOC-004", descr(e)->_name, descr(first)->_name );
                    Z_LOG2( "rtf", "Doc::truncate(\"\\" << descr(first)->_name << "\"): _doc_property_entities[\\" << descr(e)->_name << "] = NULL\n" );
                    _doc_property_entities[ e->code() ] = NULL;
                }
            }
        }
//#   endif

    _current_destination->truncate( previous_element );
    //while( previous_element->_next )  previous_element->remove_successor();
    //_last_entity_ptr = &previous_element->_next;

    rescan_properties();
}

//---------------------------------------------------------------------------Doc::rescan_properties

void Doc::rescan_properties()
{
    Entity_ptr_with_prop e ( has_rtf_context() );
    e = _entity.head();
    e.forward_to( NULL );

    _properties = e._properties;
}

//-----------------------------------------------------------Doc::delete_all_but_doc_properties
/*
void Doc::delete_all_but_doc_properties()
{
    for( Entity_listelem* e = _entity.head(); e->_next; e = e->_next )
    {
        if( !is_doc_property( e->_next->code() ) )  e->remove_successor();
    }
}
*/
//-----------------------------------------------------------------------------Doc::build_table

void Doc::build_table( Code table_code )
{
    switch( table_code )
    {
        case code_fonttbl           : build_fonttbl();            break;
        case code_stylesheet        : build_stylesheet();         break;
        case code_colortbl          : build_colortbl();           break;
        case code_listtable         : build_listtable();          break;
        case code_listoverridetable : build_listoverridetable();  break;
        default: ;
    }
}

//------------------------------------------------------------------------Doc::build_fonttbl

void Doc::build_fonttbl()
{
    Destination_entity* fonttbl_entity = cast_destination_entity( _doc_property_entities[ code_fonttbl ] );
    if( !fonttbl_entity )  return;

    Entity_ptr font_entity ( has_rtf_context(), fonttbl_entity->head() ); 
    while( font_entity )
    {
        bool       group;
        Entity_ptr e ( has_rtf_context() );
        
        if( font_entity->code() == code_group)  { group = true;  e = font_entity->head(); } 
                                          else  { group = false; e = font_entity; }
        if( e  &&  e->code() == code_f_fonttbl )
        {
            ptr<Font> font = Z_NEW( Font( has_rtf_context() ) );
            font->_number = e->param();
            font->_entity = font_entity;

            while(1) {
                int c = e.peek_char();
                if( c == ';' )  break;
                if( c == EOF )  break;
                font->_name += (char)c;
                e.skip_char();
            }

            _header.header_map(code_fonttbl)[ font->_number ] = font;
        }

        if( group ) {
            font_entity.to_next_entity();
        } else {
            while( e  &&  e->code() != code_f_fonttbl  &&  e->code() != code_group )  e = e->_next;
            if( !e )  break;
            font_entity = e->_next;
        }
    }
}

//-------------------------------------------------------------------Doc::build_stylesheet

void Doc::build_stylesheet()
{
    Destination_entity* stylesheet_entity = cast_destination_entity( _doc_property_entities[ code_stylesheet ] );
    if( !stylesheet_entity )  return;

    for( Simple_entity* style_entity = stylesheet_entity->head();  style_entity;  style_entity = style_entity->_next )
    {
        if( style_entity->code() == code_group ) 
        {
            Simple_entity* e = style_entity->head();

            if( e ) 
            {
                Param_entity styledef = code_s_stylesheet;

                if( e->code() == code_s_stylesheet  ||  e->code() == code_cs_stylesheet )   // Absatz- oder Zeichenformatvorlage 
                {
                    styledef = *cast_param_entity(e);
                    e = e->_next;
                }

                while( e  &&  e->code() != code_text )  e = e->_next;

                if( e ) 
                {
                    ptr<Style> style = Z_NEW( Style( has_rtf_context() ) );
                    style->_number = styledef.param();
                    style->_entity = style_entity;
                    style->_code   = styledef.code() == code_s_stylesheet? code_s
                                                                         : code_cs;
                    while( e  &&  e->code() == code_text )
                    {
                        style->_name += cast_text_entity(e)->_text;
                        e = e->_next;
                    }

                    size_t len = style->_name.length();
                    if( len > 0  &&  style->_name[len-1] == ';' )  style->_name.erase( len-1 );   // ohne ';'
                    _header.header_map(code_stylesheet)[ styledef.param() ] = style;
                }
            }
        }
    }
}

//------------------------------------------------------------------------Doc::build_colortbl

void Doc::build_colortbl()
{
    Destination_entity* table_entity = cast_destination_entity( _doc_property_entities[ code_colortbl ] );
    if( !table_entity )  return;

    ptr<Color> color = Z_NEW( Color( has_rtf_context() ) );
    int        nr = 0;

    for( Entity_ptr e ( has_rtf_context(), table_entity->head() ); e; e = e->_next )
    {
        if( e->code() == code_red   )  color->_default = false, color->_red   = e->param();
        else
        if( e->code() == code_green )  color->_default = false, color->_green = e->param();
        else
        if( e->code() == code_blue  )  color->_default = false, color->_blue  = e->param();
        else
        {
            color->_number = nr;
            _header.header_map(code_colortbl)[ nr ] = color;
            color = Z_NEW( Color( has_rtf_context() ) );
            nr++;
        }
    }
}

//-------------------------------------------------------------------Doc::build_listtable

void Doc::build_listtable()
{
    Destination_entity* table_entity = cast_destination_entity( _doc_property_entities[ code_listtable ] );
    if( !table_entity )  return;

    Entity_ptr group_entity ( has_rtf_context(), table_entity->head() ); 
    while( group_entity )
    {
        if( group_entity->code() == code_list )
        {
            Entity_ptr e     ( has_rtf_context(), group_entity->head() );
            ptr<List>  entry = Z_NEW( List( has_rtf_context() ) );
            
            entry->_entity = group_entity;

            while( e ) {
                if( e->code() == code_listid         )  entry->_number     = e->param();

                if( e->code() == code_listtemplateid )  entry->_listtemplateid = e->param();

                e = e->_next;
            }

            _header.header_map(code_listtable)[ entry->_number ] = entry;
        }

        group_entity.to_next_entity();
    }
}

//-----------------------------------------------------------Doc::build_listoverridetable

void Doc::build_listoverridetable()
{
    Destination_entity* table_entity = cast_destination_entity( _doc_property_entities[ code_listoverridetable ] );
    if( !table_entity )  return;


    Entity_ptr group_entity ( has_rtf_context(), table_entity->head() ); 

    while( group_entity )
    {
        if( group_entity->code() == code_listoverride )
        {
            Entity_ptr e ( has_rtf_context(), group_entity->head() );
            while( e  &&  e->code() != code_ls )  e = e->_next;
            if( e )
            {
                ptr<Listoverride> entry = Z_NEW( Listoverride( has_rtf_context() ) );
                entry->_number = e->param();
                entry->_entity = group_entity;
                _header.header_map(code_listoverridetable)[ entry->_number ] = entry;
            }
        }

        group_entity.to_next_entity();
    }
}

//------------------------------------------------------------------------Doc::prepend_info_doccomm

void Doc::prepend_info_doccomm( const string& text )
{
    Destination_entity* info = cast_destination_entity( _doc_property_entities[ code_info ] );

    if( !info )
    {
        // Möglicherweise von truncate() aus _doc_property_entities gelöscht, also suchen wir es im Dokument:
        for( Simple_entity* e = _entity.head(); e; e = e->_next )
            if( e->code() == code_info )  { info = cast_destination_entity( e );  break; }

        if( !info )
        {
            Simple_entity* last_header = find_last_header_entity();
            if( !last_header )  return;

            info = new Destination_entity( this, code_info );
            last_header->insert_successor( info );
        }

        _doc_property_entities[ code_info ] = info;
    }

    Simple_entity* e = info->head();
    while( e  &&  e->code() != code_doccomm )  e = e->next();

    Destination_entity* doccomm;

    if( e )
    {
        doccomm = cast_destination_entity( e );
    }
    else
    {
        e = info->head();
        Simple_entity* e_before = NULL;

        while( e  &&  ( e->code() == code_title    ||
                        e->code() == code_subject  ||
                        e->code() == code_author   ||
                        e->code() == code_manager  ||
                        e->code() == code_company  ||
                        e->code() == code_operator ||
                        e->code() == code_category ||
                        e->code() == code_keywords ||
                        e->code() == code_comment     ) )
        {
            e_before = e;
            e = e->next();
        }

        doccomm = new Destination_entity( this, code_doccomm );
        info->insert_after( e, doccomm );
    }

    doccomm->insert_after( NULL, new Text_entity( this, text ) );
}

//---------------------------------------------------------------------------Doc::optimize_body

void Doc::optimize_body( Entity_list* entity_list )
{
    // Entfernt direkt aufeinander folgende Eigenschaften, die sich gegenseitig aufheben.
    // Microsoft Word erzeugt solche Sequenzen, wenn Text über die Zwischenablage eingefügt wird:
    // {\bfett}{\b immer noch fett}
    

    for( Simple_entity* e = entity_list; e; e = e->_next )
    {
        if( is_destination( e->code() ) ) 
        {
            optimize_body( e->head() );
        }
        else
      //if( e->is_property() ) 
        if( descr( e->code() )->_flags & ( flag_prop_char | flag_prop_para | flag_prop_sect ) )
        {
            while( e->_next  &&  e->code() == e->_next->code() )   // Aufeinanderfolgende gleiche RTF-Codes?
            {   
                if( has_param( e->code() ) )  cast_param_entity(e)->set_param( e->_next->param() );
                e->remove_successor();
            }
        }
        else
        if( e->code() == code_text )
        {
            while( e->_next  &&  e->_next->code() == code_text )
            {
                Text_entity* te = cast_text_entity( e );

                string text = te->text();
                text += cast_text_entity( te->_next ) -> text();
                te->set_text( text );
                te->remove_successor();
            }
        }
    }
}

//-----------------------------------------------------------------Doc::modify_for_microsoft_word

void Doc::modify_for_microsoft_word( Entity_list* entity_list )
{
    Simple_entity* e = entity_list;

    if( !e )  return;

    while( e->_next )
    {
        if( e->_next->code() == code_listtext )
        {
            // {\listtext ...}...\fM...\fN  -->  \fM {\listtext ...}...        23.6.2002 (AOK Bremen, EinMik.rtf)
            // Der Font eines Textes einer Aufzählung (\listtext) muss für Word 2000 vor \listtext gesetzt werden.
            // \fN nach {\listtext} wird von Word 2000 ignoriert.
            // Also nehmen wir das \f vor dem Text und schieben es vor {\listtext}.
            // Bei mehreren \f werden die ersten (redundanten) gelöscht.

            Simple_entity* listtext_entity = e->_next;
            Simple_entity* f               = listtext_entity;
            Simple_entity* font_entity_prev = NULL;

            while( f->_next  &&  !f->_next->is_text() )      // Bis zum Beginn des Absatztextes
            {
                if( f->_next->code() == code_f )  
                {
                    if( font_entity_prev )  font_entity_prev->remove_successor();   // Wir hatten schon ein \f? Löschen!
                    font_entity_prev = f;                                           // \f merken
                }

                f = f->_next;
            }

            if( font_entity_prev )
            {
                Simple_entity* font_entity = font_entity_prev->_next;
                font_entity_prev->_next = font_entity->_next;               // \f aushängen

                font_entity->_next = e->_next;                              // \f vor {\listtext} einhängen
                e->_next = font_entity;
            }
        }
/*
        if( e->_next->code() == code_listtext && e->_next->_next && e->_next->_next->code() == code_f )
        {
            // {\listtext ...}\fN  -->  \fN {\listtext ...}
            // Sonst wird in Word 2000 der Font in einer Aufzählung nicht gesetzt.

            Simple_entity* listtext_entity = e->_next;
            Simple_entity* font_entity     = e->_next->_next;

            listtext_entity->_next = font_entity->_next;
            font_entity->_next = listtext_entity;
            e->_next = font_entity;
        }
*/
        e = e->_next;
    }
}

//-----------------------------------------------------------------------------------Doc::add_style

void Doc::add_style( Style* style )
{
    style->_number = _header.get_free_number( code_stylesheet );
    _header.header_map(code_stylesheet)[ style->_number ] = style;

    Destination_entity* stylesheet = cast_destination_entity( _doc_property_entities[ code_stylesheet ] );
    if( !stylesheet )  throw_xc( "Z-RTF-DOC-001" );

    Simple_entity* e = stylesheet->head();
    if( !e )  throw_xc( "Z-RTF-DOC-001" );

    while( e->_next )  e = e->_next;

    Destination_entity* style_entity = new Destination_entity( has_rtf_context(), code_group );

    cast_destination_entity(e)->insert_successor( style_entity );

    style_entity->append_to_destination( new Param_entity( style->_code == code_s ? code_s_stylesheet  :
                                                           style->_code == code_cs? code_cs_stylesheet 
                                                                                  : style->_code,
                                                           style->_number ) );

    style_entity->append_to_destination( new Text_entity( has_rtf_context(), style->_name ) );

    style->_entity = style_entity;
}

//------------------------------------------------------------------------------Doc::header_ordinal

int Doc::header_ordinal( Code code )
{
    switch( code )
    {
      //case code_ansi:                 return 1;
      //case code_mac:                  return 1;
      //case code_pc:                   return 1;
      //case code_pca:                  return 1;
      //case code_deff:                 return 2;
        case code_fonttbl:              return 3;
        case code_filetbl:              return 4;
        case code_colortbl:             return 5;
        case code_stylesheet:           return 6;
        case code_listtable:            return 7;
        case code_listoverridetable:    return 8;
        case code_revtbl:               return 9;
        case code_info:                 return 1000;    // \info wird hier als Dokumenteigenschaft angesehen, Microsoft tut das aber nicht.
        default:                        if( is_doc_property( code ) )  return 0;
                                                                 else  return 99;
    }
}

//-------------------------------------------------------------------------Doc::add_to_header_table

void Doc::add_to_header_table( Code code, Simple_entity* add, const string& /*debug_text*/ )
{
    // The header has the following syntax:
    //    <header>\rtf <charset> \deff? <fonttbl> <filetbl>? <colortbl>? <stylesheet>? <listtables>? <revtbl>?

    Destination_entity* doc_prop = cast_destination_entity( _doc_property_entities[ code ] );
    if( !doc_prop ) 
    {
        // Die Dokumenteigenschaft gibt es noch nicht, also an passender Stelle einfügen.

        int            o      = header_ordinal( code );
        Simple_entity* e      = _entity.head();
        Simple_entity* prev   = NULL;

        while( e )
        {
            if( header_ordinal( e->code() ) >= o )  break;
            prev = e;
            e = e->_next;
        }

        if( !prev )  throw_xc( "Doc::add_to_header_table" );

        Simple_entity* new_prop = new_entity( code );
        prev->insert_successor( new_prop );
        handle_doc_property( new_prop );

        doc_prop = cast_destination_entity(new_prop);
    }

    _doc_properties_changed = true;

    add->_next = NULL;  // Vorsichtshalber
    doc_prop->append_to_destination( add );
}

//-------------------------------------------------------------------------------------------------

} //namespace rtf
} //namespace zschimmer
