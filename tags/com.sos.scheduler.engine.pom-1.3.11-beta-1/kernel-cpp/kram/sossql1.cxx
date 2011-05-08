// $Id$

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif
#include "precomp.h"

//#define MODULE_NAME "sossql1"
//#define COPYRIGHT   "©1996 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/*
    Implementierte Klassen:

    Sql_expr...
    Sql_orderby_clause
    Sql_select
    Sql_write_stmt
    Sql_insert
    Sql_update
    Sql_delete


    Mängel:

    -   Sql_select::build_result_record_type() nimmt für ein berechnetes Feld
        den Typ Text_type(max_sossql_field_size) an.

*/

#include <stdlib.h>         // qsort()
#include "../zschimmer/regex_class.h"
#include "sosstrng.h"
#include "sos.h"
#include "sysxcept.h"
#include "log.h"
#include "sosarray.h"
#include "stdfield.h"
#include "soslimtx.h"
#include "sossql2.h"
#include "thread_semaphore.h"
#include "../file/anyfile.h"

using namespace std;
namespace sos {


Sql_select*                 Sql_select::qsort_select = 0;       // Sollte im TLS liegen
static Thread_semaphore     lock ( "sossql_qsort" );

const char* sql_op_text     [ op_highest - 1 ];
int         sql_op_priority [ op_highest - 1 ];

DEFINE_SOS_STATIC_PTR( Sql_expr )

SOS_INIT( sql_operator )
{
    for( int i = 0; i < NO_OF( sql_op_text ); i++ )  sql_op_text[ i ] = "-?-";

    sql_op_text[ op_add            ] = "+";          sql_op_priority[ op_add      ] = 6;
    sql_op_text[ op_and            ] = "AND";        sql_op_priority[ op_and      ] = 3;
    sql_op_text[ op_between        ] = "BETWEEN";    sql_op_priority[ op_between  ] = 5;
    sql_op_text[ op_concat         ] = "||";         sql_op_priority[ op_concat   ] = 6;
    sql_op_text[ op_cond           ] = "?:";         sql_op_priority[ op_cond     ] = 1;
    sql_op_text[ op_const          ] = "const";      sql_op_priority[ op_const    ] = 8;
    sql_op_text[ op_divide         ] = "/";          sql_op_priority[ op_divide   ] = 7;
    sql_op_text[ op_eq             ] = "=";          sql_op_priority[ op_eq       ] = 5;
    sql_op_text[ op_exists         ] = "EXISTS";     sql_op_priority[ op_exists   ] = 5;
    sql_op_text[ op_field          ] = "field";      sql_op_priority[ op_field    ] = 8;
    sql_op_text[ op_function       ] = "function";   sql_op_priority[ op_function ] = 8;
    sql_op_text[ op_ge             ] = ">=";         sql_op_priority[ op_ge       ] = 5;
    sql_op_text[ op_gt             ] = ">";          sql_op_priority[ op_gt       ] = 5;
    sql_op_text[ op_is_null        ] = "IS NULL";    sql_op_priority[ op_is_null  ] = 8;
    sql_op_text[ op_le             ] = "<=";         sql_op_priority[ op_le       ] = 5;
    sql_op_text[ op_like           ] = "LIKE";       sql_op_priority[ op_like     ] = 5;
    sql_op_text[ op_lt             ] = "<";          sql_op_priority[ op_lt       ] = 5;
    sql_op_text[ op_multiply       ] = "*";          sql_op_priority[ op_multiply ] = 7;
    sql_op_text[ op_negate         ] = "-";          sql_op_priority[ op_negate   ] = 8;
    sql_op_text[ op_ne             ] = "<>";         sql_op_priority[ op_ne       ] = 5;
    sql_op_text[ op_not            ] = "NOT";        sql_op_priority[ op_not      ] = 4;
    sql_op_text[ op_or             ] = "OR";         sql_op_priority[ op_or       ] = 2;
    sql_op_text[ op_param          ] = "param";      sql_op_priority[ op_param    ] = 8;
    sql_op_text[ op_regex_match    ] = "=~";         sql_op_priority[ op_regex_match] = 5;
    sql_op_text[ op_rowid          ] = "ROWID";      sql_op_priority[ op_rowid    ] = 8;
    sql_op_text[ op_select         ] = "SELECT";     sql_op_priority[ op_divide   ] = 8;
    sql_op_text[ op_subtract       ] = "-";          sql_op_priority[ op_subtract ] = 6;
}

//-------------------------------------------------------------------------------------sql_like
// Hängende Blanks bei einem Typ, der sie in write_text() abschneidet, können nicht
// berücksichtigt werden. Ist auch besser so.

Bool sql_like( const Dyn_obj& a, const Const_area& b_text, Area* hilfspuffer )
{
    if( !hilfspuffer )  return sql_like( a, b_text );

    //LOG( "sql_like( " << a << ", " << b << " )\n" );

    a.write_text( hilfspuffer );

    char*       ap     = hilfspuffer->char_ptr();
    char*       ap_end = ap + hilfspuffer->length();
    const char* bp     = b_text.char_ptr();
    const char* bp_end = bp + b_text.length();

    // Gleichheit (mit '_') bis zu dem nächsten '%' oder dem Ende
    while( ap < ap_end  &&  bp < bp_end  &&  *bp != '%'  &&  ( *bp == *ap  ||  *bp == '_' ) )  { bp++; ap++; }

    while(1)  // Für jedes %
    {
        if( bp == bp_end )  return ap == ap_end;              // Beide Strings am Ende?
        if( *bp != '%' )  return false;                       // Kein '%'?
        while( bp < bp_end  &&  *bp == '%' )  bp++;           // '%' überspringen

        if( bp == bp_end )  return true;                      // % am Ende passt auf alles

        const char* bp2 = (char*)memchr( bp, '%', bp_end - bp );  // Suche '%' in 'te_xt%'
        if( !bp2 )  bp2 = bp_end;

        // bp2 > bp !
        
        while(1) {                          // Schleife über ap: Zeichenkette bp...bp2 suchen:
            if( ap == ap_end )  return false;

            char*       ap1 = ap;
            const char* bp1 = bp;

            while( ap1 < ap_end  &&  bp1 < bp2  &&  ( *bp1 == *ap1  ||  *bp1 == '_' ) )  { bp1++; ap1++; }

            if( bp1 == bp2 ) {            // Teilstrings sind gleich
                ap = ap1;
                bp = bp1;
                break;
            }

            // Teilstring passt nicht!

            ap++;                         // Neuer Versuch ein Zeichen weiter (Wert für % um eins verlängern)
        }
    }
}

//-------------------------------------------------------------------------------------sql_like

Bool sql_like( const Dyn_obj& a, const Const_area& b_text )
{
    Dynamic_area hilfspuffer;
    Bool result = sql_like( a, b_text, &hilfspuffer );
    return result;
}

//-------------------------------------------------------------------------------------sql_like

Bool sql_like( const Dyn_obj& a, const Dyn_obj& b, Area* hilfspuffer )
{
    Dynamic_area b_text ( max_sossql_field_size+1 );
    b.write_text( &b_text );  //jz 18.3.97 Unsinn: b_text += '\0';
    Bool result = sql_like( a, b_text, hilfspuffer );
    //LOG( "sql_like(" << a << ',' << b << ") = " << result << '\n' );
    return result;
}

//------------------------------------------------------------------------------sql_regex_match
/*
bool sql_regex_match( const string& a, const string& pattern )
{
    // Das ist optimierbar. Wenn das Muster konstant ist, kann regex im Expr vorkompiliert werden.
    zschimmer::Regex regex;
    regex.compile( pattern );
    return regex.match( a );
}
*/
//---------------------------------------------------------------------------Sql_expr::is_equal

bool Sql_expr::is_equal( const Sql_expr& e ) const
{ 
    return _operator == e._operator  &&  _all == e._all  &&  _any == e._any; 
}

//------------------------------------------------------------------------------Sql_expr::print

void Sql_expr::print( ostream* s, Sql_operator parent_op ) const
{
    Bool klammern = priority() < priority( parent_op );
    if( klammern )  *s << "( ";
    *s << *this;
    if( klammern )  *s << ") ";
}

//-------------------------------------------------------------------------Sql_expr::_obj_print

void Sql_expr::_obj_print( ostream* s ) const
{
    *s << "(unknown expression)";
}

//-----------------------------------------------------------------------Sql_const_expr::create

Sos_ptr<Sql_const_expr> Sql_const_expr::create()
{
    return SOS_NEW( Sql_const_expr );
}

//---------------------------------------------------------------------Sql_const_expr::is_equal

bool Sql_const_expr::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr::is_equal(expr) )  return false;

    Sql_const_expr* e = (Sql_const_expr*)&expr;

    return _value == e->_value; 
}

