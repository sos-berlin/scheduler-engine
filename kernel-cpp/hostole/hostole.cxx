// hostole.cxx                                  (C)1996-98 SOS GmbH Berlin
// $Id: hostole.cxx 13728 2008-11-03 14:53:57Z jz $
// §1768

#include "precomp.h"

#include <wchar.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef SYSTEM_WIN
#   if !defined STRICT
#       define STRICT
#   endif

#   include <io.h>
#   include <direct.h>
#endif

#include "../kram/sos.h"
#include "../kram/sosalloc.h"
#include "../kram/log.h"
#include "../kram/sysxcept.h"
#include "../kram/sosprof.h"
#include "../kram/sosfield.h"
#include "../kram/sosdate.h"
#include "../file/anyfile.h"
#include "../file/stdfile.h"
#include "../kram/stdfield.h"
#include "../kram/licence.h"
#include "../kram/sleep.h"
#include "../kram/sosctype.h"
#include "../kram/soslimtx.h"
#include "../zschimmer/md5.h"
#include "../zschimmer/system_information.h"
#include "variables.h"

#if defined SYSTEM_WIN
#   include "../kram/sosscrpt.h"
#   include <windows.h>
#   include <shellapi.h>
#   include <ole2ver.h>
#endif

#include "../zschimmer/z_com.h"
#include "../zschimmer/z_com_server.h"
#include "../zschimmer/com_server.h"
#include "../zschimmer/log.h"
#include "../zschimmer/threads.h"
#include "../zschimmer/java.h"
#include "../kram/oleserv.h"
//#include "../kram/olestd.h"

#include "../kram/com_simple_standards.h"

#include "../kram/sosxml.h"
#include "../kram/sosdate.h"
#include "hostole.h"
#define INITGUIDS
#include "hostole2.h"

#if defined SYSTEM_WIN
    extern HINSTANCE _hinstance;
#endif

//--------------------------------------------------------------------------------------------GUID

#ifdef SYSTEM_WIN
#   include <initguid.h>
#endif

// Einzig erlaubte Implementierung von Ihostware_dynobj:
DEFINE_GUID(IID_hostware_dynobj,0x9F716A80,0xD1F0,0x11CF,0x86,0x9D,0x44,0x45,0x53,0x54,0x00,0x00);

//-------------------------------------------------------------------------------------------------

using namespace zschimmer::com;
using namespace zschimmer::third_party::md5;  // zschimmer/md5.h

namespace sos {

//------------------------------------------------------------------------Einzubindende OLE-Klassen

#ifdef SYSTEM_WIN

    namespace factory
    {
        extern Ole_class_descr* factory_processor_class_ptr;
        extern void             factory_processor_dummy(); 
    }

    SOS_INIT( hostole_modules )
    {
        ::sos::factory::factory_processor_class_ptr->dummy_call();
        ::sos::factory::factory_processor_dummy(); // js 27.7.00: jetzt klappts auch mit CreateObject ;-)
    }

#endif

//-------------------------------------------------------------------------------Hostware_type_info
// Thread-sicher, denn Objekt ist konstant.

struct Hostware_type_info : Ihostware_type_info, 
                            Sos_ole_object
{
    void*                       operator new            ( size_t size )                             { return sos_alloc( size, "Hostware_type_info" ); }
    void                        operator delete         ( void* ptr )                               { sos_free( ptr ); }


                                Hostware_type_info       ( Type_info* info )                        : Sos_ole_object( hostware_type_info_class_ptr, this, NULL ), 
                                                                                                      _type_info ( info ) {}
                             //~Hostware_type_info       ();

    USE_SOS_OLE_OBJECT
  //void                       _obj_print               ( ostream* s ) const                        { *s << "Hostware_type_info"; }

    /* Itype_info methods */
  //STDMETHODIMP                get_field_size          ( int* size )                               { *size = _type_info->_size; return NOERROR; }
    STDMETHODIMP                get_Name                ( BSTR* name )                              { *name = SysAllocString_string( _type_info->_name ); return NOERROR; }

    const Type_info* const     _type_info; 
};

//------------------------------------------------------------------------------Hostware_type_param
// Thread-sicher, denn Objekt ist konstant.

struct Hostware_type_param : Ihostware_type_param,
                             Sos_ole_object
{
    void*                       operator new            ( size_t size )                             { return sos_alloc( size, "Hostware_type_param" ); }
    void                        operator delete         ( void* ptr )                               { sos_free( ptr ); }


                                Hostware_type_param      ( Field_type* type )                       : Sos_ole_object( hostware_type_param_class_ptr, this, NULL )
                                                                                                    { type->get_param( &_type_param ); }
                             //~Hostware_type_param       ();

    USE_SOS_OLE_OBJECT

    /* Itype_param methods */
    STDMETHODIMP                get_Std_type            ( Hostware_std_type* o )        { *o = (Hostware_std_type)_type_param._std_type; return NOERROR; }
    STDMETHODIMP                get_Size                ( LONG* o )                     { *o = _type_param._size;           return NOERROR; }
    STDMETHODIMP                get_Display_size        ( LONG* o )                     { *o = _type_param._display_size;   return NOERROR; }
    STDMETHODIMP                get_Precision           ( LONG* o )                     { *o = _type_param._precision;      return NOERROR; }
    STDMETHODIMP                get_Radix               ( LONG* o )                     { *o = _type_param._radix;          return NOERROR; }
    STDMETHODIMP                get_Scale               ( VARIANT* o )                  { VariantInit( o ); if( _type_param._scale_null )  V_VT( o ) = VT_NULL; else { V_VT( o ) = VT_I4; V_I4( o ) = _type_param._scale; }  return NOERROR; }
    STDMETHODIMP                get_Usigned             ( VARIANT_BOOL* o )             { *o = _type_param._unsigned;       return NOERROR; }
    STDMETHODIMP                get_Info                ( Ihostware_type_info** o )     { *o = new Hostware_type_info( (Type_info*)_type_param._info_ptr ); (*o)->AddRef(); return NOERROR; }

    Type_param                 _type_param;             // Wird nur vom Konstruktur gesetzt
};

//----------------------------------------------------------------------------Hostware_field_type
// Thread-sicher, denn Objekt ist konstant.

struct Hostware_field_type : Ihostware_field_type, 
                             Sos_ole_object
{
    void*                       operator new            ( size_t size )                             { return sos_alloc( size, "Hostware_field_type" ); }
    void                        operator delete         ( void* ptr )                               { sos_free( ptr ); }


                                Hostware_field_type       ( const Sos_ptr<Field_type>& type, Ole_class_descr* cls = hostware_field_type_class_ptr )   : Sos_ole_object( cls, this, NULL ), 
                                                                                                    _type ( type ) {}
                             //~Hostware_field_type       ();

    USE_SOS_OLE_OBJECT

    /* Ihostware_field_type methods */
    STDMETHODIMP                get_Field_size          ( LONG* o )                     { *o = _type->field_size();  return NOERROR; }
    STDMETHODIMP                get_Info                ( Ihostware_type_info** o )     { *o = new Hostware_type_info( (Type_info*)_type->info() ); (*o)->AddRef(); return NOERROR; }
    STDMETHODIMP                get_Param               ( Ihostware_type_param** o )    { *o = new Hostware_type_param( _type ); (*o)->AddRef(); return NOERROR; }

    const Sos_ptr<Field_type>   _type;                  // Wird nicht verändert.
};

//----------------------------------------------------------------------------Hostware_field_descr
// Thread-sicher, denn Objekt ist konstant.

struct Hostware_field_descr : Ihostware_field_descr, 
                              Sos_ole_object
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware_field_descr" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Hostware_field_descr    ( const Sos_ptr<Field_descr>& field )     : Sos_ole_object( hostware_field_descr_class_ptr, this, NULL ), 
                                                                                  _field ( field ) {}
                             //~Hostware_field_descr    ();

    USE_SOS_OLE_OBJECT

    /* Ifield_descr methods */
    STDMETHODIMP_(HRESULT) get_Name( BSTR* name )          
    { 
        *name = SysAllocString_string( _field->name() ); 
        return NOERROR; 
    }

    STDMETHODIMP                get_Type                ( Ihostware_field_type** type) { *type = new Hostware_field_type( _field->type_ptr() );  (*type)->AddRef(); return NOERROR; }
    STDMETHODIMP                get_Offset              ( LONG* o )             { *o = _field->offset(); return NOERROR; }
    STDMETHODIMP                get_Remark              ( BSTR* o )             { *o = SysAllocString_string( _field->_remark ); return NOERROR; }

    const Sos_ptr<Field_descr> _field; 
};

//----------------------------------------------------------------------------Hostware_record_type

struct Hostware_record_type : Ihostware_record_type, 
                              Sos_ole_object, //Hostware_field_type,
                              z::My_thread_only
{
    void*                       operator new            ( size_t size )                             { return sos_alloc( size, "Hostware_record_type" ); }
    void                        operator delete         ( void* ptr )                               { sos_free( ptr ); }


                                Hostware_record_type       ( const Sos_ptr<Record_type>& type )     : Sos_ole_object( hostware_record_type_class_ptr, this ), _type(type) {}
                              //Hostware_record_type       ( const Sos_ptr<Record_type>& type )     : Hostware_field_type( +type, hostware_record_type_class_ptr ) {}
                             //~Hostware_record_type       ();

    USE_SOS_OLE_OBJECT

    /* Ihostware_field_type methods */
    STDMETHODIMP            get_Field_size              ( LONG* o )                                 { Z_COM_MY_THREAD_ONLY; *o = _type->field_size();  return NOERROR; }
    STDMETHODIMP            get_Info                    ( Ihostware_type_info** o )                 { Z_COM_MY_THREAD_ONLY; *o = new Hostware_type_info( (Type_info*)_type->info() ); (*o)->AddRef(); return NOERROR; }
    STDMETHODIMP            get_Param                   ( Ihostware_type_param** o )                { Z_COM_MY_THREAD_ONLY; *o = new Hostware_type_param( _type ); (*o)->AddRef(); return NOERROR; }

    Record_type*                type                    ()                                          { return (Record_type*)+_type; }

    /* Irecord_type methods */
    STDMETHODIMP            get_Name                    ( BSTR* o )                                 { Z_COM_MY_THREAD_ONLY; *o = SysAllocString_string( type()->name() );  return NOERROR; }
    STDMETHODIMP            get_Field_count             ( LONG* number )                            { Z_COM_MY_THREAD_ONLY; *number = type()->field_count();  return NOERROR; }
    STDMETHODIMP            get_Field_descr             ( LONG i, Ihostware_field_descr** o )       { Z_COM_MY_THREAD_ONLY; *o = new Hostware_field_descr( type()->field_descr_ptr( i ) );  (*o)->AddRef();  return NOERROR; }
    STDMETHODIMP                Add_field_descr         ( Ihostware_field_descr* f )                { Z_COM_MY_THREAD_ONLY; type()->add_field( ((Hostware_field_descr*)f)->_field ); return NOERROR; }
    STDMETHODIMP                Add_field               ( BSTR name, BSTR type );

    const Sos_ptr<Record_type>  _type; 
};

//----------------------------------------------------------------------------------------const

//const OLECHAR  hostware_type_library_name []     = OLETEXT( "hostOLE.tlb" );

//---------------------------------------------------------------------------------------------


Typelib_descr   hostole_typelib ( LIBID_hostWare, "hostWare", "1.0" );

DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware            , hostware            , CLSID_Global      , "hostWare.Global"      , "1.0" );
DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware_file       , hostware_file       , CLSID_File        , "hostWare.File"        , "1.0" );
DESCRIBE_CLASS_CREATABLE( &hostole_typelib, Hostware_dynobj     , hostware_dynobj     , CLSID_Dyn_obj     , "hostWare.Dyn_obj"     , "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_type_info  , hostware_type_info  , CLSID_Type_info   , "hostWare.Type_info"   , "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_type_param , hostware_type_param , CLSID_Type_param  , "hostWare.Type_param"  , "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_field_type , hostware_field_type , CLSID_Field_type  , "hostWare.Field_type"  , "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_record_type, hostware_record_type, CLSID_Record_type , "hostWare.Record_type" , "1.0" );
DESCRIBE_CLASS          ( &hostole_typelib, Hostware_field_descr, hostware_field_descr, CLSID_Field_descr , "hostWare.Field_descr" , "1.0" );

//?Ole_class_descr connection_point_container_class_descr( &hostware_typelib, IID_Ihostware_dialogbox    , "hostWare.Dialogbox"   );

//--------------------------------------------------------------------variant_as_hostware_dynobj

static Hostware_dynobj* variant_as_hostware_dynobj( Record_type* type, const VARIANT& variant )
{
    // Konvertiert eine Variante nach Hostware_dynobj in den angegebenen Typ.

    Hostware_dynobj* o = NULL;

    if( type ) 
    {
        if( ( V_VT( &variant ) & ~VT_BYREF ) == VT_DISPATCH ) 
        {
            IDispatch* d = V_VT( &variant ) & VT_BYREF? *V_DISPATCHREF( &variant ) 
                                                      :  V_DISPATCH   ( &variant );
            HRESULT hr = d->QueryInterface( IID_hostware_dynobj, (void**)&o );
            if( FAILED( hr ) )  throw_ole( hr, "QueryInterface" );
            if( o->_type != type )  { o->Release(); throw_xc( "SOS-1367", o->_type, type ); }
        } 
        else 
        {
            o = new Hostware_dynobj( type );
            o->AddRef();
            o->assign( variant );
        }
    }
    else
    {
        o = new Hostware_dynobj( type );
        o->AddRef();

        variant_to_char( variant, &o->_record );
        Sos_ptr<Text_type> t = SOS_NEW( Text_type( o->_record.length() ) );
        t->_rtrim = false;
        o->_type = +t;
    }

    return o;
}

//-------------------------------------------------------------------Hostware_record_type::_methods
#ifdef Z_COM

const Com_method Hostware_record_type::_methods[] =
{ 
   // _flags              ,     _name               , _method                                                      , _result_type, _types        , _default_arg_count
  //{ DISPATCH_PROPERTYGET,  1, "Java_class_name"   , (Com_method_ptr)&Hostware_record_type::get_Java_class_name   , VT_BSTR     },
    {}
};

#endif
//---------------------------------------------------------------Hostware_record_type::add_field

STDMETHODIMP Hostware_record_type::Add_field( BSTR name_bstr, BSTR type_name )
{
    Z_COM_MY_THREAD_ONLY; 

    //Sos_string typ = type;

    string name = bstr_as_string( name_bstr );

    Sos_ptr<String0_type> t = SOS_NEW( String0_type( 100 ) );

    type()->add_field( +t, name.c_str() );

    return NOERROR;
}

//-------------------------------------------------------------------------------Hostware::_methods
#ifdef Z_COM

