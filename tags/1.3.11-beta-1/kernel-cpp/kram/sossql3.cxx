// $Id$

#if defined _MSC_VER
#   pragma warning( disable:4018 )   // warning C4018: '>': Konflikt zwischen 'signed' und 'unsigned'
#endif

//#if defined __BORLANDC__
//#  define CHECK_STACK_OVERFLOW
//#endif

#include "precomp.h"

//#define MODULE_NAME "sossql3"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/*
    Implementierte Klassen:

    Sql_stmt



    Mängel:

    -   Outer join wird nicht stets erkannt. Bsp.: expr and ( expr or ( feld =* expr ) ).
        Das wird sowieso nicht unterstützt und muss abgelehnt werden.

    -   Bei INSERT und UPDATE kann ein Feld mehrfach aufgeführt werden. Der letzte Wert
        wird gespeichert.

    -   Vergleichsoperationen sind mit Dyn_obj realisiert. Mängel siehe dort.
        Vergleich mit Nachkommastellen geht nicht.

    -   ORDER BY funktioniert nicht mit nichtigen Werten.

    -   a LIKE b: Wenn a ein std_type_char ist, werden dessen hängende Blanks nicht
        berücksichtigt. a LIKE 'x ' liefert daher immer falsch, auch wenn a PIC XX ist.

    -   a LIKE b, wenn a Satzschlüssel ist: Wenn der konstante Teil von b länger als der
        Satzschlüssel ist, gibt's den Fehler SOS-1120 (zu lang). like liefert dann immer false
        und single_key_intervall() muß ein leeres Intervall zurückgeben. Das geht aber nicht.

    -   Was passiert bei einer Exception? (Überlauf, Datumsfehler etc)
        NULL liefern oder Fehler?
*/

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosarray.h"
#include "../kram/stdfield.h"
#include "../kram/sosdate.h"
#include "../kram/soslimtx.h"
#include "../file/anyfile.h"
#include "../kram/sosfunc.h"
#include "../kram/sossqlfn.h"
#include "../kram/sossql2.h"

using namespace std;
namespace sos {

//-----------------------------------------------------------------------------Sql_stmt::create

Sos_ptr<Sql_stmt> Sql_stmt::create( const string &statement,
                                    const Sos_string& error_log,
                                    const Source_pos& source_pos )
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_stmt> );


    istrstream input_stream ( (char*)c_str( statement ), length( statement ) );
    istream*   input = &input_stream;

    Sos_ptr<Sql_stmt> stmt;
    Sql_parser        parser ( input, source_pos );

#   if defined SOSSQL_OLE
        Sos_simple_ptr_array<Ole_object> objects;

        if( parser.next_token_is( Sql_parser::k_set ) )
        {
            objects.add( parser.parse_set_stmt() );
            Ole_object* o = objects[ objects.last_index() ];

            for( int i = 0; i < objects.last_index(); i++ ) {
                if( stricmp( c_str( objects[ i ]->_name ), c_str( o->_name ) ) == 0 )  throw_syntax_error( "SOS-SQL-81", c_str( o->_name ) );
            }
        }
#    else
        if( parser.next_token_is( Sql_parser::k_set ) )  throw_syntax_error( "SOS-1314", parser._pos );
#   endif


    if( parser.next_token_is( Sql_parser::k_select ) ) {
        Sos_ptr<Sql_select> select = SOS_NEW( Sql_select );
        parser.parse_select_stmt( select, true );
        stmt = +select;
    }
    else
    if( parser.next_token_is( Sql_parser::k_insert ) ) {
        Sos_ptr<Sql_insert> insert = SOS_NEW( Sql_insert );
        parser.parse_insert_stmt( insert, true );
        stmt = +insert;
    }
    else
    if( parser.next_token_is( Sql_parser::k_update ) ) {
        Sos_ptr<Sql_update> update = SOS_NEW( Sql_update );
        parser.parse_update_stmt( update, true );
        stmt = +update;
    }
    else
    if( parser.next_token_is( Sql_parser::k_delete ) ) {
        Sos_ptr<Sql_delete> del = SOS_NEW( Sql_delete );
        parser.parse_delete_stmt( del, true );
        stmt = +del;
    }
    else throw_syntax_error( "SOS-SQL-14", parser._pos );

    if( length( error_log ) )  {
        stmt->_error_log_file.open( error_log, Any_file::out );
        stmt->_error_log_opened = true;
    }

    //if( parser._need_error_log  &&  !stmt->_error_log.opened() )  throw_xc( "SOS-SQL-70" );

#   if defined SOSSQL_OLE
        stmt->_object_array.last_index( objects.last_index() );
        for( int i = 0; i <= objects.last_index(); i++ ) {
            stmt->_object_array[ i ] = objects[ i ];
        }
#   endif

    stmt->_statement = statement;

    return stmt;
}

//---------------------------------------------------------------------------Sql_stmt::Sql_stmt

Sql_stmt::Sql_stmt()
:
    _zero_(this+1),
    _error_log ( _error_log_file )
{
    obj_const_name( "Sql_stmt" );
    
    _max_reads = INT_MAX;

    _table_array    .obj_const_name( "Sql_stmt::_table_array" );
    _table_order    .obj_const_name( "Sql_stmt::_table_order" );
    _let_names      .obj_const_name( "Sql_stmt::_let_names" );
    _let_expr_array .obj_const_name( "Sql_stmt::_let_expr_array" );

#   if defined SOSSQL_OLE
       _object_array.obj_const_name( "Sql_stmt::_object_array" );
#   endif

    _max_field_length = max_sossql_field_size;
    _order_tables = true;
}

//--------------------------------------------------------------------------Sql_stmt::~Sql_stmt

Sql_stmt::~Sql_stmt()
{
    // Zyklus auflösen: _where -> field_expr -> table._join_expr -> expr -> ...
    for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
    {
        _table_array[ i ]->_join_expr = NULL;
    }
}

//------------------------------------------------------------------------Sql_stmt::table_index

int Sql_stmt::table_index( const Sos_string& table_name )
{
    for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
    {
        Sql_table* t = _table_array[ i ];
        if( t->_name  == table_name )  return i;
        if( t->_alias == c_str( table_name ) )  return i;
    }

    return -1;
}

//------------------------------------------------------------------Sql_stmt::prepare_join_expr

void Sql_stmt::prepare_join_expr( Sql_expr_with_operands* expr )
{
    const Sql_expr* op0 = expr->_operand_array[ 0 ];
    const Sql_expr* op1 = expr->_operand_array[ 1 ];

    if( expr->_operand_array.count() == 2 )
    {
        if( op1->_operator == op_field )       // Feld rechts? Kandidat zum Vertauschen!
        {
            const Sql_field_expr* f1    = (Sql_field_expr*)op1;
            Bool                  exchg = false;
            const char*           grund = "";

            if( f1->_outer_join ) {             // expr op feld (+) oder (!)  ==> vertauschen!
                exchg = true;  
                grund = "rechter Operand Feld einer Outer-Join-Tabelle ist";
            }    
            else
            if( op0->_operator != op_field ) { // expr op feld,  expr kein Feld  ==> vertauschen!
                exchg = true;  
                grund = "rechter Operand Feld ist, linker aber nicht";
            }
            else
            if( op0->_operator == op_field ) {                    // feld == feld ?
                const Sql_field_expr* f0 = (Sql_field_expr*)op0;

                //jz 2.2.02 if( f1->is_primary_key_field()  &&  f1->_field_descr->offset() == f1->_table->_key_pos ) 
                if( f1->is_primary_key_field() ) 
                { 
                    //jz 2.2.02  if( f0->is_primary_key_field()  &&  f0->_field_descr->offset() == f0->_table->_key_pos ) 
                    if( f0->is_primary_key_field() ) 
                    {
                        if( f0->_table_index < f1->_table_index ) {                // Reihenfolge in FROM beachten
                            exchg = true;     
                            grund = "beide Operanden Schlüsselfelder sind, und das rechte in einer vorrangig genannten Tabelle ist";
                        } else {
                            //grund = "beide Operanden Schlüsselfelder sind, und das linke in einer vorrangig genannten Tabelle ist";
                        }
                    } else {
                        exchg = true;
                        grund = "der rechte Operand ein Feld am Anfang des Primärschlüssels ist";
                    }
                }
/*
                else
                if( f1->is_secondary_key_field()                              // feld == index,
                 && !f0->is_primary_key_field() ) {                           // aber nicht key == index?
                    if( f0->is_secondary_key_field() ) {                          // index == index?
                        if( f0->_table_index < f1->_table_index )  {                // Reihenfolge in FROM beachten
                            // offset()==keypos!! wie bei primärschlüssel (s.o.)
                            exchg = true;     
                            grund = "beide Operanden Sekundärschlüsselfelder sind, und das rechte in einer vorrangig genannten Tabelle ist";
                        } else {
                            //grund = "beide Operanden Sekundärschlüsselfelder sind, und das linke in einer vorrangig genannten Tabelle ist";
                        }
                    } else {
                        exchg = true;
                        grund = "der rechte Operand Sekundärschlüsselfeld ist";
                    }
                }
*/
            }

            if( exchg ) {
                // xxx cmp field  ==>  field cmp xxx

                LOG( "Sql_stmt::prepare_join_expr: Vertausche " << *expr->_operand_array[ 0 ] << " und " << *expr->_operand_array[ 1 ] << ", weil " << grund << '\n' );
                exchange( &expr->_operand_array[ 0 ], &expr->_operand_array[ 1 ] );
                exchange( &op0, &op1 );

                switch( expr->_operator ) {
                    case op_lt: expr->_operator = op_gt;  break;
                    case op_le: expr->_operator = op_ge;  break;
                    case op_eq:                           break;
                    case op_ne:                           break;
                    case op_ge: expr->_operator = op_le;  break;
                    case op_gt: expr->_operator = op_lt;  break;
                    default:    throw_xc( "prepare_join_expr" );
                }
            }
        }
    }


    if( op0->_operator == op_field )
    {
        Sql_field_expr* f = (Sql_field_expr*)op0;
        f->_table->set_join( expr );
    }
}