//-------------------------------------------------------------------Sql_const_expr::_obj_print

void Sql_const_expr::_obj_print( ostream* s ) const
{
    //*s << _value;
    _value.print( s, '\'', '\'' );
}

//---------------------------------------------------------------Sql_field_expr::Sql_field_expr

Sql_field_expr::Sql_field_expr()
:
    Sql_expr( op_field ),
    _outer_join ( oj_none )
{
    _index_array.obj_const_name( "Sql_field_expr::_index_array" );
}

//--------------------------------------------------------------Sql_field_expr::~Sql_field_expr

Sql_field_expr::~Sql_field_expr()
{
}

//---------------------------------------------------------------------Sql_field_expr::is_equal

bool Sql_field_expr::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr::is_equal(expr) )  return false;

    Sql_field_expr* e = (Sql_field_expr*)&expr;

    return _table_name == e->_table_name  &&  _name == e->_name;
}

//-------------------------------------------------------------------Sql_field_expr::_obj_print

void Sql_field_expr::_obj_print( ostream* s ) const
{
    string table_name = _table? _table->simple_name() : _table_name;
    if( !table_name.empty() )  *s << table_name << '.';

    if( _field_descr )  *s << _field_descr->name();
                  else  *s << _name;

    if( _index_array.count() ) {
        *s << '[';
        for( int i = 0; i < _index_array.count(); i++ ) {
            if( i > 0 )  *s << ',';
            *s << *_index_array[ i ];
        }
        *s << ']';
    }

    if( _field_descr  &&  _table  &&  _table->_record_read )
    {
        //*s << " (aktueller Wert: "; //" «";
        *s << ":=";

        if( _table->_null_values_for_outer_join_read
         || _field_descr->null( _table->_base ) )
        {
            *s << "«NULL»";
        }
        else
        {
            _field_descr->print( _table->_base, s, std_text_format, '\'', '\'' );
        }

        //*s << ")";  //'»';
    }

    if( _outer_join )  *s << ( _outer_join & oj_log? " (!)" : " (+)" );   // wird von Sql_stmt::fetch2() unterdrückt!
}

//---------------------------------------------------------------------Sql_rowid_expr::is_equal

bool Sql_rowid_expr::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr::is_equal(expr) )  return false;

    Sql_rowid_expr* e = (Sql_rowid_expr*)&expr;
    
    return _table_name == e->_table_name; 
}

//-------------------------------------------------------------------Sql_rowid_expr::_obj_print

void Sql_rowid_expr::_obj_print( ostream* s ) const
{
    *s << _table->simple_name() << ".ROWID";
}

//---------------------------------------------------------------------Sql_param_expr::is_equal

bool Sql_param_expr::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr::is_equal(expr) )  return false;

    Sql_param_expr* e = (Sql_param_expr*)&expr;
    
    return _index == e->_index;
}

//-------------------------------------------------------------------Sql_param_expr::_obj_print

void Sql_param_expr::_obj_print( ostream* s ) const
{
    *s << '?' << ( _index + 1 );
}

//-----------------------------------------------Sql_expr_with_operands::Sql_expr_with_operands

Sql_expr_with_operands::Sql_expr_with_operands( Sql_operator op )
:
    Sql_expr( op )
{
    _operand_array.obj_const_name( "Sql_expr_with_operands::_operand_array" );
}

//----------------------------------------------Sql_expr_with_operands::~Sql_expr_with_operands

Sql_expr_with_operands::~Sql_expr_with_operands()
{
}

//-------------------------------------------------------------Sql_expr_with_operands::is_equal

bool Sql_expr_with_operands::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr::is_equal(expr) )  return false;

    Sql_expr_with_operands* e = (Sql_expr_with_operands*)&expr;

    if( _operand_array.count() != e->_operand_array.count() )  return false;

    for( int i = 0; i < _operand_array.count(); i++ )
    {
        if( !_operand_array[i]->is_equal( *e->_operand_array[i] ) )  return false;
    }

    return true;
}

//-----------------------------------------------------------Sql_expr_with_operands::_obj_print

void Sql_expr_with_operands::_obj_print( ostream* s ) const
{
    switch( _operator )
    {
        case op_lt:
        case op_le:
        case op_eq:
        case op_ne:
        case op_ge:
        case op_gt:
        case op_regex_match:
        {
            // Sql_table::_join_expr kann vorübergehen 0 Operanden haben
            _operand_array[ 0 ]->print( s, _operator );
            *s << ' ' << sql_op_text[ _operator ] << ' ';
            if( _any )  *s << "ANY ";
            if( _all )  *s << "ALL ";

            if( _operand_array.count() != 2 )  *s << '(';

            for( int i = 1; i <= _operand_array.last_index(); i++ )
            {
                if( i > 1 )  *s << ',';
                _operand_array[ i ]->print( s, _operator );
            }

            if( _operand_array.count() != 2 )  *s << ')';

            break;
        }

        case op_concat:
        case op_exists:
        case op_like:
        case op_and:
        case op_or:
        case op_add:
        case op_subtract:
        case op_multiply:
        case op_divide:
        case op_not:
        case op_is_null:
        {
            // Sql_table::_join_expr kann vorübergehend 0 Operanden haben
            // und op_not hat nur einen Operanden.
            if( _operator != op_is_null ) {
                if( _operand_array.count() <= 1 )  *s << sql_op_text[ _operator ] << ' ';
            }

            for( int i = 0; i <= _operand_array.last_index(); i++ )
            {
                if( i > 0  )  *s << ' ' << sql_op_text[ _operator ] << ' ';
                _operand_array[ i ]->print( s, _operator );
            }

            if( _operator == op_is_null ) {
                *s << ' ' << sql_op_text[ _operator ];
            }

            break;
        }

        case op_between:
        {
            _operand_array[ 0 ]->print( s, _operator );
            *s << " BETWEEN ";
            _operand_array[ 1 ]->print( s, _operator );
            *s << " AND ";
            _operand_array[ 2 ]->print( s, _operator );
            break;
        }

        case op_cond:
        {
            int i;
            for( i = 0; i < _operand_array.last_index(); i += 2 )
            {
                if( i >= 2 )  *s << " : ";
                _operand_array[ i ]->print( s, op_cond );
                *s << "? ";
                _operand_array[ i + 1 ]->print( s, op_cond );
            }

            if( i == _operand_array.last_index() ) {
                *s << " : ";
                _operand_array[ i ]->print( s, op_cond );
            }

            break;
        }

        case op_func_decode:
        case op_func_group_counter:
        {
            switch( _operator )
            {
                case op_func_decode:        *s << "DECODE(";
                case op_func_group_counter: *s << "GROUP_COUNTER(";
                                   default: *s << "unknown(";
            }

            for( int i = 0; i <= _operand_array.last_index(); i++ )
            {
                if( i > 0 )  *s << ',';
                _operand_array[ i ]->print( s, op_cond );
            }
            *s << ')';
            break;
        }

        default: *s << "[unknown expression " << (int)_operator << "](";
            for( int i = 0; i <= _operand_array.last_index(); i++ )
            {
                if( i > 0 )  *s << ',';
                _operand_array[ i ]->print( s, op_cond );
            }
            *s << ')';
    }
}