const Com_method Hostware::_methods[] =
{ 
   // _flags              ,     _name                   , _method                                           , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "Java_class_name"       , (Com_method_ptr)&Hostware::get_Java_class_name    , VT_BSTR     },
    { DISPATCH_METHOD     ,  2, "Copy_file"             , (Com_method_ptr)&Hostware::Copy_file              , VT_EMPTY    , { VT_BSTR, VT_BSTR } },
    { DISPATCH_METHOD     ,  3, "Get_single"            , (Com_method_ptr)&Hostware::Get_single             , VT_DISPATCH , { VT_BSTR, VT_SAFEARRAY }, 1 },
    { DISPATCH_METHOD     ,  4, "File_as_string"        , (Com_method_ptr)&Hostware::File_as_string         , VT_BSTR     , { VT_BSTR } },
    { DISPATCH_METHOD     ,  5, "File_exists"           , (Com_method_ptr)&Hostware::File_exists            , VT_BOOL     , { VT_BSTR } },
    { DISPATCH_METHOD     ,  6, "Make_path"             , (Com_method_ptr)&Hostware::Make_path              , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_METHOD     ,  7, "Date_as_string"        , (Com_method_ptr)&Hostware::Date_as_string         , VT_BSTR     , { VT_VARIANT|VT_BYREF, VT_BSTR }, 1 },
    { DISPATCH_METHOD     ,  8, "As_date"               , (Com_method_ptr)&Hostware::As_date                , VT_DATE     , { VT_BSTR, VT_BSTR }, 1 },
    { DISPATCH_METHOD     ,  9, "Open"                  , (Com_method_ptr)&Hostware::Open                   , VT_DISPATCH , { VT_BSTR } },
    { DISPATCH_PROPERTYGET, 10, "System_information"    , (Com_method_ptr)&Hostware::get_System_information , VT_VARIANT  , { VT_BSTR, VT_VARIANT|VT_BYREF}, 1 },
    { DISPATCH_METHOD     , 11, "Sleep"                 , (Com_method_ptr)&Hostware::Sleep                  , VT_EMPTY    , { VT_R8|VT_BYREF } },
    { DISPATCH_METHOD     , 12, "Check_licence"         , (Com_method_ptr)&Hostware::Check_licence          , VT_EMPTY    , { VT_BSTR } },
    {}
};

#endif
//------------------------------------------------------------------------------Hostware::~Hostware

Hostware::~Hostware()
{
    Z_LOG2( "hostole", "~Hostware()\n" );
}

//-------------------------------------------------------------------------------Hostware::init

HRESULT Hostware::init()
{
    //return Sos_ole_object::init();
    return NOERROR;
}

//-------------------------------------------------------------------------Hostware::QueryInterface

STDMETHODIMP Hostware::QueryInterface( const IID& iid, void** result )
{                                                                    
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Imodule_interface2  , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihostware           , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//--------------------------------------------------------------------------Hostware::copy_file

STDMETHODIMP Hostware::Copy_file( BSTR source_filename, BSTR dest_filename )
{
    Z_LOGI2( "hostole", "Hostware::copy_file\n" );

    Z_MUTEX( hostware_mutex )
    try 
    {
        ::sos::copy_file( bstr_as_string(source_filename), bstr_as_string(dest_filename) );
    }
    catch( const exception& x )   { return _set_excepinfo( x, "hostWare.Global::copy_file" ); }
    catch( const _com_error& x )  { return _set_excepinfo( x, "hostWare.Global::copy_file" ); }

    return NOERROR;
}

//------------------------------------------------------------------------Hostware::remove_file

STDMETHODIMP Hostware::Remove_file( BSTR filename )
{
    Z_LOGI2( "hostole", "Hostware::remove_file\n" );

    Z_MUTEX( hostware_mutex )
    try 
    {
        ::sos::remove_file( bstr_as_string(filename) );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.Global::remove_file" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.Global::remove_file" ); }

    return NOERROR;
}

//------------------------------------------------------------------------Hostware::rename_file

STDMETHODIMP Hostware::Rename_file( BSTR old_filename, BSTR new_filename )
{
    Z_LOGI2( "hostole", "Hostware::rename_file\n" );

    Z_MUTEX( hostware_mutex )
    try 
    {
        ::sos::rename_file( bstr_as_string(old_filename), bstr_as_string(new_filename) );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.Global::rename_file" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.Global::rename_file" ); }

    return NOERROR;
}

//-----------------------------------------------------------------------Hostware::remove_directory

STDMETHODIMP Hostware::Remove_directory( BSTR path_bstr, VARIANT_BOOL force )
{
    HRESULT hr = NOERROR;
    
    Z_MUTEX( hostware_mutex )
    try 
    {
        ::sos::remove_directory( bstr_as_string( path_bstr ), force != 0 );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.Global::remove_directory" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.Global::remove_directory" ); }

    return hr;
}

//----------------------------------------------------------------------------Hostware::use_version

STDMETHODIMP Hostware::Use_version( BSTR version_bstr )
{
    HRESULT hr = NOERROR;
    
    Z_MUTEX( hostware_mutex )
    try 
    {
        sos_static_ptr()->use_version( bstr_as_string( version_bstr ) );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.Global::use_version" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.Global::use_version" ); }

    return hr;
}

//---------------------------------------------------------------------------Hostware::used_version