//--------------------------------------------------------------------Sql_stmt::get_field_descr

void Sql_stmt::get_field_descr( const Sos_string& field_name, const Sos_string& table_name,
                                Field_descr** field_descr_pp, Sql_table** table_pp,
                                int* table_index_ptr,
                                Bool* error, const Source_pos& pos )
// Feldnamen auflösen
{
    if( error )  *error = false;
    *field_descr_pp = NULL;
    *table_pp       = NULL;
    *table_index_ptr = -1;

    if( table_name != "" )       // Tabellenname angegeben? "table.field"
    {
        int i = table_index( table_name );
        if( i < 0 ) {
            if( !_outer_stmt ) {
                if( error )  { *error = true; return; }
                throw_xc( "SOS-SQL-50", c_str( table_name ), pos );
            } else {
                // _outer_stmt->get_field_descr() wird unten gerufen
            }
        } else {
            *table_index_ptr = i;
            *table_pp = _table_array[ i ];

            if( field_name == "*" ) {  // Das hat der Parser eingesetzt.
                *field_descr_pp = (*table_pp)->_table_field_descr;
                //*field_descr_pp = Field_descr::create();
                //(*field_descr_pp)->_type = (*table_pp)->_table_type;
                //(*field_descr_pp)->_name = (*table_pp)->simple_name() + ".*";
                //(*field_descr_pp)->_offset = 

            } else {
                *field_descr_pp = (*table_pp)->_table_type->field_descr_ptr( c_str( field_name ) );
            }
        }
    }
    else
    {   // Alle Tabellen durchsuchen, auf Eindeutigkeit achten
        for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
        {
            Record_type* t = _table_array[ i ]->_table_type;
            if( t ) {   // 0 bei param_table
                Field_descr* f = t->field_descr_by_name_or_0( c_str( field_name ) );
                if( f ) {
                    if( *field_descr_pp ) {
                        if( error )  { *error = true; return; }
                        throw_xc( "SOS-SQL-51",
                                  Msg_insertions( c_str( field_name ),
                                                  c_str( (*table_pp)->_name ),
                                                  c_str( _table_array[ i ]->_name ) ),
                                  pos );
                    }
                    *field_descr_pp = f;
                    *table_index_ptr = i;
                    *table_pp       = _table_array[ i ];
                }
            }
        }
    }

    if( !*field_descr_pp )  {
        if( _outer_stmt ) {
            _outer_stmt->get_field_descr( field_name, table_name, field_descr_pp, table_pp,
                                          table_index_ptr, error, pos );
            *table_index_ptr += _table_array.count();
        } else {
            if( error )  { *error = true; return; }
            throw_xc( "SOS-SQL-52", c_str( field_name ), pos );
        }
    }
}

//-----------------------------------------------------------------Sql_stmt::prepare_field_expr

void Sql_stmt::prepare_field_expr( Sql_field_expr* expr, Sql_join_expr_nesting join_nesting )
// Feldnamen auflösen
{
    Field_descr* field_descr = NULL;
    Sql_table*   table       = NULL;
    int          table_index = -1;

    if( expr->_field_descr ) {
        // Nur bei select *: Von Sql_select::prepare() erledigt (weil Feldnamen hier doppelt sein dürfen ...)
    } else {
        get_field_descr( expr->_name, expr->_table_name, &field_descr, &table, &table_index, NULL, expr->_pos );
        expr->_field_descr = field_descr;
        expr->_table       = table;
        expr->_table_index = table_index;     // Nur bis order_table() gültig!
    }

    if( expr->_field_descr->type_ptr()->info()->_field_copy_possible ) {
        expr->_type = expr->_field_descr->type_ptr();
    } else {
        Type_param par;
        expr->_field_descr->type_ptr()->get_param( &par );
        Sos_ptr<Text_type> t = SOS_NEW( Text_type( par._display_size ) );  // String0_type mit null_flag macht Ärger: Das Nullflag kommt in den Schlüssel, es wird von add_field nicht richtig berücksichtigt(?) ...
        expr->_type = t;
    }

    if( expr->_outer_join ) {
        if( join_nesting == je_other )  throw_xc_expr( "SOS-SQL-71", expr );  // Outer Join an falscher Stelle?
        //wird zweimal gerufen?
        if( expr->_table->_outer_join )   throw_xc( "SOS-SQL-69", c_str( expr->_table->_name ) );
        expr->_table->_outer_join = expr->_outer_join;
    }

    expr->_used_tables.include( expr->_table_index );

    //LOG( "sossql3 update_selected_type( " << *expr->_field_descr << ")\n" );
    expr->_table->update_selected_type( expr->_field_descr );
}

//---------------------------------------------------------Sql_stmt::prepare_expr_with_operands

void Sql_stmt::prepare_expr_with_operands( Sql_expr_with_operands* expr, Sql_join_expr_nesting join_nesting )
{
    int i;

    // Struktur der Join-Ausdrücke der Tabellen erweitern:
    if( join_nesting < je_cmp ) {
        for( i = 0; i < _table_count; i++ )  _table_array[ i ]->begin_join_expr( expr->_operator );
    }


    for( i = expr->_operand_array.first_index(); i <= expr->_operand_array.last_index(); i++ )
    {
         Sql_expr* e = expr->_operand_array[ i ];
         prepare_expr( e, join_nesting );
         expr->_used_tables |= e->_used_tables;
    }

    if( join_nesting < je_other  &&  is_join_op( expr->_operator ) )     // JOIN
    {
        prepare_join_expr( (Sql_expr_with_operands*)expr );
    }

    // Erweiterung der Struktur der Join-Ausdrücke der Tabellen abschliessen:
    if( join_nesting < je_cmp ) {
        for( i = 0; i < _table_count; i++ )  _table_array[ i ]->end_join_expr();
    }
}

//-----------------------------------------------------------------------Sql_stmt::prepare_expr

