// $Id: variables.h 12634 2007-02-17 19:40:05Z jz $

#ifndef __HOSTOLE_VARIABLES_H
#define __HOSTOLE_VARIABLES_H

#include "hostole2.h"
#include "../zschimmer/xml_libxml2.h"

#ifdef __GNUC__
#   include <ext/hash_map>
#else
#   include <hash_map>
#endif




namespace sos {

//using namespace zschimmer::xml_libxml2;

extern const GUID IID_Variables;

//-------------------------------------------------------------------------------------------static

extern Ole_class_descr* variable_class_ptr;
extern Ole_class_descr* variable2_class_ptr;

extern Ole_class_descr* variables_class_ptr;
extern Ole_class_descr* variables2_class_ptr;

//-----------------------------------------------------------------------------------------Variable

struct Hostware_variable: Ivariable, Ivariable2, Sos_ole_object
{
    void*                       operator new                ( size_t size )                         { return sos_alloc( size, "Hostware.Variable" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }


                              //Hostware_variable           ();
                                Hostware_variable           ( const BSTR name, const VARIANT& value, Ivariable* dummy );
                                Hostware_variable           ( const BSTR name, const VARIANT& value, Ivariable2* dummy );
                               ~Hostware_variable           ();

    USE_SOS_OLE_OBJECT_ADDREF_RELEASE
  //USE_SOS_OLE_OBJECT_QUERYINTERFACE 
    USE_SOS_OLE_OBJECT_GETTYPEINFO

#   ifdef Z_COM
        const static ::zschimmer::com::Com_method _methods[];
#   endif


    STDMETHODIMP                QueryInterface              ( const IID&, void** );

    STDMETHODIMP                GetIDsOfNames               ( const IID&, OLECHAR** rgszNames, UINT cNames, LCID, DISPID* );
    STDMETHODIMP                Invoke                      ( DISPID, const IID&, LCID, unsigned short wFlags, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );


    // Ivariable

    STDMETHODIMP            put_Value                       ( VARIANT* index, VARIANT* );
    STDMETHODIMP            get_Value                       ( VARIANT* index, VARIANT* );
  //STDMETHODIMP            put_Evaluate                    ( VARIANT* index, VARIANT* value )      { return put_value( index, value ); }
  //STDMETHODIMP            get_Evaluate                    ( VARIANT* index, VARIANT* result )     { return get_value( index, result ); }
    STDMETHODIMP                Dim                         ( int size );
    STDMETHODIMP            get_Name                        ( BSTR* name )                          { *name = _name.copy(); return NOERROR; }


    // Ivariable2

    STDMETHODIMP            put_Obj_value                   ( VARIANT* index, VARIANT* value )      { return put_Value( index, value ); }
    STDMETHODIMP            get_Obj_value                   ( VARIANT* index, VARIANT* value )      { return get_Value( index, value ); }
  //STDMETHODIMP            put_Evaluate                    ( VARIANT* index, VARIANT* value )      { return put_Value( index, value ); }
  //STDMETHODIMP            get_Evaluate                    ( VARIANT* index, VARIANT* result )     { return get_Value( index, result ); }
    STDMETHODIMP                Obj_dim                     ( int size )                            { return Dim( size ); }
    STDMETHODIMP            get_Obj_name                    ( BSTR* name )                          { *name = _name.copy(); return NOERROR; }

  private:
    friend struct               Hostware_variables;

    HRESULT                     copy_array_element          ( VARIANT* index, VARIANT* param, bool write );
    void                        assign                      ( const VARIANT& );
    const VARIANT*              normalized_value            ( const VARIANT* value, VARIANT* helper );

    Thread_semaphore           _lock;
    Variant                    _value;
    Bstr                       _name;
    int                        _dispid;                     // 0 oder Index für Hostware_variables::_names
};

//-------------------------------------------------------------------------------Hostware_variable2
/*
struct Hostware_variable2 : Hostware_variable
{
                                Hostware_variable2          ()                                      : Hostware_variable( (Ivariable2*)NULL ) {}
};
*/
//-------------------------------------------------------------------------------Hostware_variables

struct Hostware_variables: Ivariables, Ivariables2, Sos_ole_object
{
    typedef stdext::hash_map< Bstr, ptr<Hostware_variable> >  Map;

