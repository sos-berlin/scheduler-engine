#ifndef __SOSSQL2_H
#define __SOSSQL2_H

#include "sossql.h"

#include "../zschimmer/regex_class.h"

#if !defined __SOSLIMTX_H
#   include "soslimtx.h"
#endif

#if !defined __SOSSET_H
#   include "sosset.h"
#endif

#if !defined __DECIMAL_H
#   include "decimal.h"
#endif

#if !defined __DYNOBJ_H
#   include "dynobj.h"
#endif

#if !defined __SOSDATE_H
#   include "sosdate.h"         // Für _sys_date
#endif

#if !defined __SOSDB_H
#   include "../file/sosdb.h"
#endif

#if !defined __ANYFILE_H
#   include "../file/anyfile.h"
#endif

#if !defined __FLSTREAM_H
#   include "../file/flstream.h"
#endif

#if !defined SOSFUNC_H
#   include "sosfunc.h"
#endif

#if defined SYSTEM_WIN32
#   define SOSSQL_OLE
#   include "sosole.h"
#endif

#include <vector>
using std::vector;

#include <map>
using std::map;

namespace sos
{

static const int max_sossql_field_size = 1024;     // Größe für generierte Ergebnisfelder

// Namen der vordefinierten, internen Tabellen:
#define SYSTEM_TABLE_NAME "_SYSTEM"
#define PARAM_TABLE_NAME  "_SYSTEM_PARAM"    // Parameter "?"

#if !defined SYSTEM_WIN16
    #define _USERENTRY  __cdecl      // qsort()
#endif

//struct Sossql_static;
struct Sql_expr;
struct Sql_expr_with_operands;
struct Sql_field_expr;
struct Sossql_session;
struct Sossql_file;
struct Sql_stmt;

Bool sql_like( const Dyn_obj& a, const Const_area& b_text );
Bool sql_like( const Dyn_obj& a, const Const_area& b_text, Area* hilfspuffer );
Bool sql_like( const Dyn_obj& a, const Dyn_obj& b, Area* hilfspuffer = NULL );
bool sql_regex_match( const string& a, const string& pattern );

//----------------------------------------------------------------------------Sql_key_intervall
// Ein geschlossenes Intervall von aufeinanderfolgenden, nacheinander zu lesenden Sätzen
// Oder eine Menge von Intervallen, ein Intervall für jede Kombination der ersten _offset Bytes.
// Die ersten _offset Bytes heißen Variabler Anteil, die restlichen Fester Anteil.
// Damit kann ein segmentierter Schlüssel erfasst werden.

struct Sql_key_intervall
{
                                Sql_key_intervall       ();
                                Sql_key_intervall       ( const Const_area& low, Bool low_incl,
                                                          const Const_area& high, Bool high_incl,
                                                          uint length, uint offset );
                                Sql_key_intervall       ( const Const_area& key, uint len, uint offset );
                                Sql_key_intervall       ( const Sql_key_intervall& o );
                               ~Sql_key_intervall       ();

    Sql_key_intervall&          operator =              ( const Sql_key_intervall& );
    friend void                 exchange_key_intervall  ( Sql_key_intervall*, Sql_key_intervall* );

    void                        assign                  ( const Const_area& low, Bool low_incl,
                                                          const Const_area& high, Bool high_incl,
                                                          uint length, uint offset );
    void                        assign                  ( const Const_area& key, uint len, uint offset );

    int                        _offset;                 // Bytes vor diesem Offset sind beliebig,
                                                        // also für jeden Wert der ersten _offset Bytes ein eigenes Intervall!
    int                        _length;

    const Const_area&           lowest_key              () const                        { return _lowest_key; }
    const Const_area&           highest_key             () const                        { return _same? _lowest_key : _highest_key; }

    void                        highest_key             ( const Const_area& k )         { _highest_key = k; _same = false; }

  private:
    friend Sql_key_intervall&   operator &=             ( Sql_key_intervall&, const Sql_key_intervall& );

    Dynamic_area               _lowest_key;
    Dynamic_area               _highest_key;
    Bool                       _same;                   // _lowest_key == _highest_key, ein paar Megabytes weniger
};

inline void exchange( Sql_key_intervall* a, Sql_key_intervall* b )
{
    exchange_key_intervall( a, b );
}

typedef Sos_simple_array<Sql_key_intervall> Sql_key_intervalle;

ostream&            operator<<    ( ostream&, const Sql_key_intervall& );
ostream&            operator<<    ( ostream&, const Sql_key_intervalle& );
Sql_key_intervall&  operator&=    ( Sql_key_intervall&, const Sql_key_intervall& );
void                normalize_ored( Sql_key_intervalle* );


//-------------------------------------------------------------------------------Sql_outer_join

enum Sql_outer_join
{
    oj_none       = 0,
    oj_outer_join = 0x01,     // *=  left outer join
    oj_log        = 0x02      // :=  Statt Nullwerten Protokollzeile ausgeben
};

//---------------------------------------------------------------------------------Sql_operator
// Ein Operator in einem Ausdruck

enum Sql_operator
{
    op_none,                // noch nicht definiert
  //op_null,                // Konstante NULL
    op_select_star,         // * (alle Felder, nur temporär auf oberster Stufe)
    op_const,               // Ein konstanter Wert
    op_field,               // Ein Feld einer Tabelle
    op_rowid,               // ROWID
    op_param,               // Ein Parameter "?", wird mit bind_parameters() gesetzt
    op_cond,                // ? :
    op_or,                  // OR
    op_and,                 // AND
    op_function,            // Funktionsaufruf
    op_not,                 // NOT,
    op_concat,              // ||  String-Konkatenation
    op_cmp_begin,
    op_lt = op_cmp_begin,   // <
    op_le,                  // <=
    op_eq,                  // =
    op_ne,                  // <>
    op_ge,                  // >=
    op_gt,                  // >
    op_like,                // LIKE
    op_cmp_end = op_like,
    op_regex_match,         // =~
    op_negate,              // -
    op_is_null,             // IS NULL
  //op_in,                  // IN
  //op_in_select,           // IN ( SELECT ... )
    op_exists,              // EXISTS
    op_between,             // BETWEEN
    op_add,                 // +
    op_subtract,            // -
    op_multiply,            // *
    op_divide,              // /
    op_select,              // ( SELECT ... )
    op_func_decode,         // DECODE( ... )
    op_func_field,          // FIELD( ... )
    op_func_group_counter,  // GROUP_COUNTER(...)
    op_aggregate_avg,
    op_aggregate_count,
    op_aggregate_max,
    op_aggregate_min,
  //op_aggregate_stddev,
    op_aggregate_sum,
  //op_aggregate_variance,
    op_aggregate_groupby,