//-----------------------------------------------------------------Sql_func_expr::Sql_func_expr

Sql_func_expr::Sql_func_expr( Sql_operator op )
:
    Sql_expr_with_operands( op ),
    _zero_ ( this+1 )
{
}

//----------------------------------------------------------------Sql_func_expr::~Sql_func_expr

Sql_func_expr::~Sql_func_expr()
{
}

//----------------------------------------------------------------------Sql_func_expr::is_equal

bool Sql_func_expr::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr_with_operands::is_equal(expr) )  return false;

    Sql_func_expr* e = (Sql_func_expr*)&expr;

    return _name        == e->_name
       &&  _object_name == e->_object_name;
}

//--------------------------------------------------------------------Sql_func_expr::_obj_print

void Sql_func_expr::_obj_print( ostream* s ) const
{
    if( length( _object_name ) )  *s << _object_name << '.';

    *s << _name << '(';
    for( int i = 0; i < _operand_array.count(); i++ ) {
        if( i > 0 )  *s << ',';
        *s << *_operand_array[ i ];
    }
    *s << ')';
}

//------------------------------------------------------------------Sql_aggregate_expr::is_equal

bool Sql_aggregate_expr::is_equal( const Sql_expr& expr ) const
{ 
    if( !Sql_expr_with_operands::is_equal(expr) )  return false;

    Sql_aggregate_expr* e = (Sql_aggregate_expr*)&expr;

    return _distinct == e->_distinct
       &&  _groupby_index == e->_groupby_index;
}

//----------------------------------------------------------------Sql_aggregate_expr::_obj_print

void Sql_aggregate_expr::_obj_print( ostream* s ) const
{
    switch( _operator )
    {
        case op_aggregate_avg:        *s << "AVG(";         break;
        case op_aggregate_count:      *s << "COUNT(";       break;
        case op_aggregate_max:        *s << "MAX(";         break;
        case op_aggregate_min:        *s << "MIN(";         break;
      //case op_aggregate_stddev:     *s << "STDEV(";       break;
        case op_aggregate_sum:        *s << "SUM(";         break;
      //case op_aggregate_variance:   *s << "VARIANCE(";    break;
        case op_aggregate_groupby:    *s << "GROUPBY(" << _groupby_index;   break;
        default:                      *s << "AGGREGATE." << (int)_operator << '(';
    }
    
    if( _distinct )  *s << "DISTINCT ";

    if( _operand_array.count() == 0 )
        *s << '*';  // Nur count(*)
    else
        for( int i = 0; i < _operand_array.count(); i++ ) 
        {
            if( i > 0 )  *s << ',';
            *s << *_operand_array[ i ];
        }
    
    *s << ')';
}

//-------------------------------------------------------Sql_func_group_counter_expr::is_equal

bool Sql_func_group_counter_expr::is_equal( const Sql_expr& e ) const
{ 
    return Sql_expr_with_operands::is_equal(e);
}

//--------------------------------------------------------------------Sql_select_expr::is_equal

bool Sql_select_expr::is_equal( const Sql_expr& ) const
{ 
    return false;
}

//------------------------------------------------------------------Sql_select_expr::_obj_print

void Sql_select_expr::_obj_print( ostream* s ) const
{
    *s << "(SELECT ...)";
}

//-----------------------------------------------------------------------Sql_select::Sql_select

Sql_select::Sql_select()
:
    _zero_(this+1),
    _orderby ( this ),
    _aggregate( this )
{
    //obj_const_name( "Sql_select" );
    _result_expr_array.obj_const_name( "Sql_select::_result_expr_array" );
    _result_name_array.obj_const_name( "Sql_select::_result_name_array" );
}

//----------------------------------------------------------------------Sql_select::~Sql_select

Sql_select::~Sql_select()
{
    delete_sorted_records();
}

//--------------------------------------------------------------Sql_select::prepare_select_star

void Sql_select::prepare_select_star()
{
    for( int i = _result_expr_array.last_index(); i >= 0; i-- )
    {
        Sos_ptr<Sql_expr> expr = _result_expr_array[ i ];

        if( expr->_operator == op_select_star ) 
        {
            int j;

            if( _table_count == 0 )  throw_xc( "SOS-SQL-58" );

            //Neue Ausdrücke einfügen, wenn * zu mehreren Tabellen erweitert wird:
            if( _table_count > 1 ) {
                _result_expr_array.last_index( _result_expr_array.last_index() + _table_count - 1 );
                _result_name_array.last_index( _result_name_array.last_index() + _table_count - 1 );

                for( j = _result_expr_array.last_index(); j > i + _table_count-1; j-- ) {
                    _result_expr_array[ j ] = _result_expr_array[ j - _table_count + 1 ];
                    _result_name_array[ j ] = _result_name_array[ j - _table_count + 1 ];
                }
            }

            //Für jede Tabelle ein tabelle.* einfügen:
            for( j = 0; j < _table_count; j++ ) 
            {
                Sos_ptr<Sql_field_expr> field_expr = SOS_NEW( Sql_field_expr );
                field_expr->_pos = expr->_pos;
                field_expr->_table_name = _table_array[ j ]->simple_name();
                field_expr->_name = "*";
                field_expr->_resolve_star = true;
                _result_expr_array[ i + j ] = +field_expr;
                _result_name_array[ i + j ] = field_expr->_table_name + ".*";
            }

            _resolve_star = true;   // Für sossqlfl.cxx: Felder aus tabelle.* hervorholen.
        }
    }
}

//---------------------------------------------------------------Sql_select::prepare_expr_array
// Ruft rekusiv prepare_expr() und baut einen Ergebnissatztyp auf