    void*                       operator new                ( size_t size )                         { return sos_alloc( size, "hostWare.Variables" ); }
    void                        operator delete             ( void* ptr )                           { sos_free( ptr ); }



                                Hostware_variables          ( Ivariables* dummy = NULL );
                                Hostware_variables          ( Ivariables2* dummy );
                               ~Hostware_variables          ();

    STDMETHODIMP                QueryInterface              ( REFIID, void** );
    
    USE_SOS_OLE_OBJECT_ADDREF_RELEASE
  //USE_SOS_OLE_OBJECT_QUERYINTERFACE 
    USE_SOS_OLE_OBJECT_GETTYPEINFO
  //USE_SOS_OLE_OBJECT_INVOKE           

#   ifdef Z_COM
        const static ::zschimmer::com::Com_method _methods[];
#   endif

    STDMETHODIMP                GetIDsOfNames               ( const IID&, OLECHAR** rgszNames, UINT cNames, LCID, DISPID* );
    STDMETHODIMP                Invoke                      ( DISPID, const IID&, LCID, unsigned short wFlags, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );


    // Ivariables
    
    STDMETHODIMP                Set_var                     ( BSTR name, VARIANT* value )           { return put_Value( name, value ); }
    STDMETHODIMP                put_Value                   ( BSTR, VARIANT* );
    STDMETHODIMP                get_Value                   ( BSTR, Ivariable** );
    STDMETHODIMP                get_Count                   ( int* );

    STDMETHODIMP                Clone                       ( Ivariables** );
    STDMETHODIMP                clear                       ()                                      { THREAD_LOCK( _lock )  _map.clear();  return NOERROR; }
    STDMETHODIMP                get__NewEnum                ( IUnknown** );
    STDMETHODIMP                Obj_enumerator              ( Ivariables2_idispatch_enumerator** );
  //STDMETHODIMP                merge                       ( Ivariables*, VARIANT_BOOL );

    STDMETHODIMP            put_Xml                         ( BSTR xml_text );
    STDMETHODIMP            get_Xml                         ( BSTR* result  );



    // Ivariables2

  //STDMETHODIMP                set_var                     ( BSTR name, VARIANT* value )           { return put_Value( name, value ); }

    STDMETHODIMP                put_Obj_value               ( BSTR name, VARIANT* value )           { return put_Value( name, value ); }
    STDMETHODIMP                get_Obj_value               ( BSTR name, VARIANT* result );

    STDMETHODIMP                get_Obj_is_empty            ( VARIANT_BOOL* result )                { *result = _map.empty()? VARIANT_TRUE : VARIANT_FALSE;  return S_OK; }

    STDMETHODIMP                Obj_clone                   ( Ivariables2** result );
  //STDMETHODIMP                get__NewEnum                ( Ivariables_enumerator** );
  //STDMETHODIMP                _Merge                      ( Ivariables*, VARIANT_BOOL );

    STDMETHODIMP            put_Obj_xml                     ( BSTR xml_text )                       { return put_Xml( xml_text ); }
    STDMETHODIMP            get_Obj_xml                     ( BSTR* result )                        { return get_Xml( result ); }

    Variant                     get_variant                 ( const BSTR name );

    zschimmer::xml::libxml2::Document_ptr dom                        ();
    zschimmer::xml::libxml2::Element_ptr  dom_element                ( const zschimmer::xml::libxml2::Document_ptr& doc, const string& element_name, const string& subelement_name );
    void                        load_dom                    ( const zschimmer::xml::libxml2::Document_ptr&, const zschimmer::xml::libxml2::Element_ptr& );


    Map                        _map;
    std::vector<Bstr>          _names;                      // DISPID -> Name, Falls Variablen wie Eigenschaften aufgerufen werden (VBScript: variables.name, statt variables("name"))
    Thread_semaphore           _lock;
};

//------------------------------------------------------------------------------Hostware_variables2

struct Hostware_variables2 : Hostware_variables
{
                                Hostware_variables2         ()                                      : Hostware_variables( (Ivariables2*)NULL ) {}
};

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
