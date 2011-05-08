#include "precomp.h"

//#define MODULE_NAME "sossql2"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

/*
    PARSER

    Implementierte Klassen:

    Sql_parser
*/

#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosarray.h"
#include "../kram/sosfield.h"
#include "../file/anyfile.h"
#include "../kram/msec.h"
#include "../kram/soslimtx.h"
#include "../kram/sossql2.h"

using namespace std;
namespace sos {

//-----------------------------------------------------------------Sql_parser::parse_table_name

Sos_ptr<Sql_table> Sql_parser::parse_table_name()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_table> );

  //Sos_ptr<Sql_table> t = _static->table( name );
    Sos_ptr<Sql_table> t;

    t = SOS_NEW( Sql_table );
// /*  Ein vollständiger Dateiname kann nur mit "|" angegeben werden (nur einer)
    if( next_token_is( k_string ) )
    {
        Sos_string filename = parse_string();
        //if( strncmp( c_str( filename ), "com|", 4 ) != 0 )  t->_name = "com|";
        t->_name = filename;
        t->_full_file_name = true;
    }
    else
//*/
    {
        t->_name = parse_identifier();
        if( next_token_is( k_punkt ) ) {
            t->_user_name = t->_name;
            parse_token();
            t->_name = parse_identifier();
        }
    }

    if( next_token_is( k_identifier ) )  t->_alias = parse_identifier();

    return t;
}

//----------------------------------------------------------------Sql_parser::parse_loop_clause

Sos_ptr<Sql_loop_clause> Sql_parser::parse_loop_clause()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_loop_clause> );

    Sos_ptr<Sql_loop_clause> loop;

    loop = SOS_NEW( Sql_loop_clause );

    parse( k_loop );
    loop->_name = parse_identifier();

    if( next_token_is( k_between ) ) {
        parse_token();
        loop->_begin = parse_arith_expr();
        parse( k_and );
        loop->_end = parse_arith_expr();
    }
    else
    {
        parse( k_in );
        parse( k_klammer_auf );
        while(1) {
            loop->_value_list.add( parse_const_expr() );
            if( !next_token_is( k_comma ) )  break;
            parse_token();
        }
        parse( k_klammer_zu );
    }

    return loop;
}

//------------------------------------------------------------------------Sql_parser::parse_let

void Sql_parser::parse_let( Sql_select* select )
{
    while(1) {
        select->_let_names.add( parse_identifier() );
        parse( k_eq );
        select->_let_expr_array.add( parse_expr() );

        if( !next_token_is( k_comma ) )  break;
        parse_token();
    }
}

//---------------------------------------------------------------------Sql_parser::parse_select