void Sql_select::prepare_expr_array( Sos_ptr<Record_type>* record_type,
                                     const Sos_simple_array< Sos_ptr<Sql_expr> >& expr_array,
                                     const char* name_prefix )
{
    *record_type = Record_type::create();

    for( int i = expr_array.first_index(); i <= expr_array.last_index(); i++ )
    {
        Sql_expr*            expr = expr_array[ i ];
        Sos_ptr<Field_descr> new_field_descr;
        Sos_ptr<Field_type>  type;
        int                  size = _max_result_field_length;


        prepare_expr( expr );

        // Doppelte Spalten in SELECT und ORDER BY zusammenfallen lassen:
/*jz 4.9.01 (group by)
        int                  j;
        for( j = 0; j < _result_record_type->field_count(); j++ ) {
            //if( j > _result_expr_array.last_index() )  break;  // 0 bei _select_star (damit wird orderby nicht optimiert.) jz 27.5.97
            if( _result_expr_array[ j ] == expr )  break;
        }

        if( j < _result_record_type->field_count() ) {
            // Spalte einfach kopieren:
            (*record_type)->add_field( Field_descr::create( *_result_record_type->field_descr_ptr( j ) ) );
        }
        else
*/
        {
            type = expr->_type;

            if( expr->_operator == op_field  
            && !((Sql_field_expr*)expr)->is_array_elem() ) 
            {
                Field_descr* f = ((Sql_field_expr*)expr)->_field_descr;
              //Field_type*  t = f->type_ptr();
                
                new_field_descr = SOS_NEW( Field_descr( *f ) );      // jz 8.6.01: So wird _write_null_as_empty übernommen
                new_field_descr->_offset = -1;                      // Das neue Feld bekommt einen neuen Offset
                new_field_descr->_null_flag_offset = -1;
/*
                if( t->info()->_field_copy_possible ) {
                    type = t;
                } else {
                    Type_param par;
                    t->get_param( &par );
                    size = par._display_size;
                }
*/
            }
            else    //jz 8.6.01
            {
                string name = name_prefix + as_string( i );

                if( !type ) {  
/*
                    if( expr->_operator == op_const ) {     // Besser passenden Typ wählen:
                        Sql_const_expr* e = (Sql_const_expr*)expr;
                        size = e->_text.length();
                        if( size == 0 )  size = 1;          // Text_type will size > 0
                    }
*/
                    Sos_ptr<Text_type> t = SOS_NEW( Text_type( size ) );     // String0_type mit null_flag macht Ärger: Das Nullflag kommt in den Schlüssel, es wird von add_field nicht richtig berücksichtigt(?) ...
                    type = +t;
                }

                new_field_descr = SOS_NEW( Field_descr( +type, c_str( name ) ) );
            }

            new_field_descr->add_to( *record_type );
            //Absturz später, weil null_flag_offset falsch übernommen wird.
            if( !new_field_descr->nullable()
//jz25.6.98  && expr->_operator != op_field  )  new_field_descr->add_null_flag_to( *record_type );
             && ( expr->_operator != op_field || ((Sql_field_expr*)expr)->_field_descr->has_null_flag() ) ) { // jz 25.6.98
                new_field_descr->add_null_flag_to( *record_type );   
            }
        }
    }
}

//-----------------------------------------------------------Sql_select::move_keys_to_beginning
/* 6.12.97
// Stellt die Offsets im _result_record_type so ein, dass ein zusammenhängender Schlüssel
// aller Tabellen am Satzanfang entsteht.

Was ist, wenn ein Feld einen Schlüssel überlappt? Dann kann dieses Feld nicht in den 
Schlüsselbereich am Anfang gesetzt werden.

void Sql_select::move_complete_keys_to_beginning()
{
    uint offset = 0;
    int  i;

    for( i = 0; i < _table_count; i++ )     // Jede Tabelle
    {
        Sql_table* table = _table_array[ i ];
        table->_result_key_offset = offset;
        offset += table->_key_len;
    }
        
    for( i = 0; i < _result_expr_array.count(); i++ )  // Jedes Feld im Ergebnissatz
    {
        Sql_expr* expr = _result_expr_array[ j ];

        if( expr->_operator == op_field 
         && table->is_primary_key_field( ((Sql_field_expr*)field_expr ) )
        {
            Sql_field_expr* field_expr = (Sql_field_expr*)expr;
            result_Feld_offset = table->_result_key_offset + table_feld_offset - table_offset
        } else {
            result_feld_offset = offset;
            offset += feldlen;
        }
    }
}
*/
//----------------------------------------------------------------Sql_select::set_result_key_len
// Stellt die _key_len fest.

void Sql_select::set_result_key_len()
{
/*  Liefert Sql_select::_key_len und _key_pos.
    Der Satzschlüssel umfasst die Schlüsselfelder aller Tabellen, wenn
    sie selektiert worden sind (als Feld, nicht als Ausdruck).
    Wenn nicht alle Schlüsselfelder selektiert worden sind, gibt es keinen Ergebnissatzschlüssel (key_len=0).

    Die Schlüsselfelder müssen zusammenhängend in der ursprünglichen Reihenfolge und der Reihenfolge
    der Tabellen in der FROM-Klausel als erstes selektiert werden.
    Also: SELECT tab1.keyfeld1, tab1.keyfeld2, ..., tab2.keyfeld1, ... FROM tab1, tab2, ...
*/

    uint            result_offset   = 0;        // Offset im Ergebnissatz
    int             j               = 0;        // Index in _result_expr_array[]

    for( int i = 0; i < _table_count; i++ )     // Jede Tabelle
    {
        Sql_table* table = _table_array[ i ];

        if( !table->_key_in_record ) {
            if( _need_result_key ) {
                Sos_string name = table->simple_name();
                throw_xc( "SOS-1340", c_str( name ) );
            }
            return;
        }

        uint offset = table->_key_pos;              // Offset in der Tabelle

        table->_result_key_offset = result_offset;

        while( offset < table->_key_pos + table->_key_len ) 
        {
            if( j > _result_expr_array.last_index() ) {  // Keine Spalte mehr?
                if( _need_result_key ) {
                    Sos_string name = table->simple_name();
                    throw_xc( "SOS-1339", c_str( name ) );
                }
                return;
            }
        
            Sql_expr* expr = _result_expr_array[ j++ ];
            if( expr->_operator != op_field )  {
                if( _need_result_key ) {
                    Sos_string name = table->simple_name();
                    throw_xc( "SOS-1381", expr, c_str( name ) );
                }
                return;
            }
            
            Sql_field_expr* field_expr = (Sql_field_expr*)expr;
            if( !table->is_primary_key_field( field_expr ) )  {
                if( _need_result_key ) {
                    Sos_string name = table->simple_name();
                    throw_xc( "SOS-1382", expr, c_str( name ) );
                }
                return;
            }

            if( field_expr->_field_descr->offset() != offset ) {
                if( _need_result_key )  throw_xc( "SOS-1383", expr, offset );
                return;
            }

            int l = field_expr->_field_descr->type().field_size();
            if( offset + l > table->_key_pos + table->_key_len ) {
                if( i < _table_count - 1 ) {
                    if( _need_result_key )  throw_xc( "SOS-1384", expr );
                    return;
                }
                offset = table->_key_pos + table->_key_len;
            } else {
                offset += l;
            }
        }

        if( offset != table->_key_pos + table->_key_len ) {
            if( _need_result_key )  throw_xc( "SOS-1377", offset, table->_key_pos + table->_key_len );  // Kann denn das passieren?
            return;
        }

        result_offset += table->_key_len;
    }


    if( _loop ) 
    {
        // Bei einer LOOP-Klausel muss die Schleifenvariable im Ergebnis enthalten sein, wenn ein Satzschlüssel geliefert werden soll.

        if( j > _result_expr_array.last_index() ) {
            if( _need_result_key )  throw_xc( "SOS-1378" );
            return;
        }
        
        Sql_expr* expr = _result_expr_array[ j++ ];
                    if( expr->_operator != op_field ) {
                        if( _need_result_key )  throw_xc( "SOS-1378", expr );
                        return;
                    }
        
        Sql_field_expr* field_expr = (Sql_field_expr*)expr;
                    if( field_expr->_field_descr != _system->_sys_loop_index_field )  {
                        if( _need_result_key )  throw_xc( "SOS-1378", field_expr );
                        return;
                    }
                    
        // Der LOOP-Index wird als letztes Feld dem Schlüssel angehängt und von Sql_select::get_key genutzt
        _loop->_result_key_offset = result_offset;
        result_offset += sizeof _system->_sys_loop_index;
        LOG( "build_result_key_type: LOOP-Index ist Schlüssel\n ");
    }

    _key_len = result_offset;
    LOG( "Sql_select::set_result_key_len key_len=" << _key_len << '\n' );
    return;
}