STDMETHODIMP Hostware::get_Used_version( BSTR* result )
{
    HRESULT hr = NOERROR;
    
    Z_MUTEX( hostware_mutex )
    try 
    {
        *result = SysAllocString_string( sos_static_ptr()->get_use_version() );
    }
    catch( const exception & x )   { return _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------------Hostware::Need_version

STDMETHODIMP Hostware::Need_version( BSTR version_bstr )
{
    HRESULT hr = NOERROR;
    
    Z_MUTEX( hostware_mutex )
    try 
    {
        sos_static_ptr()->need_version( bstr_as_string( version_bstr ) );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------------------Hostware::get_Version

STDMETHODIMP Hostware::get_Version( BSTR* version_bstr )
{
    HRESULT hr = NOERROR;
    
    try 
    {
        hr = String_to_bstr( sos_static_ptr()->version(), version_bstr );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------.Hostware::Is_Version_or_later

STDMETHODIMP Hostware::Is_version_or_later( BSTR version_bstr, VARIANT_BOOL* result )
{
    HRESULT hr = NOERROR;
    
    try 
    {
        *result = sos_static_ptr()->is_version_or_later( string_from_bstr( version_bstr ) );
    }
    catch( const exception&  x )   { return _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-------------------------------------------------------------------------Hostware::putref_Java_vm

STDMETHODIMP Hostware::putref_Java_vm__deleted( void* java_vm )
{
    return E_NOTIMPL;
/*
    HRESULT hr = S_OK;

    Z_MUTEX( hostware_mutex )
    try 
    {
        zschimmer::java::Vm::set_jvm( (JavaVM*)java_vm );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
*/
}

//---------------------------------------------------------------------Hostware::putref_Com_context

STDMETHODIMP Hostware::putref_Com_context( const z::com::Com_context* com_context )
{
    z::com::set_com_context( com_context );
    return S_OK; 
}

//---------------------------------------------------------------------Hostware::put_Log_categories

STDMETHODIMP Hostware::put_Log_categories( const BSTR bstr )
{ 
    zschimmer::static_log_categories.set_multiple( string_from_bstr( bstr ) ); 
    return S_OK; 
}

//-------------------------------------------------------------------------------------------------
/*
STDMETHODIMP Hostware::Set_stream_and_system_mutex ( ostream** os, z::System_mutex* m )
{
    z::Log_ptr::set_stream_and_system_mutex( os, m );
    return S_OK;
}
*/
//-------------------------------------------------------------------------------------------------
// Ab Version 1.6.131

STDMETHODIMP Hostware::put_Log_context( zschimmer::Log_context** log_context )
{
    zschimmer::Log_ptr::set_log_context( log_context );
    return S_OK;
}

//-------------------------------------------------------------------------------------------------
// Ab Version 1.6.131

STDMETHODIMP Hostware::get_Log_context( void*** result )
{
    *result = (void**)zschimmer::Log_ptr::get_log_context();
    return S_OK;
}

//-------------------------------------------------------------Hostware::shell_execute_and_wait

STDMETHODIMP Hostware::Shell_execute_and_wait( BSTR file, BSTR verb, double seconds )
{
#if !defined SYSTEM_WIN

    return E_NOTIMPL;

# else

    HRESULT hr = NOERROR;

    //PROCESS_INFORMATION process_information;
    //STARTUPINFO         startupinfo;
    SHELLEXECUTEINFO    info;
    long                ret;

    //Z_MUTEX( hostware_mutex )
    try {
        Z_LOG2( "hostole", "Hostware::shell_execute_and_wait " << file << ',' << verb << ',' << seconds << '\n' );

        Sos_string file_string = bstr_as_string(file);
        Sos_string verb_string = bstr_as_string(verb);

        memset( &info, 0, sizeof info );
        info.cbSize = sizeof info;
        info.fMask |= SEE_MASK_FLAG_NO_UI;
        if( seconds >= 0.0 )  info.fMask |= SEE_MASK_FLAG_DDEWAIT | SEE_MASK_NOCLOSEPROCESS;
        info.lpVerb = c_str(verb_string);
        info.lpFile = c_str(file_string);
        info.nShow  = SW_SHOWNORMAL;  // Oder 0 oder SW_MINIMIZE

        ret = ShellExecuteEx( &info );
        if( !ret )  throw_mswin_error( "ShellExecuteEx", c_str(file_string) );

        if( info.hProcess ) 
        {
            Z_LOG2( "hostole", "WaitForSingleObject\n" );

            if( seconds >= 0.0 ) {
                ret = WaitForSingleObject( info.hProcess,
                                           seconds < 0 || seconds > LONG_MAX / 1000? INFINITE : (long)seconds*1000 );
                if( ret == WAIT_FAILED )  throw_mswin_error( "WaitForSingleObject", bstr_as_string(file) );
            }
            
            ret = CloseHandle( info.hProcess );
            if( !ret ) Z_LOG2( "hostole", "hProcess=" << (void*)info.hProcess << '\n' );  
            info.hProcess = 0;

            if( !ret ) {
                if( GetLastError() == 6 ) {                         // Zugriffsnummer nicht definiert
                    try { throw_mswin_error( "CloseHandle" ); }     // kommt auf Dagmars PC (Word6).
                    catch( const Xc& ) {}
                } else {
                    throw_mswin_error( "CloseHandle" );  
                }
            }
        }

        hr = NOERROR;
    }
    catch( const exception& x ) 
    { 
        CloseHandle( info.hProcess );
        hr = _set_excepinfo( x, "hostWare.Global::shell_execute_and_wait" );
    }
    catch( const _com_error& x ) 
    { 
        CloseHandle( info.hProcess );
        hr = _set_excepinfo( x, "hostWare.Global::shell_execute_and_wait" );
    }

    return hr;

#endif
}

//--------------------------------------------------------------Hostware::get_single

STDMETHODIMP Hostware::Get_single( BSTR stmt, SAFEARRAY* param_array, Ihostware_dynobj** object )
{
    // Sieh auch sqlutil.cxx: get_single_value()!

    HRESULT hr = NOERROR;

    Z_LOGI2( "hostole", "Hostware::get_single()\n" );

    Z_MUTEX( hostware_mutex )
    {
      //VARIANT*            params = NULL;
        Hostware_dynobj*    o      = NULL;

        *object = NULL;

        try {
            Any_file file;

            hr = Get_single_or_array_open( &file, stmt, param_array );
            if( FAILED( hr ) )  goto FEHLER;

            o = new Hostware_dynobj( NULL );
            if( !o )  { hr = E_OUTOFMEMORY; goto FEHLER; }
            o->AddRef();

            try {
                file.get( &o->_record );
                o->_type = +file.spec()._field_type_ptr;
                o->finish();
            }
            catch( const Eof_error& )  { 
                //jz 13.4.98 throw_not_found_error( "SOS-1251" ); 
                //finish() nicht rufen, denn das setzt _type.
                //NULL zurückgeben.
            }

            file.close();
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::get_single" );  goto FEHLER; }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::get_single" );  goto FEHLER; }

        if( param_array )  SafeArrayUnaccessData( param_array );

        *object = o;
        return NOERROR;

    FEHLER:
        if( o )  o->Release();
        if( param_array )  SafeArrayUnaccessData( param_array );
    }

    return hr;
}

//--------------------------------------------------------------Hostware::get_single

STDMETHODIMP Hostware::Get_single_value( BSTR stmt, SAFEARRAY* param_array, VARIANT* result )
{
    HRESULT hr = NOERROR;

    Z_LOGI2( "hostole", "Hostware::get_single_value()\n" );

    VariantInit( result );

    ptr<Ihostware_dynobj> record;
    hr = Get_single( stmt, param_array, record.pp() );
    if( FAILED( hr ) )  return hr;

    if( record == NULL )
    {
        result->vt = VT_EMPTY;
    }
    else
    {
        Variant zero ( 0 );
        hr = record->Obj_field( &zero, result );
        if( FAILED(hr) )  return hr;
    }

    return hr;
}

//---------------------------------------------------------------Hostware::get_array

STDMETHODIMP Hostware::Get_array( BSTR stmt, SAFEARRAY* param_array, SAFEARRAY** result )
{

#if !defined SYSTEM_WIN

    return E_NOTIMPL;

# else

    HRESULT hr = E_FAIL;

    Z_LOGI2( "hostole", "Hostware::get_array()\n" );

    Z_MUTEX( hostware_mutex )
    {
        SAFEARRAY*          record_array = NULL;
        Bool                array_created = false;
    //Ihostware_dynobj**  records      = NULL;
        VARIANT*            records      = NULL;    // VBScript kann nicht mit SAFEARRAY(Ihostware_dynobj) umgehen, also SAFEARRAY(Variant)
        ptr<Hostware_dynobj> o           = NULL;
        SAFEARRAYBOUND      bound;
        int                 i            = 0;

        try {
            Any_file file;

            hr = Get_single_or_array_open( &file, stmt, param_array );
            if( FAILED( hr ) )  goto FEHLER;

            bound.lLbound   = 0;    // Erster Index
            bound.cElements = 100;

            record_array = SafeArrayCreate( VT_VARIANT, 1, &bound );   //(void*)&IID_hostware_dynobj
            if( FAILED( hr ) )  goto FEHLER;
            array_created = true;

            hr = SafeArrayAccessData( record_array, (void**)&records );
            if( FAILED( hr ) )  return hr;

            try {
                while(1)
                {
                    if( i == bound.cElements ) 
                    {
                        SafeArrayUnaccessData( record_array );  records = NULL;

                        bound.cElements *= 2;
                        hr = SafeArrayRedim( record_array, &bound );
                        if( FAILED( hr ) )  goto FEHLER;

                        hr = SafeArrayAccessData( record_array, (void**)&records );
                        if( FAILED( hr ) )  return hr;
                    }

                    o = new Hostware_dynobj( NULL );
                    if( !o )  { hr = E_OUTOFMEMORY; goto FEHLER; }

                    file.get( &o->_record );

                    o->_type = +file.spec()._field_type_ptr;
                    o->finish();

                    V_VT( &records[i] ) = VT_DISPATCH;
                    V_DISPATCH( &records[i] ) = o;
                    V_DISPATCH( &records[i] )->AddRef();
                    i++;
                    o = NULL;
                }
            }
            catch( const Eof_error& )  { }

            file.close();
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::get_array" );  goto FEHLER; }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::get_array" );  goto FEHLER; }


        SafeArrayUnaccessData( record_array );  records = NULL;

        bound.cElements = bound.lLbound + i;
        hr = SafeArrayRedim( record_array, &bound );
        if( FAILED( hr ) )  goto FEHLER;

        *result = record_array;

        return NOERROR;

    FEHLER:
        if( records )  SafeArrayUnaccessData( record_array );
        if( array_created )  SafeArrayDestroy( record_array );
    }

    return hr;

#endif
}

//-------------------------------------------------------------Hostware::read_begin_and_end_of_file

HRESULT Hostware::Read_begin_and_end_of_file( BSTR filename_bstr, int begin_bytes, int end_bytes, SAFEARRAY** result )
{
    HRESULT hr = E_FAIL;

    Z_LOGI2( "hostole", "Hostware::read_begin_and_end_of_file(\"" << filename_bstr << "\")\n" );

    Z_MUTEX( hostware_mutex )
    {
#       if !defined SYSTEM_WIN

            return E_NOTIMPL;

#        else

            HRESULT             hr           = NOERROR;
            SAFEARRAY*          safearray    = NULL;
            VARIANT*            array        = NULL;
            SAFEARRAYBOUND      bound;
            FILE*               file         = NULL;
            int                 ret;

            try {
                string          filename = bstr_as_string( filename_bstr );
                Dynamic_area    buffers[2];
                int             length[2];

                file = fopen( filename.c_str(), "rb" );
                if( !file )  throw_errno( errno, "fopen" );

                if( begin_bytes == -1 ) 
                {
                    begin_bytes = _filelength( _fileno( file ) );                   if( begin_bytes == -1 )  throw_errno( errno, "filelength" );
                    if( end_bytes > begin_bytes )  end_bytes = begin_bytes;
                }

                buffers[0].allocate( begin_bytes );  length[0] = 0;
                buffers[1].allocate( end_bytes );    length[1] = 0;
                
                if( begin_bytes > 0 )
                {
                    length[0] = fread( buffers[0].ptr(), 1, begin_bytes, file );    if( length[0] == -1 )  throw_errno( errno, "fread (begin)" );
                }
                
                if( end_bytes > 0 )
                {
                    ret = fseek( file, -end_bytes, SEEK_END );                          
                    if( ret == -1  &&  errno == EINVAL )  ret = fseek( file, 0, SEEK_SET );
                                                                                    if( ret       == -1 )  throw_errno( errno, "fseek" );

                    length[1] = fread( buffers[1].ptr(), 1, end_bytes, file );      if( length[1] == -1 )  throw_errno( errno, "fread (end)" );
                }

                bound.lLbound   = 0;    // Erster Index
                bound.cElements = 2;
                safearray = SafeArrayCreate( VT_VARIANT, 1, &bound );               if( FAILED( hr ) )  goto FEHLER;

                hr = SafeArrayAccessData( safearray, (void**)&array );              if( FAILED( hr ) )  goto FEHLER;

                for( int i = 0; i < 2; i++ )
                {
                    V_VT( &array[i] ) = VT_BSTR;
                    V_BSTR( &array[i] ) = SysAllocStringLen_char( buffers[i].char_ptr(), length[i] );
                }
            }
            catch( const exception & x )  { hr = _set_excepinfo( x, "hostWare.Global::read_begin_and_end_of_file" ); }
            catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Global::read_begin_and_end_of_file" ); }

          FEHLER:
            if( file )  fclose( file ), file = NULL;
            if( array )  SafeArrayUnaccessData( safearray ), array = NULL;

            if( FAILED( hr ) )  SafeArrayDestroy( safearray ), safearray = NULL;
                        else  *result = safearray;
#       endif
    }

    return hr;
}

//-----------------------------------------------------Hostware::get_single_or_array_open
//#if defined SYSTEM_WIN

STDMETHODIMP Hostware::Get_single_or_array_open( Any_file* file, BSTR stmt, SAFEARRAY* param_array )
{
    HRESULT     hr     = NOERROR;
    VARIANT*    params = NULL;

    Z_MUTEX( hostware_mutex )
    try 
    {
        LONG    lower_bound;
        LONG    upper_bound;
        int     n = 0;
        Dyn_obj dynobj;

        if( param_array )
        {
            hr = SafeArrayGetLBound( param_array, 1, &lower_bound );
            if( FAILED( hr ) )  return hr;

            hr = SafeArrayGetUBound( param_array, 1, &upper_bound );
            if( FAILED( hr ) )  return hr;

            n = upper_bound - lower_bound + 1;

            hr = SafeArrayAccessData( param_array, (void __huge**)&params );
            if( FAILED( hr ) )  return hr;
        }

        file->prepare( "-in -seq " + bstr_as_string( stmt ), Any_file::Open_mode(0) );

        for( int i = 0; i < n; i++ ) 
        {
            variant_to_dynobj( params[ i ], &dynobj );
            file->set_parameter( 1 + i, dynobj );
        }

        file->open();

        if( param_array )  SafeArrayUnaccessData( param_array );
    }
    catch( const exception& )
    {
        if( params )  SafeArrayUnaccessData( param_array );
        throw;
    }
    catch( const _com_error& )
    {
        if( params )  SafeArrayUnaccessData( param_array );
        throw;
    }

    return hr;
}

//#endif
//----------------------------------------------------------------------Hostware::execute_direct

STDMETHODIMP Hostware::Execute_direct( BSTR statement, SAFEARRAY* param_array )
{
    HRESULT hr = E_FAIL;

    Z_LOGI2( "hostole", "Hostware::execute_direct\n" );

    Z_MUTEX( hostware_mutex )
    {
#       if !defined SYSTEM_WIN

            return E_NOTIMPL;

#       else

            HRESULT         hr;
            VARIANT*        params = NULL;
            Hostware_file   file;

            try {
                LONG                        lower_bound;
                LONG                        upper_bound;
                int                         i, n;
                Sos_simple_array<Dyn_obj>   par;

                par.obj_const_name( "Hostware::execute_direct" );

                hr = file.Prepare( statement );
                if( FAILED( hr ) )  goto FEHLER;

                hr = SafeArrayGetLBound( param_array, 1, &lower_bound );
                if( FAILED( hr ) )  goto FEHLER;

                hr = SafeArrayGetUBound( param_array, 1, &upper_bound );
                if( FAILED( hr ) )  goto FEHLER;

                n = upper_bound - lower_bound + 1;
                par.last_index( n - 1 );

                hr = SafeArrayAccessData( param_array, (void __huge**)&params );
                if( FAILED( hr ) )  goto FEHLER;

                for( i = 0; i < n; i++ ) 
                {
                    variant_to_dynobj( params[i], &par[i] );
                    file._any_file.bind_parameter( 1 + i, &par[i] );
                }

                hr = file.Execute();
                if( FAILED( hr ) )  goto FEHLER;

                hr = file.Close();
                if( FAILED( hr ) )  goto FEHLER;

                SafeArrayUnaccessData( param_array );
                return NOERROR;

            }
            catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::execute_direct" ); }
            catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::execute_direct" ); }

          FEHLER:
            if( params )  SafeArrayUnaccessData( param_array );

            file.Close();
#       endif
    }

    return hr;
}

//----------------------------------------------------------------------Hostware::letter_factory

STDMETHODIMP Hostware::Letter_factory( BSTR )
{
    return ERROR;
}

//----------------------------------------------------------------------Hostware::file_as_string

STDMETHODIMP Hostware::File_as_string( BSTR filename, BSTR* string )
{
    Z_LOGI2( "hostole", "Hostware::file_as_string\n" );

    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try 
    {
        *string = SysAllocString_string( ::sos::file_as_string( bstr_as_string(filename), SYSTEM_NL ) );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::file_as_string" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::file_as_string" ); }

    return hr;
}

//---------------------------------------------------------------------Hostware::as_parser_string

STDMETHODIMP Hostware::As_parser_string( BSTR string, BSTR quote_string, BSTR* result )
{
#ifndef SYSTEM_HAS_COM

    return E_NOTIMPL;

#else
    // Nur der ISO-8859-1-Zeichensatz wird verwendet (linkes Byte = 0x00)

    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    {
        BSTR            buffer  = SysAllocStringLen( (BSTR)NULL, 2 + SysStringLen( string ) + SysStringLen( string ) / 25 + 100 );      
        if( !buffer )  return E_OUTOFMEMORY;

        const OLECHAR*  p       = string? string : L"";
        OLECHAR*        b       = buffer;

        if( quote_string && wmemcmp( quote_string, L"SGML", 4 ) == 0 ) 
        {
            while( *p ) 
            {
                // Nicht mehr genug Platz für \x.. und abschließende Quote? (der selbe Code wie unten!)
                if( b - buffer > (int)SysStringLen( buffer ) - 5 ) 

                { 
                    BSTR neu = NULL;
                    int ok = SysReAllocStringLen( &neu, NULL, SysStringLen( buffer ) * 2 );
                    if( !ok ) { SysFreeString( buffer );  return E_OUTOFMEMORY; }
                    memcpy( neu, buffer, (Byte*)b - (Byte*)buffer );
                    b = neu + ( b - buffer );
                    buffer = neu;
                }

                // Darstellbares Zeichen?
                if( *p & 0xFF00  ||  sos_isgraph( (Byte)*p )  || *p == L' ' ) 
                {
                    switch( (char)*p ) {
                        case '<': wmemcpy( b, L"&lt;"   , 4 );  b += 4;  p++;  break;
                        case '>': wmemcpy( b, L"&gt;"   , 4 );  b += 4;  p++;  break;
                        case '&': wmemcpy( b, L"&amp;"  , 5 );  b += 5;  p++;  break;
                        default: *b++ = *p++;
                    }
                }
                else
                {
                    switch( (char)*p ) {
    /*
                        case '\a': *b++ = '\\'; *b++ = 'a';  p++;  break;
                        case '\b': *b++ = '\\'; *b++ = 'b';  p++;  break;
                        case '\f': *b++ = '\\'; *b++ = 'f';  p++;  break;
                        case '\n': *b++ = '\\'; *b++ = 'n';  p++;  break;
                        case '\r': *b++ = '\\'; *b++ = 'r';  p++;  break;
                        case '\v': *b++ = '\\'; *b++ = 'v';  p++;  break;
                        case '\t': *b++ = '\\'; *b++ = 't';  p++;  break;
    */
                        case '\r':
                        case '\n': *b++ = (char)*p++;  break;

                        default:  
                        {
                            int n = (Byte)*p;
                            *b++ = '&';
                            *b++ = '#';
                            if( n >= 100 )  *b++ = '0' + n / 100;
                            *b++ = '0' + ( n / 10 ) % 10;
                            *b++ = '0' + n % 10;
                            *b++ = ';';
                            p++;
                        }
                    }
                }
            }
        }
        else
        {
            OLECHAR         quote   = quote_string && SysStringLen( quote_string ) > 0? quote_string[0] : L'"';
        

            *b++ = quote;

            while( *p ) 
            {
                // Nicht mehr genug Platz für \x.. und abschließende Quote? (der selbe Code wie oben!)
                if( b - buffer > (int)SysStringLen( buffer ) - 5 ) { 
                    BSTR neu = SysAllocStringLen( (BSTR)NULL, SysStringLen( buffer ) * 2 );
                    if( !neu ) { SysFreeString( buffer );  return E_OUTOFMEMORY; }
                    memcpy( neu, buffer, (Byte*)b - (Byte*)buffer );
                    b = neu + ( b - buffer );
                    SysFreeString( buffer );
                    buffer = neu;
                }

                // Darstellbares Zeichen?
                if( *p & 0xFF00  ||  sos_isgraph( (Byte)*p )  || *p == L' ' ) {
                    if( *p == quote  ||  *p == '\\' )  *b++ = '\\';
                    *b++ = *p++;
                }
                else
                {
                    switch( (char)*p ) {
                        case '\a': *b++ = '\\'; *b++ = 'a';  p++;  break;
                        case '\b': *b++ = '\\'; *b++ = 'b';  p++;  break;
                        case '\f': *b++ = '\\'; *b++ = 'f';  p++;  break;
                        case '\n': *b++ = '\\'; *b++ = 'n';  p++;  break;
                        case '\r': *b++ = '\\'; *b++ = 'r';  p++;  break;
                        case '\v': *b++ = '\\'; *b++ = 'v';  p++;  break;
                        case '\t': *b++ = '\\'; *b++ = 't';  p++;  break;

                        default:  
                        {
                            const char hex[] = "0123456789ABCDEF";
                            b[0] = '\\';
                            b[1] = 'x';
                            b[2] = hex[ ( *p & 0xF0 ) >> 4 ];
                            b[3] = hex[ *p & 0xF ];
                            b += 4;
                            p++;
                        }
                    }
                }
            }

            *b++ = quote;
        }

        *result = SysAllocStringLen( buffer, b - buffer );
        SysFreeString( buffer );

        hr =  NOERROR;
    }

    return hr;
#endif
}

//--------------------------------------------------------------------------------Hostware::sleep

STDMETHODIMP Hostware::Sleep( double* seconds )
{
    Z_LOGI2( "hostole", "Hostware::sleep " << seconds << '\n' );

    HRESULT hr = NOERROR;

    try {
        sos_sleep( *seconds );
    }
    catch( const exception& x )   { hr = _set_excepinfo( x, "hostWare.Global::sleep" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::sleep" ); }

    return hr;
}

//--------------------------------------------------------------------------------Hostware::as_xml

STDMETHODIMP Hostware::As_xml( VARIANT* field_or_dynobj, BSTR options, BSTR* result )
{
    HRESULT hr = NOERROR;

    *result = NULL;

    Z_MUTEX( hostware_mutex )
    try 
    {
        if( ( V_VT( field_or_dynobj ) & ~VT_BYREF ) == VT_UNKNOWN ) 
        {
            Ihostware_dynobj* dynobj = NULL;
            IUnknown* d = V_VT( field_or_dynobj ) & VT_BYREF? *V_UNKNOWNREF( field_or_dynobj ) 
                                                             : V_UNKNOWN   ( field_or_dynobj );
            hr = d->QueryInterface( IID_hostware_dynobj, (void**)&dynobj );
            if( FAILED( hr ) )  goto SIMPLE;  //throw_ole( hr, "QueryInterface" );

            hr = dynobj->get_Obj_xml( options, result );

            dynobj->Release();
        }
        else
        {
SIMPLE:
            *result = SysAllocString_string( sos::as_xml( variant_as_string(*field_or_dynobj), bstr_as_string(options) ) );
            if( !*result )  return E_OUTOFMEMORY;
        }
    }
    catch( const exception& x )   { hr = _set_excepinfo( x, "hostWare.Global::as_xml" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::as_xml" ); }

    return hr;
}

//------------------------------------------------------------------------------Hostware::from_xml

STDMETHODIMP Hostware::From_xml( BSTR xml, BSTR, BSTR* result )
{
    HRESULT hr = NOERROR;

    *result = NULL;

    Z_MUTEX( hostware_mutex )
    try 
    {
        *result = SysAllocString_string( xml_as_string( bstr_as_string( xml ) ) );
    }
    catch( const exception& x )   
    { 
        hr = _set_excepinfo( x, "hostWare.Global::from_xml" ); 
        if( *result )  SysFreeString( *result ), *result = NULL;
    }
    catch( const _com_error& x )   
    { 
        hr = _set_excepinfo( x, "hostWare.Global::from_xml" ); 
        if( *result )  SysFreeString( *result ), *result = NULL;
    }

    return hr;
}

//-------------------------------------------------------------------------------Hostware::as_date

STDMETHODIMP Hostware::As_date( BSTR date, BSTR format, DATE* result )
{
    HRESULT hr = NOERROR;

    *result = 0;

    try 
    {
        Sos_optional_date_time   datetime;
        SYSTEMTIME      systemtime;
        Sos_string      date_string   = bstr_as_string( date );
        Sos_string      format_string = bstr_as_string( format );

        datetime.read_text( c_str(date_string), c_str(format_string) );
                     
        // Null-Wert?
        systemtime.wYear        = datetime.year();
        systemtime.wMonth       = datetime.month();
        systemtime.wDayOfWeek   = 0;
        systemtime.wDay         = datetime.day();
        systemtime.wHour        = datetime.hour();
        systemtime.wMinute      = datetime.minute();
        systemtime.wSecond      = datetime.second();
        systemtime.wMilliseconds= 0;

        int ok = SystemTimeToVariantTime( &systemtime, result );
        if( !ok )  throw_xc( "WIN-SystemTimeToVariantTime", c_str( date_string ) );
    }
    catch( const exception& x )   { hr = _set_excepinfo( x, "hostWare.Global::as_date" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::as_date" ); }

    return hr;
}

//------------------------------------------------------------------------Hostware::date_as_string

STDMETHODIMP Hostware::Date_as_string( VARIANT* date_vt, BSTR format, BSTR* result )
{
    HRESULT hr = NOERROR;

    *result = NULL;

    try 
    {
        Sos_limited_text<100>   buffer;
        Sos_optional_date_time  datetime;
        SYSTEMTIME              systemtime;
        Sos_string              format_string = bstr_as_string(format);
        DATE                    date;

        if( date_vt->vt == VT_DATE )
        {
            date = V_DATE(date_vt);
        }
        else
        {
            Variant my_date_vt;
            hr = VariantChangeType( &my_date_vt, date_vt, 0, VT_BSTR );
            if( FAILED(hr) )  return hr;

            hr = As_date( V_BSTR(&my_date_vt), NULL, &date );
            if( FAILED(hr) )  return hr;
        }

        int ok = VariantTimeToSystemTime( date, &systemtime );
        if( !ok )  throw_xc( "WIN-VariantTimeToSystemTime" );

        datetime.assign_date( systemtime.wYear, systemtime.wMonth, systemtime.wDay );
        datetime.set_time( systemtime.wHour, systemtime.wMinute, systemtime.wSecond );
      //systemtime.wMilliseconds;
        if( datetime.time_is_zero() )  datetime.date_only( true );
        datetime.write_text( &buffer, c_str(format_string) );

        *result = SysAllocStringLen_char( buffer.char_ptr(), length( buffer ) );
        if( !*result )  return E_OUTOFMEMORY;
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::date_as_string" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::date_as_string" ); }

    return hr;
}

//------------------------------------------------------------------------Hostware::create_object
// Implementiert in script_object.cxx

//---------------------------------------------------------------------------Hostware::sql_quoted

STDMETHODIMP Hostware::Sql_quoted( VARIANT value, BSTR* result )
{
    HRESULT hr = NOERROR;

    try 
    {
        switch( V_VT( &value ) ) 
        {
            case VT_NULL: 
                *result = SysAllocString( L"NULL" );
                break;

            default:      
            {
                Sos_string text = variant_as_string( value );
                *result = SysAllocString_string( quoted_string( text, '\'', '\'' ) );
            }
        }
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::sql_quoted" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::sql_quoted" ); }

    return hr;
}

//----------------------------------------------------------------------------Hostware::sql_equal

STDMETHODIMP Hostware::Sql_equal( BSTR field_name, VARIANT value_par, BSTR* result )
{
    HRESULT hr = NOERROR;
    BSTR    quoted_value = NULL;

    try 
    {
        Sos_string expr;

        VARIANT* value = V_VT(&value_par) & (VT_BYREF|VT_VARIANT)? (VARIANT*)V_BYREF(&value_par) : &value_par;

        if( V_VT(value) == VT_NULL )
        {
            expr = bstr_as_string(field_name) + " IS NULL";
        }
        else
        {
            hr = Sql_quoted( *value, &quoted_value );
            if( FAILED(hr) )  return hr;

            expr = bstr_as_string(field_name) + "=" + bstr_as_string(quoted_value);
        }

        *result = SysAllocString_string(expr);
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::sql_equal" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::sql_equal" ); }

    if( quoted_value )  SysFreeString( quoted_value );
    return hr;
}
//--------------------------------------------------------------------------Hostware::file_exists

STDMETHODIMP Hostware::File_exists( BSTR filename_bstr, short* result )
{
    HRESULT hr = NOERROR;

    Z_LOGI2( "hostole", "Hostware::file_exists( \"" << filename_bstr << " \")\n" );

    Z_MUTEX( hostware_mutex )
    try 
    {
        Sos_string filename = bstr_as_string( filename_bstr );

        *result = sos::file_exists( filename )? -1 : 0;
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Global::file_exists" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::file_exists" ); }

    return hr;


/*
    try 
    {
        Any_file file;

        file.open( bstr_as_string(filename_bstr), File_base::in );
        file.close();

        *result = true;
    }
    catch( const Not_exist_error& x )  
    { 
        *result = false; 
    }
    catch( const exception& x )   
    { 
        if( strcmp( x.code(), "D140" ) == 0     // Rapid: Datei gibt es nicht
         || strcmp( x.code(), "D141" ) == 0     // Rapid: Datei ist leer
         || strcmp( x.code(), "D160" ) == 0     // Rapid: Bibliothek gibt es nicht
         || strcmp( x.code(), "D161" ) == 0 )   // Rapid: Bibliothek ist leer
        {
            *result = false;
        }
        else
            hr = _set_excepinfo( x ); 
    }

    return hr;
*/
}

//--------------------------------------------------------------------------Hostware::file_exists

STDMETHODIMP Hostware::Make_path( BSTR path_bstr )
{
    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try 
    {
        Z_LOGI2( "hostole", "Hostware::make_path( \"" << path_bstr << " \")\n" );

        Sos_string  path = bstr_as_string( path_bstr );
        sos::make_path( path );
    }
    catch( const exception& x )   { hr = _set_excepinfo( x, "hostWare.Global::make_path" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Global::make_path" ); }

    return hr;
}

//------------------------------------------------------------------------Hostware::check_licence

STDMETHODIMP Hostware::Check_licence( BSTR product_name_bstr )
{
    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try
    {
        string product_name = bstr_as_string( product_name_bstr );

        sos::check_licence( product_name.c_str() );
    }
    catch( const exception & x )   { hr = _set_excepinfo( x, "licence" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "licence" ); }

    return hr;
}

//---------------------------------------------------------------------------Hostware::convert_to

STDMETHODIMP Hostware::Convert_to( BSTR type_name_bstr, VARIANT* value, BSTR format, VARIANT* result )
{
    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try
    {
        VariantInit( result );

        string      type_name = bstr_as_string( type_name_bstr );
        Field_type* type = NULL;
        VARTYPE     variant_type = VT_EMPTY;

        for( int i = 0; i < (int)type_name.length(); i++ )  type_name[i] = tolower( type_name[i] );

        if( type_name == "integer"
         || type_name == "int"      )  variant_type = VT_I4,    type = &int_type;
        else
        if( type_name == "boolean"
         || type_name == "bool"     )  variant_type = VT_BOOL,  type = &bool_type;
        else
        if( type_name == "double"   )  variant_type = VT_R8,    type = &double_type;
        else
        if( type_name == "currency" )  variant_type = VT_CY,    type = &currency_type;
        else
        if( type_name == "date"     )  variant_type = VT_DATE,  type = &sos_optional_date_time_type;
        else
        if( type_name == "string"   )  variant_type = VT_BSTR;
        else
            throw_xc( "SOS-1438", type_name );

        
        if( !type  ||  V_VT(value) == variant_type )
        {
            hr = VariantChangeType( result, value, VARIANT_NOUSEROVERRIDE, variant_type );
            if( FAILED( hr ) )  return hr;
        }
        else
        {
            string        value_str = variant_as_string( *value );
            Dynamic_area  buffer    ( type->field_size() );
            Dynamic_area  hilfspuffer;

            type->read_text( buffer.byte_ptr(), value_str.c_str() );
            field_to_variant( type, buffer.byte_ptr(), result, &hilfspuffer );
        }
    }
    catch( const exception & x )   { hr = _set_excepinfo( x, "hostWare::convert_to" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare::convert_to" ); }

    return hr;
}

//--------------------------------------------------------------------------------Hostware::Open

STDMETHODIMP Hostware::Open( BSTR filename_bstr, Ihostware_file** result )
{
    HRESULT hr;
    
    *result = NULL;

    ptr<Ihostware_file> file;
    
    hr = file.CoCreateInstance( CLSID_File );
    if( FAILED(hr) )  return hr;

    hr = file->Open( filename_bstr );
    if( FAILED(hr) )  return hr;

    file.copy_to( result );
    return hr;
}

//--------------------------------------------------------------------------------Hostware::open
// veraltet

STDMETHODIMP Hostware::Get_log_( void*** ostream_, void** system_mutex )
{
    *ostream_     = NULL;
    *system_mutex = NULL;

    /*
    Z_MUTEX( hostware_mutex )
    {
        zschimmer::Log_ptr::get_stream_and_system_mutex( (ostream***)ostream_, (zschimmer::System_mutex**)system_mutex );
    }
    */

    return S_FALSE;
}

//--------------------------------------------------------------------------------Hostware::open

STDMETHODIMP Hostware::Hex_md5_from_bytes( BSTR byte_bstr, BSTR* result )
{
    MD5_CTX context;
    Byte   digest   [16];
    string str      = string_from_bstr( byte_bstr );

    MD5Init( &context );
    MD5Update( &context, (unsigned char*)str.data(), str.length() );
    MD5Final( digest, &context );

    return String_to_bstr( zschimmer::lcase_hex( digest, sizeof digest ), result );
}

//----------------------------------------------------------------------Hostware::File_version_info

STDMETHODIMP Hostware::File_version_info( BSTR filename_bstr, BSTR option_bstr, Ivariables2** result )
{
#   ifdef SYSTEM_WIN

        Byte*   info = NULL;
        HRESULT hr   = E_FAIL;

        try
        {
            string filename = string_from_bstr( filename_bstr );
            BOOL   ok;
            DWORD  w    = 0;
            int    size = GetFileVersionInfoSize( (char*)filename.c_str(), &w );
            if( size == 0 )  throw_mswin_error( "GetFileVersionInfoSize", filename );

            info = (Byte*)malloc( size );

            ok = GetFileVersionInfo( (char*)filename.c_str(), NULL, size, info );
            if( !ok )  throw_mswin_error( "GetFileVersionInfo", filename );

    /*
            Byte* child;
            {
                int value_length = *(WORD*)( info + sizeof (WORD) );
                Byte* p = info + sizeof (WORD) + sizeof (WORD) + sizeof (WORD);
                int key_size = 2*(15+1);
                if( memcmp( p, L"VS_VERSION_INFO", key_size ) != 0 )  goto FEHLER;
                p += key_size;
                p += (Byte*)( ( (size_t)p + 3 ) & ~4 );    // Auf 4 Byte ausrichten
                VS_FIXEDFILEINFO* fixed_file_info = (VS_FIXEDFILEINFO*)p;
                p += value_length;
                p += (Byte*)( ( (size_t)p + 3 ) & ~4 );    // Auf 4 Byte ausrichten
                child = p;
            }

            for( child; *(WORD*)child > 0; child += *(WORD)*child )
            {
                Byte* str;
                {
                    Byte* p = child + sizeof (WORD) + sizeof (WORD);
                    if( *(WORD*)p != 1 )  continue;     // Kein Text?
                    p += sizeof (WORD);
                    int key_size = 2*(14+1);
                    if( memcmp( p, L"StringFileInfo", key_size ) != 0 )  continue;
                    p += key_size;
                    p += (Byte*)( ( (size_t)p + 3 ) & ~4 );    // Auf 4 Byte ausrichten
                    str = p;
                }

                for( str; *(WORD*)str > 0; child += *(WORD)*child )
                {
                    {
                        Byte* p = child + sizeof (WORD) + sizeof (WORD);
                        if( *(WORD*)p != 1 )  continue;     // Kein Text?
                        p += sizeof (WORD);
                        int key_size = 2*(14+1);
                        if( memcmp( p, L"StringFileInfo", key_size ) != 0 )  continue;
                        p += key_size;
                        p += (Byte*)( ( (size_t)p + 3 ) & ~4 );    // Auf 4 Byte ausrichten
                        string_table = p;
                    }
                }
            }
    */


            *result = new Hostware_variables2();
            (*result)->AddRef();

            (*result)->put_Obj_value( Bstr( "ProductVersion" ), &Variant( version_info_detail( info, "ProductVersion" ) ) );

            hr = S_OK;
        }
        catch( const exception & x )   { hr = _set_excepinfo( x, __FUNCTION__ ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, __FUNCTION__ ); }

        free( info );
        return hr;

#   else

        return E_NOTIMPL;

#   endif
}

//----------------------------------------------------------------------------------Hostware::chdir

STDMETHODIMP Hostware::Chdir( BSTR directory_bstr )
{
    HRESULT hr = S_OK;;

    try
    {
        string directory = string_from_bstr( directory_bstr );
        int error = chdir( directory.c_str() );
        if( error )  throw_errno( errno, "chdir" );
    }
    catch( const exception & x )   { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//-----------------------------------------------------------------Hostware::get_System_information

STDMETHODIMP Hostware::get_System_information( BSTR what_bstr, VARIANT* parameter, VARIANT* result )
{
    return zschimmer::get_System_information( what_bstr, parameter, result );
}

//---------------------------------------------------------------Hostware::get_Log_indent_tls_index
// veraltet. Nimm get_Log_context()

STDMETHODIMP Hostware::get_Log_indent_tls_index( uint* result )
{
    return S_FALSE;
    //*result = sos_static_ptr()->_log_indent_tls;
    //return S_OK;
}

//--------------------------------------------------------------------------Hostware_file::_methods
#ifdef Z_COM

const Com_method Hostware_file::_methods[] =
{ 
   // _flags              ,     _name               , _method                                               , _result_type, _types        , _default_arg_count
    { DISPATCH_METHOD     ,  1, "Open"              , (Com_method_ptr)&Hostware_file::Open                  , VT_EMPTY    , { VT_BSTR }   },
    { DISPATCH_METHOD     ,  2, "Close"             , (Com_method_ptr)&Hostware_file::Close                 },
    { DISPATCH_METHOD     ,  4, "get_line"          , (Com_method_ptr)&Hostware_file::Get_line              , VT_BSTR     },
    { DISPATCH_METHOD     ,  5, "put_line"          , (Com_method_ptr)&Hostware_file::Put_line              , VT_EMPTY    , { VT_BSTR } },
    { DISPATCH_METHOD     ,  8, "get"               , (Com_method_ptr)&Hostware_file::Get                   , VT_DISPATCH },
  //{ DISPATCH_METHOD     ,  9, "put"               , (Com_method_ptr)&Hostware_file::Put                   , VT_EMPTY    , { VT_DISPATCH } },
    { DISPATCH_METHOD     , 17, "eof"               , (Com_method_ptr)&Hostware_file::Eof                   , VT_BOOL     },
    { DISPATCH_PROPERTYGET, 20, "field_name"        , (Com_method_ptr)&Hostware_file::get_Field_name        , VT_BSTR     , { VT_I4 } },
    { DISPATCH_PROPERTYGET, 21, "field_count"       , (Com_method_ptr)&Hostware_file::get_Field_count       , VT_I4       },
    { DISPATCH_PROPERTYGET, 27, "opened"            , (Com_method_ptr)&Hostware_file::get_Opened            , VT_BOOL     },
    { DISPATCH_METHOD     , 28, "rewind"            , (Com_method_ptr)&Hostware_file::Rewind                },
    { DISPATCH_PROPERTYGET, 32, "row_count"         , (Com_method_ptr)&Hostware_file::get_Row_count         , VT_I4       },
    { DISPATCH_PROPERTYGET, 33, "Java_class_name"   , (Com_method_ptr)&Hostware_file::get_Java_class_name   , VT_BSTR     },
    { DISPATCH_PROPERTYPUT, 34, "read_null_as_empty", (Com_method_ptr)&Hostware_file::put_Read_null_as_empty, VT_EMPTY    , { VT_BOOL }    },
    { DISPATCH_METHOD     , 35, "Db_open"           , (Com_method_ptr)&Hostware_file::Db_open               , VT_DISPATCH , { VT_BSTR, VT_ARRAY|VT_VARIANT }, 1 },
    { DISPATCH_METHOD     , 36, "Db_get_single"     , (Com_method_ptr)&Hostware_file::Db_get_single         , VT_DISPATCH , { VT_BSTR, VT_ARRAY|VT_VARIANT }, 1 },
    { DISPATCH_METHOD     , 37, "Db_execute"        , (Com_method_ptr)&Hostware_file::Db_execute            , VT_DISPATCH , { VT_BSTR, VT_ARRAY|VT_VARIANT }, 1 },
    { DISPATCH_METHOD     , 38, "Db_get_single_value",(Com_method_ptr)&Hostware_file::Db_get_single_value   , VT_VARIANT  , { VT_BSTR, VT_ARRAY|VT_VARIANT }, 1 },
    { DISPATCH_PROPERTYGET, 39, "Filename"          , (Com_method_ptr)&Hostware_file::get_Filename          , VT_BSTR     },
    {}
};

#endif
//-------------------------------------------------------------------Hostware_file::Hostware_file

Hostware_file::Hostware_file()
:
    Sos_ole_object( &hostware_file_class, (Ihostware_file*)this ),
    _zero_(this+1),
    _Write_empty_as_null( false ),
    _Read_null_as_empty( false )
{
    _param_values.obj_const_name( "Hostware_file" );
    _buffer_size = 32767;       // max. Satzlänge für get( buffer )
}

//------------------------------------------------------------------Hostware_file::~Hostware_file

Hostware_file::~Hostware_file()
{
    Z_LOG2( "hostole", "~Hostware_file()\n" );
}

//------------------------------------------------------------------Hostware_file::QueryInterface

STDMETHODIMP Hostware_file::QueryInterface( const IID& iid, void** result )
{                                                                    
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihostware_file      , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//---------------------------------------------------------------------------Hostware_file::init

HRESULT Hostware_file::init()
{
    //return Sos_ole_object::init();
    return NOERROR;
}

//---------------------------------------------------------------------------Hostware_file::Open

STDMETHODIMP Hostware_file::Open( BSTR filename )
{
    Z_LOGI2( "hostole", "Hostware_file::open " << filename << "\n" );

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        if( SysStringLen( filename ) == 0 ) {
            _any_file.open();
        } else {
            _any_file.open( _filename_prefix + w_as_string(filename), (Any_file::Open_mode)0 );
        }
        
        _type = +_any_file.spec()._field_type_ptr;
        _key_descr = +_any_file.spec()._key_specs._key_spec._field_descr_ptr;
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.File::open" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::open" ); }

    return NOERROR;
}

//--------------------------------------------------------------------------Hostware_file::Close

STDMETHODIMP Hostware_file::Close()
{
    Z_LOGI2( "hostole", "Hostware_file::Close\n" );

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        _any_file.close();
        RENEW( _any_file, Any_file );
    }
    catch( const exception & x )   { return _set_excepinfo( x, "hostWare.File::close" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::close" ); }

    return NOERROR;
}

//--------------------------------------------------------------------Hostware_file::close_cursor

STDMETHODIMP Hostware_file::Close_cursor()
{
    Z_LOGI2( "hostole", "Hostware_file::close_cursor\n" );

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        _any_file.close( ::sos::close_cursor );
    }
    catch( const exception & x )   { return _set_excepinfo( x, "hostWare.File::close_cursor" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::close_cursor" ); }

    return NOERROR;
}

//---------------------------------------------------------------------Hostware_file::close_file

STDMETHODIMP Hostware_file::Close_file()
{
    return Close();
}

//-----------------------------------------------------------------------Hostware_file::put_line

STDMETHODIMP Hostware_file::Put_line( BSTR text )
{
    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        bstr_to_area( text, &_buffer );
        _any_file.put( _buffer );
    }
    catch( const exception & x )   { return _set_excepinfo( x, "hostWare.File::put_line" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::put_line" ); }

    return NOERROR;
}

//----------------------------------------------------------------------------Hostware_file::eof

STDMETHODIMP Hostware_file::Eof( short* eof )
{
    Z_LOGI2( "hostole", "Hostware_file::eof\n" );

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        *eof = _any_file.eof()? VARIANT_TRUE : VARIANT_FALSE;
    }
    catch( const exception & x )   { return _set_excepinfo( x, "hostWare.File::eof" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::eof" ); }

    return NOERROR;
}

//-----------------------------------------------------------------------Hostware_file::get_line

STDMETHODIMP Hostware_file::Get_line( BSTR* buffer )
{
    Z_LOGI2( "hostole", "Hostware_file::get_line\n" );
    
    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        *buffer = NULL;  // [out] soll initialisiert werden

        _any_file.get( &_buffer );

        *buffer = SysAllocStringLen_char( _buffer.char_ptr(), _buffer.length() );  
        if( !*buffer )  return E_OUTOFMEMORY;
    }
    catch( const exception & x )   { return _set_excepinfo( x, "hostWare.File::get_line" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::get_line" ); }

    return NOERROR;
}

//------------------------------------------------------------------Hostware_file::create_record

STDMETHODIMP Hostware_file::Create_record( Ihostware_dynobj** object )
{
    Z_LOGI2( "hostole", "Hostware_file::create_record\n" );

    HRESULT          hr = NOERROR;
    Hostware_dynobj* o  = NULL;

    *object = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            if( !_type )  throw_xc( "SOS-1193" );

            o = new Hostware_dynobj( _type, this );
            if( !o )  return E_OUTOFMEMORY;
            o->AddRef();
            o->construct();

            *object = o;
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::create_record" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::create_record" ); }

        if( FAILED( hr ) )
            if( o )  o->Release();
    }

    return hr;
}

//---------------------------------------------------------------------Hostware_file::create_key

STDMETHODIMP Hostware_file::Create_key( Ihostware_dynobj** key )
{
    Z_LOGI2( "hostole", "Hostware_file::create_key\n" );

    Hostware_dynobj* k = NULL;

    *key = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        if( !_key_descr )  throw_xc( "SOS-1214", "hostOLE" );
        k = new Hostware_dynobj( SOS_CAST( Record_type, _key_descr->type_ptr() ), this );
        if( !k )  return E_OUTOFMEMORY;
        k->AddRef();

        k->construct();
        *key = k;
    }
    catch( const exception&  x )   { delete k; return _set_excepinfo( x, "hostWare.File::create_key" ); }
    catch( const _com_error& x )   { delete k; return _set_excepinfo( x, "hostWare.File::create_key" ); }

    return NOERROR;
}

//----------------------------------------------------------------------------Hostware_file::put

STDMETHODIMP Hostware_file::Put( VARIANT* record )
{
    Z_LOGI2( "hostole", "Hostware_file::put\n" );

    HRESULT             hr = NOERROR;
    Hostware_dynobj*    r  = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            r = get_record_data( *record );
            _any_file.put( r->_record );
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::put" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::put" ); }

        if( r )  r->Release();
    }

    return hr;
}

//---------------------------------------------------------------------------Hostware_file::get

STDMETHODIMP Hostware_file::Get( /*out*/ Ihostware_dynobj** object_pp )
{
    Z_LOGI2( "hostole", "Hostware_file::get\n" );

    Hostware_dynobj* o = NULL;

    *object_pp = NULL;       // [out] initialisieren

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        o = new Hostware_dynobj( _type, this );
        if( !o )  return E_OUTOFMEMORY;
        o->AddRef();

        if( o->_type )  o->_record.allocate_min( o->_type->field_size() );

        _any_file.get( &o->_record );

        o->finish();

        *object_pp = o;
    }
    catch( const exception&  x )   { delete o; return _set_excepinfo( x, "hostWare.File::get" ); }
    catch( const _com_error& x )   { delete o; return _set_excepinfo( x, "hostWare.File::get" ); }

    return NOERROR;
}

//-----------------------------------------------------------------------Hostware_file::set_key

STDMETHODIMP Hostware_file::Set_key( VARIANT* key )
{
    Z_LOGI2( "hostole", "Hostware_file::set_key\n" );

    HRESULT             hr = NOERROR;
    Hostware_dynobj*    k = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            k = get_key_data( *key );

            _any_file.set( k->_record );
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::set_key" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::set_key" ); }

        if( k )  k->Release();
    }

    return hr;
}

//--------------------------------------------------------------------Hostware_file::delete_key

STDMETHODIMP Hostware_file::Delete_key( VARIANT* key )
{
    Z_LOGI2( "hostole", "Hostware_file::delete_key\n" );

    HRESULT             hr = NOERROR;
    Hostware_dynobj*    k = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            k = get_key_data( *key );

            _any_file.del( k->_record );
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::delete_key" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::delete_key" ); }

        if( k )  k->Release();
    }

    return hr;
}

//-----------------------------------------------------------------------Hostware_file::get_key

STDMETHODIMP Hostware_file::Get_key( VARIANT* key, Ihostware_dynobj** object )
{
    Z_LOGI2( "hostole", "Hostware_file::get_key\n" );

    HRESULT          hr = NOERROR;
    Hostware_dynobj* o  = NULL;
    Hostware_dynobj* k  = NULL;

    *object = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            k = get_key_data( *key );

            o = new Hostware_dynobj( _type, this );
            if( !o )  return E_OUTOFMEMORY;
            o->AddRef();

            _any_file.get_key( &o->_record, k->_record );

            o->finish();

            *object = o;
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::get_key" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::get_key" ); }

        if( FAILED( hr )  &&  o )  o->Release();

        if( k )  k->Release();
    }

    return hr;
}

