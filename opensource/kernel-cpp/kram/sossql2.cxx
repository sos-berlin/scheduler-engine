// $Id: sossql2.cxx 11394 2005-04-03 08:30:29Z jz $

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

//--------------------------------------------------------------------------------throw_xc_expr

void throw_xc_expr( const char* error, const Sql_expr* expr )
{
    Xc x ( error );
    x.insert( expr );
    x.pos( expr->_pos );
    throw x;
}

//-----------------------------------------------------------------Sql_parser::parse_const_expr

Sos_ptr<Sql_expr> Sql_parser::parse_const_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    return parse_value_expr();      // naja
}

//-------------------------------------------------------------Sql_parser::parse_parameter_list

void Sql_parser::parse_parameter_list( Sql_expr_with_operands* e, int min_par, int max_par )
{
    parse( k_klammer_auf );

    if( !next_token_is( k_klammer_zu ) )
    {
        e->_operand_array.add( parse_expr() );

        while( next_token_is( k_comma ) ) {
            parse( k_comma );
            e->_operand_array.add( parse_expr() );
        }
    }

    parse( k_klammer_zu );

    if( e->_operand_array.count() < min_par
     || max_par != -1  &&  e->_operand_array.count() > max_par )  throw_xc( "SOS-SQL-83" );
}

//----------------------------------------------------------------Sql_parser::parse_field_index

void Sql_parser::parse_field_index( Sql_field_expr* e )
{
    parse( k_eckklammer_auf );

    e->_index_array.add( parse_expr() );

    //while( next_token_is( k_comma ) ) {
    //    parse( k_comma );
    //    e->_operand_array.add( parse_expr() );
    //}

    parse( k_eckklammer_zu );
}

//----------------------------------------------------------------Sql_parser::parse_select_expr

Sos_ptr<Sql_expr> Sql_parser::parse_select_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_select_expr> e;
    e = SOS_NEW( Sql_select_expr );
    parse_select( &e->_select );

    return +e;
}

//-----------------------------------------------------------------Sql_parser::parse_value_expr