//--------------------------------------------------------------------------Sql_select::prepare

void Sql_select::prepare()
{
    _result_record_type = NULL;
    _aggregate.clear();

    /// WHERE etc.
    Sql_stmt::prepare();

    /// LOOP
    if( !_loop_before_where )  prepare_loop( "sys_loop_index", _loop );                // Die LOOP-Klausel nach WHERE-Klausel

    /// LET
    prepare_let_record();                   // LET-Variablen

    /// GROUP BY
    prepare_expr_array( &_aggregate._groupby_type, _aggregate._groupby_expr_array, "groupby_" );
    
    /// GROUP BY-Ausdrücke erkennen:
    if( _aggregate._groupby_expr_array.count() > 0 )  
    {
        _aggregate.identify_groupby_exprs( &_result_expr_array );
        _aggregate.identify_groupby_exprs( &_orderby._expr_array );
    }

    /// HAVING
    if( _aggregate._having )  prepare_expr( _aggregate._having );

    /// SELECT
    prepare_select_star();
    prepare_expr_array( &_result_record_type, _result_expr_array, "result_" );

    /// Namen der Ergebnisspalten:
    if( _result_name_array.count() > 0 ) {   // 0 bei SELECT * (nicht mehr, s.o.)
        for( int i = 0; i < _result_record_type->field_count(); i++ ) {
            Sos_string& name = _result_name_array[ i ];
            if( length( name ) > 0 )  _result_record_type->field_descr_ptr( i )->name( name );
        }
    }

    set_result_key_len();
    //build_result_key_type();


    // SELECT DISTINCT

    if( _distinct ) {
        int i;
        // Wenn keine eindeutige Spalte in der Ergebnismenge ist: Die ganzen Sätze sortieren
        for( i = 0; i < _result_expr_array.count(); i++ ) {
            Sql_expr* expr = _result_expr_array[ i ];
            if( expr->_operator == op_field )
            {
                Sql_table*   t = ((Sql_field_expr*)expr)->_table;
                Field_descr* f = ((Sql_field_expr*)expr)->_field_descr;
                if( t->_key_pos >= 0
                 && f->offset() >= t->_key_pos
                 && f->offset() + f->type().field_size() <= t->_key_pos + t->_key_len )
                {
                    goto DISTINCT_OK;  // Eindeutiges Feld (Primärschlüssel) gefunden, Sortieren nicht nötig
                }
            }
        }

        {
            int k = _orderby._expr_array.last_index();

            // Alle Felder zum Ordnen heranziehen:
            for( i = 0; i < _result_expr_array.count(); i++ ) {
                Sql_expr* e = _result_expr_array[ i ];
                int j;
                for( j = 0; j <= k; j++ )  if( _orderby._expr_array[ j ] == e )  break;
                if( j > k )  _orderby._expr_array.add( e );
            }
        }

        DISTINCT_OK: ;
    }


    /// ORDER BY:
    prepare_expr_array( &_orderby._type, _orderby._expr_array, "orderby_" );
    move_offsets( _orderby._type, _result_record_type->field_size() );


    /// ORDER BY key1, key2, ... optimieren:
    _orderby._first = 0;

    for( int i = 0; i < _table_count; i++ )
    {
        Sql_table* table = _table_array[ i ];
        int        pos   = table->_key_pos;   // Läuft durch den Schlüssel (falls der Schlüssel
        int        len   = table->_key_len;   // mehr als ein Feld umfasst)

        while( _orderby._first <= _orderby._expr_array.last_index() )
        {
            Sql_expr* expr = _orderby._expr_array[ _orderby._first ];
            if( expr->_operator != op_field )  break;

            Sql_field_expr* e = (Sql_field_expr*)expr;

            if( e->_table != table )  break;

            Field_descr* f = e->_field_descr;
            if( f->offset() != pos )  break;

            int s = f->type().field_size();
            if( s > len )  break;      // Feld nicht vollständig im Schlüssel?

            pos += s;
            len -= s;
            _orderby._first++;
        }
    }


    /// AGGREGATE
    if( has_aggregate_function( _result_expr_array ) )
    {
        _has_aggregate_function = true;
        _aggregate.prepare();
        check_for_fields( _result_expr_array );
        if( _aggregate._having )  check_for_fields( _aggregate._having );
        check_for_fields( _orderby._expr_array );
    }
}

//--------------------------------------------------------------------------Sql_select::execute

void Sql_select::execute()
{
    /// WHERE etc.
    Sql_stmt::execute();

    /// bind_parameter():
    if( _param_table->_table_type )
    {
        if( !_param_callers_type )  throw_xc( "SOS-SQL-57" );  //jz 7.8.97

        // Parameter werden noch von Sql_select::fetch() gebraucht, also kopieren:
        // Die Parametertypen müssen kopierbar sein.
        if( _param_callers_base ) {  // In den Field_descrs sind Offsets und keine Adressen?
            _param_table->_table_type = _param_callers_type;
            _param_table->_record.allocate_min( _param_callers_type->field_size() );
            _param_table->_base = _param_table->_record.byte_ptr();
            _param_table->_table_type->field_copy( _param_table->_base, _param_callers_base );
        } else {
            // Die Felddeskriptoren enthalten Adressen, also neue Felddeskriptoren aufbauen:
            _param_table->_table_type = copy_field_descrs( _param_callers_type );
            _param_table->_record.allocate_min( _param_table->_table_type->field_size() );
            _param_table->_base = _param_table->_record.byte_ptr();
             copy_record( _param_table->_table_type, _param_table->_base,
                          _param_callers_type, _param_callers_base );
        }
    }

    _system->_sys_loop_index = 1-1; // Damit Schleife neu beginnen kann
    _system->_sys_loop_end   = 0;
    
    // select distinct:
    _last_record_valid = false;

    if( _has_aggregate_function )
    {
        _aggregate.execute();
    }

    // Evtl. erst beim fetch():
    if( _orderby._first <= _orderby._expr_array.last_index() ) 
    {
        _row_count = 0;  // Die Anzahl der Reihen der Ergebnismenge ist schon nach execute() bekannt
        fetch_all_and_sort();
        _orderby._sorted = true;
        _last_record_valid = false;
    }
}

//----------------------------------------------------------------------------Sql_select::close

void Sql_select::close()
{
    delete_sorted_records();
    _aggregate.close();
}

//-----------------------------------------------------------------Sql_orderby_clause::compare

int _USERENTRY Sql_orderby_clause::compare( const void* a, const void* b )
{
    int             cmp;
    Sql_select*     s   = Sql_select::qsort_select;
    int             i   = s->_orderby._first;

    while(1) {
        const Field_descr* f = s->_orderby._field_array[ i ];
        cmp = f->type().op_compare( f->const_ptr( *(const Byte**)a ),   
                                    f->const_ptr( *(const Byte**)b ) ); 
        if( s->_orderby._descending_array[ i ] )  cmp = -cmp;
        if( cmp != 0 )  break;
        if( ++i > s->_orderby._expr_array.last_index() )  break;
    }

    return cmp;
}