//-----------------------------------------------------------------------Hostware_file::insert

STDMETHODIMP Hostware_file::Insert( VARIANT* record )
{
    Z_LOGI2( "hostole", "Hostware_file::insert\n" );

    HRESULT             hr = NOERROR;
    Hostware_dynobj*    r  = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            r = get_record_data( *record );
            _any_file.insert( r->_record );
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::insert" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::insert" ); }

        if( r )  r->Release();
    }

    return hr;
}

//------------------------------------------------------------------------Hostware_file::update

STDMETHODIMP Hostware_file::Update( VARIANT* record )
{
    Z_LOGI2( "hostole", "Hostware_file::update\n" );

    HRESULT hr = NOERROR;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        Hostware_dynobj* r = NULL;

        try 
        {
            r = get_record_data( *record );

            _any_file.update( r->_record );
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::update" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::update" ); }

        if( r )  r->Release();
    }

    return hr;
}

//-----------------------------------------------------------------Hostware_file::update_direct
                                                                      
STDMETHODIMP Hostware_file::Update_direct( VARIANT* record )
{                                                                     
    Z_LOGI2( "hostole", "Hostware_file::update_direct\n" );

    HRESULT             hr = NOERROR;
    Hostware_dynobj*    r  = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    {
        try {
            r = get_record_data( *record );
            _any_file.update_direct( r->_record );
        }
        catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::update_direct" ); }
        catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::update_direct" ); }

        if( r )  r->Release();
    }

    return hr;
}