Sos_ptr<Sql_expr> Sql_parser::parse_value_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr> expr;
    Sql_operator      op;

    _expected_tokens_set.include( k_rowid );
    _expected_tokens_set.include( k_identifier );
    _expected_tokens_set.include( k_number );
    _expected_tokens_set.include( k_float );
    _expected_tokens_set.include( k_string );
    _expected_tokens_set.include( k_null );
    _expected_tokens_set.include( k_klammer_auf );
    _expected_tokens_set.include( k_question_mark );

    switch( next_token().kind() )
    {
        case k_identifier:
        case k_rowid:
        case k_star:
        {
            Token token [ 3 ];
            int   n     = 0;
            Bool  punkt = true;     // Vor dem ersten Token wird ein Punkt gedacht.

            while( n < 3  &&  next_token_is( k_identifier ) ) {
                punkt = false;
                token[ n++ ] = _token;
                parse_token();
                if( !next_token_is( k_punkt ) )  break;
                punkt = true;
                parse_token();
            }

            if( n >= 1  &&  n <= 2
             && next_token_is( k_klammer_auf ) )  // "[object.]name(...)" - Methode oder Funktion?
            {
                if( n == 1 ) {    // Reine Funktion (keine Methode)?
                    int          min_par = 0;
                    int          max_par = -1;
                    Sql_operator op_func = op_none;

                    if( stricmp( c_str( token[0]._name ), "field"  ) == 0 )  { op_func = op_func_field; min_par = 1; max_par = 2; }
                    else
                    if( stricmp( c_str( token[0]._name ), "decode" ) == 0 )  { op_func = op_func_decode; min_par = 3; max_par = -1; }

                    if( op_func != op_none ) {
                        Sos_ptr<Sql_expr_with_operands> e = SOS_NEW( Sql_expr_with_operands( op_func ) );
                        e->_pos = token[0]._pos;
                        parse_parameter_list( e, min_par, max_par );
                        expr = +e;
                        break;
                    }

                    if( stricmp( c_str( token[0]._name ), "group_counter" ) == 0 )
                    {
                        Sos_ptr<Sql_func_group_counter_expr> e = SOS_NEW( Sql_func_group_counter_expr() );
                        e->_pos = token[0]._pos;
                        parse_parameter_list( e, 1, -1 );
                        expr = +e;
                        break;
                    }
                }

                Sos_ptr<Sql_func_expr> e;
                e = SOS_NEW( Sql_func_expr( op_function ) );
                e->_pos  = token[ n-1 ]._pos;
                e->_name = token[ n-1 ]._name;
                if( n == 2 ) {
#                   if defined SOSSQL_OLE
                        e->_object_name = token[ 0 ]._name;
#                   else
                        throw_syntax_error( "SOS-1314", token[0]._pos );
#                   endif
                }
                parse_parameter_list( e, 0, -1 );
                expr = +e;
            }
            else
            if( n <= 1  &&  next_token_is( k_rowid ) ) {
                Sos_ptr<Sql_rowid_expr> e;
                e = SOS_NEW( Sql_rowid_expr );
                e->_pos = _token._pos;
                if( n == 1 )  e->_table_name = token[ 0 ]._name;
                parse_token();
                expr =  +e;

            }
            else
            {
                Sos_ptr<Sql_field_expr> e;
                e = SOS_NEW( Sql_field_expr );

                if( punkt && next_token_is( k_star ) ) {
                    token[ n++ ] = _token;
                    parse_token();
                    token[ n-1 ]._name = "*";       // Für get_field_descr() und parse_select()
                }

                e->_pos        = token[ n-1 ]._pos;
                e->_name       = token[ n-1 ]._name;
                if( n >= 2 )  e->_table_name = token[ n-2 ]._name;
              //if( n >= 3 )  e->_user_name  = token[ n-3 ];    // Gibt's nicht

                if( next_token_is( k_eckklammer_auf ) ) {       // Array-Element
                    parse_field_index( e );
                }

                if( next_token_is( k_plus_in_klammern ) ) {     // (+)
                    parse_token();
                    e->_outer_join = oj_outer_join;
                }
                else
                if( next_token_is( k_ausr_in_klammern ) ) {     // (!)
                    parse_token();
                    e->_outer_join = Sql_outer_join( oj_outer_join | oj_log );
                }

                expr = +e;
            }

            break;
        }

        case k_avg:         op = op_aggregate_avg;        goto AGGREGATE;
        case k_count:       op = op_aggregate_count;      goto AGGREGATE;
        case k_max:         op = op_aggregate_max;        goto AGGREGATE;
        case k_min:         op = op_aggregate_min;        goto AGGREGATE;
      //case k_stddev:      op = op_aggregate_stddev;     goto AGGREGATE;
        case k_sum:         op = op_aggregate_sum;        goto AGGREGATE;
      //case k_variance:    op = op_aggregate_variance;   goto AGGREGATE;
        {
AGGREGATE:
            Sos_ptr<Sql_aggregate_expr> e;
            e = SOS_NEW( Sql_aggregate_expr( op ) );
            e->_pos = _token._pos;

            parse_token();
            parse( k_klammer_auf );
            
          //if( next_token_is( k_all      ) )  parse_token();
          //else
          //if( next_token_is( k_distinct ) )  parse_token(), e->_distinct = true;

            if( op == op_aggregate_count  &&  next_token_is( k_star ) )     // count(*)
            {
                parse_token();
            }
            else
            {
                e->_operand_array.add( parse_expr() );      // SQLServer: "Aggregate functions and subqueries are not permitted."
            }

            parse( k_klammer_zu );

            expr = +e;
            break;
        }

        case k_number:
        {
            Sos_ptr<Sql_const_expr> e;
            e = Sql_const_expr::create();
            e->_pos = _token._pos;
            e->_value = parse_number();

            expr = +e;
            break;
        }
/*
        Decimal_type muss Skalierung gegeben werden!
        case k_decimal:
        {
            Sos_ptr<Sql_const_expr> e;
            e = Sql_const_expr::create();
            e->_pos = _token._pos;
            check_token( k_decimal );
            e->_value.assign( &decimal_type, &next_token()._decimal );
            parse_token();
            expr = +e;
            break;
        }
*/
        case k_float:
        {
            Sos_ptr<Sql_const_expr> e;
            e = Sql_const_expr::create();
            e->_pos = _token._pos;
            check_token( k_float );
            e->_value = next_token()._float;
            parse_token();

            expr = +e;
            break;
        }

        case k_string:
        {
            Sos_ptr<Sql_const_expr> e;
            e = SOS_NEW( Sql_const_expr );
            e->_pos = _token._pos;
            e->_value = parse_string();

            expr = +e;
            break;
        }

        case k_null:
        {
            parse_token();
            Sos_ptr<Sql_const_expr> e;
            e = Sql_const_expr::create();
            e->_pos = _token._pos;
            e->_null = true;

            expr = +e;
            break;
        }

        case k_minus:
        {
            parse_token();
            Sos_ptr<Sql_expr_with_operands> e;
            e = SOS_NEW( Sql_expr_with_operands( op_negate ) );
            e->_pos = _token._pos;
            e->_operand_array.add( parse_value_expr() );

            expr = +e;
            break;
        }

        case k_klammer_auf:
        {
            parse_token();

            if( next_token_is( k_select ) ) {
                expr = parse_select_expr();
            } else {
                Sos_ptr<Sql_expr> e;
                e = parse_expr();
                expr = e;
            }

            parse( k_klammer_zu );
            break;
        }

        case k_question_mark:
        {
            Sos_ptr<Sql_param_expr> e;
            e = SOS_NEW( Sql_param_expr );
            e->_pos  = _token._pos;
            e->_index = _param_count++;
            parse_token();

            expr = +e;
            break;
        }

        default:
            //throw_syntax_error( "SOS-SQL-12", _token._pos );
            check_token( k_identifier );  // Löst Exception aus
            return NULL;
    }


    if( next_token_is( k_is ) )
    {
        bool not_ = false;
        parse_token();

        if( next_token_is( k_not ) ) {
            parse_token();
            not_ = true;
        }
        parse( k_null );

        Sos_ptr<Sql_expr_with_operands> e2;
        e2 = SOS_NEW( Sql_expr_with_operands( op_is_null ) );
        e2->_operand_array.add( +expr );

        if( not_ ) {
            Sos_ptr<Sql_expr_with_operands> e3;
            e3 = SOS_NEW( Sql_expr_with_operands( op_not ) );
            e3->_operand_array.add( +e2 );
            return +e3;
        }

       return +e2;
    }

    return expr;
}