//---------------------------------------------------------------Sql_select::fetch_all_and_sort

void Sql_select::fetch_all_and_sort()
{
    /// Die ganze Ergebnismenge einsammeln:

/*  Optimierung:
    Schlüssel in ein Array kopieren, normieren und mit memcmp vergleichen (außer double?).
    Dann muss nicht mehr die Funktion f->type().compare(), die bei gepackten und gezonten Zahlen
    aufwendig ist, gerufen werden.
    Die Ordnung von EBCDIC soll aber erhalten bleiben. Normierungsfunktion 
    f->type().norm_for_sort( p, Area* dest ) erzeugt memcmp-fähige Bytefolge.
    Ebcdic_text: memcpy
    Ebcdic_packed: X'00..00', X'40..40' und Vorzeichen beachten.
    Littlen_endian_int: Vorzeichen beachten
    int: Bei Intel Bytes umdrehen und Vorzeichen beachten
    double: ? Vorzeichen Basis, Vorzeichen Exponent, Exponent, Basis
    Date: yyyymmdd (binär)
    Sos_string, Area: Welche Größe? - nicht sortierbar? -
    Sos_limited_text<> Größe ist bekannt (size muss bei allen Werten gleich sein)

    NACHTEIL: erhöhter Hauptspeicherbedarf für die normierten Schlüsselwerte.
    
    Erst die Werte für ORDER BY und ROWID lesen, dann Direkzugriff mit Rowid? Ja bei schneller 
    (lokaler) Datei, nein bei BS2000. sossql prüft Geschwindigkeit der Datei:
    int4 Any_file::bytes_per_sec(). Any_file-Option: "-bytes-per-sec=100K", sos.ini:
    [stdfile] bytes-per-sec=2M. 
    [fs bs2] server=bs2/4001 -bytes-per-sec=40K

    VORTEIL: Wenn Schlüssel und Sätze in verschiedenen Hauptspeicherseiten sind,
    können die Sätze während der Sortierung ausgelagert werden, sodass der Hauptspeicher
    allein für die zur Sortierung relevanten Werte verfügbar ist. Beim bisherigen Verfahren
    wird direkt auf die Sätze zugegriffen, sodass die nicht relevanten Teile nicht ausgelagert
    werden können.
    Wie werden die Sätze in separaten Speicherseiten gehalten? Besonderer Aufruf, große mallocs,
    temporäre Datei im Adressraum, ...: Bei jedem Fetch ein Plattenzugriff: 10ms!!

    SORT/MERGE: 
    Soviel Sätze lesen, wie in den Hauptspeicher passen, ganze Sätze ordnen (d.h. die Zeiger 
    darauf), alle Sätze geordnet in eine temporäre Datei schreiben. Dann die nächsten Sätze bis 
    EOF.
    Der letzte (evtl. einzige) Block wird nicht in eine Datei geschrieben, sondern bleibt im 
    Hauptspeicher.

    Anschließend rewind() auf alle temporären Dateien und bei fetch() den jeweils kleinsten 
    nächsten Satz lesen (mischen).
*/
    LOGI( "fetch_all_and_sort()\n" );

    delete_sorted_records();
    _orderby._records.increment( 16000 );  // 64KB

    int   record_size          = _result_record_type->field_size();
    int   orderby_record_size = _orderby._type->field_size();
    void* ptr;

    while(1) {
        ptr = sos_alloc( orderby_record_size, "sossql sort" );
        _orderby._records.add( ptr );
        Area area ( ptr, record_size );

        try {
            fetch( &area );
        }
        catch( const Eof_error& )  { break; }

        _row_count++;

        Area orderby_area( (Byte*)ptr, orderby_record_size );
        assign_expr_array( *_orderby._type, &orderby_area, _orderby._expr_array );
    }

    sos_free( ptr );   // Wegen eof ist ein Satz zuviel im Array.
    _orderby._records[ _orderby._records.last_index() ] = 0;
    _orderby._records.last_index( _orderby._records.last_index() - 1 );


    // Die Ergebnismenge ordnen:
    // Die Elemente von _sorted_records müssen zusammenhängend im Speicher liegen!!

    if( _orderby._records.count() > 1 ) 
    {
        for( int i = 0; i < _orderby._type->field_count(); i++ ) {
            _orderby._field_array[ i ] = _orderby._type->field_descr_ptr( i );
        }

        THREAD_LOCK( lock )
        {
            if( qsort_select )  throw_xc( "SOS-SQL-64" );  // qsort_select ist static!

            LOGI( "fetch_all_and_sort: qsort(" << _orderby._records.count() << " Sätze)\n" );

            try {
                qsort_select = this;
                qsort( &_orderby._records[0], _orderby._records.count(),
                    (Byte*)&_orderby._records[1] - (Byte*)&_orderby._records[0],
                    ::sos::Sql_orderby_clause::compare );
                qsort_select = 0;
            }
            catch( const Xc& ) {
                qsort_select = 0;  // Vorsichtshalber
                throw;
            }
            catch( const exception& ) {
                qsort_select = 0;  // Vorsichtshalber
                throw;
            }
        }
    }

    _orderby._record_no = 0;  // Satznummer für fetch_sorted()
}

//------------------------------------------------------------Sql_select::delete_sorted_records

void Sql_select::delete_sorted_records()
{
    for( int i = _orderby._records.last_index(); i >= 0; i-- ) {
        void*& r = _orderby._records[ i ];
        sos_free( r );  r = 0;
    }

    _orderby._records.last_index( -1 );
}

//---------------------------------------------------------------------Sql_select::fetch_sorted

void Sql_select::fetch_sorted( Area* record )
{
    if( _orderby._record_no > _orderby._records.last_index() )  throw_eof_error();
    void*& r = _orderby._records[ _orderby._record_no++ ];

    //if( _result_record_type->memcpy_possible() ) {
    //  record->assign( r, _result_record_type->field_size() );
    //} else {
        record->allocate_min( _result_record_type->field_size() );
        _result_record_type->field_copy( record->byte_ptr(), (const Byte*)r );
    //}

    if( _sossql_file->_open_mode & Any_file::seq ) {
        sos_free( r );  r = 0;
    }
}

//----------------------------------------------------------------Sql_select::assign_expr_array
// Berechnet die Ausdrücke und speichert das Ergebnis im Ergebnissatz.

void Sql_select::assign_expr_array( const Record_type& record_type, Area* record,
                                    const Sos_simple_array< Sos_ptr<Sql_expr> >& expr_array )
{
    Dyn_obj result;

    for( int i = 0; i < expr_array.count(); i++ )  // Jedes Ergebnis-Feld
    {
        Field_descr* f = record_type.field_descr_ptr( i );
        if( f ) {
            Sql_expr*       e0 = expr_array[ i ];
            Sql_field_expr* e  = NULL;

            if( e0->_operator == op_field ) {    // SELECT ...,field,.. ?
                e = (Sql_field_expr*)e0;
                if( e->_table->_null_values_for_outer_join_read ) {
                    f->set_null( record->byte_ptr() );
                    continue;
                }
                if( e->_index_array.count() 
                 || e->_field_descr->type_ptr() != f->type_ptr() )  e = NULL;
            }

            if( e ) {
                if( e->_field_descr->has_null_flag()  &&  e->_field_descr->null_flag( e->_table->_base ) ) {  //jz 25.6.98
                    f->set_null( record->byte_ptr() );
                } else {
                    if( f->has_null_flag() )  f->set_null_flag( record->byte_ptr(), false );
                    // Feld kopieren (optimierung)
                    if( !e->_index_array.count() ) {
                        f->type().field_copy( f->ptr( record->byte_ptr() ),
                                              e->_field_descr->const_ptr( e->_table->_base ) );
                    } else {
                        Array_field_descr* a = SOS_CAST( Array_field_descr, e->_field_descr );
                        f->type().field_copy( f->ptr( record->byte_ptr() ), 
                                              e->_table->_base + a->elem_offset( _system->_sys_loop_index ) );
                    }
                }
            }
            else
            {
                eval_expr( &result, expr_array[ i ] );
                f->assign( record->byte_ptr(), result, &_hilfspuffer );
            }
        }
    }
}