    op_highest
};

extern const char* sql_op_text     [ op_highest - 1 ];
extern int         sql_op_priority [ op_highest - 1 ];
inline bool        is_aggregate_op( Sql_operator op ) { return op == op_aggregate_avg    || 
                                                               op == op_aggregate_count  ||
                                                               op == op_aggregate_max    ||
                                                               op == op_aggregate_min    ||
                                                             //op == op_aggregate_stddev ||
                                                               op == op_aggregate_sum    ||
                                                               op == op_aggregate_groupby /*||
                                                               op == op_aggregate_variance*/; }

Bool is_join_op( Sql_operator );

inline ostream& operator << ( ostream& s, Sql_operator op )  { s << sql_op_text[ op ]; return s; }

//-----------------------------------------------------------------------------Sql_aggregate_op
/*
enum Sql_aggregate_op
{
    ag_none,
    ag_avg,
    ag_count,
    ag_max,
    ag_min,
  //ag_stddev,
    ag_sum,
  //ag_variance,
};
*/
//------------------------------------------------------------------------Sql_join_expr_nesting

enum Sql_join_expr_nesting
{
    je_ground,              // Anfang
    je_or,                  // x OR x
    je_and,                 // x AND x .. x OR .. x AND xx
    je_not,                 // NOT
    je_cmp,                 // =
    je_other                // tiefere Verschachtelung
};

//--------------------------------------------------------------------------------Sql_table_set

typedef Sos_set< sql_max_tables_per_select >  Sql_table_set;

//------------------------------------------------------------------------------------Sql_table
// Eine Tabelle kann eine Datei oder eine spezielle Tabelle sein.

struct Sql_table : Sos_self_deleting
{
                                Sql_table               ();
                               ~Sql_table               ();

    Bool                        is_primary_key_field    ( const Field_descr* );
    Bool                        is_primary_key_field    ( const Sql_field_expr* );
    Bool                        is_secondary_key_field  ( const Sql_field_expr* );
    void                        set_join                ( Sql_expr* );   // Macht einen Join bekannt
    void                        begin_join_expr         ( Sql_operator );
    void                        end_join_expr           ();

    void                        prepare                 ();
    void                        open                    ();
    void                        update_selected_type    ( Field_descr* );
    void                        prepare_where           ( Sql_expr* where_clause );
    void                        close                   ();
    void                        rewind                  ();

  //Sql_key_intervall           single_key_intervall    ( const Sql_expr* );
    void                        add_key_intervalls_cmp  ( Sql_key_intervalle*, Sql_expr*, Bool single_only = false );
    void                        add_key_intervalls      ( Sql_key_intervalle*, Sql_expr* );
    void                        add_key_intervalls2     ( Sql_key_intervalle*, Sql_expr* );
    void                        print_intervall         ( ostream*, const Sql_key_intervall& );
    void                        print_intervalls        ( ostream*, const Sql_key_intervalle&  );
    void                        print_intervalls        ( ostream* );


    Bool                        get_single_key_interval ();
    void                        get_seq                 ( const Const_area* until_key );
    void                        get                     ();

    void                        get_key                 ();
    void                        set_key                 ();
    void                        set_key_intervall       ();
    void                        to_next_key_intervall   ();
    Bool                        key_in_offset_intervall ( const Byte* key );
    void                        update                  ();
    void                        insert                  ();
    void                        del                     ();

    void                        prepare_record_buffer   ();
    void                        construct_record        ();
    void                        set_appended_null_flags ( Bool );
    void                        set_null                ();

    Sos_string                  simple_name             ();         // Liefert alias oder name

    void                       _obj_print               ( ostream* ) const;

    Fill_zero                  _zero_;
    Sossql_session*            _session;
    Sql_stmt*                  _stmt;

    int                        _index;                  // this == Sos_stmt::_table_array[ _index ]
    Sos_string                 _user_name;              // Eigentümer der Tabelle oder ""
    Sos_string                 _name;                   // Dateiname
    Bool                       _full_file_name;         // Tabellenname ist vollständiger Dateiname
    Sos_limited_text<32>       _alias;

    Sos_ptr<Record_type>       _table_type;             // Satzbeschreibung der Datei
    Sos_ptr<Record_type>       _selected_type;          // Nur die selektierten Felder (von allen Klauseln)
    Sos_ptr<Field_descr>       _table_field_descr;      // Der ganze Satz als ein Feld für table.*
    Byte*                      _base;                   // Zeiger auf den Satz
    Dynamic_area               _record;                 // Puffer für den Satz

    Field_descr*               _table_key_descr;        // Feldbeschreibung des Tabellenschlüssels
    Dynamic_area               _key;

    int                        _result_key_offset;      // Offset des Schlüssels dieser Tabelle im Schlüssel der Ergebnismenge. Sql_stmt::_table_array[ 0 ]->_result_key_offset == 0. (s.a. Sql_stmt::_result_key_descr)
  //Sos_simple_array<int>      _callers_key_table;      // Für Sql_select::get_key()

    Any_file::Open_mode        _open_mode;
    Any_file                   _file;                   // Datei, wenn nicht Systemtabelle
    int                        _key_pos;                // von _file
    int                        _key_len;                // von _file
    Bool                       _key_in_record;          // == _file.key_in_record()
    Bool                       _unique_key;             // == _file.key_length()  &&  !_file.spec()._key_specs._key_spec._duplicate