//-----------------------------------------------------------------Sql_parser::parse_arith_expr
/*
Sos_ptr<Sql_expr> Sql_parser::parse_arith_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr> expr;
    expr = parse_value_expr();
    if( !next_token_is( k_doppelstrich ) )  return expr;

    Sos_ptr<Sql_expr_with_operands> e;
    e = SOS_NEW( Sql_expr_with_operands( op_concat ) );
    e->_operand_array.add( expr );

    while(1) {
        parse_token();
        e->_operand_array.add( parse_value_expr() );
        if( !next_token_is( k_doppelstrich ) )  break;
    }

    return +e;
}
*/
//------------------------------------------------------------------Sql_parser::parse_mult_expr

Sos_ptr<Sql_expr> Sql_parser::parse_mult_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr>               expr;
    Sos_ptr<Sql_expr_with_operands> e;
    Sql_operator                    op;
    Sql_operator                    current_op = op_none;

    expr = parse_value_expr();

    while(1) {

        if( next_token_is( k_star  ) )  op = op_multiply;
        else
        if( next_token_is( k_slash ) )  op = op_divide;
        else return expr;

        parse_token();

        if( op == current_op ) {
            ((Sql_expr_with_operands*)+expr)->_operand_array.add( parse_value_expr() );   // expr op expr op expr zu einer Kette zusammenfassen
        } else {
            e = SOS_NEW( Sql_expr_with_operands( op ) );
            e->_operand_array.add( expr );
            e->_operand_array.add( parse_value_expr() );
            expr = +e;

            current_op = op;
        }
    }
}

//-------------------------------------------------------------------Sql_parser::parse_add_expr