void Sql_stmt::prepare_expr( Sql_expr* expr, Sql_join_expr_nesting join_nesting )
{
    if( expr->_prepared )  return;

    switch( expr->_operator )
    {
        case op_field:
        {
            Sql_field_expr* e = (Sql_field_expr*)+expr;
            prepare_field_expr( e, join_nesting );

            // Array-Indices
            for( int i = e->_index_array.first_index(); i <= e->_index_array.last_index(); i++ )
            {
                Sql_expr* ex = e->_index_array[ i ];
                prepare_expr( ex, je_other );
                expr->_used_tables |= ex->_used_tables;
            }

            break;
        }

        case op_rowid:
        {
            Sql_rowid_expr* e         = (Sql_rowid_expr*)+expr;
            int             table_idx = 0;

            if( length( e->_table_name ) == 0 ) {
                if( _table_count != 1 )  throw_xc( "SOS-SQL-60", (const char*)0, e->_pos );
            } else {
                table_idx = table_index( e->_table_name );
                if( table_idx < 0  ||  table_idx >= _table_count )  throw_xc( "SOS-SQL-50", c_str( e->_table_name ), e->_pos );
            }

            expr->_used_tables.include( table_idx );

            e->_table = _table_array[ table_idx ];
            if( !e->_table->_key_len )  throw_xc( "SOS-SQL-61", c_str( e->_table->_name ), e->_pos );

            Sos_ptr<Text_type> type = SOS_NEW( Text_type( 2 * e->_table->_key_len ) );  // Besser Binary_type!
            e->_type = type;

            break;
        }

        case op_param:
        {
            ((Sql_param_expr*)+expr)->_table = _param_table;
            //_type = ???;
            // Nix für _used_tables
            // Typ aus anderen Operanden der Operation ermitteln. (int) * ? liefert (int)  (ANSI-SQL)
            break;
        }

        case op_const:
        {
            Sql_const_expr* e = (Sql_const_expr*)expr;
            e->_null = e->_value.null();
            if( !e->_null )  {
                e->_value.write_text( &e->_text );
                e->_text.resize_min( e->_text.length() + 1 );      // '\0' hinten dran für _text.char_ptr() in throw_xc
                e->_text.char_ptr()[ e->_text.length() ] = '\0';
            }
            e->_computed = true;
            e->_type = e->_value.type();
            break;
        }

        case op_negate:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr,
                                        /*join_nesting < je_not? je_not :*/ je_other );
            expr->_type = ((Sql_expr_with_operands*)expr)->_operand_array[ 0 ]->_type;
            break;
        }

        case op_or:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr,
                                        join_nesting < je_or? je_or : je_other );
            expr->_type = &bool_type;
            break;
        }

        case op_and:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr,
                                        join_nesting < je_and? je_and : je_other );
            expr->_type = &bool_type;
            break;
        }

        case op_not:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr,
                                        /*join_nesting < je_not? je_not :*/ je_other );
            expr->_type = &bool_type;
            break;
        }

        case op_lt:
        case op_le:
        case op_eq:
        case op_ne:
        case op_ge:
        case op_gt:
        case op_like:
      //case op_in:
      //case op_in_select:
        case op_between:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr,
                                        join_nesting < je_cmp? je_cmp : je_other );
            expr->_type = &bool_type;
            break;
        }

        case op_regex_match:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr, je_other );
            expr->_type = &bool_type;
            break;
        }

        case op_exists:
        case op_is_null:
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr, je_other );
            expr->_type = &bool_type;
            break;
        }

        case op_cond:
        {
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;
            prepare_expr_with_operands( e, je_other );

            Field_type* t = e->_operand_array[ 1 ]->_type;
            if( t
             && t->info()->_std_type != std_type_char
             && t->info()->_std_type != std_type_varchar )
            {
                expr->_type = +t;
            }
            else
            {
/*
                ulong display_size = 0;
                for( int i = 1; i <= e->_operand_array.last_index(); i++ ) {
                    Sql_expr* e2 = e->_operand_array[ i ];
                    if( !e2->_type )  { display_size = _max_field_length; break; }
                    display_size = max( display_size, e2->_type->display_size() );
                }

                if( display_size > _max_field_length )  display_size = _max_field_length;
*/
                uint display_size = _max_field_length;
                Sos_ptr<Text_type> t = SOS_NEW( Text_type( display_size ) );       // String0_type mit null_flag macht Ärger: Das Nullflag kommt in den Schlüssel, es wird von add_field nicht richtig berücksichtigt(?) ...
                expr->_type = t;
            }
            //if( !expr->_type  &&  e->_operand_array.last_index() >= 2 ) {
            //    expr->_type = e->_operand_array[ 2 ]->_type;
            //}
            break;
        }

        case op_concat:
        {
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;

            prepare_expr_with_operands( e, je_other );
/*
            ulong display_size = 0;
            for( int i = 0; i <= e->_operand_array.last_index(); i++ ) {
                Sql_expr* e2 = e->_operand_array[ i ];
                if( !e2->_type )  { display_size = _max_field_length; break; }
                display_size += e2->_type->display_size();
            }
            if( display_size > _max_field_length )  display_size = _max_field_length;
*/
            int display_size = _max_field_length;
            Sos_ptr<Text_type> t = SOS_NEW( Text_type( display_size ) );       // String0_type mit null_flag macht Ärger: Das Nullflag kommt in den Schlüssel, es wird von add_field nicht richtig berücksichtigt(?) ...
            expr->_type = t;
            break;
        }

        case op_add:
        case op_subtract:
        case op_multiply:
        case op_divide:
        {
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;

            prepare_expr_with_operands( e, je_other );

            if( expr->_operator == op_add || expr->_operator == op_subtract )
            {
                if( e->_operand_array[0]->_type  &&  e->_operand_array[1]->_type )
                {
                    Std_type a_std_type = e->_operand_array[0]->_type->info()->_std_type;
                    Std_type b_std_type = e->_operand_array[1]->_type->info()->_std_type;

                    if( ( a_std_type == std_type_date || a_std_type == std_type_date_time ) && is_numeric( b_std_type ) )
                    {
                        Sos_ptr<Text_type>   tt = SOS_NEW( Text_type(strlen(std_date_time_format_iso)) );
                        Sos_ptr<As_date_type> t = SOS_NEW( As_date_type( +tt, std_date_time_format_iso ) );
                        expr->_type = t;
                        break;
                    }
                }
            }

            expr->_type = &double_type;
            break;
        }

        case op_select:
        {
            Sql_select_expr* e = (Sql_select_expr*)expr;
            e->_select._outer_stmt = this;

            e->_select._session = _session;
            //e->_select._sossql_file = _sossql_file;
            //e->_select._max_result_field_length = _max_result_field_length;

            e->_select.prepare();
            if( e->_select._result_record_type->field_count() != 1 )  throw_xc( "SOS-SQL-85", Msg_insertions(), e->_pos );
            expr->_type = e->_select._result_record_type->field_descr_ptr( 0 )->type_ptr();
            break;
        }

        case op_func_field:  // Funktion FIELD() - Typ ist immer Text, schlecht bei Double und "record/tabbed -decimal-comma" und bei Sos_date
        {
            prepare_expr_with_operands( (Sql_expr_with_operands*)expr, je_other );
            Sos_ptr<Text_type> t = SOS_NEW( Text_type( _max_field_length ) );       // String0_type mit null_flag macht Ärger: Das Nullflag kommt in den Schlüssel, es wird von add_field nicht richtig berücksichtigt(?) ...
            expr->_type = +t;
            _using_func_field = true;  // Für Sql_table: Alle Felder lesen!
            break;
        }

        case op_func_decode:
        {
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;
            prepare_expr_with_operands( e, je_other );
            Field_type* t = e->_operand_array[ 2 ]->_type;
            if( t
             && t->info()->_std_type != std_type_char
             && t->info()->_std_type != std_type_varchar )
            {
                expr->_type = +t;
            }
            //if( !expr->_type  &&  e->_operand_array.last_index() >= 3 ) {
            //    expr->_type = e->_operand_array[ 3 ]->_type
            //}
            break;
        }

        case op_func_group_counter:
        {
            Sql_func_group_counter_expr* e = (Sql_func_group_counter_expr*)expr;
            prepare_expr_with_operands( e, je_other );
            expr->_type = &long_type;
            break;
        }

        case op_function:
        {
            Sos_function_descr* func_descr;
            Sql_func_expr*      e = (Sql_func_expr*)expr;

            prepare_expr_with_operands( e, je_other );

            if( stricmp( c_str( e->_name ), "tab" ) == 0 ) {
                if( e->_operand_array.count() != 1 )  throw_xc( "SOS-1311", "tab", 2 );
                expr->_type = &int_type;
            }
            else
            if( stricmp( c_str( e->_name ), "warning" ) == 0 ) {
                if( e->_operand_array.count() != 2 )  throw_xc( "SOS-1311", "warning", 2 );
                expr->_type = e->_operand_array[ 0 ]->_type;
            }
            else
            if( stricmp( c_str( e->_name ), "ifnull" ) == 0 ) {
                if( e->_operand_array.count() != 2 )  throw_xc( "SOS-1311", "ifnull", 2 );
                expr->_type = e->_operand_array[ 0 ]->_type;
            }
            else
            //if( e->_name != "field" ) {     // field() ist eine spezielle Funktion
            {
#               if defined SOSSQL_OLE
                if( length( e->_object_name ) ) {
                    Ole_object* o = NULL;
                    int i;
                    for( i = 0; i <= _object_array.last_index(); i++ ) {
                        o = _object_array[ i ];
                        if( stricmp( c_str( o->_name ), c_str( e->_object_name ) ) == 0 )  break;
                    }
                    if( i > _object_array.last_index() )  throw_xc( "SOS-SQL-82", c_str( e->_object_name ), e->_pos );
                    e->_method = o->method( e->_name );
                } else
#               endif
                {
                    func_descr = sos_static_ptr() ->
                                 _function_register->function_descr( c_str( e->_name ),
                                                                     e->_operand_array.count() );
                    if( !func_descr )  throw_xc( "SOS-SQL-80", c_str( e->_name ), e->_pos );
                    e->_func_descr = func_descr;
                }
            }

            if( stricmp( c_str( ((Sql_func_expr*)expr)->_name ), "to_date" ) == 0
             || stricmp( c_str( ((Sql_func_expr*)expr)->_name ), "last_day" ) == 0
             || stricmp( c_str( ((Sql_func_expr*)expr)->_name ), "current_date" ) == 0 )
            {
                // Das Funktionsregister muss um Ergebnistypen erweitert werden!!
                Sos_ptr<Text_type> t = SOS_NEW( Text_type( 8 ) );
                Sos_ptr<As_date_type> t2 = SOS_NEW( As_date_type( +t, "yyyymmdd" ) );
                expr->_type = +t2;
            } 
            else 
            if( stricmp( c_str( ((Sql_func_expr*)expr)->_name ), "to_timestamp" ) == 0 )
            {
                // Das Funktionsregister muss um Ergebnistypen erweitert werden!!
                Sos_ptr<Text_type> t = SOS_NEW( Text_type( 8+6 ) );
                Sos_ptr<As_date_time_type> t2 = SOS_NEW( As_date_time_type( +t, "yyyymmddHHMMSS" ) );
                expr->_type = +t2;
            } 
            else 
            {
                Sos_ptr<Text_type> t = SOS_NEW( Text_type( _max_field_length  ) );      // String0_type mit null_flag macht Ärger: Das Nullflag kommt in den Schlüssel, es wird von add_field nicht richtig berücksichtigt(?) ...
                expr->_type = +t;
            }
            break;
        }

        case op_aggregate_avg:
        case op_aggregate_count:
        case op_aggregate_max:
        case op_aggregate_min:
      //case op_aggregate_stddev:
        case op_aggregate_sum:
      //case op_aggregate_variance:
        case op_aggregate_groupby:
        {
            if( !obj_is_type( tc_Sql_select ) )  throw_xc( "SOS-SQL-13" );

            Sql_select*         select = ((Sql_select*)this);
            Sql_aggregate_expr* e      = (Sql_aggregate_expr*)expr;

            prepare_expr_with_operands( e, je_other );

            switch( e->_operator )
            {
                case op_aggregate_avg:          e->_type = &double_type;    break;
                case op_aggregate_count:        e->_type = &int_type;       break;
                case op_aggregate_max:          e->_type = e->_operand_array[0]->_type;  break;
                case op_aggregate_min:          e->_type = e->_operand_array[0]->_type;  break;
              //case op_aggregate_stddev:       
                case op_aggregate_sum:          e->_type = &double_type;    break;
              //case op_aggregate_variance:
                case op_aggregate_groupby:      e->_type = select->_aggregate._groupby_expr_array[e->_groupby_index]->_type;  break;
                default:;
            }

            select->_aggregate._aggregate_array.push_back( e );

            break;
        }

        default: throw_xc( "SQL-PREPARE-OP", expr->_operator );
    }

    expr->_prepared = true;
}

//--------------------------------------------------------------Sql_stmt::prepare_system_record

void Sql_stmt::prepare_system_record()
{
    _system_table = SOS_NEW( Sql_table );
    _system_table->_system = true;
    _system_table->_name   = "SYSTEM";
    _system_table->_record.allocate( sizeof (Sql_system_record) );
    _system_table->_base   = _system_table->_record.byte_ptr();

    new( _system_table->_base ) Sql_system_record;
    _system = (Sql_system_record*)_system_table->_base;
    _system_table->_table_type = _system->make_type();

    _table_array.add( _system_table );


    _system->_sys_date = Sos_date::today();
  //_system->_user = ??;

#   if NL_IS_CRLF
        _system->_sys_newline = "\r\n";
#    else
        _system->_sys_newline = "\n";
#   endif
    _system->_sys_cr = "\r";
}

//-----------------------------------------------------------------Sql_stmt::prepare_let_record

