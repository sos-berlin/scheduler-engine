// rtf.h                                                ©2000 Joacim Zschimmer
// $Id$

#ifndef __RTF_H
#define __RTF_H

#include "../zschimmer.h"
#include "../file.h"

namespace zschimmer {
namespace rtf {

const int max_token_length  = 50;
const int invalid_param     = INT_MIN;

//---------------------------------------------------------------------------------------------Code

enum Code
{
    code_none,
    code_eof,
    code_text,              // einfacher Text inklusive \'xx
    code_group,             // Gruppe {..} (nicht Text), z.B. in {\stylesheet {\s1 ...}...}  
    code_any,
  //code_star_any,          // Wird nicht benutzt, sondern code_any && _star !!

#   include "rtf_code.h"    // Generiert von generate_rtf.h aus rtf.xml

  //code__tab_properties,   // Selbst erfundene Destination, die die Tabulatoreigenschaften sammelt

    code__predefined_count,
};

inline Code&                operator ++             ( Code& code, int )                             { return code = (Code)( code + 1 ); }
//-------------------------------------------------------------------------------------------------
#ifdef __GNUC__

    inline size_t               hash_value              ( Code code )                                   { return (size_t)code; }

    } //namespace rtf
    } //namespace zschimmer

    Z_DEFINE_GNU_HASH_VALUE( zschimmer::rtf, Code )

    namespace zschimmer {
    namespace rtf {

#endif
//--------------------------------------------------------------------------------------Entity_type
// Die Entities werden zur Speicherersparnis in verschiedenen Strukturen gespeichert.
// Jeder Code ist einer Struktur zugeordnet
// Entity_type beschreibt die Vererbungsstruktur der Strukturen Simple_entity, Param_entity, Destination_entity und Text_entity

enum Entity_type
{
    undefined_entity = 0,
    simple_entity,
    param_entity,
    destination_entity,
    text_entity
};


struct Rtf_context;
struct Has_rtf_context;
struct Simple_entity;
struct Param_entity;
struct Destination_entity;
struct Text_entity;
struct Properties;

//--------------------------------------------------------------------------------------------Kind 

enum Kind
{
    kind_none,
    kind_symbol,            // Code bezeichnet ein Symbol
    kind_toggle,            // Code schaltet etwas an oder aus, z.B. \b (_param=1) und \b0 (_param=0)
    kind_flag,              // Ohne Parameter
    kind_destination,       // {\... }
    kind_value,             // Mit Parameter 

    kind_ignore_destination,    // Diese Destination lesen wir nicht ein
    kind_ignore                 // Diese Nicht-Destination lesen wir nicht ein
};

//--------------------------------------------------------------------------------------------Flags

enum Flags                  
{
    flag_all                = -1,       // Alle Bits gesetzt

    flag_none               = 0,
    flag_prop_char          = 0x01,     // Eigenschaft eines Zeiches            Mit Embed_flags in hostole.odl abgleichen!
    flag_prop_para          = 0x02,     // Eigenschaft eines Absatzes           Mit Embed_flags in hostole.odl abgleichen!
    flag_prop_sect          = 0x04,     // Eigenschaft eines Abschnitts         Mit Embed_flags in hostole.odl abgleichen!
    flag_prop_doc           = 0x08,     // Eigenschaft des Dokuments            Mit Embed_flags in hostole.odl abgleichen!

    flag_prop_row           = 0x10,     // Eigenschaft einer Tabellenreihe
    flag_prop_tab           = 0x20,     // Eigenschaft eines Tabulatorposition
    flag_prop_cell          = 0x40,     // Eigenschaft einer Tabellenzelle
    flag_prop_dest          = 0x80,     // Eigenschaft in einer Destination
    flag_prop               = flag_prop_char | flag_prop_para | flag_prop_sect | flag_prop_doc
                            | flag_prop_tab  | flag_prop_row  | flag_prop_cell | flag_prop_dest,   //??Diese Eigenschaften ist komplexer, Tabelleneigenschaften z.B. werden für jede Spalte wiederholt, ebenso Tabulator, und dürfen nicht optimiert werden.
    flag_prop_border        = 0x100,
    flag_prop_para_border   = flag_prop_para | flag_prop_border,
    flag_prop_para_complete = flag_prop_para | flag_prop_border | flag_prop_tab,

    flag_header             = 0x200,    // "Eigenschaft" in einer Destination im Dokumentkopf (\colortbl etc.)
    flag_special_char       = 0x400,
    flag_has_text           = 0x800,    // Bei kind_destination: Gruppe enthält Text
    flag_own_prop           = 0x1000 | flag_has_text,   // Destination mit eigenen Eigenschaften, z.B. {\footnote ...}
    flag_star               = 0x2000,   // Destination mit Stern schreiben, z.B. {\*\fldinst ...}
    flag_print_newline      = 0x4000,   // Bei der Ausgabe eine neue Zeile beginnen (damit's besser lesbar ist)

    flag_learned_prop_doc   = 0x8000,   // Hinzugelernte Dokumenteeigenschaft (gilt nicht als Eigenschaft, weil code >= code__properties_max
    flag_learned_prop       = flag_learned_prop_doc,
    flag_unknown            = 0x80000000,   // Damit ein Flag gesetzt ist (flag_none nicht verwenden!)
};

Z_DEFINE_BITWISE_ENUM_OPERATIONS( Flags )

//--------------------------------------------------------------------------------------Known_descr
// Beschreibt einem im Quellcode bekannten RTF-Code (im Gegensatz zu einem gelernten).

struct Known_descr
{
    bool                    is_toggle               () const                { return _kind == kind_toggle; }                        
    bool                    is_flag                 () const                { return _kind == kind_flag; }
    bool                    is_on_off               () const                { return _kind == kind_toggle || _kind == kind_flag; }
    bool                    is_destination          () const                { return _kind == kind_destination; }
    bool                    has_value               () const                { return _kind == kind_value; }
    bool                    has_param               () const                { return is_toggle() || has_value(); }
    bool                    has_text                () const                { return ( _flags & flag_has_text  ) != 0; }
    bool                    is_char_property        () const                { return ( _flags & flag_prop_char ) != 0; }
    bool                    is_tab_property         () const                { return ( _flags & flag_prop_tab  ) != 0; }
    bool                    is_para_property        () const                { return ( _flags & flag_prop_para ) != 0; }
    bool                    is_sect_property        () const                { return ( _flags & flag_prop_sect ) != 0; }
    bool                    is_doc_property         () const                { return ( _flags & flag_prop_doc  ) != 0; }
    bool                    is_property             () const                { return ( _flags & flag_prop      ) != 0; }
    bool                    is_property_or_header   () const                { return ( _flags & ( flag_prop | flag_header ) ) != 0; }
    bool                    is_header               () const                { return ( _flags & flag_header    ) != 0; }
    bool                    has_own_properties      () const                { return ( _flags & flag_own_prop  ) != 0; }
    Flags                   prop_flags              () const                { return (Flags)( _flags & flag_prop ); }
    bool                    star                    () const                { return ( _flags & flag_star ) != 0; }

    Entity_type             type                    ( Code ) const;

    Code                   _code;
    Byte                   _word_version;
    const char*            _name;
    Kind                   _kind;
    int                    _flags;
    int                    _default;
};

//--------------------------------------------------------------------------------------------const

const extern Known_descr    known_descr_array       [ code__predefined_count ];          // Alle RTF-Codes
const extern Known_descr    const_new_descr;        // Vorlage für neue RTF-Codes

//-------------------------------------------------------------------------------------------------

void                        print_rtf_table         ( ostream* );

//-------------------------------------------------------------------------------------------------

inline const Known_descr*   known_descr             ( Code code )           { return code < code__predefined_count? &known_descr_array[ code ] : &const_new_descr; }

inline bool                 is_toggle               ( Code code )                                   { return known_descr( code )->is_toggle(); }
inline bool                 is_flag                 ( Code code )                                   { return known_descr( code )->is_flag(); }
inline bool                 is_on_off               ( Code code )                                   { return known_descr( code )->is_on_off(); }
inline bool                 is_char_property        ( Code code )                                   { return known_descr( code )->is_char_property(); }
inline bool                 is_tab_property         ( Code code )                                   { return known_descr( code )->is_tab_property(); }
inline bool                 is_para_property        ( Code code )                                   { return known_descr( code )->is_para_property(); }
inline bool                 is_sect_property        ( Code code )                                   { return known_descr( code )->is_sect_property(); }
inline bool                 is_doc_property         ( Code code )                                   { return known_descr( code )->is_doc_property(); }
inline bool                 is_property             ( Code code )                                   { return known_descr( code )->is_property(); }
inline bool                 is_property_or_header   ( Code code )                                   { return known_descr( code )->is_property_or_header(); }
inline bool                 is_header               ( Code code )                                   { return known_descr( code )->is_header(); }
inline bool                 has_own_properties      ( Code code )                                   { return known_descr( code )->has_own_properties(); }
inline bool                 is_text                 ( Code code )                                   { return(known_descr( code )->_flags & flag_special_char ) || code == code_text; }

//--------------------------------------------------------------------------------------------Descr

struct Descr
{
                            Descr                   ()                      {}

    bool                    is_toggle               () const                { return _kind == kind_toggle; }                        
    bool                    is_flag                 () const                { return _kind == kind_flag; }
    bool                    is_on_off               () const                { return _kind == kind_toggle || _kind == kind_flag; }
    bool                    is_destination          () const                { return _kind == kind_destination; }
    bool                    has_value               () const                { return _kind == kind_value; }
    bool                    has_param               () const                { return is_toggle() || has_value(); }
    bool                    has_text                () const                { return ( _flags & flag_has_text  ) != 0; }
    bool                    is_char_property        () const                { return ( _flags & flag_prop_char ) != 0; }
    bool                    is_tab_property         () const                { return ( _flags & flag_prop_tab  ) != 0; }
    bool                    is_para_property        () const                { return ( _flags & flag_prop_para ) != 0; }
    bool                    is_sect_property        () const                { return ( _flags & flag_prop_sect ) != 0; }
    bool                    is_doc_property         () const                { return ( _flags & flag_prop_doc  ) != 0; }
    bool                    is_property             () const                { return ( _flags & flag_prop      ) != 0; }
    bool                    is_property_or_header   () const                { return ( _flags & ( flag_prop | flag_header ) ) != 0; }
    bool                    is_header               () const                { return ( _flags & flag_header    ) != 0; }
    bool                    has_own_properties      () const                { return ( _flags & flag_own_prop  ) != 0; }
    Flags                   prop_flags              () const                { return (Flags)( _flags & ( flag_prop | flag_learned_prop_doc ) ); }
    bool                    is_non_allowed_property ( Flags allowed_properties ) const { return ( _flags & (flag_prop|flag_learned_prop_doc) & ~allowed_properties ) != 0; }
    bool                    star                    () const                { return ( _flags & flag_star ) != 0; }
    bool                    is_learned_doc_property () const                { return ( _flags & flag_learned_prop_doc  ) != 0; }  // => is_property() == false!

    Entity_type             type                    () const                { if( !_type )  compute_type();  return _type; }
    void                    compute_type            () const;               // Verändert _type

    friend ostream&         operator <<             ( ostream& s, const Descr& d )             { if( d._flags & flag_star )  s << "\\*";  s << '\\' << d._name;  return s; }


    Code                   _code;
    string                 _name;
    Kind                   _kind;
    int                    _flags;
    int                    _default;
    mutable Entity_type    _type;
};

//----------------------------------------------------------------------------------Has_rtf_context

struct Has_rtf_context
{
                            Has_rtf_context         ( Rtf_context* c )                              : _rtf_context(c) {}
                            Has_rtf_context         ( Has_rtf_context* );

    Has_rtf_context*        has_rtf_context         ()                                              { return this; }

    const Descr*            descr                   ( Code code ) const;
    const Descr*            descr                   ( const Simple_entity* ) const;

    bool                    is_destination          ( Code code ) const;
    bool                    has_value               ( Code code ) const;
    bool                    has_param               ( Code code ) const;
    bool                    has_text                ( Code code ) const;


          Param_entity*         cast_param_entity      (       Simple_entity* ) const;
    const Param_entity*         cast_param_entity      ( const Simple_entity* ) const;
          Destination_entity*   cast_destination_entity(       Simple_entity* ) const;
    const Destination_entity*   cast_destination_entity( const Simple_entity* ) const;
          Text_entity*          cast_text_entity       (       Simple_entity* ) const;
    const Text_entity*          cast_text_entity       ( const Simple_entity* ) const;

    Simple_entity*          new_entity              ( Code code, int param = 0 );
    Simple_entity*          new_entity              ( const Simple_entity& );

    void                    print_entity            ( ostream*, const Simple_entity& ) const;
    void                    print_entity            ( zschimmer::file::File_base*, const Simple_entity& ) const;


    Rtf_context* const     _rtf_context;
};

//------------------------------------------------------------------------------------Simple_entity

struct Simple_entity
{
    struct Iterator         // NICHT GETESTET
    {
                            Iterator                ( Simple_entity* e )                            : _ptr(e) {}

        Iterator&           operator ++             ();
                            operator bool           ()                                              { return _ptr != NULL; }
        Simple_entity*      operator *              ()                                              { return _ptr; }
        Simple_entity*      operator ->             ()                                              { return _ptr; }


        Simple_entity*     _ptr;
        typedef std::stack<Simple_entity*> Stack;
        Stack              _stack;
    };



    void*                   operator new            ( size_t );
    void                    operator delete         ( void*, size_t );

                            Simple_entity           ( Code code = code_none, Entity_type type = simple_entity ) : _code(code), _next(NULL) { assert_matching_class(type);  count_entities(); }
                            Simple_entity           ( const Simple_entity& e )                      : _code(e._code), _next(NULL) { count_entities(); }
    virtual                ~Simple_entity           ()                                              {}

    void                    count_entities          ()                                              { if( ++static_entity_count > static_entity_count_high )  static_entity_count_high = static_entity_count; }

    void                    assert_matching_class   ( Entity_type ) const;
    void                    clear                   ()                                              { _code = code_none; }

    Code                    code                    () const                                        { return _code; }
    Simple_entity*          next                    ()                                              { return _next; }

    const Known_descr*      known_descr             () const                                        { return rtf::known_descr( _code ); }
    Entity_type             known_descr_type        () const                                        { return known_descr()->type(_code); }


    // Siehe rtf_parser.cxx, Aufruf von add_rtf_descr(), welche Eigenschaften hinzugelernter RTF-Codes dynamisch erkannt werden, 
    // und hier NICHT von known_descr() gelesen weden dürfen.
    // Das sind u.a. kind_destination, kind_value, kind_flag.

    bool                    is_toggle               () const                                        { return known_descr()->is_toggle(); }
    bool                    has_text                () const                                        { return known_descr()->has_text(); }
    bool                    is_char_property        () const                                        { return known_descr()->is_char_property(); }
    bool                    is_tab_property         () const                                        { return known_descr()->is_tab_property(); }
    bool                    is_para_property        () const                                        { return known_descr()->is_para_property(); }
    bool                    is_sect_property        () const                                        { return known_descr()->is_sect_property(); }
    bool                    is_doc_property         () const                                        { return known_descr()->is_doc_property(); }
    bool                    is_property             () const                                        { return known_descr()->is_property(); }
    bool                    is_property_or_header   () const                                        { return known_descr()->is_property_or_header(); }
    bool                    is_header               () const                                        { return known_descr()->is_header(); }
    bool                    has_own_properties      () const                                        { return known_descr()->has_own_properties(); }
    bool                    is_text                 ()                                              { return rtf::is_text( _code ); }


    virtual void            print                   ( Rtf_context*, ostream* ) const;
    virtual void            print                   ( Rtf_context*, zschimmer::file::File_base* ) const;

    void                    remove_successor        ();                                             // Entfernt *_next aus der Liste
    void                    insert_successor        ( Simple_entity* e )                            { e->_next = _next; _next = e; }
    void                    append                  ( Simple_entity* e )                            { assert( _next == NULL);  _next = e; }
  //string                  special_as_string       ( int len, bool fill );

    virtual int             param                   () const                                        { return known_descr()->is_flag()? 1 : 0; }
    virtual void        set_param                   ( int p )                                       { if( p != param() )  throw_xc( "Z-RTF-005", known_descr()->_name ); }
    virtual Simple_entity*  head                    () const                                        { throw_xc( "Z-RTF-004", known_descr()->_name ); }

    void                    change_code             ( Code new_code );

  protected:
    void                set_code                    ( Code code )                                   { _code = code; }


    Code                   _code;

  public:
    Simple_entity*         _next;

    static int              static_entity_count;
    static int              static_entity_count_high;

    static size_t           static_allocated_bytes;
    static size_t           static_allocated_bytes_high;
};

typedef Simple_entity       Entity_listelem;
typedef Entity_listelem     Entity_list;            // Wenn die ganze Liste gemeint ist.

//-------------------------------------------------------------------------------------Param_entity

struct Param_entity : Simple_entity
{
                            Param_entity            ( Code code = code_none, int param = 0, Entity_type t = param_entity ) : Simple_entity(code,t), _param(param) {}

    void                    clear                   ()                                              { Simple_entity::clear();  _param = 0; }

    void                set_param                   ( int param )                                   { _param = param; }
    int                     param                   () const                                        { return _param; }

    void                    print                   ( Rtf_context*, ostream* ) const;
    void                    print                   ( Rtf_context*, zschimmer::file::File_base* ) const;


    int                    _param;                  // kind_value, kind_toggle (normiert auf 1 oder 0), kind_flag (immer 1)
};

//-------------------------------------------------------------------------------Destination_entity

struct Destination_entity : Param_entity, 
                            Has_rtf_context
{
                            Destination_entity      ( Has_rtf_context* c, Code code = code_none, int param = 0, Entity_type t = destination_entity ) : Param_entity( code, param, t ), 
                                                                                                                                                       Has_rtf_context(c), 
                                                                                                                                                       _head(NULL), _last(NULL) {}
                            Destination_entity      ( const Destination_entity& );
                           ~Destination_entity      ();

    Destination_entity&     operator =              ( const Destination_entity& );
    
    void                    deep_copy               ( const Destination_entity& );
    void                    shallow_copy            ( const Destination_entity& );                  // _head = NULL
    void                    set_head                ( Simple_entity* new_head );
    void                    move_head_to            ( Destination_entity* );
    Simple_entity*          head                    () const                                        { return _head; }
    void                    append_to_destination   ( Simple_entity* new_entity );
    void                    insert_after            ( Simple_entity* entity_before, Simple_entity* new_entity );
    void                    truncate                ( Simple_entity* previous_element );

    void                    print                   ( Rtf_context*, ostream* ) const;
    void                    print                   ( Rtf_context*, zschimmer::file::File_base* ) const;


  private:
    Simple_entity*         _head;
    Simple_entity*         _last;
};

//--------------------------------------------------------------------------------------Text_entity

struct Text_entity : Destination_entity
{
                            Text_entity             ( Has_rtf_context* c, Code code = code_text )   : Destination_entity( c, code, 0, text_entity ) {}
                            Text_entity             ( Has_rtf_context* c, const string& text )      : Destination_entity( c, code_text, 0, text_entity ), _text(text) {}

    void                    clear                   ()                                              { Param_entity::clear();  _text.erase(); }

    void                set_text                    ( const string& text )                          { _text = text; }
    void                set_text                    ( const char* text )                            { _text = text; }
    const string&           text                    () const                                        { return _text; }
    int                     text_length             () const                                        { return (int)length( _text ); }

    void                    print                   ( Rtf_context*, ostream* ) const;
    void                    print                   ( Rtf_context*, zschimmer::file::File_base* ) const;


  public:
    string                 _text;
};

//--------------------------------------------------------------------------------------------Entity 

struct Entity : Text_entity
{
                            Entity                  ( Has_rtf_context* c, Code code = code_none )   : Text_entity( c, code ) {}
                            Entity                  ( Has_rtf_context* c, Code code, int param )    : Text_entity( c, code ) { _param = param; }

    void                set_code                    ( Code code )                                   { Text_entity::set_code( code ); }
};

//---------------------------------------------------------------------------------------Properties

struct Properties : Has_rtf_context                 // Zeichen-, Absatz- und Abschnittseigenschaften
{
    Fill_zero _zero_;


    typedef void (*Write_property_callback)( void*, const Simple_entity& );


    struct Entry
    {
                            Entry                   ()                                              : _value(0), _entity(NULL), _next(code_none), _prev(code_none) {}

                            operator int            () const                                        { return _value; }

        int                _value;
        Simple_entity*     _entity;
        Code               _next;
        Code               _prev;            
    };


                            Properties              ( Has_rtf_context* );

  //void                    clear                   ();
    int                     property                ( Code code ) const                             { return _properties.at( code )._value; }
    Entry&                  operator []             ( Code code )                                   { return _properties.at( code ); }
    const Entry&            operator []             ( Code code ) const                             { return _properties.at( code ); }
  //void                    set_property            ( Code code, int val )                          { _properties[ code ] = val; }
    void                    set_property            ( Simple_entity* );
    void                    set_property            ( Code, int, Simple_entity* e = NULL );
    void                    reset_properties        ( Flags );
    void                    reset_property          ( Code, int param );
    void                    reset_char_properties   ()                                              { reset_properties( flag_prop_char ); }
    void                    reset_para_properties   ()                                              { reset_properties( flag_prop_para_complete ); }
    void                    reset_sect_properties   ()                                              { reset_properties( flag_prop_sect ); }
    bool                    is_default              ( Code code ) const                             { return property( code ) == known_descr(code)->_default; }
    bool                    is_set                  ( Code code ) const                             { return _properties.at( code )._entity != NULL; }

    void                    write                   ( void* callback_this, Write_property_callback, Flags allowed_properties ) const;

    Simple_entity*          insert                  ( Simple_entity*, const Properties& base  );
    
    void                    print                   ( ostream* ) const;
    friend ostream&         operator <<             ( ostream& s, const Properties& p )             { p.print( &s ); return s; }


    void                    set_property2           ( Code, int, Simple_entity* );


    enum 
    { 
        before_first_property = code_none,
        after_last_property   = code__properties_max + 1
    };



    Properties&             operator =              ( const Properties& p )                         { _properties                = p._properties;
                                                                                                      _handle_complex_properties = p._handle_complex_properties;
                                                                                                      _tabs                      = p._tabs;
                                                                                                      _prop_flags                = p._prop_flags; 
                                                                                                      return *this; }


    std::vector<Entry>     _properties;                              // Das sind einfach die entsprechenden Rtf_codes
                                                                     // _properties[before_first_property]._next = Die zuerst gesetzte und nicht mehr geänderte Eigenschaft
                                                                     // _properties[after_last_property  ]._prev  = Die zuletzt gesetzte Eigenschaft
    bool                   _handle_complex_properties;
    Destination_entity     _tabs;
    Flags                  _prop_flags;
};

//---------------------------------------------------------------------------------------Entity_ptr

struct Entity_ptr : Has_rtf_context                 // Zeiger auf das (nächste) Item oder die Textstelle darin (bei _code == code_text)
{
                            Entity_ptr              ( Has_rtf_context* c, Entity_list* e = NULL )       : Has_rtf_context(c), _next_entity(e),_char_pos(0) {}
    virtual                ~Entity_ptr              ()                              {};

  //void                    init                    ( const Entity_list* e )        { _next_entity = e;  _char_pos = 0; }

    Entity_ptr&             operator =              ( const Entity_ptr& p )         { _next_entity = p._next_entity;  _char_pos = p._char_pos;  return *this; }
    Entity_ptr&             operator =              ( Entity_list* e )              { _next_entity = e;  _char_pos = 0;  return *this; }
    Simple_entity&          operator *              ()                              { return *_next_entity; }
    Simple_entity*          operator ->             ()                              { return _next_entity; }
    operator                Simple_entity*          ()                              { return _next_entity; }
    void                    split_text              ();                             // code_text an der Stelle _char_pos aufspalten in zwei Entities
    void                    correct_char_ptr_after_split();
    const char*             char_ptr                () const;
    size_t                  rest_length             () const;
    string                  paragraph_text          () const;

    bool                    operator ==             ( const Entity_ptr& p ) const   { return _next_entity == p._next_entity  &&  _char_pos == p._char_pos; }
    bool                    operator ==             ( const Simple_entity* e ) const{ return _next_entity == e  &&  _char_pos == 0; }
    bool                    operator !=             ( const Entity_ptr& e ) const   { return !( *this == e ); }
    bool                    operator !=             ( const Simple_entity* e ) const{ return !( *this == e ); }
    
    bool                    eof                     () const                        { return _next_entity == NULL; }

    // Entität für Entität:
    void                    to_next_entity          ()                              { _char_pos = 0;  _next_entity = _next_entity->next(); }

    // Zur nächsten Entität, dabei Eigenschaften überspringen
    void                    skip_and_ignore_props   ();

    // Nur bei code_text:
    void                    to_next_char            ( size_t n = 1 )                { _char_pos += n; if( *char_ptr() == '\0' )  to_next_entity(); }


    // Zeichen für Zeichen:
    int                     peek_char               ();
    int                     peek_char_until         ( const Entity_ptr& );
    int                     peek_char_until_2       ( const Simple_entity* );
    void                    skip_char               ();
    
    // Für Entity_ptr_with_prop:
    bool                    process_properties      ()                              { return process_properties_until( NULL ); }
    virtual bool            process_properties_until( const Simple_entity* );


    void                    print                   ( ostream* ) const;
    friend ostream&         operator <<             ( ostream& s, const Entity_ptr& p )  { p.print( &s ); return s; }


    Simple_entity*         _next_entity;
    size_t                 _char_pos;               // _item->_code == code_text
};


string read_text( const Entity_ptr& );

//-----------------------------------------------------------------------------Entity_ptr_with_prop

struct Entity_ptr_with_prop : Entity_ptr            // Mit den Eigenschaften, die an der Stelle gelten
{
                            Entity_ptr_with_prop    ( Has_rtf_context* c )                              : Entity_ptr(c), _properties(c) { _properties._handle_complex_properties = true; }
                          //Entity_ptr_with_prop    ( Has_rtf_context* c, Entity_list* e = NULL )       : Entity_ptr(c), _properties(c) { _properties._handle_complex_properties = true; }

    bool                    reset_properties        ( Flags flag )                                      { _properties.reset_properties( flag ); return true; }
    bool                    process_properties_until( const Simple_entity* );
    void                    forward_to              ( const Simple_entity* until );
    bool                    set_property            ();
    void                    skip_and_ignore_props   ();

    void                    operator =              ( Entity_list* e )                                  { Entity_ptr::operator=( e ); }
    void                    operator =              ( const Entity_ptr_with_prop& e )                   { Entity_ptr::operator=( e ); _properties = e._properties; }


    Properties             _properties;
};

//--------------------------------------------------------------------------------------Rtf_context

struct Rtf_context : Object,
                     Has_rtf_context
{
                            Rtf_context             ();

    void                    ignore_pgdsctbl         ( bool );

    const Descr*            descr                   ( Code code ) const                             { return &_descr_array[ code ]; }
    const Descr*            descr                   ( const Simple_entity* e ) const                { return &_descr_array[ e->code() ]; }

    void                    add_to_descr_map        ( Code code, bool is_new );
    Code                    add_rtf_descr           ( const string& name, Kind kind, Flags flags );
    void                    set_rtf_descr_no_default_param( Code );
    void                    print_as_rtf_text       ( ostream*, const char* text, size_t len );
    void                    print_as_rtf_text       ( zschimmer::file::File_base*, const char* text, size_t len );
    Code                    code_from_name          ( const string& control_word );
    Code                    section_from_doc_property( Code doc_property );

    typedef stdext::hash_map< string, Code >  Descr_map;
    Descr_map              _descr_map;
    
    std::vector<Descr>     _descr_array;
    bool                   _change_doc_to_section_properties;

    typedef stdext::hash_map<Code,Code>  Doc_to_section_property_map;
    Doc_to_section_property_map         _doc_to_section_property_map;
};

//--------------------------------------------------------------------------Has_rtf_context inlines

inline                      Has_rtf_context::Has_rtf_context         ( Has_rtf_context* c )         : _rtf_context( c->_rtf_context ) {}
inline const Descr*         Has_rtf_context::descr                   ( Code code ) const            { return _rtf_context->descr( code ); }
inline const Descr*         Has_rtf_context::descr                   ( const Simple_entity* e ) const { return _rtf_context->descr( e->code() ); }
inline bool                 Has_rtf_context::is_destination          ( Code code ) const            { return _rtf_context->descr( code )->is_destination(); }
inline bool                 Has_rtf_context::has_value               ( Code code ) const            { return _rtf_context->descr( code )->has_value(); }
inline bool                 Has_rtf_context::has_param               ( Code code ) const            { return _rtf_context->descr( code )->has_param(); }
inline bool                 Has_rtf_context::has_text                ( Code code ) const            { return descr( code )->has_text(); }

inline void                 Has_rtf_context::print_entity            ( ostream* s   , const Simple_entity& e ) const   { if( &e )  e.print( _rtf_context, s ); }
inline void                 Has_rtf_context::print_entity            ( zschimmer::file::File_base* file, const Simple_entity& e ) const   { if( &e )  e.print( _rtf_context, file ); }

//---------------------------------------------------------------Has_rtf_context::cast_param_entity

inline Param_entity* Has_rtf_context::cast_param_entity( Simple_entity* e ) const
{
    //if( e )  e->assert_cast( param_entity );
    //return (Param_entity*)e;
    if( !e )  return NULL;
    Param_entity* result = DYNAMIC_CAST( Param_entity*, e );
    if( !result )  throw_xc( "Z-RTF-002", e->known_descr()->_name, "Param_entity" );
    return result;
}

//---------------------------------------------------------------Has_rtf_context::cast_param_entity

inline const Param_entity* Has_rtf_context::cast_param_entity( const Simple_entity* e ) const
{
    //if( e )  e->assert_cast( param_entity );
    //return (const Param_entity*)e;
    if( !e )  return NULL;
    const Param_entity* result = DYNAMIC_CAST( const Param_entity*, e );
    if( !result )  throw_xc( "Z-RTF-002", e->known_descr()->_name, "Param_entity" );
    return result;
}

//---------------------------------------------------------Has_rtf_context::cast_destination_entity

inline Destination_entity* Has_rtf_context::cast_destination_entity( Simple_entity* e )  const
{
    //if( e )  e->assert_cast( destination_entity );
    //return (Destination_entity*)e;
    if( !e )  return NULL;
    Destination_entity* result = DYNAMIC_CAST( Destination_entity*, e );
    if( !result )  throw_xc( "Z-RTF-002", e->known_descr()->_name, "Destination_entity" );
    return result;
}

//---------------------------------------------------------Has_rtf_context::cast_destination_entity

inline const Destination_entity* Has_rtf_context::cast_destination_entity( const Simple_entity* e )  const
{
    //if( e )  e->assert_cast( destination_entity );
    //return (const Destination_entity*)e;
    if( !e )  return NULL;
    const Destination_entity* result = DYNAMIC_CAST( const Destination_entity*, e );
    if( !result )  throw_xc( "Z-RTF-002", e->known_descr()->_name, "Destination_entity" );
    return result;
}

//----------------------------------------------------------------Has_rtf_context::cast_text_entity

inline Text_entity* Has_rtf_context::cast_text_entity( Simple_entity* e )  const
{
    //if( e )  e->assert_cast( text_entity );
    //return (Text_entity*)e;
    if( !e )  return NULL;
    Text_entity* result = DYNAMIC_CAST( Text_entity*, e );
    if( !result )  throw_xc( "Z-RTF-002", e->known_descr()->_name, "Text_entity" );
    return result;
}

//----------------------------------------------------------------Has_rtf_context::cast_text_entity

inline const Text_entity* Has_rtf_context::cast_text_entity( const Simple_entity* e ) const
{
    //if( e )  e->assert_cast( text_entity );
    //return (const Text_entity*)e;
    if( !e )  return NULL;
    const Text_entity* result = DYNAMIC_CAST( const Text_entity*, e );
    if( !result )  throw_xc( "Z-RTF-002", e->known_descr()->_name, "Text_entity" );
    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace rtf
} //namespace zschimmer

//-------------------------------------------------------------------------------------------------

#endif