Sos_ptr<Sql_expr> Sql_parser::parse_add_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr>               expr;
    Sos_ptr<Sql_expr_with_operands> e;
    Sql_operator                    op;
    Sql_operator                    current_op = op_none;

    expr = parse_mult_expr();

    while(1) {
        if( next_token_is( k_doppelstrich ) )  op = op_concat;
        else
        if( next_token_is( k_plus         ) )  op = op_add;
        else
        if( next_token_is( k_minus        ) )  op = op_subtract;
        else return expr;

        parse_token();

        if( op == current_op ) {
            ((Sql_expr_with_operands*)+expr)->_operand_array.add( parse_mult_expr() );   // expr op expr op expr zu einer Kette zusammenfassen
        } else {
            e = SOS_NEW( Sql_expr_with_operands( op ) );
            e->_operand_array.add( expr );
            e->_operand_array.add( parse_mult_expr() );
            expr = +e;

            current_op = op;
        }
    }
}

//-------------------------------------------------------------------Sql_parser::parse_cmp_expr

Sos_ptr<Sql_expr> Sql_parser::parse_cmp_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr>               sub_expr;
    Sos_ptr<Sql_expr>               expr;
    Sos_ptr<Sql_expr_with_operands> e;                      // == expr
    Sql_operator                    op          = op_none;
    bool                            any         = false;
    Sql_outer_join                  outer_join  = oj_none;
    bool                            not_        = false;
    bool                            right_join  = false;

    while( next_token_is( k_not ) ) {
        parse_token();
        not_ = !not_;
    }

    if( next_token_is( k_exists ) )
    {
        parse_token();
        e = SOS_NEW( Sql_expr_with_operands( op_exists ) );
        expr = +e;
        parse( k_klammer_auf );
        e->_operand_array.add( parse_select_expr() );
        parse( k_klammer_zu );
    }
    else
    {
        sub_expr = parse_add_expr();

        _expected_tokens_set.include( (int)k_lt );
        _expected_tokens_set.include( (int)k_le );
        _expected_tokens_set.include( (int)k_eq );
        _expected_tokens_set.include( (int)k_ne );
        _expected_tokens_set.include( (int)k_ge );
        _expected_tokens_set.include( (int)k_gt );
        _expected_tokens_set.include( (int)k_like );
        _expected_tokens_set.include( (int)k_regex_match );
        _expected_tokens_set.include( (int)k_in );
        _expected_tokens_set.include( (int)k_star_eq );
        _expected_tokens_set.include( (int)k_eq_star );
        _expected_tokens_set.include( (int)k_colon_eq );
        _expected_tokens_set.include( (int)k_eq_colon );
        _expected_tokens_set.include( (int)k_not );
        _expected_tokens_set.include( (int)k_between );

        if( next_token_is( k_not ) ) {
            parse_token();
            not_ = !not_;
        }

        switch( next_token().kind() )
        {
            case k_lt     : op = op_lt; break;
            case k_le     : op = op_le; break;
            case k_eq     : op = op_eq; break;
            case k_ne     : op = op_ne; break;
            case k_ge     : op = op_ge; break;
            case k_gt     : op = op_gt; break;
            case k_like   : op = op_like; break;   // nur Text
            case k_in     : op = op_eq; any = true; break;
            case k_between: op = op_between; break;

            case k_star_eq : op = op_eq;
                             outer_join = oj_outer_join;
                             break;

            case k_eq_star : op = op_eq;
                             outer_join = oj_outer_join;
                             right_join = true;
                             break;

            case k_colon_eq: op = op_eq;
                             outer_join = Sql_outer_join( oj_outer_join | oj_log );
                             break;

            case k_eq_colon: op = op_eq;
                             outer_join = Sql_outer_join( oj_outer_join | oj_log );
                             right_join = true;
                             break;

            case k_regex_match: 
            {
                op = op_regex_match;
                Sos_ptr<Sql_regex_expr> ex = SOS_NEW( Sql_regex_expr ); 
                e = ex;
                break;
            }

            default        : expr = sub_expr;//return expr;
        }

        if( op )
        {
            if( !e )  e = SOS_NEW( Sql_expr_with_operands( op ) );
            expr = +e;

            e->_operand_array.add( sub_expr );
            parse_token();

            e->_any = any;  // IN

            if( !any        // nicht IN
            && ( op == op_lt || op == op_le || op == op_ne || op == op_eq || op == op_ge || op == op_gt ) )
            {
                if( next_token_is( k_all ) ) {
                    parse_token();
                    e->_all = true;
                }
                else
                if( next_token_is( k_any ) ) {
                    parse_token();
                    e->_any = true;
                }
            }

            if( e->_any | e->_all )
            {
                parse( k_klammer_auf );

                if( next_token_is( k_select ) )
                {
                    //op = op_in_select;
                    e->_operand_array.add( parse_select_expr() );
                }
                else
                {
                    while(1) {
                        e->_operand_array.add( parse_add_expr() );
                        if( !next_token_is( k_comma ) )  break;
                        parse_token();
                    }
                }
                parse( k_klammer_zu );
            }
            else
            {
                e->_operand_array.add( parse_add_expr() );

                if( op == op_between ) {
                    parse( k_and );
                    e->_operand_array.add( parse_add_expr() );
                }
            }

            if( right_join )  exchange( &e->_operand_array[ 0 ], &e->_operand_array[ 1 ] );

            if( outer_join )
            {
                if( outer_join & oj_log )  _need_error_log = true;

                Sql_expr* op0 = e->_operand_array[ 0 ];
                if( op0->_operator != op_field )  throw_xc_expr( "SOS-SQL-15", e->_operand_array[ 0 ] );

                ((Sql_field_expr*)op0)->_outer_join = outer_join;
            }
        }
    }  // op_exists


    if( not_ )
    {
        switch( op ) {
            case op_lt: op = op_gt; break;
            case op_le: op = op_ge; break;
            case op_eq: op = op_ne; break;    // Auch bei Outer Join!
            case op_ne: op = op_eq; break;
            case op_ge: op = op_le; break;
            case op_gt: op = op_lt; break;
            default:
            {
                Sos_ptr<Sql_expr_with_operands> e2;
                e2 = SOS_NEW( Sql_expr_with_operands( op_not ) );
                e2->_operand_array.add( +expr );
                return +e2;
            }
        }

        expr->_operator = op;
    }

    return +expr;
}