    Bool                       _read_position;          // Nur die Satzposition lesen
  //Bool                       _prepared;               // _file.prepare() gerufen
    Bool                       _opened;
    Bool                       _rewind;                 // _file.rewind() aufrufen!
    Bool                       _rewound;                // Um überflüssige rewind() zu vermeiden, nicht jeder Dateityp kennt rewind()
    Bool                       _system;                 // Systemtabelle (ohne _file)
    Sql_key_intervalle         _key_intervalle;
    int                        _ki_idx;                 // Key_intervall index für get_seq()
    Bool                       _set_key;                // Beim nächsten _file.get() neu positionieren!
    int                        _get_count;              // Statistik: Anzahl gelesener Sätze (rewind: setzt auf 0 zurück)
    int                        _set_count;              // Statistik: Anzahl Positionierungen
    int                        _get_key_count;          // Statistik: Anzahl getkeys

    // Join:
    Sql_outer_join             _outer_join;             // Fehlt ein Satz, dann NULL-Werte erzeugen
    Sql_expr_with_operands*    _build_expr_ptr;         // Zum Aufbau von _join_expr;
    Sos_static_ptr<Sql_expr>   _join_expr;
    Bool                       _join_expr_accepted;     // WHERE-Klausel für den Join erfüllt? (pro fetch())
    Bool                       _record_read;            // Für Outer Join: Bei false NULL-Werte erzeugen
    Bool                       _record_accepted;        // Für Outer Join: Ein Satz durch die WHERE-Klausel gekommen
    Bool                       _null_values_for_outer_join_read;  // Null-Satz wird gerade eingefügt (nach EOF) ==> Alles NULL
    Sql_table_set              _master_tables;
    Bool                       _key_join;               // Join auf den Schlüssel
    Bool                       _equijoin;               // Join mit = (nur, wenn _key_join)


  //int                        _appended_null_flags_offset;    // Null-Flags für outer join
  //int                        _index;                  // in Sossql_static::_tables oder -1
  //Sos_simple_array<int>      _callers_key_table;      // callers field index -> table field index
  //Sos_simple_array<Field_descr*>      _callers_record_table;   // callers field index -> table field index
};

//-------------------------------------------------------------------------------------Sql_expr
// Basisklasse für die verschiedenen Teilausdrücke

struct Sql_expr : Sos_self_deleting
{
    BASE_CLASS( Sos_self_deleting )

                                Sql_expr                ( Sql_operator o ) : _operator( o ), _prepared(false), _null(false) {}

    bool                        operator ==             ( const Sql_expr& e ) const             { return is_equal(e); }
    virtual bool                is_equal                ( const Sql_expr& ) const;

    static int                  priority                ( Sql_operator op )                     { return sql_op_priority[ op ]; }   // priority( op_or ) < priority( op_mult )
    int                         priority                () const                                { return priority( _operator ); }

    void                        print                   ( ostream*, Sql_operator parent_op ) const;
    void                       _obj_print               ( ostream* ) const;
    Bool                       _obj_is_type             ( Sos_type_code t ) const               { return t == tc_Sql_expr || Base_class::_obj_is_type( t ); }


    Sql_operator               _operator;
    Source_pos                 _pos;                    // Für Fehlermeldung
    Bool                       _prepared;
    Sql_table_set              _used_tables;
    Sos_ptr<Field_type>        _type;

    Bool                       _all;                    // op_lt,op_le,op_eq,op_ne,op_ge,op_gt: Bedingung muss für alle erfüllt sein?
    Bool                       _any;                    // op_lt,op_le,op_eq,op_ne,op_ge,op_gt: Bedingung muss für einen erfüllt sein?
                                                        // _all || _any ==> 2. Operand ist eine Liste!

    // Errechneter Wert dieses Knotens:
  //Bool                       _save_value;             // Wert merken?
  //int                        _value_stamp;            // _value ist gültig, wenn _value_status == Sql_stmt::_stamp
    Bool                       _computed;               // Wert dieses Knotens ist bereits berechnet
    Bool                       _null;                   // == _value.null()
    Dyn_obj                    _value;
    Dynamic_area               _text;                   // Ergebnis von _value.write_text()
};

void throw_xc_expr( const char* error, const Sql_expr* );

//-------------------------------------------------------------------------------Sql_const_expr

struct Sql_const_expr : Sql_expr
{
                                Sql_const_expr          () : Sql_expr( op_const ) {}

    static Sos_ptr<Sql_const_expr> create               ();

    virtual bool                is_equal                ( const Sql_expr& ) const;
    void                       _obj_print               ( ostream* ) const;
};

//-------------------------------------------------------------------------------Sql_field_expr

struct Sql_field_expr : Sql_expr
{
                                Sql_field_expr          ();
                               ~Sql_field_expr          ();

    virtual bool                is_equal                ( const Sql_expr& e ) const;

    Bool                        is_primary_key_field    () const          { return _table->is_primary_key_field( this ); }
    Bool                        is_secondary_key_field  () const          { return _table->is_secondary_key_field( this ); }
    Bool                        is_array_elem           () const          { return _index_array.count() > 0; }

    void                       _obj_print               ( ostream* ) const;

    Sos_string                 _table_name;
    Sos_string                 _name;
    Sos_ptr<Sql_table>         _table;
    int                        _table_index;            // NACH order_table() UNGÜLTIG! _table_index > Sql_stmt::_table_array.last_index() ==> Tabelle von _outer_stmt
    Sos_ptr<Field_descr>       _field_descr;
    Sos_simple_array< Sos_ptr<Sql_expr> > _index_array; // Für Array-Indices
    Sql_outer_join             _outer_join;
    Bool                       _resolve_star;           // tabelle.* auflösen 
};

//-------------------------------------------------------------------------------Sql_rowid_expr

struct Sql_rowid_expr : Sql_expr
{
                                Sql_rowid_expr          () : Sql_expr(op_rowid) {}
//                             ~Sql_rowid_expr          ();

    virtual bool                is_equal                ( const Sql_expr& ) const;
    void                       _obj_print               ( ostream* ) const;

    Sos_string                 _table_name;
    Sos_ptr<Sql_table>         _table;
  //Sos_ptr<Field_type>        _type;
};

//-------------------------------------------------------------------------------Sql_param_expr

struct Sql_param_expr : Sql_expr                        // Parameter "?"
{
                                Sql_param_expr          () : Sql_expr( op_param ) {}