void Sql_stmt::prepare_let_record()
{
    int i;

    if( _let_names.count() == 0 )  return;

    for( i = 0; i <= _let_expr_array.last_index(); i++ )  prepare_expr( _let_expr_array[ i ] );

    _let_table = SOS_NEW( Sql_table );
    _let_table->_system = true;
    _let_table->_name   = "LET";

    _let_table->_table_type = Record_type::create();
    _let_table->_table_type->allocate_fields( _let_names.count() );

    for( i = 0; i <= _let_names.last_index(); i++ )
    {
        Sql_expr* expr = _let_expr_array[ i ];
        if( !expr->_type )  throw_xc( "sossql-LET", "Typ fehlt" );
        Sos_ptr<Field_descr> field = SOS_NEW( Field_descr( expr->_type,
                                                           c_str( _let_names[ i ] ) ) );
        field->add_to( _let_table->_table_type );
        if( !field->nullable()
         && expr->_operator != op_field  )  field->add_null_flag_to( _let_table->_table_type );
    }

    _let_table->_record.allocate( _let_table->_table_type->field_size() );
    _let_table->_base = _let_table->_record.byte_ptr();
    _let_table->_table_type->construct( _let_table->_base );

    _table_array.add( _let_table );
}

//---------------------------------------------------------------Sql_stmt::prepare_param_record

void Sql_stmt::prepare_param_record()
{
    _param_table = SOS_NEW( Sql_table );
    _param_table->_name = PARAM_TABLE_NAME;
  //_param_table->_record_type;        // bleibt offen bis bind_parameters()
  //_param_table->_record;             // bleibt offen bis bind_parameters()
    _param_table->_system = true;

    _table_array.add( _param_table );
  //_param_table_index = _table_array.last_index();
}

//-----------------------------------------------------------------------Sql_stmt::order_tables
// Ordnet die Tabellen in der Reihenfolge des geplanten Zugriffs.
// Tabellen so ordnen, dass
// für alle i, j mit i >= 0 && i < _table_count && j > i && j < _table_count gilt
// !_table_array[ i ]->_master_tables.is_elem( j )
// Mit der Methode ordered_table(int) wird auf die geordneten Tabellen zugegriffen.
// _table_array[] selbst wird nicht geordnet, weil sonst alle _table_index aktualisiert werden müssten.

void Sql_stmt::order_tables()
{
    Bool reordered = false;
    int  i, j, k;

    // Transitive Hülle:
    // i hat j als Mastertabelle: [i] -> [j]
    // [i] -> [j]  &&  [j] -> [k]  =>  [i] -> [k]

    for( i = 0; i < _table_array.count(); i++ ) 
    {
        Sql_table* table = _table_array[ i ];

        for( j = 0; j < _table_array.count(); j++ ) 
        {
            if( j != i  &&  table->_master_tables.is_elem( j ) ) 
            {
                LOG( "Sql_stmt::order_tables: " << _table_array[i]->simple_name() << " ist abhängig von " << _table_array[j]->simple_name() << '\n' );

                for( k = 0; k < _table_array.count(); k++ ) 
                {
                    if( k != j  &&  _table_array[ j ]->_master_tables.is_elem( k ) ) 
                    {
                        table->_master_tables.include( k );
                        LOG( "Sql_stmt::order_tables: " << _table_array[i]->simple_name() 
                            << " ist über "             << _table_array[j]->simple_name() 
                            << " abhängig von "         << _table_array[k]->simple_name() << '\n' );
                    }
                }
            }
        }
    }

    // _ordered_table ordnen, _table_array bleibt:

    for( i = 0; i < _table_count; i++ )   // i, j indizieren _table_order
    {
        int        t     = i;
        Sql_table* table = ordered_table( i );

        for( j = i + 1; j < _table_count; j++ ) {
            if( table->_master_tables.is_elem( _table_order[ j ] ) )  { t = j;  table = ordered_table( j ); }
        }
        
        if( t != i ) 
        {
          //if( table->_master_tables.is_elem( _table_order[ i ] ) )  throw_xc( "SOS-SQL-77", c_str( table->_name ), c_str( ordered_table( i )->_name ) );
            if( table->_master_tables.is_elem( _table_order[ i ] ) )
            {
                LOG( "Sql_stmt::order_tables: " << _table_array[i]->simple_name() << " und " << _table_array[t]->simple_name() 
                     << " sind voneinander abhängig. Die Rangfolge der FROM-Klausel entscheidet.\n" );
                table->_master_tables.exclude( _table_order[ i ] );
            }
            else
            {
                exchange( &_table_order[ i ], &_table_order[ t ] );
                reordered = true;
            }
        }
    }


    if( reordered )
    {
        Log_ptr log;

        *log << "Sql_stmt::order_tables: Die Tabellen der FROM-Klausel sind neu geordnet: FROM ";
        for( i = 0; i < _table_count; i++ ) {
            if( i > 0 )  *log << ", ";
            *log << ordered_table( i )->simple_name();
        }
        *log << endl;
    }
}

//-----------------------------------------------------------------------Sql_stmt::order_tables
/*
// Ordnet die Tabellen in der Reihenfolge des geplanten Zugriffs.
// Tabellen so ordnen, dass
// für alle i, j mit i >= 0 && i < _table_count && j > i && j < _table_count gilt
// !_table_array[ i ]->_master_tables.is_elem( j )
// Mit der Methode ordered_table(int) wird auf die geordneten Tabellen zugegriffen.
// _table_array[] selbst wird nicht geordnet, weil sonst alle _table_index aktualisiert werden müssten.

void Sql_stmt::order_tables()
{
    Bool reordered = false;
    int  i, j, k;

    // Transitive Hülle:
    // i hat j als Mastertabelle: [i] -> [j]
    // [i] -> [j]  &&  [j] -> [k]  =>  [i] -> [k]

    for( i = 0; i < _table_array.count(); i++ ) 
    {
        Sql_table* table = _table_array[ i ];

        for( j = 0; j < _table_array.count(); j++ ) 
        {
            if( j != i  &&  table->_master_tables.is_elem( j ) ) 
            {
                LOG( "Sql_stmt::order_tables: " << _table_array[i]->simple_name() << " ist abhängig von " << _table_array[j]->simple_name() << '\n' );

                for( k = 0; k < _table_array.count(); k++ ) 
                {
                    if( k != j  &&  _table_array[ j ]->_master_tables.is_elem( k ) ) 
                    {
                        table->_master_tables.include( k );
                        LOG( "Sql_stmt::order_tables: " << _table_array[i]->simple_name() 
                            << " ist über "             << _table_array[j]->simple_name() 
                            << " abhängig von "         << _table_array[k]->simple_name() << '\n' );
                    }
                }
            }
        }
    }


    // _ordered_table ordnen, _table_array bleibt:

    for( i = 0; i < _table_count; i++ )   // i, j indizieren _table_order
    {
        int        t     = i;
        Sql_table* table = ordered_table( i );

        for( j = i + 1; j < _table_count; j++ ) {
            if( table->_master_tables.is_elem( _table_order[ j ] ) )  { t = j;  table = ordered_table( j ); }
        }
        
        if( t != i ) 
        {
          //if( table->_master_tables.is_elem( _table_order[ i ] ) )  throw_xc( "SOS-SQL-77", c_str( table->_name ), c_str( ordered_table( i )->_name ) );
            if( table->_master_tables.is_elem( _table_order[ i ] ) )
            {
                LOG( "Sql_stmt::order_tables: " << _table_array[i]->simple_name() << " und " << _table_array[t]->simple_name() 
                     << " sind voneinander abhängig. Die Rangfolge der FROM-Klausel entscheidet.\n" );
                table->_master_tables.exclude( _table_order[ i ] );
            }
            else
            {
                exchange( &_table_order[ i ], &_table_order[ t ] );
                reordered = true;
            }
        }
    }


    // Prüfen, ob keine Schleife ist:
    //for( i = 0; i < _table_array.count(); i++ ) {
    //    Sql_table* table = _table_array[ i ];
    //    for( j = i; j < _table_array.count(); j++ ) {
    //        if( table->_master_tables.is_elem( j ) ) {
    //            throw_xc( "SOS-SQL-77", c_str( table->_name ), c_str( _table_array[ j ]->_name ) );
    //        }
    //    }
    //}

    if( reordered  &&  log_ptr ) {
        *log_ptr << "Sql_stmt::order_tables: Die Tabellen der FROM-Klausel sind neu geordnet: FROM ";
        for( i = 0; i < _table_count; i++ ) {
            if( i > 0 )  *log_ptr << ", ";
            *log_ptr << ordered_table( i )->simple_name();
        }
        *log_ptr << '\n';
    }
}
*/
//------------------------------------------------------------------Sql_stmt::print_access_plan
/*
void Sql_stmt::print_access_plan( ostream* s )
{
    for( int i = 0; i < _table_count; i++ )
    {
        Sql_table* table = _table_array[ i ];
        *s << "Tabelle " << table->_name << ": ";

        if( table->_key_join ) {
            // if( table->_complete_key_join )  *s << "Direkter Zugriff ..." ;
            *s << "Zugriff über den Satzschlüssel";
        } else {
            *s << "wird vollständig gelesen";
        }

        *s << '\n';
    }
}
*/
//---------------------------------------------------------------------Sql_stmt::prepare_tables

void Sql_stmt::prepare_tables()
{
    Bool rowid_complete = true;
    int  rowid_len      = 0;

    _table_order.last_index( _table_array.last_index() );

    for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
    {
        Sql_table* t = _table_array[ i ];
        t->_session = _session;
        t->_stmt = this;
        if( !t->_system )  t->prepare();
        if( !t->_key_len )  rowid_complete = false;
        rowid_len += t->_key_len;

        _table_order[ i ] = i;        // order_tables() verändert die Reihenfolge!
    }

    if( rowid_complete ) {
        _rowid_len = rowid_len;
        _rowid.allocate_min( rowid_len );
    }
}

//------------------------------------------------------------------------------Sql_stmt::close

void Sql_stmt::close()
{
    for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
    {
        _table_array[ i ]->close();
    }

    //_error_log.sync();
    _error_log_file.close();
}

//-----------------------------------------------------------------------Sql_stmt::prepare_loop

void Sql_stmt::prepare_loop( const char* name, Sql_loop_clause* loop )
{
    if( loop ) {
        _system_table->_table_type->field_descr_ptr( name )->name( loop->_name );  // Index umbenennen
        prepare_expr( loop->_begin );
        prepare_expr( loop->_end );
    }
}