//-------------------------------------------------------------------Sql_parser::parse_and_expr

Sos_ptr<Sql_expr> Sql_parser::parse_and_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr>               expr;
    Sos_ptr<Sql_expr_with_operands> e;

    expr = parse_cmp_expr();
    if( !next_token_is( k_and ) )  return expr;

    e = SOS_NEW( Sql_expr_with_operands( op_and ) );
    e->_operand_array.add( expr );

    while(1) {
        parse_token();
        e->_operand_array.add( parse_cmp_expr() );
        if( !next_token_is( k_and ) )  break;
    }

    return +e;
}

//--------------------------------------------------------------------Sql_parser::parse_or_expr

Sos_ptr<Sql_expr> Sql_parser::parse_or_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr>               expr;
    Sos_ptr<Sql_expr_with_operands> e;

    expr = parse_and_expr();
    if( !next_token_is( k_or ) )  return expr;

    e = SOS_NEW( Sql_expr_with_operands( op_or ) );
    e->_operand_array.add( expr );

    while(1) {
        parse_token();
        e->_operand_array.add( parse_and_expr() );
        if( !next_token_is( k_or ) )  break;
    }

    return +e;
}

//------------------------------------------------------------------Sql_parser::parse_cond_expr

Sos_ptr<Sql_expr> Sql_parser::parse_cond_expr()
{
    ZERO_RETURN_VALUE( Sos_ptr<Sql_expr> );

    Sos_ptr<Sql_expr>               expr;
    Sos_ptr<Sql_expr_with_operands> e;

    expr = parse_or_expr();
    if( !next_token_is( k_question_mark ) )  return expr;
    parse_token();

    e = SOS_NEW( Sql_expr_with_operands( op_cond ) );
    e->_operand_array.add( expr );

    e->_operand_array.add( parse_or_expr() );        // then-expr

    while( next_token_is( k_colon ) ) {
        parse_token();
        e->_operand_array.add( parse_or_expr() );    // else-expr oder nächste cond-expr
        if( !next_token_is( k_question_mark ) )  goto COND_OK;
        parse_token();
        e->_operand_array.add( parse_or_expr() );    // then-expr
    }

    _need_error_log = true;   // Else-Fall ist ein Fehler fürs Fehlerprotokoll

  COND_OK:
    return +e;
}


} //namespace sos