    virtual bool                is_equal                ( const Sql_expr& ) const;
    void                       _obj_print               ( ostream* ) const;

    Sos_ptr<Sql_table>         _table;                  // == Sql_stmt::_param_table
    int                        _index;                  // 0...n-1
};

//------------------------------------------------------------------------------Sql_select_expr

struct Sql_select_expr;         // Hinter Sql_select definiert.

//-----------------------------------------------------------------------Sql_expr_with_operands
// Teilausdrücke mit Operanden

struct Sql_expr_with_operands : Sql_expr
{
                                Sql_expr_with_operands  ( Sql_operator );
                               ~Sql_expr_with_operands  ();

    virtual bool                is_equal                ( const Sql_expr& e ) const;
    bool                       _obj_is_type             ( Sos_type_code t ) const   { return t == tc_Sql_expr_with_operands || Sql_expr::_obj_is_type( t ); }    
    void                       _obj_print               ( ostream* ) const;

    Sos_simple_array< Sos_ptr<Sql_expr> > _operand_array;
};

//-------------------------------------------------------------------------------Sql_regex_expr

struct Sql_regex_expr : Sql_expr_with_operands
{
                                Sql_regex_expr          () : Sql_expr_with_operands( op_regex_match ) {}

    static Sos_ptr<Sql_const_expr> create               ();

    string                     _pattern;
    zschimmer::Regex           _regex;                  // Für Operator op_regex_match (=~)
};

//----------------------------------------------------------------------------------Sql_in_expr
/*
struct Sql_in_expr : Sql_expr                           // {IN,<,<=,=,<>,>,>=} {ALL,ANY} ( ... )
{
                                Sql_select_expr         () : Sql_expr( op_in ) {}   // Auch op_in_select

    void                       _obj_print               ( ostream* ) const;

    Sql_operator               _in_operator;            // op_lt, op_le, op_eq, op_ne, op_ge, op_gt
    Bool                       _all;                    // Bedingung muss für alle erfüllt sein?
};
*/
//--------------------------------------------------------------------------------Sql_func_expr

struct Sql_func_expr : Sql_expr_with_operands
{
                                Sql_func_expr           ( Sql_operator );
                               ~Sql_func_expr           ();

    virtual bool                is_equal                ( const Sql_expr& ) const;
    void                       _obj_print               ( ostream* ) const;

    Fill_zero                  _zero_;
    Sos_string                 _name;                   // Name der Funktion oder Methode

    Sos_ptr<Sos_function_descr> _func_descr;            // Funktion ohne Objekt (kein OLE)

    Sos_string                 _object_name;            // Objektname (OLE) oder ""

#   if defined SOSSQL_OLE
        Ole_object*            _object;                 // OLE-Objekt (s.a. Sql_stmt)
        Ole_method*            _method;                 // Methode des Objekts
#   endif
};

//---------------------------------------------------------------------------Sql_aggregate_expr

struct Sql_aggregate_expr : Sql_expr_with_operands
{
                                Sql_aggregate_expr      ( Sql_operator op ) : Sql_expr_with_operands( op ), _zero_(this+1) { _aggregate_field_index = -1; }
                             //~Sql_aggregate_expr      ();

    virtual bool                is_equal                ( const Sql_expr& ) const;
    void                       _obj_print               ( ostream* ) const;

    Fill_zero                  _zero_;
    bool                       _distinct;
    int                        _aggregate_field_index;  // Index für Sql_aggregate::_aggregate_record_type[]
    int                        _groupby_index;          // Nur für Groupby-Ausdruck
};

//------------------------------------------------------------------Sql_func_group_counter_expr

struct Sql_func_group_counter_expr : Sql_expr_with_operands
{
                                Sql_func_group_counter_expr() : Sql_expr_with_operands( op_func_group_counter ), _counter(0) { _last_value_array.obj_const_name( "Sql_func_group_counter_expr::_last_value_array" ); }

    virtual bool                is_equal                ( const Sql_expr& ) const;
  //void                       _obj_print               ( ostream* ) const;

    Sos_simple_array<Dyn_obj>  _last_value_array;
    long                       _counter;
};

//------------------------------------------------------------------------------Sql_loop_clause
// Die LOOP-Klausel läßt eine Variable aus der Systemtabelle zwischen zwei Werten iterieren.

struct Sql_loop_clause : Sos_self_deleting
{
                                Sql_loop_clause         ()  : _zero_(this+1) { _value_list.obj_const_name( "Sql_loop_clause::_value_list" ); }

    Fill_zero                  _zero_;
    Sos_string                 _name;                   // Laufvariable  LOOP _name
    Sos_ptr<Sql_expr>          _begin;                  // Anfang        BETWEEEN _from
    Sos_ptr<Sql_expr>          _end;                    // Ende          AND _to
    Sos_simple_array< Sos_ptr<Sql_expr> > _value_list;            // Einzelne werte  IN (_values[0],...)
    int                        _result_key_offset;
};

//----------------------------------------------------------------------------Sql_system_record
// Der Satz aus der Systemtabelle

struct Sql_system_record
{
                                Sql_system_record       () : _zero_(this+1) {}

    Fill_zero                  _zero_;
    long                       _sys_loop_index;         // für LOOP-Klausel
    long                       _sys_loop_end;
    Sos_date                   _sys_date;               // Tagesdatum
    long                       _sys_record_no;          // Nummer des Satzes in der Ergebnismenge
                                                        // select .. where sys_record_no <= 3 liefert die ersten drei Sätze
                                                        // Wird vor LOOP und ORDER BY berechnet
    Sos_limited_text<100>      _user;
    Sos_limited_text<2>        _sys_newline;            // "\n" oder "\r\n" (Windows)
    Sos_limited_text<1>        _sys_cr;                 // "\r" (für MS Word)

    Field_descr*               _sys_loop_index_field;

    Sos_ptr<Record_type>        make_type               ();
};

//-------------------------------------------------------------------------------------Sql_stmt
// Basisklasse für eine SQL-Anweisung

struct Sql_stmt : Sos_self_deleting
{
                                Sql_stmt                ();
                               ~Sql_stmt                ();