//------------------------------------------------------------------Sql_select::fetch_table_records

void Sql_select::fetch_table_records()
{
    _update_allowed = false;

    Bool try_next = false;

    do 
    {
        try {
            _expr_error = false;

            if( !_loop_before_where  &&  _loop ) {  // LOOP nach WHERE, innere Schleife
                _system->_sys_loop_index++;

                while( _system->_sys_loop_index > _system->_sys_loop_end )   // nach execute(): 1 und 0
                {
                    Sql_stmt::fetch();  // WHERE-Klausel wird hier geprüft

                    _system->_sys_loop_index = eval_expr_as_long( _loop->_begin ) - 1;
                    _system->_sys_loop_end   = eval_expr_as_long( _loop->_end   );
                }
            } else {
                Sql_stmt::fetch();  // WHERE-Klausel wird hier geprüft
            }
        }
        catch( const Xc& x ) 
        {
            if( !_expr_error )  throw;   // Fehler beim Lesen durchlassen (wiederholt sich sonst)

            // Fehler in der Where-Klausel
            _expr_error = false;
            log_error( x );
            try_next = true;
        }


        try 
        {
            if( _let_expr_array.count() ) {
                Dyn_obj result;

                for( int i = 0; i <= _let_expr_array.last_index(); i++ ) {
                    eval_expr( &result, _let_expr_array[ i ] );
                    _let_table->_table_type->field_descr_ptr( i )->assign( _let_table->_base, result, &_hilfspuffer );
                }
            }

            try_next = false;

            _update_allowed = true;
        }
        catch( const Xc& x )
        {
            log_error( x );
            try_next = true;
        }
    }
    while( try_next );
}

//---------------------------------------------------------------------------Sql_select::fetch2

void Sql_select::fetch2( Area* record )             // get_record
{
    /// Ergebnispuffer:
    record->allocate_min( _result_record_type->field_size() );
    record->length( _result_record_type->field_size() );

    if( _orderby._sorted ) 
    {
        fetch_sorted( record );
    }
    else
    if( _has_aggregate_function )
    {
        _aggregate.fetch_result();
        assign_expr_array( *_result_record_type, record, _result_expr_array );
    }
    else
    {
        fetch_table_records();
        assign_expr_array( *_result_record_type, record, _result_expr_array );
    }
}

//----------------------------------------------------------------------------Sql_select::fetch

void Sql_select::fetch( Area* record )             // get_record
{
    if( _distinct )      // SELECT DISTINCT
    {
        fetch2( record );

        if( _last_record_valid ) {
            while( _result_record_type->op_compare( _last_record.byte_ptr(), record->byte_ptr() ) == 0 ) {   // DISTINCT
                fetch2( record );
            } 
        }

        _last_record = *record;
        _last_record_valid = true;
    } 
    else 
    {
        fetch2( record );
    }
}

//----------------------------------------------------------------------Sql_select::current_key

const Const_area& Sql_select::current_key()
{
    if( !_key_len )  throw_xc( "SOS-1214" );

    _rowid.length( 0 );

    for( int i = 0; i < _table_count; i++ )  
    {
        Sql_table* table = _table_array[ i ];
        const Const_area& key = table->_file.current_key();

        if( key.length() != table->_key_len ) {
            Sos_string name = table->simple_name();
            throw_xc( "SOS-SQL-79", c_str( name ), key.length() );
        }
        _rowid.append( key );
    }

    if( _loop ) {
        // LOOP-Klausel: LOOP-Index muss selektiert sein und ist letzter Eintrag im Schlüssel (Sql_select::build_result_key)
        _rowid.append( CONST_AREA( _system->_sys_loop_index ) );
    }

    return _rowid;
}

//---------------------------------------------------------------------------Sql_select::rewind

void Sql_select::rewind()
{
    _update_allowed = false;
    _last_record_valid = false;

    if( _orderby._sorted ) {
        if( _sossql_file->_open_mode & Any_file::seq )  throw_xc( "seq und rewind?", _sossql_file );
        _orderby._record_no = _orderby._records.first_index();
    } else {
        _system->_sys_loop_index = 1-1; // Damit Schleife neu beginnen kann
        _system->_sys_loop_end   = 0;
        for( int i = 0; i < _table_count; i++ )  _table_array[ i ]->rewind();
    }
}

//----------------------------------------------------------------Sql_select::assign_callers_key

void Sql_select::assign_callers_key( Sql_table* table, const Const_area& key )
{
    //LOGI( "assign_callers_key "<<*table<<"_callers_key_table.last_index()="<<table->_callers_key_table.last_index()<<"\n");
    table->_key.resize_min( table->_key_len );

    memcpy( table->_key.byte_ptr(), 
            key.byte_ptr() + table->_result_key_offset,
            table->_key_len );
}

//--------------------------------------------------------------------------Sql_select::set_key

void Sql_select::set_key( const Const_area& key )
{
    _update_allowed = false;

    if( !_key_len )  throw_xc( "SOS-1214" );

    if( _orderby._expr_array.count() )  throw_xc( "SOS-1260" );
    //? order by als separater Dateityp, so dass die Ordnung hier nicht betrachtet werden muss.

    LOGI( "Sql_select::set_key " << hex << key << dec << "\n" );

    for( int i = 0; i < _table_count; i++ )  
    {
        assign_callers_key( _table_array[ i ], key );
        _table_array[ i ]->set_key();
    }
}

//--------------------------------------------------------------------------Sql_select::get_key

void Sql_select::get_key( Area* buffer, const Const_area& key )
{
    _update_allowed = false;

    if( !_key_len )  throw_xc( "SOS-1214" );

    if( _orderby._expr_array.count() )  throw_xc( "SOS-1260" );
    //? order by als separater Dateityp, so dass die Ordnung hier nicht betrachtet werden muss.

    // Ergebnispuffer:
    buffer->allocate_length( _result_record_type->field_size() );

    for( int i = 0; i < _table_count; i++ ) 
    {
        LOG( "Sql_select::get_key " << hex << key << dec << "\n" );
        assign_callers_key( _table_array[ i ], key );
        _table_array[ i ]->get_key();
    }

    if( _loop ) {
        // siehe Sql_select::build_result_key_type
        //Record_type* key_type = SOS_CAST( Record_type, _result_key_descr->type_ptr() );
        //Field_descr* loop_index_field = key_type->field_descr_ptr( key_type->field_count() - 1 );
        //memcpy( &_system->_sys_loop_index, loop_index_field->const_ptr( key.byte_ptr() ), sizeof _system->_sys_loop_index );
        memcpy( &_system->_sys_loop_index, key.byte_ptr() + _loop->_result_key_offset, sizeof _system->_sys_loop_index );
    }

    if( _where  &&  !eval_expr_as_bool( _where, false ) )  throw_not_found_error();

    //if( _select_star ) { // select *
    //    buffer->assign( _table_array[ 0 ]->_record );
    //} else {
        assign_expr_array( *_result_record_type, buffer, _result_expr_array );
    //}

    _update_allowed = true;
}