//-------------------------------------------------------------------------Hostware_file::store

STDMETHODIMP Hostware_file::Store( VARIANT* record )
{
    Z_LOGI2( "hostole", "Hostware_file::store\n" );

    HRESULT             hr = NOERROR;
    Hostware_dynobj*    r  = NULL;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try {
        r = get_record_data( *record );
        _any_file.store( r->_record );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::store" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::store" ); }

    if( r )  r->Release();

    return hr;
}

//----------------------------------------------------------------Hostware_file::put_date_format

STDMETHODIMP Hostware_file::put_Date_format( BSTR format )
{
    Z_LOGI2( "hostole", "Hostware_file::put_date_format\n" );

    HRESULT hr = NOERROR;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try {
        Sos_string fmt = bstr_as_string(format);      // Konvertiert Unicode nach Ascii

        _text_format.date( c_str( fmt ) );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::date_format" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::date_format" ); }

    return hr;
}

//----------------------------------------------------------------Hostware_file::put_date_format

STDMETHODIMP Hostware_file::put_Date_time_format( BSTR format )
{
    Z_LOGI2( "hostole", "Hostware_file::put_date_time_format\n" );

    HRESULT hr = NOERROR;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try {
        Sos_string fmt = bstr_as_string(format);      // Konvertiert Unicode nach Ascii

        _text_format.date_time( c_str( fmt ) );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::date_time_format" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::date_time_format" ); }

    return hr;
}