  //static Sos_ptr<Sql_stmt>    create                  ( istream*,         // Erzeugt ein Sql_stmt
  //                                                      const Sos_string& empty_join_log,
  //                                                      const Source_pos& = std_source_pos );
    static Sos_ptr<Sql_stmt>    create                  ( const string& statement,
                                                          const string& empty_join_log,
                                                          const Source_pos& = std_source_pos );

    virtual void                prepare                 ();
    virtual void                execute                 ();
    virtual void                close                   ();
    void                        bind_parameters         ( const Record_type*, const Byte* );
    void                        start_next_table_fetch  ();
    void                        print_table_keys        ( ostream*, const Sql_table_set& );
    void                        insert_table_keys       ( Xc* );

    void                        put_empty_join_log      ( Sql_table* );
    void                        put_assert_error_log    ();
    void                        fetch                   ();    // LOOP
    void                        fetch2                  ();    // JOIN
    void                        fetch3                  ();    // Tabelle

    void                        order_tables            ();
    void                        print_access_plan       ( ostream* );
    void                        prepare_loop            ( const char* name, Sql_loop_clause* );
    void                        prepare_tables          ();
    void                        prepare_let_record      ();
    void                        prepare_system_record   ();
    void                        prepare_param_record    ();
    int                         table_index             ( const Sos_string& table_name );  // -1: Keine Tabelle
    void                        get_field_descr         ( const Sos_string& field_name, const Sos_string& table_name,
                                                          Field_descr**, Sql_table**, int*,
                                                          Bool* error,
                                                          const Source_pos&  = std_source_pos );
    void                        prepare_field_expr      ( Sql_field_expr*, Sql_join_expr_nesting );
    void                        prepare_join_expr       ( Sql_expr_with_operands* );
    void                        prepare_expr_with_operands( Sql_expr_with_operands*, Sql_join_expr_nesting );
    void                        prepare_expr            ( Sql_expr*, Sql_join_expr_nesting = je_other );

    void                        open_tables             ();                 // wird von execute() gerufen
    void                        eval_bool_expr          ( Bool* result, Bool* null, Sql_expr* );
    void                        put_false_cond_log      ( Sql_expr_with_operands* );
    void                        log_error               ( const Xc& );            // throw x if !_error_log.opened()
    void                        eval_negate             ( Dyn_obj*, Sql_expr_with_operands* );
    void                        eval_add                ( Dyn_obj*, Sql_expr_with_operands* );
    void                        eval_concat             ( Dyn_obj*, Sql_expr_with_operands* );
    void                        eval_field              ( Dyn_obj*, Sql_field_expr* );
    void                        eval_rowid              ( Dyn_obj*, Sql_rowid_expr* );
    void                        eval_param              ( Dyn_obj*, Sql_param_expr* );
    void                        eval_select             ( Dyn_obj*, Sql_select_expr* );
    void                        eval_cond               ( Dyn_obj*, Sql_expr_with_operands* );
    void                        eval_func_decode        ( Dyn_obj*, Sql_expr_with_operands* );
    void                        eval_func_field         ( Dyn_obj*, Sql_expr_with_operands* );
    void                        eval_func_group_counter ( Dyn_obj*, Sql_func_group_counter_expr* );
    void                        eval_function           ( Dyn_obj*, Sql_func_expr* );
    void                        eval_expr               ( Dyn_obj*, Sql_expr* );
    void                        eval_expr               ( Dyn_obj**, Sql_expr* );
    Dyn_obj                     eval_expr               ( Sql_expr* );
    int                         eval_expr_as_int        ( Sql_expr* );
    long                        eval_expr_as_long       ( Sql_expr* );
    Bool                        eval_expr_as_bool       ( Sql_expr*, Bool deflt );
    Sos_string                  eval_expr_as_string     ( Sql_expr* );

    Fill_zero                               _zero_;
    string                                  _statement;     // Zum Debuggen
    Sossql_session*                         _session;
    
    int                                     _max_reads;
    int                                     _read_count;

    Sos_simple_array< Sos_ptr<Sql_table> >  _table_array;    // Dateien und Systemtabellen
    int                                     _table_count;    // 0.._table_count-1 sind Dateien (SELECT FROM, INSERT INTO, UDPATE, DELETE FROM)
                                                             // Ab _table_count sind Sondertabellen (_system_table und _param_table).
    Sos_simple_array< int >                 _table_order;
    Sql_table*                               ordered_table        ( int i )  { return _table_array[ _table_order[ i ] ]; }
    Bool                                    _constant_select_fetched;  // für SELECT ohne FROM
    int                                     _fetch_table_index;   // Diese Tabelle wird gerade gelesen (join), für ordered_table()!!

    Sos_ptr<Sql_table>                      _system_table;        // Die Tabelle SYSTEM (Sql_system_record)
    Sql_system_record*                      _system;              // Der Satz der Systemtabelle

    Sos_simple_array<Sos_string>            _let_names;
    Sos_ptr<Sql_table>                      _let_table;           // Die Tabelle LET
    Sos_simple_ptr_array<Sql_expr>          _let_expr_array;      // Ausdrücke

    int                                     _param_count;         // Anzahl der ?-Parameter
    Sos_ptr<Sql_table>                      _param_table;         // Tabelle mit dem Parametersatz
    Sos_ptr<Record_type>                    _param_callers_type;  // für Sql_select::execute()
    const Byte*                             _param_callers_base;  // für execute()