void Sql_parser::parse_select( Sql_select* select )
{
/*
    Für jedes im Select angegebenes Ergebnisfeld die Rechenvorschrift speichern und den
    Feldtyp ermitteln.
*/
    parse( k_select );

    if( next_token_is( k_all ) )  parse_token();
    else
    if( next_token_is( k_distinct ) ) {
        parse_token();
        select->_distinct = true;
    }

    while(1) {
        if( next_token_is( k_star ) ) 
        {
            parse_token();
            select->_result_expr_array.add( SOS_NEW( Sql_expr( op_select_star ) ) );     // SELECT *
            select->_result_name_array.add( "*" );
        } 
        else 
        {
            Sos_ptr<Sql_expr> expr = parse_expr();
            select->_result_expr_array.add( expr );

            if( next_token_is( k_as )  || next_token_is( k_identifier ) ) 
            {
                if( next_token_is( k_as ) )  parse_token();
                select->_result_name_array.add( parse_identifier() );
            } 
            else 
            {
                if( expr->_operator == op_field  &&  ((Sql_field_expr*)+expr)->_name == "*" )   // tabelle.* ?
                {
                    ((Sql_field_expr*)+expr)->_resolve_star = true;
                    select->_resolve_star = true;
                    // tabelle.* wird von sossqlfl.cxx dem Aufrufer gegenüber in die einzelnen Felder aufgelöst.
                }

                select->_result_name_array.add( empty_string );
            }
        }

        if( !next_token_is( k_comma ) )  break;
        parse_token();
    }


    if( next_token_is( k_from ) ) {
        parse_token();
        assert( select->_table_array.count() == 0 );
        while(1) {
            Sos_ptr<Sql_table> table = parse_table_name();
            select->_table_array.add( table );
            table->_index = select->_table_array.last_index();
            select->_table_count++;

            if( !next_token_is( k_comma ) )  break;
            parse_token();
        }
    }

    if( next_token_is( k_loop ) ) {
        select->_loop_before_where = true;
        select->_loop = parse_loop_clause();  // LOOP vor WHERE, äußere Schleife
    }

    if( next_token_is( k_where ) ) {
        parse_token();
        select->_where = parse_bool_expr();
    }

    if( next_token_is( k_assert ) ) {
        _need_error_log = true;
        parse_token();
        select->_assert = parse_bool_expr();
    }

    if( next_token_is( k_loop ) ) {
        if( select->_loop )  throw_xc( "SOS-SQL-86", "LOOP" );
        select->_loop = parse_loop_clause();  // LOOP nach WHERE, innere Schleife
    }

    if( next_token_is( k_let ) ) {
        parse_token();
        parse_let( select );
    }

    if( next_token_is( k_group ) )
    {
        parse_token();
        parse( k_by );

        int i = 0;
        while(1) 
        {
            if( i >= max_groupby_params )  throw_xc( "SOS-SQL-65", max_groupby_params );

            select->_aggregate._groupby_expr_array.add( parse_expr() );

            i++;
            if( !next_token_is( k_comma ) )  break;
            parse_token();
        }
    }

    if( next_token_is( k_having ) )
    {
        parse_token();
        select->_aggregate._having = parse_bool_expr();
    }

    if( next_token_is( k_order ) )
    {
        parse_token();
        parse( k_by );

        int i = 0;

        while(1) {
            if( i >= max_orderby_params )  throw_xc( "SOS-SQL-65", max_orderby_params );

            if( next_token_is( k_number ) ) {
                if( !select->_result_expr_array.count() )  throw_xc( "SOS-SQL-63" );  // SELECT *
                uint n = parse_number();
                if( n < 1  ||  n > select->_result_expr_array.count() )  throw_xc( "SOS-SQL-62", n );
                select->_orderby._expr_array.add( select->_result_expr_array[ n - 1 ] );
            } else {
                select->_orderby._expr_array.add( parse_expr() );
            }

            Bool descending = false;
            if( next_token_is( k_asc ) ) {
                parse_token();
            } else
            if( next_token_is( k_desc ) ) {
                parse_token();
                descending = true;
            }
            select->_orderby._descending_array[ i ] = descending;

            i++;

            if( !next_token_is( k_comma ) )  break;
            parse_token();
        }
    }

    if( next_token_is( k_pipe ) ) {
        Sos_ptr<Sql_table> t;
        //if( select->_table_count )  throw_syntax_error( "SOS-SQL-16", _token._pos );
        t = SOS_NEW( Sql_table );
        t->_alias = "sys_pipe";
        t->_name = parse_rest();
        t->_full_file_name = true;
        select->_table_array.add( t );
        t->_index = select->_table_array.last_index();
        select->_table_count++;
    }

    select->_param_count = _param_count;


    // Falsche Reihenfolge der Klauseln?
    if( next_token_is( k_from )
     || next_token_is( k_where )
     || next_token_is( k_assert )
     || next_token_is( k_loop )
     || next_token_is( k_let )
     || next_token_is( k_group ) 
     || next_token_is( k_having ) 
     || next_token_is( k_order ) )
    {
        throw_syntax_error( "SOS-SQL-86", Token::repr( _token._kind ), _token._pos );
    }

}