//-------------------------------------------------------------Hostware_file::put_decimal_symbol

STDMETHODIMP Hostware_file::put_Decimal_symbol( BSTR symbol )
{
    Z_LOGI2( "hostole", "Hostware_file::put_decimal_symbol\n" );

    HRESULT hr = NOERROR;

    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try 
    {
        Sos_string decimal_symbol = bstr_as_string(symbol);      // Konvertiert Unicode nach Ascii
        if( length( decimal_symbol ) >= 1 )  _text_format.decimal_symbol( decimal_symbol[ 0 ] );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::decimal_symbol" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::decimal_symbol" ); }

    return hr;
}

//-----------------------------------------------------------------Hostware_file::get_field_name

STDMETHODIMP Hostware_file::get_Field_name( LONG field_no, BSTR* name )
{
    Z_LOGI2( "hostole", "Hostware_file::get_field_name\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    try
    {
        if( !_type )  throw_xc( "SOS-1193" );

        *name = SysAllocString_string( _type->field_descr_ptr( field_no )->name() );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::field_name" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::field_name" ); }

    return hr;
}

//----------------------------------------------------------------Hostware_file::get_field_count

STDMETHODIMP Hostware_file::get_Field_count( LONG* number )
{
    Z_LOGI2( "hostole", "Hostware_file::get_field_count\n" );
    Z_COM_MY_THREAD_ONLY;
    
    *number = _type? _type->field_count() : 0;

    return NOERROR;
}

//------------------------------------------------------------------------Hostware_file::prepare

STDMETHODIMP Hostware_file::Prepare( BSTR filename )
{
    Z_COM_MY_THREAD_ONLY;
    Z_MUTEX( hostware_mutex )
    try {
        _any_file.prepare( _filename_prefix + bstr_as_string(filename), (Any_file::Open_mode)0 );
        _type      = +_any_file.spec()._field_type_ptr;
        _key_descr = +_any_file.spec()._key_specs._key_spec._field_descr_ptr;
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.File::prepare" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::prepare" ); }

    return NOERROR;
}

//-----------------------------------------------------------------Hostware_file::put_parameters

STDMETHODIMP Hostware_file::put_Parameters( SAFEARRAY* param_array )
{
    Z_LOGI2( "hostole", "Hostware_file::put_parameters\n" );
    Z_COM_MY_THREAD_ONLY;
    
    HRESULT     hr          = ERROR;
    VARIANT*    params      = NULL;
    LONG        lower_bound;
    LONG        upper_bound;
    int         i, n;

    Z_MUTEX( hostware_mutex )
    try {
        hr = SafeArrayGetLBound( param_array, 1, &lower_bound );
        if( FAILED( hr ) )  goto FEHLER;

        hr = SafeArrayGetUBound( param_array, 1, &upper_bound );
        if( FAILED( hr ) )  goto FEHLER;

        n = upper_bound - lower_bound + 1;

        _param_values.clear();
        _param_values.first_index( 1 );
        _param_values.last_index( n );

        hr = SafeArrayAccessData( param_array, (void __huge**)&params );
        if( FAILED( hr ) )  goto FEHLER;

        for( i = 0; i < n; i++ ) 
        {
            Z_LOG2( "hostole", "parameter(" << i << ")=" << zschimmer::com::variant_type_name( params[i] ) << ':' << params[i] << '\n' );
            variant_to_dynobj( params[ i ], &_param_values[ i+1 ] );
            _any_file.bind_parameter( 1 + i, &_param_values[ i+1 ] );
        }

        SafeArrayUnaccessData( param_array );
        return NOERROR;
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::parameters" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::parameters" ); }

  FEHLER:
    if( params )  SafeArrayUnaccessData( param_array );
    return hr;
}

//------------------------------------------------------------------Hostware_file::put_parameter

STDMETHODIMP Hostware_file::put_Parameter( LONG no, VARIANT* value )
{
    Z_LOGI2( "hostole", "Hostware_file::parameter(" << no << ")=" << zschimmer::com::variant_type_name( value ) << ':' << *value << '\n' );
    Z_COM_MY_THREAD_ONLY;
    
    HRESULT hr = ERROR;

    Z_MUTEX( hostware_mutex )
    try {
        _param_values.first_index( 1 );
        _param_values.last_index( 100 );        // max 100 Parameter
      //if( _param_values.last_index() < no )  _param_values.last_index( no );

        variant_to_dynobj( *value, &_param_values[ no ] );

        _any_file.bind_parameter( no, &_param_values[ no ] );

        hr = NOERROR;
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.File::parameters" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.File::parameters" ); }

    return hr;
}

//------------------------------------------------------------------------Hostware_file::execute

STDMETHODIMP Hostware_file::Execute()
{
    Z_LOGI2( "hostole", "Hostware_file::execute\n" );
    Z_COM_MY_THREAD_ONLY;
    
    Z_MUTEX( hostware_mutex )
    try 
    {
        _any_file.execute();
    }
    catch( const exception&  x )   { return _set_excepinfo( x, "hostWare.File::execute" ); }
    catch( const _com_error& x )   { return _set_excepinfo( x, "hostWare.File::execute" ); }

    return NOERROR;
}

//-----------------------------------------------------------------------Hostware_file::get_type

STDMETHODIMP Hostware_file::get_Type( Ihostware_record_type** type )
{
    Z_LOGI2( "hostole", "Hostware_file::get_type\n" );
    Z_COM_MY_THREAD_ONLY;
    
    Z_MUTEX( hostware_mutex )
    {
        *type = new Hostware_record_type( +_any_file.spec()._field_type_ptr );
    }
    (*type)->AddRef();  

    return NOERROR; 
}

//-------------------------------------------------------------------------Hostware_file::rewind

STDMETHODIMP Hostware_file::Rewind()
{
    HRESULT hr = NOERROR;

    Z_LOGI2( "hostole", "Hostware_file::rewind\n" );
    Z_COM_MY_THREAD_ONLY;
    
    Z_MUTEX( hostware_mutex )
    try {
        _any_file.rewind();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.File::rewind" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.File::rewind" ); }

    return hr;
}

//----------------------------------------------------------------Hostware_file::get_debug_info

STDMETHODIMP Hostware_file::get_Debug_info( BSTR* text )
{
    //Z_LOGI2( "hostole", "Hostware_file::get_debug_info\n" );

    HRESULT hr = NOERROR;
    
    Z_MUTEX( hostware_mutex )
    try
    {
        if( !_any_file.opened() ) 
        {
            *text = SysAllocString( L"CLOSED" );
        } 
        else 
        {
            *text = SysAllocString_string( _any_file.name() );
        }
    }
    catch( const Xc& x ) 
    { 
        Dynamic_area buffer;
        x.get_text( &buffer );
        *text = SysAllocStringLen_char( buffer.char_ptr(), buffer.length() );
    }

    return hr;
}

//-----------------------------------------------------------------Hostware_file::get_row_count