    Sos_ptr<Sql_loop_clause>                _loop;                // LOOP-Klausel
    Bool                                    _loop_before_where;   // LOOP vor WHERE ausführen!
    Sos_ptr<Sql_expr>                       _where;               // WHERE-Klausel
    Sos_ptr<Sql_expr>                       _assert;              // ASSERT-Klausel
    Sql_expr*                               _last_not_true_expr;  // Für Fehlermeldung bei nicht erfüllter WHERE- oder ASSERT-Klausel
    int4                                    _row_count;           // Anzahl verarbeiteter Sätze
    Dynamic_area                            _hilfspuffer;         // Für Konvertierungen
    Bool                                    _log_eval;            // Auswertung der Ausdrücke protokollieren
    Bool                                    _xc_expr_inserted;    // Beim Fehler in eval_expr() Sql_expr bereits der Exception hinzugefügt?
    Bool                                    _expr_error;          // Fehler bei eval_expr() oder eval_bool_expr() aufgetreten
    int                                     _rowid_len;           // 0: Keine ROWID
    Dynamic_area                            _rowid;               // ROWIDs aller Tabellen hintereinander
    Any_file                                _error_log_file;
    Any_file_stream                         _error_log;
    Bool                                    _error_log_opened;
    Sql_stmt*                               _outer_stmt;          // Verschachteltes Select: Zeiger auf die Umgebung
    int                                     _stamp;               // Stempel für Sql_expr::_value_stamp
    uint                                    _max_field_length;
    Bool                                    _using_func_field;    // field()-Funktion wird verwendet, Sql_table soll alle Felder lesen (==> _read_position = false)
    Bool                                    _order_tables;        // Zugriffsreihenfolge der Tabellen entsprechend der Joins ordnen (default)
  //bool                                    _with_aggregates;
#   if defined SOSSQL_OLE
        Sos_simple_ptr_array< Ole_object >  _object_array;
#   endif
};

DEFINE_SOS_DELETE_ARRAY( Sql_table* )

struct Sql_select;
//--------------------------------------------------------------------------Sql_orderby_clause

struct Sql_orderby_clause
{
                                            Sql_orderby_clause( Sql_select* s ) : _zero_(this+1), _select(s) { _expr_array.obj_const_name( "Sql_orderby_clause::_expr_array" ); _index_array.obj_const_name( "Sql_orderby_clause::_index_array" );}

    Fill_zero                               _zero_;
    Sql_select*                             _select;
    Sos_simple_array< Sos_ptr<Sql_expr> >   _expr_array;
    Sos_simple_array< int >                 _index_array;       // Indices in select->_result_expr_array[]
    int                                     _first;             // Erster Index (optimiert)
    Sos_ptr<Record_type>                    _type;
    Sos_simple_array<void*>                 _records;           // Für qsort(): Die Elemente müssen zusammenhängend im Speicher liegen!
    Bool                                    _sorted;
    int1 /*Bool*/                           _descending_array[ max_orderby_params ];
    Field_descr*                            _field_array[ max_orderby_params ];
    int                                     _record_no;         // Nächster zu lesender sortierter Satz

    static int _USERENTRY                    compare            ( const void*, const void* );
};

//--------------------------------------------------------------------------Sql_group_by_clause
/*
struct Sql_group_by_clause
{
                                            Sql_group_by_clause( Sql_select* s ) : _zero_(this+1), _select(s) { _expr_array.obj_const_name( "Sql_group_by_clause::_expr_array" ); }

    Fill_zero                               _zero_;
    Sql_select*                             _select;
};
*/
//--------------------------------------------------------------------------------Sql_aggregate

struct Sql_aggregate
{
                                Sql_aggregate           ( Sql_select* s )  : _zero_(this+1),_select(s) {}
                               ~Sql_aggregate           ()                      { close(); }

    void                        clear                   ();
    void                        close                   ()                      { clear(); }
    void                        prepare                 ();
    void                        execute                 ();
    void                        fetch_result            ();

    void                        identify_groupby_exprs  ( Sos_simple_array< Sos_ptr<Sql_expr> >* );
    void                        identify_groupby_exprs  ( Sos_ptr<Sql_expr>* );
    int                         groupby_expr_index      ( Sql_expr* );

    void                        prepare_aggregate_functions( Sql_expr* );

    void                        process_record          ();
    void                        finish_aggregates       ( Byte* record_ptr );
    void                        eval_aggregate_functions( Byte* aggregate_record );
    void                        eval_aggregate_function ( Byte* aggregate_record, Sql_aggregate_expr* );


    Fill_zero                               _zero_;
    Sql_select*                             _select;
    Sos_ptr<Record_type>                    _groupby_type;
    Sos_simple_array< Sos_ptr<Sql_expr> >   _groupby_expr_array;
    Dynamic_area                            _groupby_record;
    typedef vector<Sql_aggregate_expr*>      Aggregate_array;
    Aggregate_array                         _aggregate_array;
    Sos_ptr<Record_type>                    _aggregate_record_type;
    const Byte*                             _current_groupby_record;
    const Byte*                             _current_aggregate_record;
    Sos_ptr<Sql_expr>                       _having;                    // HAVING-Klausel
    
    typedef std::map<string,Byte*>           Result_map;                // ( Gruppenschüssel, Datensatz )
    Result_map                              _result_map;
    Result_map::iterator                    _result_iterator;
};

//-----------------------------------------------------------------------------------Sql_select

struct Sql_select : Sql_stmt
{
    // nicht thread-safe wegen static qsort_select!
    BASE_CLASS( Sql_stmt )

                                Sql_select              ();
                               ~Sql_select              ();

    void                        prepare                 ();
    void                        execute                 ();
    void                        close                   ();
    void                        fetch                   ( Area* );
    void                        fetch2                  ( Area* );
    void                        fetch_table_records     ();
    void                        get_position            ( Area* );
    const Const_area&           current_key             ();
    void                        rewind                  ();
    void                        get_key                 ( Area*, const Const_area& );
    void                        set_key                 ( const Const_area& );
    void                        update                  ( const Const_area& );
    void                        insert                  ( const Const_area& );
    void                        del                     ();

    void                        prepare_select_star     ();
    void                        prepare_expr_array      ( Sos_ptr<Record_type>*,
                                                          const Sos_simple_array< Sos_ptr<Sql_expr> >&,
                                                          const char* name_prefix );

    bool                        has_aggregate_function  ( Sos_simple_array< Sos_ptr<Sql_expr> >& );
    bool                        has_aggregate_function  ( Sql_expr* );
    void                        check_for_fields        ( Sos_simple_array< Sos_ptr<Sql_expr> >& );
    void                        check_for_fields        ( Sql_expr* );