//----------------------------------------------------------Sql_select::check_where_for_writing

void Sql_select::check_where_for_writing()
{
    _last_not_true_expr = NULL;
    if( _where  &&  !eval_expr_as_bool( _where, false ) )  throw_xc_expr( "SOS-SQL-71", _last_not_true_expr? +_last_not_true_expr : +_where );
    if( _assert &&  !eval_expr_as_bool( _assert, false ) )  throw_xc_expr( "SOS-SQL-71", _last_not_true_expr? +_last_not_true_expr : +_assert );
}

//------------------------------------------------------------Sql_select::assign_callers_record

void Sql_select::assign_callers_record( const Const_area& record )
{
    for( int i = 0; i <= _result_expr_array.last_index(); i++ )
    {
        Field_descr* f    = _result_record_type->field_descr_ptr( i );
        Sql_expr*    expr = _result_expr_array[ i ];

        if( expr->_operator == op_field
         && ((Sql_field_expr*)expr)->_index_array.count() == 0 ) 
        {
            Sql_field_expr* e = (Sql_field_expr*)expr;
            if( e->_table != _table_array[ 0 ] )  throw_xc_expr( "SOS-SQL-73", expr );
            copy_field( e->_field_descr, e->_table->_base, f, record.byte_ptr(), &_hilfspuffer );

            int highest_offset = e->_field_descr->offset() + e->_field_descr->type_ptr()->field_size();
            if( e->_table->_record.length() < highest_offset )  e->_table->_record.length( highest_offset );
        }
        else
        if( !f->null( record.byte_ptr() ) ) {         // NULL wird ignoriert!
            if( expr->_operator == op_const ) {
                Sql_const_expr* e = (Sql_const_expr*)expr;
                f->write_text( record, &_hilfspuffer );
                if( _hilfspuffer != e->_text )  throw_xc( "SOS-SQL-74", f->name(), e->_text.char_ptr() );
            }
            else throw_xc( "SOS-SQL-75", f->name() );
        }
    }
}

//---------------------------------------------------------------------------Sql_select::update

void Sql_select::update( const Const_area& record )
{
    if( record.length() != _result_record_type->field_size() )  throw_xc( "Sql_select::update-field_size" );

    if( _table_count == 0 )  throw_xc( "SOS-SQL-76" );

    if( !_update_allowed )  throw_xc( "SOS-1233" );
    _update_allowed = false;

    assign_callers_record( record );
    check_where_for_writing();
    _table_array[ 0 ]->update();
}

//---------------------------------------------------------------------------Sql_select::insert

void Sql_select::insert( const Const_area& record )
{
    _update_allowed = false;

    if( record.length() != _result_record_type->field_size() )  throw_xc( "Sql_select::update-field_size" );

    if( _table_count == 0 )  throw_xc( "SOS-SQL-67" );


    Sql_table* table = _table_array[ 0 ];

    table->construct_record();
    assign_callers_record( record );

    // "feld = Konstante" aus der Where-Klausel, die nicht im result_record sind, übernehmen!

    check_where_for_writing();
    table->insert();
}

//------------------------------------------------------------------------------Sql_select::del

void Sql_select::del()
{
    if( _table_count == 0 )  throw_xc( "SOS-SQL-67" );

    _update_allowed = false;

    _table_array[ 0 ]->del();
}

//---------------------------------------------------------------Sql_write_stmt::Sql_write_stmt

Sql_write_stmt::Sql_write_stmt()
{
    _field_name_array.obj_const_name( "Sql_write_stmt::_field_name_array" );
    _values.obj_const_name( "Sql_write_stmt::_values" );
}

//----------------------------------------------------------------------Sql_write_stmt::prepare

void Sql_write_stmt::prepare()
{
    Base_class::prepare();

    for( int i = _field_name_array.first_index(); i <= _field_name_array.last_index(); i++ ) {
        // Doppelte Felder werden geduldet
        _record_type.add_field( _table_array[ 0 ]->_table_type->field_descr_ptr( _field_name_array[ i ] ) );
    }
}

//----------------------------------------------------------------Sql_write_stmt::assign_fields

void Sql_write_stmt::assign_fields()
{
    Sql_table* table = _table_array[ 0 ];
    Dyn_obj    result;

    table->_open_mode = Any_file::inout;


    for( int i = 0; i < _record_type.field_count(); i++ )
    {
        _xc_expr_inserted = false;
        eval_expr( &result, _values[ i ] );
        _record_type.field_descr_ptr( i )->assign( table->_record.byte_ptr(),
                                                   result,
                                                   &_hilfspuffer );
    }
}

//-----------------------------------------------------------------------Sql_insert::Sql_insert

Sql_insert::Sql_insert()
{
    obj_const_name( "Sql_insert" );
}

//----------------------------------------------------------------------Sql_insert::~Sql_insert

Sql_insert::~Sql_insert()
{
}

//--------------------------------------------------------------------------Sql_insert::prepare

void Sql_insert::prepare()
{
    Base_class::prepare();
}

//--------------------------------------------------------------------------Sql_insert::execute

void Sql_insert::execute()
{
    Sql_stmt::execute();
    _row_count = 0;

    Sql_table* table = _table_array[ 0 ];

    table->construct_record();

    assign_fields();

    table->_file.insert( table->_record );

    _row_count = 1;

    /// Nach execute() muß vom Aufrufer close() gerufen werden
}

//-----------------------------------------------------------------------Sql_update::Sql_update

Sql_update::Sql_update()
{
    obj_const_name( "Sql_update" );
}

//----------------------------------------------------------------------Sql_update::~Sql_update

Sql_update::~Sql_update()
{
}

//--------------------------------------------------------------------------Sql_update::prepare

void Sql_update::prepare()
{
    Base_class::prepare();
}

//--------------------------------------------------------------------------Sql_update::execute

void Sql_update::execute()
{
    Base_class::execute();
    _row_count = 0;

    Sql_table* table = _table_array[ 0 ];

    while(1)
    {
        try {
            fetch();
        }
        catch( const Eof_error& )  { break; }

        try {
            assign_fields();

            //LOG( "table->_file.update( " << hex << table->_record << " )\n" );
            table->_file.update( table->_record );
            _row_count++;
        }
        catch( const Xc& x )
        {
            if( !_error_log_opened )  throw;
            _error_log << x << '\n' << flush;
        }
    }

    /// Nach execute() muß vom Aufrufer close() gerufen werden
}

//-----------------------------------------------------------------------Sql_delete::Sql_delete

Sql_delete::Sql_delete()
{
    obj_const_name( "Sql_delete" );
}

//----------------------------------------------------------------------Sql_delete::~Sql_delete

Sql_delete::~Sql_delete()
{
}

//--------------------------------------------------------------------------Sql_delete::execute

void Sql_delete::execute()
{
    Base_class::execute();
    _row_count = 0;

    Sql_table* table = _table_array[ 0 ];

    while(1)
    {
        try {
            fetch();
        }
        catch( const Eof_error& )  { break; }

        try {
            table->_file.del();
            _row_count++;
        }
        catch( const Xc& x )
        {
            if( !_error_log_opened )  throw;
            _error_log << x << '\n' << flush;
        }
    }

    /// Nach execute() muß vom Aufrufer close() gerufen werden
}

} //namespace sos
