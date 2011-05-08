// sosole.h                                         ©1996 SOS GmbH Berlin
//                                                  Joacim Zschimmer

#ifndef __SOSOLE_H
#define __SOSOLE_H

namespace sos
{

#if defined SYSTEM_WIN32
// Für Win16 muss UNICODE umgestellt werden.

/*
// Einige OLE-Typen:
struct          IDispatch;
typedef long    DISPID;
typedef         EXCEPINFO;
*/
#include <windows.h>

struct Ole_object;

typedef Sos_simple_array<Dyn_obj>  Sos_dyn_obj_array;



struct Ole_method : Sos_self_deleting
{
                                Ole_method              ( Ole_object* = NULL );
                               ~Ole_method              ();
                               
    void                        prepare                 ();
    void                        invoke                  ( Dyn_obj* result, const Sos_dyn_obj_array& );

    void                        throw_excepinfo         ( HRESULT, EXCEPINFO* );
    
    Fill_zero                  _zero_;
    Sos_string                 _name;
    Ole_object*                _object;
    DISPID                     _dispid;
};


struct Ole_object : Sos_self_deleting
{
                                Ole_object              ();
                                Ole_object              ( const Sos_string& class_name );
                               ~Ole_object              ();

    void                        prepare                 ();

#   if defined __SOSSTRNG_H
        Ole_method*             method                  ( const Sos_string& name )  { return method( c_str( name ) ); }
#   endif

    Ole_method*                 method                  ( const char* name ); 

    Fill_zero                  _zero_;
    Sos_string                 _name;
    Sos_string                 _class_name;
    Sos_simple_ptr_array< Ole_method > _methods;
    IDispatch*                 _pIDispatch;
    Dynamic_area               _hilfspuffer; 
};

#endif

} //namespace sos

#endif