STDMETHODIMP Hostware_file::get_Row_count( int* result )
{
    HRESULT hr = NOERROR;

    Z_LOGI2( "hostole", __FUNCTION__ << "\n" );
    Z_COM_MY_THREAD_ONLY;
    
    Z_MUTEX( hostware_mutex )
    try 
    {
        *result = _any_file.record_count();
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//--------------------------------------------------------------------Hostware_file::get_Db_name_in

HRESULT Hostware_file::get_Db_name_in( BSTR* result )
{
    HRESULT hr;
    Bstr    db_name;
    Bstr    filename;

    hr = get_Filename( &db_name );
    if( FAILED(hr) )  return hr;

    filename += "-in ";
    filename += db_name;
    filename += "  ";

    *result = filename.take();

    return S_OK;
}

//---------------------------------------------------------------------------Hostware_file::Db_open

STDMETHODIMP Hostware_file::Db_open( BSTR select_statement, SAFEARRAY* param_array, Ihostware_file** result )
{
    Z_LOGI2( "hostole", Z_FUNCTION << "\n" );

    HRESULT hr = S_OK;
    Bstr    filename;
    
    hr = get_Db_name_in( &filename );
    if( FAILED(hr) )  return hr;

    filename += select_statement;

    ptr<Hostware_file> file = new Hostware_file();
    hr = file->Prepare( filename );
    if( FAILED(hr) )  return hr;


    Locked_safearray<VARIANT> params ( param_array );
    int n = params.count();

    for( int i = 0; i < n; i++ ) 
    {
        hr = file->put_Parameter( 1 + i, &params[i] );
        if( FAILED(hr) )  return hr;
    }


    hr = file->Execute();
    if( FAILED(hr) )  return hr;


    *result = file.take();
    return hr;
}

//---------------------------------------------------------------------Hostware_file::Db_get_single

STDMETHODIMP Hostware_file::Db_get_single( BSTR select_statement, SAFEARRAY* param_array, Ihostware_dynobj** result )
{
    Z_LOGI2( "hostole", Z_FUNCTION << "\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = S_OK;
    Bstr    filename;
    
    hr = get_Db_name_in( &filename );
    if( FAILED(hr) )  return hr;

    filename += select_statement;

    hr = Hostware().Get_single( filename, param_array, result );
    if( FAILED(hr) )  return hr;

    return hr;
}

//---------------------------------------------------------------Hostware_file::Db_get_single_value

STDMETHODIMP Hostware_file::Db_get_single_value( BSTR select_statement, SAFEARRAY* param_array, VARIANT* result )
{
    Z_LOGI2( "hostole", Z_FUNCTION << "\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = S_OK;
    Bstr    filename;
    
    hr = get_Db_name_in( &filename );
    if( FAILED(hr) )  return hr;

    filename += "  ";
    filename += select_statement;


    hr = Hostware().Get_single_value( filename, param_array, result );
    if( FAILED(hr) )  return hr;

    return hr;
}

//---------------------------------------------------------------------Hostware_file::Db_get_array

STDMETHODIMP Hostware_file::Db_get_array( BSTR select_statement, SAFEARRAY* param_array, SAFEARRAY** result )
{
    Z_LOGI2( "hostole", Z_FUNCTION << "\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = S_OK;
    Bstr    filename;
    
    hr = get_Db_name_in( &filename );
    if( FAILED(hr) )  return hr;

    filename += select_statement;

    hr = Hostware().Get_array( filename, param_array, result );
    if( FAILED(hr) )  return hr;

    return hr;
}

//------------------------------------------------------------------------Hostware_file::Db_execute

STDMETHODIMP Hostware_file::Db_execute( BSTR statement, SAFEARRAY* param_array, int* result )
{
    Z_LOGI2( "hostole", Z_FUNCTION << "\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT                   hr     = S_OK;
    Locked_safearray<VARIANT> params ( param_array );

    int n = params.count();

    if( n == 0 )
    {
        hr = Put_line( statement );
        if( FAILED(hr) )  return hr;

        hr = get_Row_count( result );
        if( FAILED(hr) )  return hr;
    }
    else
    {
        Bstr            filename;
        Hostware_file   file;

        hr = get_Filename( &filename );
        if( FAILED(hr) )  return hr;

        filename += "  ";
        filename += statement;


        hr = file.Prepare( filename );
        if( FAILED(hr) )  return hr;

        for( int i = 0; i < n; i++ ) 
        {
            hr = file.put_Parameter( 1 + i, &params[i] );
            if( FAILED(hr) )  return hr;
        }


        hr = file.Execute();
        if( FAILED(hr) )  return hr;

        hr = file.get_Row_count( result );
        if( FAILED(hr) )  return hr;
    }

    return hr;
}

//----------------------------------------------------------------------Hostware_file::get_Filename

STDMETHODIMP Hostware_file::get_Filename( BSTR* result )
{
    LOGI( Z_FUNCTION << '\n' );

    HRESULT hr = S_OK;

    try
    {
        hr = String_to_bstr( _any_file.filename(), result );
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }

    return hr;
}

//---------------------------------------------------------------Hostware_file::Db_get_blob_or_clob

STDMETHODIMP Hostware_file::Db_get_blob_or_clob( BSTR table_name, BSTR field_name, BSTR where_clause, SAFEARRAY* param_array, bool clob, VARIANT* result )
{
    LOGI( Z_FUNCTION << '\n' );
    
    HRESULT hr = S_OK;
    
    try
    {
        Bstr filename_bstr;

        hr = get_Db_name_in( &filename_bstr );
        if( FAILED(hr) )  return hr;

        string rest;
        rest += " -table=" + string_from_bstr( table_name );
        rest += clob? " -clob=" : " -blob=";
        rest += string_from_bstr( field_name );
        rest += "  ";
        rest += string_from_bstr( where_clause );

        filename_bstr += rest;


        Hostware_file             file;
        Locked_safearray<VARIANT> params ( param_array );
        int n = params.count();

        try
        {
            if( n == 0 )
            {
                file._any_file.open( string_from_bstr( filename_bstr ), (Any_file::Open_mode)0 );
            }
            else
            {
                file._any_file.prepare( string_from_bstr( filename_bstr ), (Any_file::Open_mode)0 );

                for( int i = 0; i < n; i++ ) 
                {
                    hr = file.put_Parameter( 1 + i, &params[i] );
                    if( FAILED(hr) )  return hr;
                }

                file._any_file.execute();
            }
        }
        catch( const Not_exist_error& )
        {
            result->vt = VT_EMPTY;
            return hr;
        }


        VariantInit( result );  result->vt = VT_BSTR;  V_BSTR( result ) = NULL;

        hr = String_to_bstr( sos::file_as_string( &file._any_file, SYSTEM_NL ), &V_BSTR( result ) );
        if( FAILED(hr) )  return hr;

        hr = file.Close();
        if( FAILED(hr) )  return hr;
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, __FUNCTION__ ); }
    
    return hr;
}

//-----------------------------------------------------------------------Hostware_file::Db_get_blob

STDMETHODIMP Hostware_file::Db_get_blob( BSTR table_name, BSTR field_name, BSTR where_clause, SAFEARRAY* param_array, VARIANT* result )
{
    return Db_get_blob_or_clob( table_name, field_name, where_clause, param_array, false, result );
}

//-----------------------------------------------------------------------Hostware_file::Db_get_clob

STDMETHODIMP Hostware_file::Db_get_clob( BSTR table_name, BSTR field_name, BSTR where_clause, SAFEARRAY* param_array, VARIANT* result )
{
    return Db_get_blob_or_clob( table_name, field_name, where_clause, param_array, true, result );
}

//---------------------------------------------------------------Hostware_file::get_record_data

Hostware_dynobj* Hostware_file::get_record_data( const VARIANT& record )
{
    if( !_type )  throw_xc( "SOS-1193" );

    return variant_as_hostware_dynobj( _type, record );
}

//-------------------------------------------------------------------Hostware_file::get_key_data

Hostware_dynobj* Hostware_file::get_key_data( const VARIANT& key )
{
    //if( !_key_descr  ||  !_key_descr->type_ptr() )  throw_xc( "SOS-1214" );

    return variant_as_hostware_dynobj( _key_descr && _key_descr->type_ptr()? SOS_CAST( Record_type, _key_descr->type_ptr() ) 
                                                                           : NULL, 
                                       key );
}

//---------------------------------------------------------------------Hostware_file::_obj_print

void Hostware_file::_obj_print( ostream* s ) const
{
    *s << "Hostware_file(" << _any_file.name() << ')';
               //else  *s << "Hostware_file (empty)";
}

//------------------------------------------------------------------------Hostware_dynobj::_methods
#ifdef Z_COM

const Com_method Hostware_dynobj::_methods[] =
{ 
   // _flags              ,     _name                   , _method                                                     , _result_type, _types        , _default_arg_count
    { DISPATCH_PROPERTYGET,  1, "Java_class_name"       , (Com_method_ptr)&Hostware_dynobj::get_Java_class_name       , VT_BSTR     },
    { DISPATCH_PROPERTYGET,  2, "obj_field_count"       , (Com_method_ptr)&Hostware_dynobj::get_Obj_field_count       , VT_I4       },
    { DISPATCH_PROPERTYGET,  3, "obj_field_name"        , (Com_method_ptr)&Hostware_dynobj::get_Obj_field_name        , VT_BSTR     , { VT_I4 } },
    { DISPATCH_PROPERTYGET,  4, "obj_field_index"       , (Com_method_ptr)&Hostware_dynobj::get_Obj_field_index       , VT_I4       , { VT_BSTR } },
    { DISPATCH_PROPERTYGET,  5, "obj_field"             , (Com_method_ptr)&Hostware_dynobj::Obj_field                 , VT_VARIANT  , { VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYPUT,  6, "obj_set_field"         , (Com_method_ptr)&Hostware_dynobj::Obj_set_field             , VT_EMPTY    , { VT_BSTR, VT_VARIANT|VT_BYREF } },
    { DISPATCH_PROPERTYPUT,  7, "obj_read_null_as_empty", (Com_method_ptr)&Hostware_dynobj::put_Obj_read_null_as_empty, VT_EMPTY    , { VT_BOOL } },
    {}
};

#endif
//---------------------------------------------------------------Hostware_dynobj::Hostware_dynobj

Hostware_dynobj::Hostware_dynobj( Record_type* type, Hostware_file* file )
:
    Sos_ole_object( &hostware_dynobj_class, (Ihostware_dynobj*)this, NULL ),
    _type ( type ),
    _Obj_write_empty_as_null(false),
    _Obj_read_null_as_empty(false)
{
    //?jz 1.6.98  if( !_type )  _type = Record_type::create();

    // Parameter file nicht merken, wird evtl. vor diesem Objekt geschlossen!
    if( file ) {
        _text_format = file->_text_format;
        _Obj_write_empty_as_null = file->_Write_empty_as_null;
        _Obj_read_null_as_empty  = file->_Read_null_as_empty;
    }
}


//---------------------------------------------------------------Hostware_dynobj::Hostware_dynobj

Hostware_dynobj::Hostware_dynobj( const Hostware_dynobj& o )
:
    Sos_ole_object( &hostware_dynobj_class, (Ihostware_dynobj*)this, NULL )
{
    *this = o;
}

//--------------------------------------------------------------Hostware_dynobj::~Hostware_dynobj

Hostware_dynobj::~Hostware_dynobj()
{
    Z_LOG2( "hostole", "~Hostware_dynobj()\n" );
}

//--------------------------------------------------------------------Hostware_dynobj::operator =

Hostware_dynobj& Hostware_dynobj::operator = ( const Hostware_dynobj& o )
{
    _type           = o._type;
    _record         = o._record;
    _text_format    = o._text_format;

    return *this;
}

//--------------------------------------------------------------Hostware_dynobj::Clone

STDMETHODIMP Hostware_dynobj::Clone( Ihostware_dynobj** object )
{
    Z_LOGI2( "hostole", "Hostware_dynobj::clone\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT          hr = NOERROR;
    Hostware_dynobj* o  = NULL;

    *object = NULL;

    try 
    {
        //o = new Hostware_dynobj( NULL, NULL ); // zweiter Copy-Konstruktor gab Probleme wg. DESCRIBE_CLASS o.ä.
        o = new Hostware_dynobj( *this );

        if( !o )  return E_OUTOFMEMORY;

        o->AddRef();

        // Werte kopieren
        ///o->_type                    = _type; // Probs mit Field_type vs. Record_type, js 28.04.98
        //o->_text_format             = _text_format;
        //o->_obj_write_empty_as_null = _obj_write_empty_as_null;
        //o->construct();

        *object = o;
    }
    catch( const exception& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::clone" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::clone" ); }

    if( FAILED( hr ) ) {
        if( o )  o->Release();
    }

    return hr;
}

//---------------------------------------------------------------------------Hostware_dynobj::field

Field_descr* Hostware_dynobj::field( VARIANT* name )
{
    HRESULT      hr    = NOERROR;
    Field_descr* field = NULL;
    VARIANT      vt;
    
    VariantInit( &vt );

    if( !_type )  throw_xc( "SOS-1193" );

    try
    {
        if( V_VT( name ) == VT_I4 )  field = _type->field_descr_ptr( V_I4( name ) );
        else
        if( V_VT( name ) == VT_I2 )  field = _type->field_descr_ptr( V_I2( name ) );
        else {
            hr = VariantChangeType( &vt, name, 0, VT_BSTR );
            if( FAILED( hr ) )  throw_ole( hr, "VariantChangeType" );

            Sos_string name = bstr_as_string( V_BSTR( &vt ) );  // Feldname oder Feldnummer
            field = SOS_CAST( Record_type, _type ) -> field_descr_ptr( name );
        }

        VariantClear( &vt );
    }
    catch(...) {
        VariantClear( &vt );
        throw;
    }

    return field;
}

//----------------------------------------------------------------Hostware_dynobj::QueryInterface

STDMETHODIMP Hostware_dynobj::QueryInterface( const IID& iid, void** result )
{                                                                    
	if( iid == IID_hostware_dynobj )
	{
		AddRef();
		*result = this;
		return S_OK;
	}

    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihostware_dynobj    , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Ihas_java_class_name, result );

    return Sos_ole_object::QueryInterface( iid, result );
}                                                                                                                                       

//------------------------------------------------------------------------Hostware_dynobj::assign

void Hostware_dynobj::assign( const VARIANT& variant )
{
    _record.allocate_min( _type->field_size() );

    if( V_VT( &variant ) == VT_ARRAY ) 
    {
        LONG        lower_bound;
        LONG        upper_bound;
        int         i;
        int         n;
        VARIANT*    array = NULL;
        HRESULT hr;


        hr = SafeArrayGetLBound( V_ARRAY( &variant ), 1, &lower_bound );
        if( FAILED( hr ) )  throw_ole( hr, "SafeArrayGetLBound" );

        hr = SafeArrayGetUBound( V_ARRAY( &variant ), 1, &upper_bound );
        if( FAILED( hr ) )  throw_ole( hr, "SafeArrayGetUBound" );

        n = upper_bound - lower_bound + 1;
        if( n > _type->field_count() )  { Sos_string n_str = as_string( n ); throw_xc( "SOS-1375", _type, c_str(n_str) ); }


        hr = SafeArrayAccessData( V_ARRAY( &variant ), (void __huge**)&array );
        if( FAILED( hr ) )  throw_ole( hr, "SafeArrayAccessData" );

        try 
        {
            for( i = 0; i < _type->field_count(); i++ ) {
                if( i >= n )  break;
                variant_to_char( array[ i ], &_value_buffer );
                _type->read_text( _record.byte_ptr(), _value_buffer.char_ptr() );
            }

            SafeArrayUnaccessData( V_ARRAY( &variant ) );
        }
        catch( const exception& )
        {
            SafeArrayUnaccessData( V_ARRAY( &variant ) );
            throw;
        }
        catch( const _com_error& )
        {
            SafeArrayUnaccessData( V_ARRAY( &variant ) );
            throw;
        }

        // Nicht gesetzte Felder löschen:
        for(; i < _type->field_count(); i++ )  _type->clear( _record.byte_ptr() );
    } 
    else 
    {
        variant_to_char( variant, &_value_buffer );
        _value_buffer.append( '\0' );
        _type->read_text( _record.byte_ptr(), _value_buffer.char_ptr() );
        _record.length( _type->field_size() );
    }
}

//----------------------------------------------------------------Hostware_dynobj::GetIDsOfNames

STDMETHODIMP Hostware_dynobj::GetIDsOfNames( const IID& riid, OLECHAR** rgszNames, UINT cNames,
                                             LCID lcid, DISPID* rgDispID )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT     hr;
    const char* name_ptr;

    if( hostole_log_all )  log_GetIDsOfNames( riid, rgszNames, cNames, lcid );

#   ifdef Z_COM
        hr = ::zschimmer::com::Com_get_dispid( _methods, riid, rgszNames, cNames, lcid, rgDispID );
#    else
        hr = Sos_ole_object::GetIDsOfNames( riid, rgszNames, cNames, lcid, rgDispID );                                                    
#   endif
    
    if( hr == DISP_E_UNKNOWNNAME
     || hr == DISP_E_MEMBERNOTFOUND )  // Dieser Fehlercode kommt, wenn die Methode nicht bekannt ist.
    {
        hr = NOERROR;

        if( riid != IID_NULL )  return DISP_E_UNKNOWNINTERFACE;
        if( cNames != 1 )       return DISP_E_UNKNOWNNAME;
        if( !_type )            return DISP_E_UNKNOWNNAME;       // NULL-Objekt

        char name [ 80 ];
        WideCharToMultiByte( CP_ACP, 0, rgszNames[0], -1, name, sizeof name, NULL, NULL );
        name_ptr = name;

        if( !_type->obj_is_type( tc_Record_type ) )  return (HRESULT)DISP_E_UNKNOWNNAME;

        Field_descr* f = ((Record_type*)+_type)->field_descr_by_name_or_0( name_ptr );

        if( !f )  return DISP_E_UNKNOWNNAME;

        *rgDispID = (LONG)f;   // DISPID ist ein Field_descr*
        // Falls das zu wackelig ist: schrittweise Array mit Feldnamen und Field_descr* aufbauen
    }

    return hr;
}

//-----------------------------------------------------------------------Hostware_dynobj::Invoke

STDMETHODIMP Hostware_dynobj::Invoke( DISPID dispID, const IID& riid, LCID lcid,
                                      unsigned short wFlags, DISPPARAMS* pDispParams,
                                      VARIANT* pVarResult, EXCEPINFO* excepinfo,
                                      UINT* puArgErr )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    if( dispID > 0  &&  dispID < 1000 )      // Standard-Methode aus hostole.odl
    { 
#       ifdef Z_COM
            return ::zschimmer::com::Com_invoke( this, _methods, dispID, riid, lcid, wFlags, pDispParams, puArgErr, pVarResult, excepinfo );
#        else
            return Sos_ole_object::Invoke( dispID, riid, lcid, wFlags, pDispParams, pVarResult, excepinfo, puArgErr );  
#       endif
    }

    Z_MUTEX( hostware_mutex )
    {
        if( hostole_log_all )  log_invoke( dispID, riid, lcid, wFlags, pDispParams ); //Z_LOG2( "hostole", *this << ".Invoke() " );

        Field_descr*    field           = NULL;
        Field_descr     field_descr;
        VARIANT         vt;             VariantInit( &vt );

        // Prüfen
        if( wFlags & DISPATCH_PROPERTYPUT ) 
        {
            if( pDispParams->cNamedArgs == 1  &&  pDispParams->rgdispidNamedArgs[0] == DISPID_PROPERTYPUT ) ;//ok
            else
            {
                hr = DISP_E_BADPARAMCOUNT;
                goto ENDE;
            }
        }
        else
        {
            if( pDispParams->cNamedArgs != 0 )  { hr = (HRESULT)DISP_E_BADPARAMCOUNT; goto ENDE; }
        }



        if( riid != IID_NULL )  { hr = (HRESULT)DISP_E_UNKNOWNINTERFACE; goto ENDE; }

        try 
        {
            if( dispID == DISPID_VALUE
             || dispID == DISPID_EVALUATE )
            {
                if( pDispParams->cArgs - pDispParams->cNamedArgs == 0 )  // o = value, o für sich selbst, nur möglich, wenn o nur ein Feld (bzw. Gruppenfeld) ist. Oder o(name)
                {
                    field_descr._type_ptr = _type;   // Kann NULL sein
                    field_descr._offset = 0;
                    field = &field_descr;
                }
                else
                if( pDispParams->cArgs - pDispParams->cNamedArgs == 1 )    // o!name [ = value ]
                {
                    VARIANT* arg = &pDispParams->rgvarg[ pDispParams->cArgs - 1 ];  // Der erste Parameter steht am Ende

                    if( V_VT( arg ) == VT_I4 )  field = _type->field_descr_ptr( V_I4( arg ) );
                    else
                    if( V_VT( arg ) == VT_I2 )  field = _type->field_descr_ptr( V_I2( arg ) );
                    else
                    {
                        hr = VariantChangeType( &vt, arg, 0, VT_BSTR );
                        if( FAILED( hr ) ) {  if( puArgErr )  *puArgErr = 0;  goto ENDE; }
                        
                        Sos_string name = bstr_as_string(V_BSTR(&vt));  // Feldname oder Feldnummer
                        field = SOS_CAST( Record_type, _type ) -> field_descr_ptr( name );
                    }
                }
                else 
                {
                    hr = (HRESULT)DISP_E_BADPARAMCOUNT;
                    goto ENDE;
                }
            }
            else
            if( dispID < 0 && dispID > -4096 )  { hr = (HRESULT)DISP_E_MEMBERNOTFOUND; goto ENDE; }  // Besser keine Adresse in dispID speichern!
            else
            {
                if( pDispParams->cArgs != ( wFlags & DISPATCH_PROPERTYPUT? 1 : 0 ) )  { hr = (HRESULT)DISP_E_BADPARAMCOUNT; goto ENDE; }

                field = (Field_descr*)dispID;
            }

            if( wFlags & ( DISPATCH_PROPERTYGET | DISPATCH_METHOD ) )
            {
                if( hostole_log_all )  Z_LOG2( "hostole", *field << " => " );

                if( !pVarResult )  { hr = (HRESULT)E_INVALIDARG; goto ENDE; }

                // Warum ist lcid=0x409 (englisch)? Das führt zum Dezimalpunkt, VB5 erwartet aber Dezimalkomma! jz 4.5.98
                field_to_variant( field, _record.byte_ptr(), pVarResult, &_value_buffer, (LCID)0 /*lcid*/ );

                if( V_VT(pVarResult) == VT_NULL  &&  _Obj_read_null_as_empty )  V_VT(pVarResult) = VT_BSTR, V_BSTR(pVarResult) = NULL;

                if( hostole_log_all )  Z_LOG2( "hostole", *pVarResult );
            }
            else
            if( wFlags & DISPATCH_PROPERTYPUT )
            {
                if( hostole_log_all )  Z_LOG2( "hostole", *field << " := " );

                VARIANT*  arg = &pDispParams->rgvarg[ 0 ];

                if( !field->type_ptr() )  throw_xc( "SOS-1193" );                  // Satzbeschreibung fehlt. NULL-Objekt ist nicht beschreibbar

                if( V_VT( arg ) == VT_NULL ) {
                    if( hostole_log_all )  Z_LOG2( "hostole", "NULL" );
                    field->set_null( _record.byte_ptr() );
                } else {
                    variant_to_char( *arg, &_value_buffer, lcid );

                    if( _Obj_write_empty_as_null  &&  _value_buffer.length() == 0 ) {
                        if( hostole_log_all )  Z_LOG2( "hostole", "NULL (empty)" );
                        field->set_null( _record.byte_ptr() );
                    } else {
                        _value_buffer.append( '\0' );
                        if( hostole_log_all )  Z_LOG2( "hostole", _value_buffer.char_ptr() );
                        field->read_text( _record.byte_ptr(), _value_buffer.char_ptr() );
                    }
                }
            }
        }
        catch( const exception&  x )   { fill_excepinfo( x, excepinfo ); hr = DISP_E_EXCEPTION; }
        catch( const _com_error& x )   { fill_excepinfo( x, excepinfo ); hr = DISP_E_EXCEPTION; }

    ENDE:
        if( log_ptr && hr ) {
            if( !hostole_log_all ) { log_invoke( dispID, riid, lcid, wFlags, pDispParams ); ; if( field ) *log_ptr << *field; }
            *log_ptr << " hresult=" << hex << hr << dec << endl;
        } else {
            if( hostole_log_all )  Z_LOG2( "hostole", '\n' );
        }

        VariantClear( &vt );
    }

    return hr;
}

//----------------------------------------------------------------Hostware_dynobj::obj_add_field

STDMETHODIMP Hostware_dynobj::Obj_add_field( BSTR name_bstr, VARIANT* type_vt )
{
    Z_LOGI2( "hostole", "Hostware_dynobj::obj_add_field()\n" );  // \"" << name << "\",\"" << type << "\")\n" );
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try {
        Sos_string          name;
        Sos_string          type_name;
        Sos_ptr<Field_type> type;
        Field_descr*        field = NULL;

        name = bstr_as_string(name_bstr);
        type_name = variant_as_string( *type_vt );

        if( !_type )  _type = Record_type::create();

        if( _type->obj_ref_count() > 1 )  _type = copy_record_type( SOS_CAST( Record_type, _type ) );

        try
        {
            field = SOS_CAST( Record_type, _type ) -> add_field( type_name, name, true );
        }
        catch( const Xc& x )
        {
            if( strcmp( x.code(), "SOS-1209" ) == 0 )       // Spezial für sosfact.dll (VB-Implementierung) 5.10.00
            {
                field = SOS_CAST( Record_type, _type ) -> add_field( "Text(1024)", name, true );
            }
        }

        _record.resize_min( _type->field_size() );
        _record.length( _type->field_size() );
        field->construct( _record.byte_ptr() );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_add_field" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_add_field" ); }

    return hr;
}

//-------------------------------------------------------------Hostware_dynobj::get_obj_field_count

STDMETHODIMP Hostware_dynobj::get_Obj_field_count( int* field_count )
{
    Z_COM_MY_THREAD_ONLY;

    *field_count = _type? _type->field_count() : 0;
    return NOERROR;
}

//--------------------------------------------------------------Hostware_dynobj::get_obj_field_name

STDMETHODIMP Hostware_dynobj::get_Obj_field_name( int field_no, BSTR* name )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try 
    {
        if( !_type ) {
            *name = SysAllocString( L"" );
        } else {
            *name = SysAllocString_string( _type->field_descr_ptr( field_no )->name() );
        }
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_field_name" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_field_name" ); }

    return hr;
}