//----------------------------------------------------------------------------Sql_stmt::prepare

void Sql_stmt::prepare()
{
    sos_sql_register_functions();

#   if defined SOSSQL_OLE
        for( int i = 0; i <= _object_array.last_index(); i++ )  _object_array[ i ]->prepare();
#   endif

    _hilfspuffer.allocate_min( max_display_field_size + 1 );
    prepare_tables();
    prepare_system_record();                // Einige Systemfelder
  //prepare_let_record();   // LET erst vorbereiten, wenn Name der LOOP-Variablen gesetzt. S. Sql_select::prepare()                // LET-Variablen
    prepare_param_record();                 // Parameterfelder für bind_parameters()
    if( _loop_before_where )  prepare_loop( "sys_loop_index", _loop );  // Die LOOP-Klausel vor WHERE
    if( _where )  prepare_expr( _where, je_ground );
    if( _assert )  prepare_expr( _assert );
    
    if( _order_tables )  order_tables();


    ////jz 12.4.97:  open_tables() und _table_type aus execute() hergezogen.
    //jz 7.5.97  open_tables();
    //jz 7.5.97  Sql_table::_file._key_len ist jetzt definiert für prepare_where

    if( _param_count && !_param_table->_table_type ) { // throw_xc( "SOS-SQL-57" );
        _param_table->_table_type = Record_type::create();      // leer
    }
}

//------------------------------------------------------------------Sql_stmt::bind_parameters

void Sql_stmt::bind_parameters( const Record_type* type, const Byte* base  )
{
    // type == 0  ==>  Parameterbindung aufheben

    _param_callers_type = (Record_type*)type;
    _param_callers_base = base;

    _param_table->_table_type = (Record_type*)type;
    _param_table->_base       = (Byte*)base;
}

//------------------------------------------------------------------------Sql_stmt::open_tables

void Sql_stmt::open_tables()
{
    for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
    {
        _table_array[ i ]->open();
    }
}

//----------------------------------------------------------------------------Sql_stmt::execute

void Sql_stmt::execute()
{
    open_tables();  

    if( _table_count > 0 )  ordered_table( 0 )->prepare_where( _where );  // Schlüsselintervalle der ersten Tabelle errechnen

    for( int i = _table_array.first_index(); i <= _table_array.last_index(); i++ )
    {
        _table_array[ i ]->rewind();
    }

    _row_count = -1;        // "nicht verfügbar"
    _constant_select_fetched = false;
}

//----------------------------------------------------------------------Sql_stmt::eval_function
/*
Dyn_obj Sql_stmt::eval_function( Sql_expr_with_operands* e )
{
    ZERO_RETURN_VALUE( Dyn_obj );

    Dyn_obj o;
    eval_function( &o, e );
    return o;
}
*/
//----------------------------------------------------------------------Sql_stmt::eval_function

void Sql_stmt::eval_function( Dyn_obj* result, Sql_func_expr* e )
{
    if( stricmp( c_str( e->_name ), "warning" ) == 0 ) {
        _error_log << eval_expr( e->_operand_array[ 1 ] ) << '\n';    // 2. Parameter: Warntext
        eval_expr( result, e->_operand_array[ 0 ] );
    }
    else
    if( stricmp( c_str( e->_name ), "ifnull" ) == 0 ) {
        eval_expr( result, e->_operand_array[ 0 ] );
        if( result->null() )  eval_expr( result, e->_operand_array[ 1 ] );
    }
    else
    {
        Sos_simple_array<Dyn_obj> param_array;
        param_array.obj_const_name( "Sql_stmt::eval_function" );
        param_array.first_index( 1 );
        param_array.last_index( e->_operand_array.count() );

        for( int i = 0; i <= e->_operand_array.last_index(); i++ ) {
            eval_expr( &param_array[ i+1 ], e->_operand_array[ i ] );
        }

#       if defined SOSSQL_OLE
        if( e->_method ) {     // OLE-Methode?
            e->_method->invoke( result, param_array );
        }
        else
#       endif
        {
            // Funktion aus dem Funktionsregister
            (*e->_func_descr->_func)( result, param_array );
        }
    }
}

//-----------------------------------------------------------------Sql_stmt::put_false_cond_log
/*
void Sql_stmt::put_false_cond_log( Sql_expr_with_operands* expr )
{
    Dynamic_area satz ( 1000 );
    ostrstream   s    ( satz.char_ptr(), satz.size() );

    s << "Bedingung ist nicht erfüllt: " << *expr;

    satz.length( s.pcount() );
    _error_log.put( satz );
}
*/
//--------------------------------------------------------------------------Sql_stmt::eval_expr

Dyn_obj Sql_stmt::eval_expr( Sql_expr* expr )
{
    ZERO_RETURN_VALUE( Dyn_obj );

    Dyn_obj o;
    _xc_expr_inserted = false;
    eval_expr( &o, expr );
    return o;
}

//-------------------------------------------------------------------------Sql_stmt::eval_field

void Sql_stmt::eval_field( Dyn_obj* result, Sql_field_expr* e )
{
    if( !e->_field_descr  ||  e->_table->_null_values_for_outer_join_read ) 
    {
        result->set_null();
    } 
    else 
    {
        if( !e->_index_array.count() ) 
        {
            //? if( !e->_table->_base )  throw_xc( "SOS-SQL-94", e->obj_as_string() );
            result->assign( e->_field_descr, e->_table->_base );
            //LOG( e->_name << "=" << o << "  null=" << o.null() << "\n" );
        } 
        else 
        {
            int index = eval_expr_as_int( e->_index_array[ 0 ] );
            Array_field_descr* a = SOS_CAST( Array_field_descr, e->_field_descr );
            result->assign( a->type_ptr(), e->_table->_base + a->elem_offset( index ) );
        }
    }
}

//-------------------------------------------------------------------------Sql_stmt::eval_rowid

void Sql_stmt::eval_rowid( Dyn_obj* result, Sql_rowid_expr* e )
{
    Sql_table*      table = e->_table;
    Const_area      key;

    if( table->_key_in_record ) {
        key = Const_area( table->_base + table->_key_pos, table->_key_len );
    }
    else
    if( table->_key_len ) {
        key = table->_file.current_key();
    }
    else throw_xc( "SOS-SQL-61", c_str( e->_table_name ), e->_pos );

    result->alloc( e->_type );
    ostrstream s ( (char*)result->ptr(), e->_type->field_size() );
    s << hex << key;
}

//-------------------------------------------------------------------------Sql_stmt::eval_param

void Sql_stmt::eval_param( Dyn_obj* result, Sql_param_expr* e )
{
    Record_type*    t = _param_table->_table_type;

    if( e->_index < t->field_count() ) {
        Field_descr* f = t->field_descr_ptr( e->_index );
        result->assign( f, _param_table->_base );
    }
    else *result = null_dyn_obj;
}

//------------------------------------------------------------------------Sql_stmt::eval_negate

void Sql_stmt::eval_negate( Dyn_obj* result, Sql_expr_with_operands* e )
{
    Dyn_obj a;

    eval_expr( &a, e->_operand_array[ 0 ] );  if( a.null() )  { result->set_null();  return; }

    *result = -a;
}

//---------------------------------------------------------------------------Sql_stmt::eval_add

void Sql_stmt::eval_add( Dyn_obj* result, Sql_expr_with_operands* e )
{
    Dyn_obj a, b;

    eval_expr( &a, e->_operand_array[ 0 ] );  if( a.null() )  { result->set_null();  return; }

    for( int i = 1; i <= e->_operand_array.last_index(); i++ )
    {
        eval_expr( &b, e->_operand_array[ i ] );  if( b.null() )  { result->set_null();  return; }

        //if( _log_eval )  LOG( "sossql3: " << *a.type() << '(' << a << ") " << sql_op_text[ e->_operator ] << ' ' << *b.type() << '(' << b << ")" );

        switch( e->_operator )
        {
            case op_add     : a += b;  break;
            case op_subtract: a -= b;  break;
            case op_multiply: a *= b;  break;
            case op_divide  : a /= b;  break;
                    default : throw_xc( "Sql_stmt::eval_add-op" );
        }
    }

    *result = a;

    //if( _log_eval )  LOG( "  liefert " << *result << "\n" );
}

//------------------------------------------------------------------------Sql_stmt::eval_concat

void Sql_stmt::eval_concat( Dyn_obj* result, Sql_expr_with_operands* e )
{
    Dyn_obj         a;
    Dynamic_area    buffer;

    buffer.allocate_min( /*e->_type? e->_type->field_size() :*/ _max_field_length );
    buffer.length( 0 );

    for( int i = 0; i <= e->_operand_array.last_index(); i++ )
    {
        Sql_expr* e2 = e->_operand_array[ i ];

        if( e2->_operator == op_function 
         && stricmp( c_str( ((Sql_func_expr*)e2)->_name ), "tab" ) == 0 )        // Spezialfunktion TAB()
        {
            //if( ((Sql_func_expr*)e2)->_operand_array.count() != 1 )  throw_xc( "SOS-1311", "tab", 1 );
            
            eval_expr( &a, ((Sql_func_expr*)e2)->_operand_array[0] );

            if( !a.null() ) {
                int tab = as_int( a ) - 1;
                if( tab > buffer.length()  &&  tab <= _max_field_length ) {
                    int len = buffer.length();
                    buffer.length( tab );
                    memset( buffer.char_ptr() + len, ' ', tab - len );
                }
            }

        }
        else
        {
            eval_expr( &a, e2 );  
            Area rest ( buffer.char_ptr() + buffer.length(),
                        buffer.size() - buffer.length() );
            a.write_text( &rest );
            //LOG( "Sql_stmt::eval_concat rest=" << hex << rest << dec << '\n' );
            buffer.length( buffer.length() + rest.length() );
        }
    }

    result->assign( buffer.char_ptr(), buffer.length() );

    //*result = as_string( a ) + as_string( b );  break;
}

//------------------------------------------------------------------------Sql_stmt::eval_select

