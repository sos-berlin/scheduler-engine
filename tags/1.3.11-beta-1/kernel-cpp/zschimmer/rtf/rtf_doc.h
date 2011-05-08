// rtf_doc.h                                            ©2000 Joacim Zschimmer
// $Id$

#ifndef __RTF_DOC_H
#define __RTF_DOC_H

#include "../zschimmer.h"
#include "../file.h"

#include <iostream>

#include "rtf.h"

namespace zschimmer {
namespace rtf {

const int doc_table_count = 5;      // fonttbl, stylesheet, colortbl, listtable, listoverridetable

struct Doc : Object, 
             Has_rtf_context
{
    struct Header_entry : Object, // Eintrag in einem Verzeichnis im Dokumentkopf (Style, Farbe etc.)
                          Has_rtf_context
    {
                                Header_entry            ( Has_rtf_context* c )          : Has_rtf_context(c) { _number=0; _entity=NULL;}

        int                     number                  ()                              { return _number; }

        int                    _number;
        string                 _name;
        Simple_entity*         _entity;
    };


    struct Font : Header_entry
    {
                                Font                    ( Has_rtf_context* c )          : Header_entry(c) {}
    };


    struct Style : Header_entry // Absatzformatvorlage
    {
                                Style                   ( Has_rtf_context* c, Code type = code_none, const string& name = "" )  : Header_entry(c), _code(type) { _name=name; }

        Simple_entity*          insert_into_doc         ( Simple_entity* after_this );
        Code                    code                    () const                        { return _code; }
        void                    set_property            ( Code, int param = 0 );


        Code                   _code;                   // code_s, code_cs
    };


    struct Color : Header_entry
    {
                                Color                   ( Has_rtf_context* c )                               : Header_entry(c) { _red=_green=_blue=0; _default=true; }
                                Color                   ( Has_rtf_context* c, int red, int blue, int green ) : Header_entry(c) { _red=red; _green=green; _blue=blue; _default=false; }

        bool                    operator ==             ( const Color& c ) const        { return _default == c._default && _red==c._red && _green==c._green && _blue==c._blue; }

        bool                   _default;                // _red, _green und _blue ohne Wert
        int                    _red;
        int                    _green;
        int                    _blue;
    };


    struct List : Header_entry 
    {
                                List                    ( Has_rtf_context* c )          : Header_entry(c), _listid(0),_listtemplateid(0)  {}

        int                    _listid;
        int                    _listtemplateid;
    };


    struct Listoverride : Header_entry 
    {
                                Listoverride            ( Has_rtf_context* c )          : Header_entry(c) {}
    };

    
    typedef stdext::hash_map< int, ptr<Header_entry> >  Header_map;


    struct Destination_entry
    {
                                Destination_entry       ( Destination_entity* d, bool need_intbl, bool restore ) : _destination(d), _need_intbl(need_intbl), _restore_properties(restore) {}

        Destination_entity*    _destination;
      //Properties             _properties;             // Sollten eigentlich auch gesichert werden, diese Implementierung ist aber langsam, und sowieso kommt ein \plain usw.
                                                        // Wird das nicht von factory_rtf.cxx bereits erledigt?
        bool                   _need_intbl;
        bool                   _restore_properties;
    };


    struct Header
    {
        static int              table_index             ( Code code )               { return code == code_fonttbl          ? 0 :
                                                                                             code == code_stylesheet       ? 1 :
                                                                                             code == code_colortbl         ? 2 :
                                                                                             code == code_listtable        ? 3 :
                                                                                             code == code_listoverridetable? 4
                                                                                                                           : -1; }

        Header_map&             header_map              ( Code code )               { return _maps[ table_index(code) ]; }

        int                     get_free_number         ( Code, int wish = 1 );     // Liefert eine frei Nummer einer Tabelle
        Header_entry*           entry_by_name           ( Code, const string& );
        void                    remove                  ( Code, Header_entry* );


        Font*                   font                    ( int no )                  { return (Font*)         +header_map(code_fonttbl)[no]; }
        Style*                  style                   ( int no )                  { return (Style*)        +header_map(code_stylesheet)[no]; }
        Color*                  color                   ( int no )                  { return (Color*)        +header_map(code_colortbl)[no]; }
        List*                   list                    ( int no )                  { return (List*)         +header_map(code_listtable)[no]; }
        Listoverride*           listoverride            ( int no )                  { return (Listoverride*) +header_map(code_listoverridetable)[no]; }

        Font*                   font_by_name            ( const string& name )      { return (Font*)        +entry_by_name( code_fonttbl, name ); }
        Style*                  style_by_name           ( const string& name )      { return (Style*)       +entry_by_name( code_stylesheet, name ); }
        List*                   list_by_name            ( const string& name )      { return (List*)        +entry_by_name( code_listtable, name ); }
        Listoverride*           listoverride_by_name    ( const string& name )      { return (Listoverride*)+entry_by_name( code_listoverridetable, name ); }

        Header_map             _maps [ doc_table_count ];
    };



                                Doc                     ( Has_rtf_context*, const string& debug_name = "" );
                               ~Doc                     ();

  //void                    set_dont_ignore_pgdsctbl    ( bool b )                      { _dont_ignore_pgdsctbl = b;  }
    void                        read_file               ( const string& filename );
    void                        read_file               ( FILE* );
    void                        read                    ( const char* rtf_text );

    bool                        filled                  () const                        { return _entity.head() != NULL; }
    Entity_list*                first_entity            ()                              { return _entity.head(); }

    void                        build_table             ( Code table_code );
    void                        build_fonttbl           ();
    void                        build_stylesheet        ();
    void                        build_colortbl          ();
    void                        build_listtable         ();
    void                        build_listoverridetable ();

    void                        prepend_info_doccomm    ( const string& );
    void                        optimize_body           ( Entity_list* );
    void                        optimize_body           ()                              { optimize_body( _entity.head() ); }
    void                        optimize                ()                              { optimize_body(); }

    void                        modify_for_microsoft_word( Entity_list*  );
    void                        modify_for_microsoft_word()                             { modify_for_microsoft_word( _entity.head() ); }

    // Zum Aufbauen:
    Simple_entity*              append_entity           ( const Simple_entity& );              // Behandelt Entity vorm Anhängen
    void                        append_entity           ( Code, int param );
    void                        set_property            ( Code prop, int par = 0 );
    void                        set_properties          ( const Properties& );

  //void                        reset_borders           ();
    // Destination anhängen:
    Destination_entity*         append_destination      ( Code, int param = 0 );
    void                        end_destination         ( Destination_entity* );

    void                        push_current_destination( bool has_own_properties );
    void                        pop_current_destination ();
    int                         is_in_destination       () const                        { return !_current_destination_stack.empty(); }

    void                        handle_doc_property     ( Simple_entity* );

    int                         header_ordinal          ( Code );

    void                        remove_style            ( Style* style )                { _header.remove( code_stylesheet, style ); }
    void                        add_style               ( Style* );
    void                        add_to_header_table     ( Code, Simple_entity* add, const string& /*debug_text*/ );
    Color*                      color                   ( int red, int green, int blue );     // Legt ggf. neue Color an

    void                        print                   ( zschimmer::file::File_base* ) const;
    void                        print_begin             ( zschimmer::file::File_base* file ) const         { file->print( "{\\rtf1" ); }
    void                        print_end               ( zschimmer::file::File_base* file ) const         { file->print( "}\r\n" ); }
    void                        print_entity_list       ( zschimmer::file::File_base*, const Entity_list*, const Entity_list* end = NULL, Flags allowed_properties = flag_all, bool need_blank = false, int indent = 0 ) const;
  //void                        print                   ( ostream* ) const;
  //void                        print_begin             ( ostream* s ) const            { *s << "{\\rtf1"; }
  //void                        print_end               ( ostream* s ) const            { *s << "}\r\n"; }
  //void                        print_entity_list       ( ostream*, const Entity_list*, const Entity_list* end = NULL, Flags allowed_properties = flag_all, bool need_blank = false, int indent = 0 ) const;

    Simple_entity*              find_last_header_entity ( Entity_listelem* ) const;
    Simple_entity*              find_last_header_entity () const                        { return find_last_header_entity( _current_destination->head() ); }
    void                        truncate                ( Entity_list* begin );
    void                        rescan_properties       ();
  //void                        delete_all_but_doc_properties();

  //friend ostream&             operator <<             ( ostream& s, const Doc& doc )  { doc.print( &s ); return s; }
    
    Fill_zero                  _zero_;
    string                     _debug_name;
  //bool                       _dont_ignore_pgdsctbl;
    bool                       _optimize_properties;
    Header                     _header;
    Simple_entity*             _doc_property_entities[ code__properties_max ];
    bool                       _doc_properties_changed;
    Entity                     _entity;                 // code_group

    // Zum Aufbau:
    Destination_entity*        _current_destination;
  //std::stack<Destination_entity*> _current_destination_stack;
    std::stack<Destination_entry>   _current_destination_stack;
    Properties                 _properties;             // Die Eigenschaften während des Aufbaus
    int                        _in_doc_prop_destination;// In {\fonttbl, {\list, {\stylesheet etc. keine Eigenschaften optimieren

    bool                       _need_intbl;
    bool                       _section_break_set;      // Kein \sbk... durchlassen, ist schon gesetzt (für Serien-Factory)
    bool                       _single_lines;           // Jede RTF-Anweisung auf eine Zeile, für diff
    int                        _indent;                 // RTF-Code einrücken (zum Test, ist damit nicht mehr RTF-konform)
};


} //namespace rtf
} //namespace zschimmer

#endif
