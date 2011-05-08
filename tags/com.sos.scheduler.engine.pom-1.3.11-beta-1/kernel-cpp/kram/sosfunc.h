// sosfunc.h                                        ©1996 SOS GmbH Berlin
//                                                  Joacim Zschimmer

#ifndef __SOSFUNC_H
#define __SOSFUNC_H

#ifndef __DYNOBJ_H
#   include "dynobj.h"
#endif

#ifndef __SOSARRAY_H
#   include "sosarray.h"
#endif

namespace sos
{


typedef Sos_simple_array<Dyn_obj>  Sos_dyn_obj_array;

#define SOS_FUNC __cdecl
typedef void SOS_FUNC Sos_function( Dyn_obj* result, const Sos_dyn_obj_array& );


struct Sos_function_descr : Sos_self_deleting
{
                                Sos_function_descr      ();
                               ~Sos_function_descr      ();

    Fill_zero                  _zero_;
    Sos_string                 _name;
  //int                        _min_param_count;
  //int                        _max_param_count;
    int                        _param_count; 
    Sos_function*              _func;
  //Sos_ptr<Field_type>        _result_type;            // Erstmal nur Dyn_obj
  //Sos_ptr<Record_type>       _param_types;            // Erstmal nur Dyn_obj
  //Sos_ptr<Field_type>        _array_param_type;       // Wiederholungsparameter
};


struct Sos_function_register : Sos_self_deleting
{
                                Sos_function_register   ();
                               ~Sos_function_register   ();


    static Sos_function_register* init                  ();

    void                        add                     ( Sos_function*, const char* name, int param_count );
    void                        add                     ( const Sos_ptr<Sos_function_descr>& );
    void                        add                     ( Sos_function_descr* );
    Sos_function*               function                ( const char* name, int param_count )   { return function_descr( name, param_count )->_func; }
    Sos_function*               function_or_0           ( const char* name, int param_count );
    Sos_function_descr*         function_descr          ( const char* name, int param_count );
    Sos_function_descr*         function_descr_or_0     ( const char* name, int param_count );

    Fill_zero                  _zero_;
    Sos_simple_ptr_array< Sos_function_descr >  _func_array;
};


} //namespace sos

#endif

