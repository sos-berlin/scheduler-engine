// $Id$

#include "precomp.h"

#include "sosstrng.h"
#include "sos.h"
#include "log.h"
#include "sosarray.h"
#include "sosfield.h"
#include "../file/anyfile.h"
#include "soslimtx.h"
#include "sossql2.h"

using namespace std;
namespace sos {

//---------------------------------------------------------------Sql_select::has_aggregate_function

bool Sql_select::has_aggregate_function( Sos_simple_array< Sos_ptr<Sql_expr> >& expr_array )
{
    for( int i = 0; i < expr_array.count(); i++ ) 
    {
        if( has_aggregate_function( expr_array[i] ) )  return true;
    }

    return false;
}

//----------------------------------------------------------------Sql_select::has_aggregate_function

bool Sql_select::has_aggregate_function( Sql_expr* expr )
{
    if( is_aggregate_op( expr->_operator ) )  return true;

    if( expr->obj_is_type( tc_Sql_expr_with_operands ) )
    {
        Sql_expr_with_operands* e = (Sql_expr_with_operands*)expr;

        for( int i = 0; i < e->_operand_array.count(); i++ ) 
        {
            if( has_aggregate_function( e->_operand_array[ i ] ) )  return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------Sql_aggregate::clear

void Sql_aggregate::clear()
{
    for( Result_map::iterator it = _result_map.begin(); it != _result_map.end(); it++ )  delete [] it->second;
    _result_map.clear();
}

//-----------------------------------------------------------------Sql_aggregate::groupby_expr_index

int Sql_aggregate::groupby_expr_index( Sql_expr* expr )
{
    for( int i = 0; i < _groupby_expr_array.count(); i++ )
    {
        if( *expr == *_groupby_expr_array[i] )  return i;
    }

    return -1;
}

//-------------------------------------------------------------Sql_aggregate::identify_groupby_exprs

void Sql_aggregate::identify_groupby_exprs( Sos_ptr<Sql_expr>* expr )
{
    int groupby_index = _select->_aggregate.groupby_expr_index( *expr );
    if( groupby_index >= 0 ) 
    {
        LOG( "Group by: Teilausdruck " << **expr << " mit dem " << (groupby_index+1) << ". Groupby-Ausdruck identifiziert\n" );

        Sos_ptr<Sql_aggregate_expr> e = SOS_NEW( Sql_aggregate_expr( op_aggregate_groupby ) );
        e->_groupby_index = groupby_index;
        e->_type = _groupby_expr_array[groupby_index]->_type;
        *expr = +e;
    }
    else
    if( (*expr)->obj_is_type( tc_Sql_expr_with_operands ) )
    {
        Sql_expr_with_operands* e = (Sql_expr_with_operands*)+*expr;

        for( int i = 0; i < e->_operand_array.count(); i++ ) 
        {
            identify_groupby_exprs( &e->_operand_array[i] );
        }
    }
}

//---------------------------------------------------------------------Sql_select::check_for_fields

void Sql_select::check_for_fields( Sos_simple_array< Sos_ptr<Sql_expr> >& expr_array )
{
    for( int i = 0; i < expr_array.count(); i++ ) 
    {
        check_for_fields( expr_array[i] );
    }
}

//---------------------------------------------------------------------Sql_select::check_for_fields

void Sql_select::check_for_fields( Sql_expr* expr )
{
    string field_name;

    switch( expr->_operator )
    {
        case op_field:
            field_name = ((Sql_field_expr*)expr)->_name;

        case op_rowid:
            if( field_name.empty() )  field_name = "ROWID";
            
            throw_xc( "SOS-SQL-93", field_name );
            break;

        default: 
            if( !is_aggregate_op( expr->_operator ) )
            {
                if( expr->obj_is_type( tc_Sql_expr_with_operands ) )
                {
                    Sql_expr_with_operands* e = (Sql_expr_with_operands*)+expr;

                    for( int i = 0; i < e->_operand_array.count(); i++ ) 
                    {
                        check_for_fields( e->_operand_array[i] );
                    }
                }
            }
    }
}

//-------------------------------------------------------------Sql_aggregate::identify_groupby_exprs

void Sql_aggregate::identify_groupby_exprs( Sos_simple_array< Sos_ptr<Sql_expr> >* expr_array )
{
    for( int i = 0; i < expr_array->count(); i++ )
    {
        identify_groupby_exprs( &(*expr_array)[i] );
    }
}

//----------------------------------------------------------------------------Sql_aggregate::prepare

void Sql_aggregate::prepare()
{
    int i;

    _aggregate_record_type = SOS_NEW( Record_type );
    _aggregate_record_type->name( "sossql_aggregate_record" );  // Ergebnisse der Aggregatsfunktionen

    i = 0;
    for( Aggregate_array::iterator it = _aggregate_array.begin(); it != _aggregate_array.end(); it++, i++ )
    {
        Sql_aggregate_expr* e = *it;

        string field_name = "aggregate_" + as_string(1+i);
        bool   need_null_flag = false;

        switch( e->_operator )
        {
            case op_aggregate_avg:      
            {
                string field_name_count = field_name + "_avg_counter";
                Sos_ptr<Field_descr> f = SOS_NEW( Field_descr( &int_type, field_name_count.c_str() ) );
                f->add_to( _aggregate_record_type );
                need_null_flag = true;
                break;
            }

            default: ;
        }

        e->_aggregate_field_index = _aggregate_record_type->field_count();

        if( e->_type )
        {
            Sos_ptr<Field_descr> f = SOS_NEW( Field_descr( e->_type, field_name.c_str() ) );
            f->add_to( _aggregate_record_type );
            if( need_null_flag  &&  !f->nullable())  f->add_null_flag_to( _aggregate_record_type );
        }
    }
}

//----------------------------------------------------------------------------Sql_aggregate::execute

void Sql_aggregate::execute()
{
    _groupby_record.allocate_length( _groupby_type->field_size() );

    try
    {
        while(1)  process_record();
    }
    catch( const Eof_error& ) {}

    _result_iterator = _result_map.begin();
}

//------------------------------------------------------------Sql_aggregate::eval_aggregate_function

void Sql_aggregate::eval_aggregate_function( Byte* aggregate_record, Sql_aggregate_expr* expr )
{
    Dyn_obj o;

    if( expr->_operand_array.count() > 0 )
    {
        _select->eval_expr( &o, expr->_operand_array[0] );
        if( o.null() )  return;
    }

    if( expr->_operator != op_aggregate_groupby )
    {
        Field_descr* f = _aggregate_record_type->field_descr_ptr( expr->_aggregate_field_index );
        Byte*        p = f->ptr(aggregate_record);

        switch( expr->_operator )
        {
            case op_aggregate_avg:
            {
                if( f->has_null_flag() )  f->set_null_flag( aggregate_record, false );

                int* c = (int*)_aggregate_record_type->field_descr_ptr( expr->_aggregate_field_index - 1 )->ptr(aggregate_record);
                ++*c;
                *(double*)p += as_double(o); 
                break;
            }

            case op_aggregate_count:
            {
                if( expr->_operand_array.count() == 0  ||  !o.null() )  ++*(int*)p;
                break;
            }

            case op_aggregate_max:
            {
                if( f->type_ptr() != o.type() )  throw_xc( "SOS-SQL-90", f->type_ptr(), o.type() );

                if( f->null(aggregate_record)  ||  f->type_ptr()->op_compare( p, o.ptr() ) < 0 )  
                {
                    if( f->has_null_flag() )  f->set_null_flag( aggregate_record, false );
                    memcpy( p, o.ptr(), f->type_ptr()->field_size() );
                }

                break;
            }

            case op_aggregate_min:
            {
                if( f->type_ptr() != o.type() )  throw_xc( "SOS-SQL-90", f->type_ptr(), o.type() );
            
                if( f->null(aggregate_record)  ||  f->type_ptr()->op_compare( p, o.ptr() ) > 0 )  
                {
                    if( f->has_null_flag() )  f->set_null_flag( aggregate_record, false );
                    memcpy( p, o.ptr(), f->type_ptr()->field_size() );
                }

                break;
            }

          //case op_aggregate_stddev:

            case op_aggregate_sum:
            {
              //if( f->type_ptr() != o.type() )  throw_xc( "Sql_aggregate::eval_aggregate_function" );
              //f->type_ptr()->op_add( p, o.ptr() );
                *(double*)p += as_double(o);
                break;
            }

          //case op_aggregate_variance:

            default: throw_xc( "Sql_aggregate::eval_aggregate_function", (int)expr->_operator );
        }
    }
}

//-----------------------------------------------------------Sql_aggregate::eval_aggregate_functions

void Sql_aggregate::eval_aggregate_functions( Byte* aggregate_record )
{
    for( Aggregate_array::iterator it = _aggregate_array.begin(); it != _aggregate_array.end(); it++ )
    {
        eval_aggregate_function( aggregate_record, *it );
    }
}

//---------------------------------------------------------------------Sql_aggregate::process_record

void Sql_aggregate::process_record()
{
    int i;

    _select->fetch_table_records();

    // Gruppenschlüssel bilden
    _select->assign_expr_array( *_groupby_type, &_groupby_record, _groupby_expr_array );


    Byte*& aggregate_record = _result_map[ string( _groupby_record.char_ptr(), _groupby_record.length() ) ];
    
    if( !aggregate_record ) 
    {
        aggregate_record = (Byte*)sos_alloc( _aggregate_record_type->field_size(), "aggregate_record" );

        for( i = 0; i < _aggregate_record_type->field_count(); i++ )
        {
            Field_descr* f = _aggregate_record_type->field_descr_ptr(i);
            if( f->has_null_flag() )  f->set_null_flag( aggregate_record );
        }
    }

    _current_aggregate_record = aggregate_record;
    _current_groupby_record   = _groupby_record.byte_ptr();

    eval_aggregate_functions( aggregate_record );

    _current_aggregate_record = NULL;
    _current_groupby_record   = NULL;
}

//-----------------------------------------------------------------Sql_aggregate::finish_aggregates

void Sql_aggregate::finish_aggregates( Byte* aggregate_record )
{
    for( Aggregate_array::iterator it = _aggregate_array.begin(); it != _aggregate_array.end(); it++ )
    {
        switch( (*it)->_operator )
        {
            case op_aggregate_avg:
            {
                Field_descr* f = _aggregate_record_type->field_descr_ptr( (*it)->_aggregate_field_index );
                Byte*        p = f->ptr(aggregate_record);

                int* c = (int*)_aggregate_record_type->field_descr_ptr( (*it)->_aggregate_field_index - 1 )->ptr(aggregate_record);

                if( *c > 0 )  *(double*)p /= *c;       // Durchschnitt ausrechnen
                        else  f->set_null(aggregate_record);

                break;
            }

            default: ;
        }
    }
}

//-----------------------------------------------------------------------Sql_aggregate::fetch_result

void Sql_aggregate::fetch_result()
{
    while(1)    // Solange HAVING-Klausel nicht erfüllt oder fehlerhaft ist oder LOOP vor WHERE läuft
    {
        if( _result_iterator == _result_map.end() )  throw_eof_error();

        finish_aggregates( _result_iterator->second );

        _current_groupby_record   = (const Byte*)_result_iterator->first.data();
        _current_aggregate_record = _result_iterator->second;
        _result_iterator++;

        if( !_having )  break;
        if( _select->eval_expr_as_bool( _having, false ) )  break;    // HAVING-Klausel erfüllt?
    }
}

//-------------------------------------------------------------------------------------------------

} //namespace sos