//-------------------------------------------------------------Hostware_dynobj::get_obj_field_index

STDMETHODIMP Hostware_dynobj::get_Obj_field_index( BSTR name_bstr, int* field_no )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT     hr      = NOERROR;
    Sos_string  name    = bstr_as_string(name_bstr);

    Z_MUTEX( hostware_mutex )
    try 
    {
        if( !_type )  throw_xc( "SOS-1193" );

        *field_no = SOS_CAST( Record_type, _type ) -> field_index( c_str(name) );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_field_index" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_field_index" ); }

    return hr;
}

//--------------------------------------------------------------------------Hostware_dynobj::get_Obj_xml

STDMETHODIMP Hostware_dynobj::get_Obj_xml( BSTR, BSTR* result )
{
    HRESULT hr = NOERROR;
    Z_LOGI2( "hostole", "Hostware_dynobj::get_obj_xml()\n" );
    Z_COM_MY_THREAD_ONLY;

    *result = NULL;

    Z_MUTEX( hostware_mutex )
    try 
    {
        if( !_type  ||  !_type->obj_is_type( tc_Record_type ) )   throw_xc( "SOS-1193" );

        Xml_processor xml_processor;
        xml_processor.allocate( 100*1024 );
        xml_processor.write_record_fields( (Record_type*)+_type, _record );

        *result = SysAllocStringLen_char( xml_processor._buffer.char_ptr(), xml_processor._buffer.length() );
        if( !*result )  return E_OUTOFMEMORY;
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_xml" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_xml" ); }

    return hr;
}

//--------------------------------------------------------------------------Hostware_dynobj::put_Obj_xml

STDMETHODIMP Hostware_dynobj::put_Obj_xml( BSTR, BSTR xml )
{
    HRESULT hr = NOERROR;
    Z_LOGI2( "hostole", "Hostware_dynobj::put_obj_xml()\n" );
    Z_COM_MY_THREAD_ONLY;

    Z_MUTEX( hostware_mutex )
    try 
    {
        Sos_string      xml_string = bstr_as_string( xml );
        Xml_processor   xml_processor;

        xml_processor.init_read( c_str( xml_string), length( xml_string ) );
        
        _record.allocate_min( _type->field_size() );
        _record.length( _type->field_size() );

        xml_processor.parse_record_fields( SOS_CAST( Record_type, _type ), _record.byte_ptr() );

        xml_processor.expect( Xml_processor::sym_eof );
    }
    catch( const exception&  x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_xml" ); }
    catch( const _com_error& x )   { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_xml" ); }

    return hr;
}

//-----------------------------------------------------------------------Hostware_dynobj::obj_field

STDMETHODIMP Hostware_dynobj::Obj_field( VARIANT* name, VARIANT* result )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    VariantInit( result );

    Z_MUTEX( hostware_mutex )
    try {

        Field_descr* field = this->field( name );

        if( field->null( _record.byte_ptr() ) ) 
        {
            if( _Obj_read_null_as_empty )
            {
                V_VT( result ) = VT_BSTR;
                V_BSTR( result ) = NULL;
            }
            else
            {
                V_VT( result ) = VT_NULL;
            }
        } 
        else 
        {
            field_to_variant( field, _record.byte_ptr(), result, &_value_buffer, (LCID)0 /*lcid*/ );
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_field" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_field" ); }

    return hr;
}

//-------------------------------------------------------------------Hostware_dynobj::obj_set_field

STDMETHODIMP Hostware_dynobj::Obj_set_field( VARIANT* name, VARIANT* value )
{
    Z_COM_MY_THREAD_ONLY;

    HRESULT hr = NOERROR;

    Z_MUTEX( hostware_mutex )
    try {
        Field_descr* field = this->field( name );

        if( !field->type_ptr() )  throw_xc( "SOS-1193" );                  // Satzbeschreibung fehlt. NULL-Objekt ist nicht beschreibbar

        if( V_VT( value ) == VT_NULL ) 
        {
            field->set_null( _record.byte_ptr() );
        } 
        else 
        {
            variant_to_char( *value, &_value_buffer, (LCID)0 );

            if( _Obj_write_empty_as_null  &&  _value_buffer.length() == 0 ) 
            {
                field->set_null( _record.byte_ptr() );
            } 
            else 
            {
                _value_buffer.append( '\0' );
                field->read_text( _record.byte_ptr(), _value_buffer.char_ptr() );
            }
        }
    }
    catch( const exception&  x )  { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_set_field" ); }
    catch( const _com_error& x )  { hr = _set_excepinfo( x, "hostWare.Dyn_obj::obj_set_field" ); }

    return hr;
}

//------------------------------------------------------------------------Hostware_dynobj::get_type

STDMETHODIMP Hostware_dynobj::get_Obj_type( Ihostware_record_type** type )
{
    Z_COM_MY_THREAD_ONLY;

    if( _type ) 
    {
        if( _type->obj_is_type( tc_Record_type ) )
        {
            *type = new Hostware_record_type( (Record_type*)+_type );
        }
        else
        {
            Sos_ptr<Record_type> record_type = SOS_NEW( Record_type );
            record_type->add_field( _type, "field", 0 );
            *type = new Hostware_record_type( record_type );
        }

        (*type)->AddRef();  
    }
    else
    {
        *type = NULL;
    }

    return NOERROR; 
}

//-----------------------------------------------------------------------Hostware_dynobj::value
/*
STDMETHODIMP Hostware_dynobj::value( BSTR name, VARIANT FAR* value )
{
    try {
        _type->field_descr_ptr( name )->write_text( _record.byte_ptr(), &_value_buffer, _text_format );

        VariantInit( value );
        V_VT( value ) = VT_BSTR;  // Text
        V_BSTR( value ) = SysAllocStringLen( _value_buffer.char_ptr(), _value_buffer.length() );
    }
    catch( const exception& x )  { return DISP_E_EXCEPTION; }  //_set_excepinfo( x ); }
}
*/
//--------------------------------------------------------------------Hostware_dynobj::construct

void Hostware_dynobj::construct()
{
    _record.allocate_min( _type->field_size() );
    _record.length( _type->field_size() );
    _type->construct( _record.byte_ptr() );
}

//-----------------------------------------------------------------------Hostware_dynobj::finish

void Hostware_dynobj::finish()
{
    if( !_type ) {
        _record += '\0';
        Sos_ptr<String0_type> t = SOS_NEW( String0_type( _record.length() - 1 ) );
        _type = +t;
    }

    int zu_kurz = _type->field_size() - _record.length();
    if( zu_kurz > 0 ) {
        if( _record.size() < _type->field_size() )  throw_xc( "Hostware_dynobj::finish" );  // FEHLER
        memset( _record.char_ptr() + _record.length(), 0, zu_kurz );
    }
}

//-------------------------------------------------------------------Hostware_dynobj::_obj_print

void Hostware_dynobj::_obj_print( ostream* s ) const
{
    *s << "Hostware_dynobj(";
    
    if( _type && _record.byte_ptr() ) {
        if( _type->obj_is_type( tc_Record_type ) )  *s << ((Record_type*)+_type)->name();
                                              else  *s << _type->info()->_name;
        //_type->print( _record.byte_ptr(), s, std_text_format );
    } else {
        *s << "NULL";
    }

    *s << ')';
}

//-------------------------------------------------------------------Ole_appl::Ole_appl

Ole_appl::Ole_appl( Typelib_descr* typelib_descr /* HINSTANCE hInst, HINSTANCE hInstPrev,
                            LPSTR pszCmdLine, UINT nCmdShow*/ )
:
    Ole_server( typelib_descr ),
    _zero_(this+1)
{
/*    //Initialize WinMain parameter holders.
    _hInst     =hInst;
    _hInstPrev =hInstPrev;
    _pszCmdLine=pszCmdLine;
    _nCmdShow  =nCmdShow;

    _hWnd=NULL;
*/
}

//------------------------------------------------------------------Ole_appl::~Ole_appl

Ole_appl::~Ole_appl()
{
    // (§1768) exit(); // jz 21.9.04.   Wird gerufen, wenn hostole.dll entladen wird. Andere DLLs sind vielleicht schon weg und dann knallts (ODBC, JDBC, Java)
}

//---------------------------------------------------------------------------Ole_appl::init

void Ole_appl::init()
{
    if( _initialized )  return;

    sos_static_ptr()->_dont_check_licence = true;       // Lizenzen werden erst in DllGetClassObject() geprüft
    sos_static_ptr()->add_ref();    //jz 22.1.2001

    Ole_server::init();

    if( !log_ptr ) {
        Sos_string log_file;
        log_file = read_profile_string( "", "hostole", "log" );
        //fprintf(stderr,"%s read_profile_string() => %s\n", Z_FUNCTION, log_file.c_str() );
        if( !empty( log_file ) )  log_start( c_str( log_file ) );
    }

    _initialized = true;
}

//---------------------------------------------------------------------------Ole_appl::exit
//void terminate_word_applications();

void Ole_appl::exit()
{
    //terminate_word_applications();
    Ole_server::exit();
    sos_static_ptr()->remove_ref(); //jz 22.1.2001
}

//-------------------------------------------------------------------------------------------------

} //namespace sos

//----------------------------------------------------------------------------------create_hostware

HRESULT create_hostware( const CLSID& clsid, IUnknown** result )
{                                                                            
    if( clsid != CLSID_Global )  return CLASS_E_CLASSNOTAVAILABLE;
    *result = (IUnknown*)(Ihostware*) new sos::Hostware;
    return S_OK;
}

//-----------------------------------------------------------------------------create_hostware_file

HRESULT create_hostware_file( const CLSID& clsid, IUnknown** result )
{                                                                            
    if( clsid != CLSID_File )  return CLASS_E_CLASSNOTAVAILABLE;
    *result = (IUnknown*)(Ihostware_file*) new sos::Hostware_file;
    return S_OK;
}

//---------------------------------------------------------------------------create_hostware_dynobj

HRESULT Create_hostware_dynobj( const CLSID& clsid, IUnknown** result )
{                                                                            
    if( clsid != CLSID_Dyn_obj )  return CLASS_E_CLASSNOTAVAILABLE;
    *result = (IUnknown*)(Ihostware_dynobj*) new sos::Hostware_dynobj;
    return S_OK;
}

//-------------------------------------------------------------------------------------------------