  //void                        build_result_key_type   ();
    void                        set_result_key_len      ();
    void                        assign_expr_array       ( const Record_type& record_type,
                                                          Area* record,
                                                          const Sos_simple_array< Sos_ptr<Sql_expr> >& );
    void                        fetch_all_and_sort      ();
    void                        delete_sorted_records   ();
    void                        fetch_sorted            ( Area* );

    void                        assign_callers_key      ( Sql_table*, const Const_area& );
    void                        assign_callers_record   ( const Const_area& );
    void                        check_where_for_writing ();


    Bool                       _obj_is_type             ( Sos_type_code t ) const               { return t == tc_Sql_select || Base_class::_obj_is_type( t ); }

    static Sql_select*                      qsort_select;        // Für Aufruf von qsort()

    Fill_zero                               _zero_;
    Sossql_file*                            _sossql_file;

    Sos_simple_array< Sos_ptr<Sql_expr> >   _result_expr_array;
    Sos_simple_array<Sos_string>            _result_name_array;
    Sos_ptr<Record_type>                    _result_record_type;
  //Sos_ptr<Field_descr>                    _result_key_descr;
    int                                     _key_pos;                   // Immer 0
    int                                     _key_len;                   // Schlüssellänge der Ergebnismenge, wenn alle Schlüsselfelder korrekt selektiert sind.
  //Sos_ptr<Sql_loop_clause>                _loop_2;                    // LOOP nach WHERE (innere Schleife)
    Sql_orderby_clause                      _orderby;

    Bool                                    _need_result_key;
    Dynamic_area                            _last_record;               // zuletzt gelesener Satz für select distinct
    Bool                                    _last_record_valid;
    Bool                                    _distinct;                  // select distinct ...; Wird auch bei verschachteltem Select gesetzt: expr IN ( SELECT xxx )  => expr IN ( SELECT DISTINCT xxx )
    Bool                                    _update_allowed;
  //Bool                                    _select_star;               // select *
    uint                                    _max_result_field_length;
    Bool                                    _resolve_star;              // tabelle.* kommt vor und soll aufgelöst werden (s. Sql_field_expr)
    bool                                    _has_aggregate_function;
    Sql_aggregate                           _aggregate;           // Nur für select!
};

//------------------------------------------------------------------------------Sql_select_expr

struct Sql_select_expr : Sql_expr                        // ( SELECT ... )
{
                                Sql_select_expr         () : Sql_expr( op_select ) {}

    virtual bool                is_equal                ( const Sql_expr& ) const;
    void                       _obj_print               ( ostream* ) const;

    Sql_select                 _select;
};

//-------------------------------------------------------------------------------Sql_write_stmt

struct Sql_write_stmt : Sql_stmt    // Für INSERT, UPDATE und DELETE
{
    BASE_CLASS( Sql_stmt )

                                Sql_write_stmt          ();

    void                        prepare                 ();
    void                        assign_fields           ();

    Sos_simple_array< Sos_string >        _field_name_array;
    Sos_simple_array< Sos_ptr<Sql_expr> > _values;
    Record_type                           _record_type;          // nach prepare()
};

//-----------------------------------------------------------------------------------Sql_insert

struct Sql_insert : Sql_write_stmt
{
    BASE_CLASS( Sql_write_stmt )

                                Sql_insert              ();
                               ~Sql_insert              ();

    void                        prepare                 ();
    void                        execute                 ();

    Sos_ptr<Sql_select>        _select;                 // SELECT-Klausel
};

//-----------------------------------------------------------------------------------Sql_update

struct Sql_update : Sql_write_stmt
{
    BASE_CLASS( Sql_write_stmt )

                                Sql_update              ();
                               ~Sql_update              ();

    void                        prepare                 ();
    void                        execute                 ();
};

//-----------------------------------------------------------------------------------Sql_delete

struct Sql_delete : Sql_stmt
{
    BASE_CLASS( Sql_stmt )

                                Sql_delete              ();
                               ~Sql_delete              ();

    void                        execute                 ();
};


//-------------------------------------------------------------------------------Sossql_session

struct Sossql_session : Sos_database_session
{
    BASE_CLASS( Sos_database_session )

  //                            Sossql_session          ();
  //                           ~Sossql_session          ();

    void                       _open                    ( Sos_database_file* );
    void                       _close                   ( Close_mode = close_normal );
    void                       _execute_direct          ( const Const_area& );
  //void                       _commit                  ();
    void                       _rollback                ();
    Bool                       _equal_session           ( Sos_database_file* );
    string                      translate_limit         ( const string& stmt, int limit );

    void                       _obj_print               ( ostream* s ) const                { *s << "sossql "; Sos_database_session::_obj_print(s); }

//private:
    friend struct               Sossql_file;

  //Fill_zero                  _zero_;
    Sos_string                 _catalog_name;
    Sos_string                 _filename_prefix;        // Präfix für alle Tabelen-Dateinamen
    Any_file                   _catalog;
    int                        _limit;
};

//----------------------------------------------------------------------------------Sossql_file

struct Sossql_file : Sos_database_file
{
    BASE_CLASS( Sos_database_file )

                                Sossql_file             ();
                               ~Sossql_file             ();

    void                        prepare_open            ( const char*, Open_mode, const File_spec& );
    void                        bind_parameters         ( const Record_type*, const Byte* );
    void                        open                    ( const char*, Open_mode, const File_spec& );
    void                        close                   ( Close_mode = close_normal );
    void                        execute                 ();

  protected:
    virtual void               _create_static           ();

    virtual Sos_ptr<Sos_database_session> 
                               _create_session          ();

    Sossql_session*             session                 ()                                      { return (Sossql_session*) +_session; }

    virtual void                get_record              ( Area& );
    void                        get_position            ( Area* );
    void                        rewind                  ( Key::Number );
    void                        get_record_key          ( Area&, const Key& );
    void                        set                     ( const Key& );
    void                        update                  ( const Const_area& );
    void                        insert                  ( const Const_area& );
    void                        del                     ();
    const Const_area&           current_key             ();

  private:
    friend                      struct Sossql_session;