//-------------------------------------------------------------------Sql_parser::parse_set_stmt
#if defined SOSSQL_OLE

Sos_ptr<Ole_object> Sql_parser::parse_set_stmt()
{
    Sos_ptr<Ole_object> object = SOS_NEW( Ole_object );

    parse( k_set );
    object->_name = parse_identifier();
    parse( k_eq );
    parse( k_createobject );
    parse( k_klammer_auf );
    object->_class_name = parse_string();
    parse( k_klammer_zu );
    parse( k_semikolon );

    return object;
}

#endif
//----------------------------------------------------------------Sql_parser::parse_select_stmt

void Sql_parser::parse_select_stmt( Sql_select* select, Bool check_eof )
{
    parse_select( select );

    if( check_eof )  {
        if( next_token_is( k_semikolon ) )  parse_token();
        parse( k_eof );
    } else {
        parse( k_semikolon );
    }
}

//----------------------------------------------------------------Sql_parser::parse_insert_stmt

void Sql_parser::parse_insert_stmt( Sql_insert* insert, Bool check_eof )
{
    parse( k_insert );
    parse( k_into );
    insert->_table_array.add( parse_table_name() );
    insert->_table_count = 1;

    parse( k_klammer_auf );

    while(1) {
        insert->_field_name_array.add( parse_identifier() );
        if( !next_token_is( k_comma ) )  break;
        parse_token();
    }

    parse( k_klammer_zu );

    //if( next_token_is( k_values ) )
    {
        parse( k_values );
        parse( k_klammer_auf );
        while(1) {
            insert->_values.add( parse_const_expr() );
            if( !next_token_is( k_comma ) )  break;
            parse_token();
        }
        parse( k_klammer_zu );
        if( insert->_values.count() != insert->_field_name_array.count() )  throw_syntax_error( "SOSSQL-field/value-count", _token._pos );
        insert->_param_count = _param_count;
    }
/*
    else {
        parse( k_select );
        insert->_select = parse_select( this );
    }
*/

    insert->_param_count = _param_count;

    if( check_eof )  {
        if( next_token_is( k_semikolon ) )  parse_token();
        parse( k_eof );
    } else {
        parse( k_semikolon );
    }
}

//----------------------------------------------------------------Sql_parser::parse_update_stmt

void Sql_parser::parse_update_stmt( Sql_update* update, Bool check_eof )
{
    parse( k_update );
    update->_table_array.add( parse_table_name() );
    update->_table_count = 1;

    parse( k_set );

    while(1) {
        update->_field_name_array.add( parse_identifier() );
        parse( k_eq );
        update->_values.add( parse_expr() );
        if( !next_token_is( k_comma ) )  break;
        parse_token();
    }

    if( next_token_is( k_where ) ) {
        parse_token();
        update->_where = parse_bool_expr();
    }

    if( next_token_is( k_assert ) ) {
        _need_error_log = true;
        parse_token();
        update->_assert = parse_bool_expr();
    }

    update->_param_count = _param_count;

    if( check_eof )  {
        if( next_token_is( k_semikolon ) )  parse_token();
        parse( k_eof );
    } else {
        parse( k_semikolon );
    }
}

//----------------------------------------------------------------Sql_parser::parse_delete_stmt

void Sql_parser::parse_delete_stmt( Sql_delete* stmt, Bool check_eof )
{
    parse( k_delete );
    parse( k_from );
    stmt->_table_array.add( parse_table_name() );
    stmt->_table_count = 1;

    if( next_token_is( k_where ) ) {
        parse_token();
        stmt->_where = parse_bool_expr();
    }

    if( next_token_is( k_assert ) ) {
        _need_error_log = true;
        parse_token();
        stmt->_assert = parse_bool_expr();
    }

    stmt->_param_count = _param_count;

    if( check_eof )  {
        if( next_token_is( k_semikolon ) )  parse_token();
        parse( k_eof );
    } else {
        parse( k_semikolon );
    }
}

} //namespace sos