void Sql_stmt::eval_select( Dyn_obj* result, Sql_select_expr* e )
{
    //Dynamic_area buffer;

    e->_select._max_reads = _max_reads - _read_count;

    e->_select.execute();

    try {
        e->_select.fetch( &_hilfspuffer );
    }
    catch( const Eof_error& ) {
        *result = null_dyn_obj;
        return;
    }

    result->assign( e->_select._result_record_type->field_descr_ptr( 0 ), _hilfspuffer.ptr() );

    //LOGI( "eval_select_expr: try fetch  " );
    try {
        e->_select.fetch( &_hilfspuffer );
        throw_xc( "SOS-SQL-87", Msg_insertions(), e->_pos );
    }
    catch( const Eof_error& ) {}  // Gut so

    _read_count += e->_select._read_count;
}

//--------------------------------------------------------------------------Sql_stmt::eval_cond

void Sql_stmt::eval_cond( Dyn_obj* result, Sql_expr_with_operands* e )
{
    int i;
    for( i = 0; i < e->_operand_array.last_index(); i += 2 )
    {
        // e->_operand_array[ i   ]: Bedingung
        // e->_operand_array[ i+1 ]: then
        // e->_operand_array[ i+2 ]: else

        Bool cond, cond_null;
        eval_bool_expr( &cond, &cond_null, e->_operand_array[ i ] );  if( cond_null )  { result->set_null();  return; }

        if( cond ) {
            eval_expr( result, e->_operand_array[ i+1 ] );
            return;
        }
    }

    // Else:
    if( i == e->_operand_array.last_index() ) {
        eval_expr( result, e->_operand_array[ i ] );
    } else {
        throw_xc( "SOS-SQL-1003" ); //, e );
    }
}

//-------------------------------------------------------------------Sql_stmt::eval_func_decode

void Sql_stmt::eval_func_decode( Dyn_obj* result, Sql_expr_with_operands* e )
{
    Dyn_obj                 value;
    Dyn_obj                 v;

    eval_expr( &value, e->_operand_array[ 0 ] );
    if( value.null() )  { *result = null_dyn_obj; return; }

    int i;
    for( i = 1; i < e->_operand_array.last_index(); i += 2 )
    {
        // e->_operand_array[ i   ]: = wert
        // e->_operand_array[ i+1 ]: Ergebnis, wenn [i] == [0]

        eval_expr( &v, e->_operand_array[ i ] );
        if( !v.null()  &&  v == value ) {
            eval_expr( result, e->_operand_array[i+1] );
            return;
        }
    }

    if( i == e->_operand_array.last_index() ) {
        eval_expr( result, e->_operand_array[ i ] );
    } else {
        throw_xc( "SOS-SQL-1003" ); //, e );
        //put_false_cond_log( e );
        //*result = null_dyn_obj;     // Besser: throw Sql_suppress_result_record;
    }
}

//-------------------------------------------------------------------Sql_stmt::eval_func_field

void Sql_stmt::eval_func_field( Dyn_obj* result, Sql_expr_with_operands* e )
{
    Sos_string              table_name;
    Field_descr*            field_descr = NULL;
    Sql_table*              table       = NULL;
    Dyn_obj                 field_name_obj;

    if( e->_operand_array.count() != 1  &&  e->_operand_array.count() != 2 ) {
        throw_xc( "SOS-SQL-54", e->_pos, "field", 1, e->_operand_array.count() );
    }

    eval_expr( &field_name_obj, e->_operand_array[ 0 ] );
    field_name_obj.write_text( &_hilfspuffer );

    if( e->_operand_array.count() == 2 ) {
        table_name = eval_expr_as_string( e->_operand_array[ 1 ] );
    }

    Sos_string field_name = as_string( _hilfspuffer );
    Bool       error;
    int        table_index;

    get_field_descr( field_name, table_name, &field_descr, &table, &table_index, &error );
    if( error )  {            // Auch, wenn Name nicht eindeutig ist!?
        result->set_null();
    } else {
        if( field_descr->null( table->_base ) ) {
            result->set_null();
        } else {
            field_descr->write_text( table->_base, &_hilfspuffer );
            _hilfspuffer += '\0';
            *result = _hilfspuffer.char_ptr();  // Wird zu Dyn_obj. OPTIMIERBAR!
        }
    }
}

//------------------------------------------------------------Sql_stmt::eval_func_group_counter

void Sql_stmt::eval_func_group_counter( Dyn_obj* result, Sql_func_group_counter_expr* e )
{
    // Die Funktion group_counter(...) hat für jede Stelle, an der sie benutzt wird, einen Speicher,
    // in dem die letzten Operanden gemerkt werden.
    // Beim ersten Aufruf liefert die Funktion 1.
    // Beim folgenden Aufruf liefert sie 1, wenn die Parameterwerte sich ändern, sonst den
    // Nachfolger des vorangegangenen Funktionergebnisses.

    Dyn_obj                      value;
    int                          i      = 0;

    while( i <= e->_operand_array.last_index() ) {
        if( i > e->_last_value_array.last_index() )  goto CHANGED;
        eval_expr( &value, e->_operand_array[ i ] );
        if( value != e->_last_value_array[ i ] )  goto CHANGED;
        i++;
    }

    *result = ++e->_counter;
    return;

  CHANGED:
    if( e->_last_value_array.last_index() < i )  e->_last_value_array.add_empty();
    e->_last_value_array[ i ] = value;

    for( ; i <= e->_operand_array.last_index(); i++ ) {
        if( e->_last_value_array.last_index() < i )  e->_last_value_array.add_empty();
        eval_expr( &e->_last_value_array[ i ], e->_operand_array[ i ] );
    }

    e->_counter = 1;
    *result = e->_counter;
}

//--------------------------------------------------------------------------Sql_stmt::eval_expr

void Sql_stmt::eval_expr( Dyn_obj** result, Sql_expr* expr )
{
    if( expr->_computed ) {
        *result = &expr->_value;
    } else {
        eval_expr( *result, expr ); 
    }
}

//--------------------------------------------------------------------------Sql_stmt::eval_expr

void Sql_stmt::eval_expr( Dyn_obj* result, Sql_expr* expr )
{
    LOGI2( "sossql.eval_expr", Z_FUNCTION << " " << *expr << "  _computed=" << expr->_computed << '\n' );

    if( expr->_computed ) {
        *result = expr->_value;
        return;
    }

    try {
      //LOGI( "Sql_stmt::eval_expr " << expr->_operator << "\n" );

        switch( expr->_operator )
        {
            case op_negate:         eval_negate     ( result, (Sql_expr_with_operands*)expr );  break;
            case op_add:
            case op_subtract:
            case op_multiply:
            case op_divide:         eval_add        ( result, (Sql_expr_with_operands*)expr );  break;
            case op_concat:         eval_concat     ( result, (Sql_expr_with_operands*)expr );  break;
          //case op_const: *result = ((Sql_const_expr*)+expr)->_value;  break;
          //case op_array_element:
            case op_field:          eval_field      ( result, (Sql_field_expr*)expr );  break;
            case op_rowid:          eval_rowid      ( result, (Sql_rowid_expr*)expr );  break;
            case op_param:          eval_param      ( result, (Sql_param_expr*)expr );  break;
            case op_select:         eval_select     ( result, (Sql_select_expr*)expr        );  break;
            case op_cond:           eval_cond       ( result, (Sql_expr_with_operands*)expr );  break;
            case op_func_decode:    eval_func_decode( result, (Sql_expr_with_operands*)expr );  break;
            case op_func_field:     eval_func_field ( result, (Sql_expr_with_operands*)expr );  break;
            case op_func_group_counter: eval_func_group_counter( result, (Sql_func_group_counter_expr*)expr );  break;
            case op_function:       eval_function   ( result, (Sql_func_expr*)expr          );  break;

            case op_exists:
            case op_is_null:
            case op_lt:
            case op_le:
            case op_eq:
            case op_ne:
            case op_ge:
            case op_gt:
            case op_like:
            case op_regex_match:
          //case op_in:
          //case op_in_select:
            case op_between:
            case op_not:
            case op_and:
            case op_or:
            {
                Bool bool_result, null;
                eval_bool_expr( &bool_result, &null, expr );
                if( null )  result->set_null();
                      else  *result = bool_result;
                break;
            }

            case op_aggregate_avg:
            case op_aggregate_count:
            case op_aggregate_max:
            case op_aggregate_min:
          //case op_aggregate_stddev:
            case op_aggregate_sum:
          //case op_aggregate_variance:
            {
                if( !obj_is_type( tc_Sql_select ) )  throw_xc( "SOS-SQL-13" );

                Sql_aggregate_expr* e = (Sql_aggregate_expr*)expr;
                result->assign( ((Sql_select*)this)->_aggregate._aggregate_record_type->field_descr_ptr( e->_aggregate_field_index ), 
                                ((Sql_select*)this)->_aggregate._current_aggregate_record );
                break;
            }

            case op_aggregate_groupby:
            {
                if( !obj_is_type( tc_Sql_select ) )  throw_xc( "SOS-SQL-13" );

                Sql_aggregate_expr* e = (Sql_aggregate_expr*)expr;
                result->assign( ((Sql_select*)this)->_aggregate._groupby_type->field_descr_ptr( e->_groupby_index ), 
                                ((Sql_select*)this)->_aggregate._current_groupby_record );
                break;
            }

            default: throw_xc( "Sql_stmt::eval_expr" ); //, e );
        }


        // Besser nur, wenn dieser Ausdruck in einem Join verwendet wird:
    /*
        int WERT_NUR_BEI_JOIN_MERKEN;
        expr->_null = result->null();
        if( !expr->_null )  {
            expr->_value = *result;
            expr->_value.write_text( &expr->_text );
            expr->_text.allocate_min( expr->_text.length() + 1 );      // '\0' hinten dran für _text.char_ptr() in throw_xc
            expr->_text.char_ptr()[ expr->_text.length() ] = '\0';
        }
    */
    }
    catch( Xc& x )
    {
        if( !_xc_expr_inserted ) {
            _xc_expr_inserted = true;

            if( expr->_operator == op_function
             && stricmp( c_str( ((Sql_func_expr*)expr)->_name ), "error" ) == 0 ) ;
            else {
                insert_table_keys( &x );
                x.insert( expr );
            }
        }

        _expr_error = true;

        throw;
    }
}

