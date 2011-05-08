//#define MODULE_NAME "sossql3"
//#define COPYRIGHT   "©1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include "precomp.h"
#include "sos.h"
#include "sosstat.h"
#include "sosfunc.h"

using namespace std;
namespace sos {


DEFINE_SOS_STATIC_PTR( Sos_function_register )

//------------------------------------------------------Sos_function_descr::Sos_function_descr

Sos_function_descr::Sos_function_descr() 
: 
    _zero_(this+1) 
{
    obj_const_name( "Sos_function_descr" );
}

//-----------------------------------------------------Sos_function_descr::~Sos_function_descr

Sos_function_descr::~Sos_function_descr()
{
}

//-------------------------------------------------Sos_function_register::Sos_function_register

Sos_function_register::Sos_function_register()
:
    _zero_ (this+1)
{
    obj_const_name( "Sos_function_register" );
    _func_array.obj_const_name( "Sos_function_descr::_func_array" );
}

//------------------------------------------------Sos_function_register::~Sos_function_register
    
Sos_function_register::~Sos_function_register()
{
}

//------------------------------------------------------------------Sos_function_register::init

Sos_function_register* Sos_function_register::init()
{
    Sos_static* st = sos_static_ptr();

    if( !st->_function_register )  {
        Sos_ptr<Sos_function_register> reg = SOS_NEW( Sos_function_register );
        st->_function_register = +reg;
    }
     
    return st->_function_register;
}

//-------------------------------------------------------------------Sos_function_register::add

void Sos_function_register::add( Sos_function* func, const char* name, int param_count )
{
    Sos_ptr<Sos_function_descr> func_descr = SOS_NEW( Sos_function_descr );

    func_descr->_name = name;
    func_descr->_param_count = param_count;
    func_descr->_func = func;

    add( func_descr );
}

//-------------------------------------------------------------------Sos_function_register::add

void Sos_function_register::add( const Sos_ptr<Sos_function_descr>& func_descr )
{
    add( +func_descr );
}

//-------------------------------------------------------------------Sos_function_register::add

void Sos_function_register::add( Sos_function_descr* func_descr )
{
    if( function_or_0( c_str( func_descr->_name ), func_descr->_param_count ) )  throw_xc( "SOS-1312", c_str( func_descr->_name ) ); 
    _func_array.add( func_descr );
}

//---------------------------------------------------------Sos_function_register::function_or_0

Sos_function* Sos_function_register::function_or_0( const char* name, int param_count )
{
    Sos_function_descr* func_descr = function_descr_or_0( name, param_count );
    return func_descr? func_descr->_func : NULL;
}

//---------------------------------------------------------Sos_function_register::function_descr

Sos_function_descr* Sos_function_register::function_descr( const char* name, int param_count )
{
    Sos_function_descr* func_descr = function_descr_or_0( name, param_count );
    if( !func_descr )  throw_xc( "SOS-1311", name, param_count );
    return func_descr;
}

//---------------------------------------------------Sos_function_register::function_descr_or_0

Sos_function_descr* Sos_function_register::function_descr_or_0( const char* name_par,
                                                                int param_count )
{
    Sos_string name = name_par;
    for( int i = 0; i <= _func_array.last_index(); i++ )
    {
        Sos_function_descr* func_descr = _func_array[ i ];
        if( stricmp( c_str( func_descr->_name ), c_str( name ) ) == 0 
         && func_descr->_param_count == param_count )  return func_descr;
    }

    return NULL;
}

} //namespace sos