    Fill_zero                  _zero_;
    Sos_string                 _catalog_name;           // Für Sossql_session
    Sos_ptr<Sql_stmt>          _stmt;
    Sql_select*                _select_stmt;            // == (Select_stmt*)_stmt oder NULL
    Any_file                   _file;                   // Für -catalog
    int                        _limit;                  // Liefert Eof_exception
    int                        _max_records;            // Liefert Fehler
    int                        _record_count;
};

//-----------------------------------------------------------------------------------Sql_parser

struct Sql_parser
{
    enum Kind
    {
        k__first = 0,
        k_none = 0,

        k_eof = 1,
        k_comma,
        k_semikolon,
        k_colon,
        k_punkt,
        k_klammer_auf,
        k_klammer_zu,
        k_eckklammer_auf,
        k_eckklammer_zu,
        k_star,
        k_slash,
        k_plus,
        k_minus,
        k_lt,
        k_le,
        k_eq,
        k_ne,
        k_ge,
        k_gt,
        k_star_eq,
        k_eq_star,
        k_colon_eq,
        k_eq_colon,
        k_regex_match,
        k_plus_in_klammern,
        k_ausr_in_klammern,
        k_question_mark,
        k_doppelstrich,
        k_pipe,

        k_identifier,
        k_string,
        k_number,
        k_decimal,
        k_float,

                            // Schlüsselwörter nach Wahrscheinlichkeit geordnet:
        k__keyword_first,
        k_like    = k__keyword_first,
        k_and,
        k_or,
        k_not,
        k_as,
        k_in,
        k_is,
        k_null,
        k_between,
        k_all,
        k_any,

        k_select,
        k_distinct,
        k_from,
        k_loop,
        k_where,
        k_exists,
        k_assert,
        k_order,
        k_group,
        k_by,
        k_having,
        k_asc,
        k_desc,
        k_insert,
        k_into,
        k_values,
        k_update,
        k_set,
        k_let,
        k_delete,
        k_avg,
        k_count,
        k_max,
        k_min,
      //k_stddev
        k_sum,
      //k_variance,

        k_rowid,

        k_createobject,

        k__keyword_last,

        k__last = k__keyword_last
    };

    static const char*          name_of                 ( Kind );

    struct Token
    {
        static void             init                    ();

                                Token                   ();

        Kind                    kind                    () const  { return _kind; }
        const Sos_string&       name                    () const  { return _name; }
        Big_int                 number                  () const  { return _number; }

        Source_pos             _pos;

        Kind                   _kind;
        Sos_string             _name;
        Big_int                _number;
        double                 _float;
        Decimal                _decimal;

        static const char*      repr                    ( Kind k )          { return _token_array[ k ]; }
        //static Sos_simple_array<Sos_string> _token_array;
        static const char*      _token_array [ k__last ];
    };

                                Sql_parser              ( istream*, const Source_pos& = Source_pos() );
                               ~Sql_parser              ();

    void                        check_token             ( Kind );

    Bool                        next_token_is           ( Kind k )          { _expected_tokens_set.include( (int)k ); return  next_token().kind() == k; }
    const Token&                next_token              ()                  { return _token; }
    void                        read_token              ();
    void                        parse                   ( Kind k )          { check_token( k ); parse_token(); }
    void                        parse_token             ()                  { read_token(); }
    Sos_string                  parse_rest              ();
    Big_int                     parse_number            ();                 // >= 0
    Sos_string                  parse_identifier        ();
    Sos_string                  parse_string            ();
    Sos_string                  parse_identifier_or_string();
    uint4                       parse_size              ();
    Sos_ptr<Field_type>         parse_field_type        ();
    Sos_ptr<Field_descr>        parse_field_decl     ();
    uint4                       parse_at_clause         ();
    Sos_ptr<Dyn_record_type>    parse_record_decl       ();
    void                        parse                   ();

    void                        parse_parameter_list    ( Sql_expr_with_operands*, int min_par = 0, int max_par = -1 );
    void                        parse_field_index       ( Sql_field_expr* );
    Sos_ptr<Sql_expr>           parse_select_expr       ();
    Sos_ptr<Sql_expr>           parse_value_expr        ();
    Sos_ptr<Sql_expr>           parse_const_expr        ();
    Sos_ptr<Sql_expr>           parse_mult_expr         ();
    Sos_ptr<Sql_expr>           parse_add_expr          ();
    Sos_ptr<Sql_expr>           parse_arith_expr        ()    { return parse_add_expr(); }  // für BETWEEN x AND y
    Sos_ptr<Sql_expr>           parse_expr              ()    { return parse_cond_expr(); }
    Sos_ptr<Sql_expr>           parse_cmp_expr          ();
    Sos_ptr<Sql_expr>           parse_and_expr          ();
    Sos_ptr<Sql_expr>           parse_or_expr           ();
    Sos_ptr<Sql_expr>           parse_cond_expr         ();
    Sos_ptr<Sql_expr>           parse_bool_expr         ()    { return parse_or_expr(); }
    Sos_ptr<Sql_loop_clause>    parse_loop_clause       ();
    Sos_ptr<Sql_table>          parse_table_name        ();

#   if defined SOSSQL_OLE
        Sos_ptr<Ole_object>     parse_set_stmt          ();
#   endif

    void                        parse_let               ( Sql_select* );
    void                        parse_select            ( Sql_select* );
    void                        parse_select_stmt       ( Sql_select*, Bool check_eof = false );
    void                        parse_insert_stmt       ( Sql_insert*, Bool check_eof = false );
    void                        parse_update_stmt       ( Sql_update*, Bool check_eof = false );
    void                        parse_delete_stmt       ( Sql_delete*, Bool check_eof = false );

    int                         get_char                ();

    Fill_zero                  _zero_;
    int                        _next_char;
    istream*                   _input;
    Source_pos                 _pos;
    Token                      _token;
    int                        _offset;             // für create table
    int                        _param_count;
    Bit_set                    _expected_tokens_set;
    Bool                       _need_error_log;
    Dynamic_area               _hilfspuffer;
};


//---------------------------------------------------------------------------------------------

//extern Dynamic_area sql_low_value;
//extern Dynamic_area sql_high_value;
extern const Const_area sql_low_value;
extern const Const_area sql_high_value;

extern Text_format  sql_format;


} //namespace sos

#endif