//---------------------------------------------------------------------Sql_stmt::eval_bool_expr

void Sql_stmt::eval_bool_expr( Bool* result, Bool* result_null, Sql_expr* expr )
{
    LOGI2( "sossql.eval_bool_expr", Z_FUNCTION << " " << *expr << "  _computed=" << expr->_computed << '\n' );

    if( expr->_computed ) {
        *result_null = expr->_null;
        if( !expr->_null ) {
            if( +expr->_value.type() == &bool_type )  *result = *(Bool*)expr->_value.ptr();
                                                else  *result = as_bool( expr->_value );
        }
        return;
    }

  try {
    _last_not_true_expr = NULL;            // Für Fehlermeldung

    switch( expr->_operator )
    {
        case op_lt:
        case op_le:
        case op_eq:
        case op_ne:
        case op_ge:
        case op_gt:
        case op_like:
        case op_between:
        case op_regex_match:
        {
            try {
                Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;

                Dyn_obj   a_;
                Dyn_obj*  a   = &a_;
                Sql_expr* op0 = e->_operand_array[ 0 ];

                eval_expr( &a, op0 ); 

                if( a->null() )  {
                    *result_null = true;
                }
                else
                if( expr->_operator == op_like )
                {
                    Sql_expr* e1 = e->_operand_array[ 1 ];
                    if( e1->_operator == op_const ) {
                        if( ((Sql_const_expr*)e1)->_null )  {
                            *result_null = true;
                        } else {
                            *result_null = false;
                            *result = sql_like( *a, ((Sql_const_expr*)e1)->_text, &_hilfspuffer );
                        }
                    } else {
                        Dyn_obj  b_;
                        Dyn_obj* b = &b_;
                        eval_expr( &b, e->_operand_array[ 1 ] );
                        if( b->null() )  {
                            *result_null = true;
                        } else {
                            *result_null = false;
                            *result = sql_like( *a, *b, &_hilfspuffer );
                        }
                    }
                }
                else
                if( expr->_operator == op_regex_match )
                {
                    Sql_regex_expr* e  = (Sql_regex_expr*)+expr;
                    Sql_expr*       e1 = e->_operand_array[ 1 ];
                    string          pattern;

                    if( e1->_operator == op_const ) {
                        if( ((Sql_const_expr*)e1)->_null )  {
                            *result_null = true;
                        } else {
                            *result_null = false;
                            pattern = as_string( ((Sql_const_expr*)e1)->_text );
                        }
                    } else {
                        Dyn_obj  b_;
                        Dyn_obj* b = &b_;
                        eval_expr( &b, e->_operand_array[ 1 ] );
                        if( b->null() )  {
                            *result_null = true;
                        } else {
                            *result_null = false;
                            pattern = as_string(*b);
                        }
                    }

                    try 
                    {
                        if( e->_pattern != pattern )  e->_pattern = pattern, e->_regex.compile( pattern );
                        *result = e->_regex.match( as_string( *a ) );
                    }
                    catch( const Xc&        ) { *result_null = true; }
                    catch( const exception& x ) { LOG(x<<'\n'); *result_null = true; }
                }
                else
                if( expr->_operator == op_between )
                {
                    *result_null = false;

                    Dyn_obj  b_;
                    Dyn_obj* b  = &b_;

                    eval_expr( &b, e->_operand_array[ 1 ] );

                    if( b->null() )  { *result_null = true; break; }

                    *result = *a >= *b;
                    if( *result ) {
                        Dyn_obj  c_;
                        Dyn_obj* c = &c_;
                        eval_expr( &c, e->_operand_array[ 2 ] );  if( c->null() )  { *result_null = true; break; }
                        *result = *a <= *c;
                    }
                    break;
                }
                else
                {
                    Dyn_obj          b_;
                    Dyn_obj*         b           = &b_;
                    Bool             res;
                    Sql_select_expr* select_expr = NULL;
                    int              i           = 1;   // if !select_expr
                  //Dynamic_area     buffer;            // if select_expr
                    Sql_expr*        op1         = e->_operand_array[ 1 ];

                    if( ( e->_any | e->_all )  &&  op1->_operator == op_select ) {
                        select_expr = (Sql_select_expr*)op1;
                        select_expr->_select.execute();
                    }

                    // Ergebnis, wenn die Liste zum Ende abgearbeitet (auch leer) ist:
                    *result_null = true;
                    *result      = e->_all;

                    while(1) {
                        if( select_expr ) {
                            try { select_expr->_select.fetch( &_hilfspuffer ); }
                            catch( const Eof_error& ) { break; }
                            b->assign( select_expr->_select._result_record_type->field_descr_ptr( 0 ), _hilfspuffer.ptr() );
                        } else {
                            if( i > e->_operand_array.last_index() )  break;
                            eval_expr( &b, e->_operand_array[ i++ ] );
                        }

                        if( !b->null() ) {
                            switch( expr->_operator )
                            {
                                case op_lt: res = *a <  *b;  break;
                                case op_le: res = *a <= *b;  break;
                                case op_eq: res = *a == *b;  break;
                                case op_ne: res = *a != *b;  break;
                                case op_ge: res = *a >= *b;  break;
                                case op_gt: res = *a >  *b;  break;
                                default   : throw_xc( "Sql_stmt::eval_bool_expr-op" );
                            }

                            LOG( "sossql3: " << *a->type()
                                 << '\'' << *a << "' 0x" << hex << *a << dec << ' '
                                 << sql_op_text[ expr->_operator ] << ' ' << *b->type() 
                                 << '\'' << *b << "' 0x" << hex << *b << dec 
                                 << " => " << res << '\n' );

                            *result_null = false;
                            if( !!res == !e->_all )  { *result = res; break; }
                        }
                    }
                }
            }
            catch( const Null_error& )
          //catch( const Xc& )  jz 13.4.97
            {
                *result_null = true;
            }

            if( _log_eval )  LOG( "  liefert " << *result << "\n" );

          //if( !*result_null &&  !*result )  _last_false_expr = expr;   // Für Fehlermeldung
            if( *result_null ||  !*result )  _last_not_true_expr = expr;   // Für Fehlermeldung
            break;
        }

        case op_exists:
        {
            Sql_select_expr*  select_expr =  (Sql_select_expr*) +((Sql_expr_with_operands*)expr)->_operand_array[ 0 ];

            *result_null = false;

            select_expr->_select.execute();

            try 
            { 
                select_expr->_select.fetch( &_hilfspuffer ); 
                *result = true;
            }
            catch( const Eof_error& ) 
            { 
                *result = false;
            }

            if( !*result )  _last_not_true_expr = expr;   // Für Fehlermeldung
            break;
        }

        case op_not:
        {
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;
            Bool                    r;

            eval_bool_expr( &r, result_null, e->_operand_array[ 0 ] );
            *result = !r;
            if( !*result )  _last_not_true_expr = expr;   // Für Fehlermeldung

            break;
        }

        case op_is_null:
        {
            Sql_expr_with_operands* e       = (Sql_expr_with_operands*)expr;
            Sql_expr*               operand = e->_operand_array[ 0 ];

            *result_null = false;

            if( operand->_operator == op_field )
            {
                Sql_field_expr* fe = (Sql_field_expr*)operand;

                if( !fe->_field_descr  ||  fe->_table->_null_values_for_outer_join_read ) {
                    *result = true;
                }
                else
                if( !fe->_index_array.count() ) {
                    *result = fe->_field_descr->null( fe->_table->_base );
                    LOG( "sossql3: " << fe->_field_descr->name() << ' ' << fe->_field_descr->type() 
                         << '\'' << as_string( fe->_field_descr, fe->_table->_base ) << "'"
                         " 0x" << hex << Const_area( fe->_field_descr->ptr(fe->_table->_base), fe->_field_descr->type_ptr()->field_size() ) << dec << " IS NULL => " << *result << '\n' );
                }
                else {
                    int index = eval_expr_as_int( fe->_index_array[ 0 ] );
                    Array_field_descr* a = SOS_CAST( Array_field_descr, fe->_field_descr );
                    *result = a->type_ptr()->null( fe->_table->_base + a->elem_offset( index ) );
                }
            } 
            else
            {
                Dyn_obj r;
                eval_expr( &r, operand );
                *result = r.null();
    
                LOG( "sossql3: " << r.type() << '\'' << r << "' 0x" << hex << r << dec << " IS NULL => " << *result << '\n' );
            }

            if( !*result )  _last_not_true_expr = expr;   // Für Fehlermeldung

            break;
        }

        case op_and:
        {
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;

            *result_null = true;
            *result      = true;

            for( int i = 0; i <= e->_operand_array.last_index(); i++ ) {
                Bool r, null;
                eval_bool_expr( &r, &null, e->_operand_array[ i ] );
                if( !null ) {
                    *result_null = false;
                    if( !r )  { *result = false;  break; }
                }
            }

            break;
        }

        case op_or:
        {
            // Was, wenn alle Operanden nichtig sind? jetzt: false
            Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;

            *result_null = true;
            *result      = false;

            for( int i = 0; i <= e->_operand_array.last_index(); i++ )
            {
                Bool r, null;
                eval_bool_expr( &r, &null, e->_operand_array[ i ] );
                if( !null ) {
                    *result_null = false;
                    if( r ) { *result = true;  break; }
                }
            }

            break;
        }

        default: {                              // Vorsicht, Rekursion!!
            Dyn_obj r;
            eval_expr( &r, expr );
            if( r.null() )  *result_null = true;
                      else { *result_null = false; *result = as_bool( r ); }
          //if( !*result_null &&  !*result )  _last_false_expr = expr;   // Für Fehlermeldung
            if( *result_null  ||  !*result )  _last_not_true_expr = expr;   // Für Fehlermeldung
            break;
        }
    }
  }
  catch( Xc& x )
  {
      if( !_xc_expr_inserted ) {
          _xc_expr_inserted = true;
          insert_table_keys( &x );
          x.insert( expr );
      }

      _expr_error = true;
      throw;
  }
}

//-------------------------------------------------------------------Sql_stmt::eval_expr_as_int

int Sql_stmt::eval_expr_as_int( Sql_expr* expr )
{
    // Wegen BC 4.53 Destruktor-Aufwand

    Dyn_obj result;
    _xc_expr_inserted = false;
    eval_expr( &result, expr );
    return as_int( result );
}

//------------------------------------------------------------------Sql_stmt::eval_expr_as_long

long Sql_stmt::eval_expr_as_long( Sql_expr* expr )
{
    // Wegen BC 4.53 Destruktor-Aufwand

    Dyn_obj result;
    _xc_expr_inserted = false;
    eval_expr( &result, expr );
    return as_long( result );
}

//------------------------------------------------------------------Sql_stmt::eval_expr_as_bool

Bool Sql_stmt::eval_expr_as_bool( Sql_expr* expr, Bool deflt )
{
    // Wegen BC 4.53 Destruktor-Aufwand
    Bool result, null;
    _xc_expr_inserted = false;
    eval_bool_expr( &result, &null, expr );
    return null? deflt : result;
}

//----------------------------------------------------------------Sql_stmt::eval_expr_as_string

Sos_string Sql_stmt::eval_expr_as_string( Sql_expr* expr )
{
    // Wegen BC 4.53 Destruktor-Aufwand

    Dyn_obj result;
    _xc_expr_inserted = false;
    eval_expr( &result, expr );
    return as_string( result );
}

//-------------------------------------------------------------Sql_stmt::start_next_table_fetch

void Sql_stmt::start_next_table_fetch()
{
    _fetch_table_index++;

    for( int i = _fetch_table_index; i < _table_count; i++ )  // Nachrangige Tabellen
    {
        Sql_table* t = ordered_table( i );
        t->_null_values_for_outer_join_read = false;
        t->_record_accepted = false;
    }

    Sql_table* table = ordered_table( _fetch_table_index );

    table->_record_read = false;                        // Für outer join
    table->_join_expr_accepted = false;

    // Key_intervalls aufbauen:
    table->prepare_where( _where );
/*jz 7.12.97
    table->_key_intervalle.clear();
    if( _where ) {
        table->add_key_intervalls( &table->_key_intervalle, _where );
        if( log_ptr && table->_key_intervalle.count() ) {
            *log_ptr << "Key_intervall für " << *table << " (Join): ";
            table->print_intervalls( log_ptr );  //LOG( table->_key_intervalle );
            *log_ptr << '\n';
        }
    }
*/
    table->rewind();
}

//-----------------------------------------------------------------------------Sql_stmt::fetch3

void Sql_stmt::fetch3()
{
    // _fetch_table_index > 0 !
    // Hält den try-Block aus Sql_stmt::fetch heraus

    while(1)
    {
        Sql_table* table = ordered_table( _fetch_table_index );

        try {
            if( table->_null_values_for_outer_join_read )  throw_eof_error();

            while(1)
            {
                table->get();
                table->_record_read = true;

                if( !table->_join_expr )  break;
                if( eval_expr_as_bool( table->_join_expr, false ) ) {
                    table->_join_expr_accepted = true;
                    break;
                }
            }

            if( _fetch_table_index + 1 == _table_count )  break;  // Select-Satz vollständig
            start_next_table_fetch();
        }
        catch( const Eof_error& )
        {
            // eof bei nachrangiger Tabelle ==> vorrangige Tabelle weiterlesen
            table->_record_read = false;

            if( !table->_record_accepted  &&  table->_outer_join )  // WHERE-Klausel für keinen Satz erfüllt und Outer Join?
            {
                if( ( table->_outer_join & oj_log )  &&  !table->_join_expr_accepted )
                {
                    //put_empty_join_log( table );

/*jz 10.7.97
                    int  i = -1;
                    while(1) {
                        i = table->_master_tables.scan( i + 1, 1 );     // Tabellen, von denen diese Tabelle abhängt
                        if( i == -1 )  break;
                        ordered_table( i )->_record_accepted = true;
                    }
*/
                    for( int i = 0; i <= _fetch_table_index; i++ )      //jz 10.7.97 Alle vorrangigen Tabellen
                    {
                        Sql_table* t = ordered_table( i );
                        t->_record_accepted = true;
                    }

                    Xc x = "SOS-SQL-1002";
                    insert_table_keys( &x );   // Schlüsselwerte der vorrangigen Tabellen

                    if( table->_key_intervalle.count() ) {
                        Dynamic_area buffer;
                        {
                            Area_stream s ( &buffer );
                            table->print_intervalls( &s );
                        }
                        x.insert( buffer );
                    } else {
                        x.insert( table->_join_expr );
                    }

                    x.insert( table->_name );

                    log_error( x );

                    // 2.1.97:
                    if( _fetch_table_index == 0 )  throw;                 // Keine vorrangige Tabelle?

                    // Vorrangige Tabelle weiterlesen
                    _fetch_table_index--;
                }
                else
                {                                                   // Normaler Outer Join: NULL setzen
/*2.1.97
                    for( int i = _fetch_table_index; i < _table_count; i++ )
                    {
                        table = ordered_table( i );
                        table->_null_values_for_outer_join_read = true;
                        table->_record_read = true;
                    }
                    _fetch_table_index--;
                    return;
*/
                    table->_null_values_for_outer_join_read = true;
                    table->_record_read = true;
                    table->_record_accepted = true;

                    if( _fetch_table_index + 1 == _table_count )  break;  // Select-Satz vollständig
                    start_next_table_fetch();
                }
            }
            else
            {
                if( _fetch_table_index == 0 )  throw;                 // Keine vorrangige Tabelle?

                // Vorrangige Tabelle weiterlesen
                _fetch_table_index--;
            }
        }
    }
}

//------------------------------------------------------------------------------Sql_stmt::fetch

void Sql_stmt::fetch2()
{
    if( _table_count == 0 ) {           // kein FROM?
        // Genau einen Satz erzeugen
        if( _constant_select_fetched )  throw_eof_error();
        _constant_select_fetched = true;
    }
    else
    {
        if( _fetch_table_index == 0 ) {
            Sql_table* t = ordered_table( 0 );
            t->get();       // EOF durchlassen
            t->_record_read = true;
            if( _table_count > 1 )  start_next_table_fetch();
        }

        if( _fetch_table_index > 0 )  fetch3();
    }
}

//------------------------------------------------------------------------------Sql_stmt::fetch

void Sql_stmt::fetch()
{
    Bool try_next;

    do  // Solange WHERE-Klausel nicht erfüllt oder fehlerhaft ist oder LOOP vor WHERE läuft
    {
        if( _loop_before_where  &&  _loop ) {  // LOOP vor WHERE, äußere Schleife
            _system->_sys_loop_index++;

            while( _system->_sys_loop_index > _system->_sys_loop_end )   // nach execute(): 1 und 0
            {
                Sql_stmt::fetch2();  // WHERE-Klausel wird hier geprüft
    
                _system->_sys_loop_index = eval_expr_as_long( _loop->_begin );
                _system->_sys_loop_end   = eval_expr_as_long( _loop->_end   );
            }
        } else {
            Sql_stmt::fetch2();  // WHERE-Klausel wird hier geprüft
        }

        try_next = false;

        try {
            if( !_where  ||  eval_expr_as_bool( _where, false ) ) {    // WHERE-Klausel erfüllt?
                for( int i = 1; i < _table_count; i++ )  ordered_table( i )->_record_accepted = true;
                if( _assert  &&  !eval_expr_as_bool( _assert, false ) ) {  //put_assert_error_log();
                    Xc x ( "SOS-SQL-1001" );
                    insert_table_keys( &x );
                    x.insert( _last_not_true_expr? +_last_not_true_expr : +_assert );
                    log_error( x );
                    try_next = true;  // ASSERT-Klausel nicht erfüllt, weiterlesen
                } 
            } else {
                try_next = true;   // WHERE-Klausel nicht erfüllt, weiterlesen
            }
        }
        catch( const Xc& x )
        {
            log_error( x );
            try_next = true;
        }
    }
    while( try_next );

    _system->_sys_record_no++;    // ungültig, wenn Sql_select::get_key() o.ä. zwischendurch gerufen
}

//-------------------------------------------------------------------Sql_stmt::print_table_keys
/* Gibt alle angesprochenen Schlüsselfelder bzw. Satznummern der
   Tabellen im table_set aus.
*/

void Sql_stmt::print_table_keys( ostream* s, const Sql_table_set& table_set )
{
    int  i = -1;
    Bool need_comma = false;

    while(1)
    {
        i = table_set.scan( i + 1, 1 );
        if( i == -1 )  break;

        if( need_comma )  *s << ", ";
        need_comma = true;

        Sql_table* t = _table_array[ i ];

        if( t->_key_in_record ) {
            // Alle selektierten Schlüsselfelder ausgeben:
            for( int j = 0; j < t->_selected_type->field_count(); j++ )
            {
                Field_descr* f = t->_selected_type->field_descr_ptr( j );
                if( f->offset() >= t->_key_pos
                 && f->offset() + f->type().field_size() <= t->_key_pos + t->_key_len )
                {
                    *s << t->_name << '.' << f->name() << " = ";
                    f->print( t->_base, s, std_text_format, '\'', '\'' );
                }
            }
        } else {
            *s << t->_get_count << ". Satz der Tabelle " << t->_name;
        }
    }
}

//------------------------------------------------------------------Sql_stmt::insert_table_keys

void Sql_stmt::insert_table_keys( Xc* x )
{
  //Sql_table_set table_set;    table_set.include( 0, _table_index );
    Dynamic_area  text          ( 256 );
    ostrstream    s             ( text.char_ptr(), text.size() );

    if( _table_count )  print_table_keys( &s, ordered_table( _fetch_table_index )->_master_tables );
                  else  x->insert( "" );

    x->insert( text.char_ptr(), s.pcount() );
}

//----------------------------------------------------------------------Sql_stmt::log_error

void Sql_stmt::log_error( const Xc& x )
{
    if( !_error_log_opened )  throw x;

    _error_log << x << '\n' << flush;
}


} //namespace sos
