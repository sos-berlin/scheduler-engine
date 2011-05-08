// $Id$

// §1735

#ifndef __HOSTOLE_H
#define __HOSTOLE_H

#include "../zschimmer/com.h"

#if defined _WIN32

#   if defined _DEBUG
#       import "debug/hostole.tlb" no_namespace raw_interfaces_only named_guids
#   else
#       import "release/hostole.tlb" no_namespace raw_interfaces_only named_guids
#   endif

#else

#   include "../zschimmer/com_server.h"
#   include "../zschimmer/java_odl.h"
    using namespace odl;

//#ifdef COM_STATIC
    extern "C" BOOL WINAPI      hostole_DllMain( HANDLE hInst, DWORD ul_reason_being_called, void* )            COM_LINKAGE;
    extern "C" HRESULT APIENTRY hostole_DllGetClassObject( const CLSID& rclsid, const IID& riid, void** ppv )   COM_LINKAGE;
//#endif

DEFINE_GUID( LIBID_hostWare              , 0x9F716A00, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

DEFINE_GUID( IID_Ihostware_type_info     , 0x9F716A0A, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Type_info             , 0x9F716A0C, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
struct Ihostware_type_info : IDispatch {};

DEFINE_GUID( IID_Ihostware_type_param    , 0x9F716A0B, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Type_param            , 0x9F716A0D, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
struct Ihostware_type_param : IDispatch {};

DEFINE_GUID( IID_Ihostware_field_type    , 0x9F716A09, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Field_type            , 0x9F716A10, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
struct Ihostware_field_type : IDispatch {};

DEFINE_GUID( IID_Ihostware_record_type   , 0x9F716A07, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Record_type           , 0x9F716A0E, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
struct Ihostware_record_type : IDispatch {};

DEFINE_GUID( IID_Ihostware_field_descr   , 0x9F716A08, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Field_descr           , 0x9F716A0F, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
struct Ihostware_field_descr : IDispatch {};

//-------------------------------------------------------------------------------------------------

struct Ivariables2;

//---------------------------------------------------------------------------------Ihostware_dynobj

DEFINE_GUID( IID_Ihostware_dynobj        , 0x9F716A02, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Dyn_obj               , 0x9F716A06, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ihostware_dynobj : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ihostware_dynobj* )             { return IID_Ihostware_dynobj; }

    //Z_DEFINE_GETIDSOFNAMES_AND_INVOKE

    virtual STDMETHODIMP        Obj_add_field           ( BSTR name, VARIANT* type ) = 0;
    virtual STDMETHODIMP    get_Obj_field_count         ( int* field_count ) = 0;
    virtual STDMETHODIMP    get_Obj_field_name          ( int field_no, BSTR* name ) = 0;
    virtual STDMETHODIMP    put_Obj_write_empty_as_null ( VARIANT_BOOL b ) = 0;
    virtual STDMETHODIMP    get_Obj_field_index         ( BSTR name, int* field_no ) = 0;
    virtual STDMETHODIMP        Clone                   ( Ihostware_dynobj** record ) = 0;
    virtual STDMETHODIMP        Obj_field               ( VARIANT* name, VARIANT* value ) = 0;
    virtual STDMETHODIMP        Obj_set_field           ( VARIANT* name, VARIANT* value ) = 0;
    virtual STDMETHODIMP    get_Obj_xml                 ( BSTR, BSTR* ) = 0;
    virtual STDMETHODIMP    put_Obj_xml                 ( BSTR options, BSTR xml ) = 0;
    virtual STDMETHODIMP    get_Obj_type                ( Ihostware_record_type** ) = 0;
};

//-----------------------------------------------------------------------------------Ihostware_file

DEFINE_GUID( IID_Ihostware_file          , 0x9F716A01, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_File                  , 0x9F716A03, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ihostware_file : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ihostware_file* )             { return IID_Ihostware_file; }

    //Z_DEFINE_GETIDSOFNAMES_AND_INVOKE

    virtual STDMETHODIMP        Open                    ( BSTR filename ) = 0;
    virtual STDMETHODIMP        Close                   () = 0;
    virtual STDMETHODIMP        Close_file              ( void ) = 0;
    virtual STDMETHODIMP        Get_line                ( BSTR* buffer ) = 0;
    virtual STDMETHODIMP        Put_line                ( BSTR record ) = 0;
    virtual STDMETHODIMP        Create_record           ( Ihostware_dynobj** record ) = 0;
    virtual STDMETHODIMP        Create_key              ( Ihostware_dynobj** key ) = 0;
    virtual STDMETHODIMP        Get                     ( Ihostware_dynobj** object ) = 0;
    virtual STDMETHODIMP        Put                     ( VARIANT* object ) = 0;
    virtual STDMETHODIMP        Set_key                 ( VARIANT* key ) = 0;
    virtual STDMETHODIMP        Delete_key              ( VARIANT* key ) = 0;
    virtual STDMETHODIMP        Get_key                 ( VARIANT* key, Ihostware_dynobj** object ) = 0;
    virtual STDMETHODIMP        Insert                  ( VARIANT* record ) = 0;
    virtual STDMETHODIMP        Update                  ( VARIANT* record ) = 0;
    virtual STDMETHODIMP        Update_direct           ( VARIANT* record ) = 0;
    virtual STDMETHODIMP        Store                   ( VARIANT* record ) = 0;
    virtual STDMETHODIMP        Eof                     ( VARIANT_BOOL* eof ) = 0;
    virtual STDMETHODIMP    put_Date_format             ( BSTR format ) = 0;
    virtual STDMETHODIMP    put_Decimal_symbol          ( BSTR decimal_symbol ) = 0;
    virtual STDMETHODIMP    get_Field_name              ( LONG number, BSTR* name ) = 0;
    virtual STDMETHODIMP    get_Field_count             ( LONG* number ) = 0;
    virtual STDMETHODIMP        Prepare                 ( BSTR filename ) = 0;
    virtual STDMETHODIMP    put_Parameter               ( LONG no, VARIANT* value ) = 0;
    virtual STDMETHODIMP        Execute                 () = 0;
    virtual STDMETHODIMP    put_Parameters              ( SAFEARRAY* param_array ) = 0;
    virtual STDMETHODIMP    get_Type                    ( Ihostware_record_type** type ) = 0;
    virtual STDMETHODIMP    get_Opened                  ( VARIANT_BOOL* result ) = 0;
    virtual STDMETHODIMP        Rewind                  () = 0;
    virtual STDMETHODIMP    put_Date_time_format        ( BSTR format ) = 0;
    virtual STDMETHODIMP    get_Debug_info              ( BSTR* text ) = 0;
    virtual STDMETHODIMP    put_Write_empty_as_null     ( VARIANT_BOOL b ) = 0;
    virtual STDMETHODIMP        Close_cursor            () = 0;
    virtual STDMETHODIMP    get_Row_count               ( int* ) = 0;
    virtual STDMETHODIMP    put_Read_null_as_empty      ( VARIANT_BOOL ) = 0;
    virtual STDMETHODIMP        Db_open                 ( BSTR select_statement, SAFEARRAY* param_array, Ihostware_file** result ) = 0;
    virtual STDMETHODIMP        Db_get_single           ( BSTR filename, SAFEARRAY* param_array, Ihostware_dynobj** ) = 0;
    virtual STDMETHODIMP        Db_execute              ( BSTR statement, SAFEARRAY* param_array, int* result ) = 0;
    virtual STDMETHODIMP        Db_get_single_value     ( BSTR select_statement, SAFEARRAY* param_array, VARIANT* result ) = 0;
    virtual STDMETHODIMP    get_Filename                ( BSTR* result ) = 0;
    virtual STDMETHODIMP        Db_get_blob             ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, VARIANT* result ) = 0;
    virtual STDMETHODIMP        Db_get_clob             ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, VARIANT* result ) = 0;
  //virtual STDMETHODIMP        Db_put_blob             ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, BSTR value );
  //virtual STDMETHODIMP        Db_put_clob             ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, BSTR value );
};

//----------------------------------------------------------------------------------------Ihostware

DEFINE_GUID( CLSID_Global                , 0x9F716A05, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( IID_Ihostware               , 0x9F716A04, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ihostware : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ihostware* )                  { return IID_Ihostware; }

    virtual STDMETHODIMP        Copy_file               ( BSTR source_file, BSTR dest_file ) = 0;
    virtual STDMETHODIMP        Shell_execute_and_wait  ( BSTR file, BSTR verb, double seconds ) = 0;   // seconds < 0: Keine Zeitbeschränkung
    virtual STDMETHODIMP        Get_single              ( BSTR filename, SAFEARRAY* param_array, Ihostware_dynobj** record ) = 0;
    virtual STDMETHODIMP        Execute_direct          ( BSTR statement, SAFEARRAY* param_array ) = 0;
    virtual STDMETHODIMP        Letter_factory          ( BSTR parameter ) = 0;
    virtual STDMETHODIMP        Null                    ( VARIANT* o, VARIANT_BOOL* result ) = 0;
    virtual STDMETHODIMP        Empty                   ( VARIANT* o, VARIANT_BOOL* result ) = 0;
    virtual STDMETHODIMP        Sleep                   ( double* seconds ) = 0;
    virtual STDMETHODIMP        Remove_file             ( BSTR filename ) = 0;
    virtual STDMETHODIMP        Rename_file             ( BSTR old_filename, BSTR new_filename ) = 0;
    virtual STDMETHODIMP        File_as_string          ( BSTR filename, BSTR* string ) = 0;
    virtual STDMETHODIMP        As_parser_string        ( BSTR o, BSTR quote, BSTR* result ) = 0;  // Baut einen von Hostware_parser erkennbaren String zusammen
    virtual STDMETHODIMP        As_xml                  ( VARIANT* field_or_dynobj, BSTR options, BSTR* result ) = 0;
    virtual STDMETHODIMP        As_date                 ( BSTR date, BSTR format, DATE* result ) = 0;
    virtual STDMETHODIMP        Date_as_string          ( VARIANT* date, BSTR format, BSTR* result ) = 0;
    virtual STDMETHODIMP        Get_array               ( BSTR filename, SAFEARRAY* param_array, SAFEARRAY** result ) = 0;
    virtual STDMETHODIMP        From_xml                ( BSTR xml_string, BSTR options, BSTR* result ) = 0;
    virtual STDMETHODIMP        Create_object           ( BSTR script_text_or_class_name, BSTR language, IDispatch** ) = 0;
    virtual STDMETHODIMP        Sql_quoted              ( VARIANT value, BSTR* result ) = 0;
    virtual STDMETHODIMP        Sql_equal               ( BSTR field_name, VARIANT value, BSTR* expr_string ) = 0;
    virtual STDMETHODIMP        File_exists             ( BSTR hostware_filename, VARIANT_BOOL* result ) = 0;
    virtual STDMETHODIMP        Make_path               ( BSTR path ) = 0;
    virtual STDMETHODIMP        Ghostscript             ( BSTR parameters ) = 0;
    virtual STDMETHODIMP        Read_begin_and_end_of_file( BSTR filename, int begin_bytes, int end_bytes, SAFEARRAY** result ) = 0;
    virtual STDMETHODIMP        Parse_mt940             ( BSTR source_text, BSTR options, IDispatch** dom_document ) = 0;
    virtual STDMETHODIMP        Check_licence           ( BSTR product_name ) = 0;
    virtual STDMETHODIMP        Convert_to              ( BSTR type_name, VARIANT* value, BSTR format_not_used, VARIANT* result ) = 0;
    virtual STDMETHODIMP        Open                    ( BSTR filename, Ihostware_file** file ) = 0;
    virtual STDMETHODIMP        Remove_directory        ( BSTR path, VARIANT_BOOL force ) = 0;
    virtual STDMETHODIMP        Use_version             ( BSTR version ) = 0;
    virtual STDMETHODIMP    get_Used_version            ( BSTR* version ) = 0;
    virtual STDMETHODIMP        Get_log_                ( void*** ostream, void** mutex ) = 0;
    virtual STDMETHODIMP    get_Log_indent_tls_index    ( uint* result ) = 0;
    virtual STDMETHODIMP        Get_single_value        ( BSTR filename, SAFEARRAY* param_array, VARIANT* result ) = 0;
    virtual STDMETHODIMP        Need_version            ( BSTR version ) = 0;
    virtual STDMETHODIMP putref_Java_vm__deleted        ( void* ) = 0;
    virtual STDMETHODIMP    get_Version                 ( BSTR* version ) = 0;
    virtual STDMETHODIMP        Is_version_or_later     ( BSTR version, VARIANT_BOOL* ) = 0;
    virtual STDMETHODIMP        Hex_md5_from_bytes      ( BSTR bytes, BSTR* result ) = 0;
    virtual STDMETHODIMP        File_version_info       ( BSTR filename, BSTR option, Ivariables2** result ) = 0;
    virtual STDMETHODIMP        Chdir                   ( BSTR ) = 0;
    virtual STDMETHODIMP    get_System_information      ( BSTR, VARIANT*, VARIANT* ) = 0;
    virtual STDMETHODIMP    get_Log_context             ( void*** ) = 0;
};

//--------------------------------------------------------------------------------Hostware_std_type

enum Hostware_std_type 
{ 
    hwst_none       = 0,
    hwst_char       = 1,
    hwst_varchar    = 2,
    hwst_decimal    = 3,
    hwst_integer    = 4,
    hwst_float      = 5,
    hwst_date       = 6, 
    hwst_time       = 7, 
    hwst_date_time  = 8,
    hwst_bool       = 9
};

//----------------------------------------------------------------------------------------Ivariable

DEFINE_GUID( CLSID_Variable, 0x9F716A21, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( IID_Ivariable , 0x9F716A20, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ivariable : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ivariable* )                  { return IID_Ivariable; }

    virtual STDMETHODIMP        GetTypeInfoCount        ( UINT* )                                   = 0;
    virtual STDMETHODIMP        GetTypeInfo             ( UINT, LCID, ITypeInfo** )                 = 0;
    virtual STDMETHODIMP        GetIDsOfNames           ( REFIID, LPOLESTR*, UINT, LCID, DISPID* )  = 0;
    virtual STDMETHODIMP        Invoke                  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* ) = 0;

    virtual STDMETHODIMP    put_Value                   ( VARIANT* index, VARIANT* value )          = 0;
    virtual STDMETHODIMP    get_Value                   ( VARIANT* index, VARIANT* value )          = 0;
    virtual STDMETHODIMP        Dim                     ( int size )                                = 0;
    virtual STDMETHODIMP    get_Name                    ( BSTR* name )                              = 0;
};

//---------------------------------------------------------------------------------------Ivariables

DEFINE_GUID( CLSID_Variables, 0x9F716A23, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( IID_Ivariables , 0x9F716A22, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ivariables : IDispatch
{
    friend inline const GUID& __uuidof_                     ( Ivariables* )                 { return IID_Ivariables; }

    virtual STDMETHODIMP        Set_var                     ( BSTR name, VARIANT* value )           = 0;
    virtual STDMETHODIMP    put_Value                       ( BSTR name, VARIANT* value )           = 0;
    virtual STDMETHODIMP    get_Value                       ( BSTR name, Ivariable** result )       = 0;
    virtual STDMETHODIMP    get_Count                       ( int* value )                          = 0;
    virtual STDMETHODIMP        Clone                       ( Ivariables** result )                 = 0;

  //[id(DISPID_NEWENUM),propget,restricted]
  //virtual STDMETHODIMP   ´get__NewEnum                     ( IUnknown** enumerator );
};

//----------------------------------------------------------------------------Ivariables_enumerator

DEFINE_GUID(  IID_Ivariables_enumerator, 0x9F716A22, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables_enumerator, 0x9F716A23, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ivariables_enumerator : IEnumVARIANT
{ 
    friend inline const GUID& __uuidof_                 ( Ivariables_enumerator* )                  { return IID_Ivariables_enumerator; }

    virtual STDMETHODIMP        Next                    ( ULONG celt, VARIANT* rgvar, ULONG* pceltFetched ) = 0;
    virtual STDMETHODIMP        Skip                    ( ULONG celt )                      = 0;
    virtual STDMETHODIMP        Reset                   ()                                          = 0;
};

//---------------------------------------------------------------------------------------Ivariable2

DEFINE_GUID( CLSID_Variable2, 0x9F716A27, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( IID_Ivariable2 , 0x9F716A26, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ivariable2 : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ivariable2* )                             { return IID_Ivariable2; }

    virtual STDMETHODIMP        GetTypeInfoCount        ( UINT* )                                   = 0;
    virtual STDMETHODIMP        GetTypeInfo             ( UINT, LCID, ITypeInfo** )                 = 0;
    virtual STDMETHODIMP        GetIDsOfNames           ( REFIID, LPOLESTR*, UINT, LCID, DISPID* )  = 0;
    virtual STDMETHODIMP        Invoke                  ( DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* ) = 0;

    virtual STDMETHODIMP    put_Obj_value               ( VARIANT* index, VARIANT* value )          = 0;
    virtual STDMETHODIMP    get_Obj_value               ( VARIANT* index, VARIANT* value )          = 0;
    virtual STDMETHODIMP        Obj_dim                 ( int size )                                = 0;
    virtual STDMETHODIMP    get_Obj_name                ( BSTR* name )                              = 0;
};

//--------------------------------------------------------------------------------------Ivariables2

DEFINE_GUID( CLSID_Variables2, 0x9F716A29, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( IID_Ivariables2 , 0x9F716A28, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Ivariables2 : IDispatch
{
    friend inline const GUID& __uuidof_                     ( Ivariables2* )                        { return IID_Ivariables2; }

  //virtual STDMETHODIMP        Set_var                     ( BSTR name, VARIANT* value )           = 0;
    virtual STDMETHODIMP    put_Obj_value                   ( BSTR name, VARIANT* value )           = 0;
    virtual STDMETHODIMP    get_Obj_value                   ( BSTR name, VARIANT* result )          = 0;
    virtual STDMETHODIMP    get_Obj_is_empty                ( VARIANT_BOOL* result )                = 0;
    virtual STDMETHODIMP        Obj_clone                   ( Ivariables2** result )                = 0;
    virtual STDMETHODIMP    put_Obj_xml                     ( BSTR xml)                             = 0;
    virtual STDMETHODIMP    get_Obj_xml                     ( BSTR* result )                        = 0;

  //[id(DISPID_NEWENUM),propget,restricted]
  //virtual STDMETHODIMP   ´get__NewEnum                     ( IUnknown** enumerator );
};

//---------------------------------------------------------------------------Ivariables2_enumerator

DEFINE_GUID(  IID_Ivariables2_enumerator, 0x9F716A30, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID( CLSID_Variables2_enumerator, 0x9F716A31, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );


struct Ivariables2_enumerator : IEnumVARIANT
{ 
    friend inline const GUID& __uuidof_                 ( Ivariables2_enumerator* )                 { return IID_Ivariables2_enumerator; }

    virtual STDMETHODIMP        Next                    ( ULONG celt, VARIANT* rgvar, ULONG* pceltFetched ) = 0;
    virtual STDMETHODIMP        Skip                    ( ULONG celt )                      = 0;
    virtual STDMETHODIMP        Reset                   ()                                          = 0;
};

//-----------------------------------------------------------------Ivariables2_idispatch_enumerator

DEFINE_GUID( IID_Ivariables2_idispatch_enumerator , 0x9F716A24, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x02 );
DEFINE_GUID( CLSID_Variables2_idispatch_enumerator, 0x9F716A25, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x02 );

struct Ivariables2_idispatch_enumerator : IDispatch
{ 
    friend inline const GUID& __uuidof_                 ( Ivariables2_idispatch_enumerator* )       { return IID_Ivariables2_idispatch_enumerator; }

    virtual STDMETHODIMP        Next                    ( Ivariable2** result )                     = 0;
    virtual STDMETHODIMP    get_Has_next                ( VARIANT_BOOL* result )                    = 0;
};

//--------------------------------------------------------------------------------Script_item_flags

enum Script_item_flags
{
    sif_isvisible      = 0x0002,
    sif_issource       = 0x0004,
    sif_globalmembers  = 0x0008,
    sif_ispersistent   = 0x0010,
    sif_nocode         = 0x0400,
    sif_codeonly       = 0x0200,
};

//---------------------------------------------------------------------------------Scripttext_flags

enum Scripttext_flags
{
    scripttext_isvisible    = 0x00000002,
    scripttext_isexpression = 0x00000020,
    scripttext_ispersistent = 0x00000040
};

//------------------------------------------------------------------------------------Ausgabeformat

enum Ausgabeformat
{
    format_std,
    format_waehrung         // 0.009,99
};

//--------------------------------------------------------------------------------------Embed_flags 

enum Embed_flags                        // Mit Flags in rtf.h abgleichen!
{
    embed_char_prop          = 0x01,    // Eigenschaft eines Zeiches    (flag_char_prop)
    embed_para_prop          = 0x02,    // Eigenschaft eines Absatzes   (flag_para_prop)
    embed_sect_prop          = 0x04,    // Eigenschaft eines Abschnitts (flag_sect_prop)
    embed_doc_prop           = 0x08,    // Eigenschaft des Dokuments    (flag_doc_prop)
};

//-------------------------------------------------------------------------Factory_text_constructor

DEFINE_GUID( CLSID_Factory_text_constructor, 0x6EB42D32, 0xA6BF, 0x11D0, 0x83, 0x81, 0x00, 0xA0, 0xC9, 0x1E, 0xF7, 0xB9 );
DEFINE_GUID(  IID_Ifactory_text_constructor, 0x6EB42D31, 0x1BAD, 0x11d1, 0x9A, 0x05, 0x00, 0x60, 0x97, 0x36, 0x92, 0xFD );

struct Ifactory_text_constructor : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ifactory_text_constructor* )          { return IID_Ifactory_text_constructor; }

    virtual STDMETHODIMP        Put                     ( VARIANT* text, VARIANT_BOOL raw )     = 0;
    virtual STDMETHODIMP        Put_formatted           ( VARIANT* text, Ausgabeformat, BSTR debug )        = 0;
    virtual STDMETHODIMP        Embed                   ( BSTR filename )                       = 0;
    virtual STDMETHODIMP        Rtf_start               ()                                      = 0;
    virtual STDMETHODIMP        Rtf_finish              ()                                      = 0;
};

//--------------------------------------------------------------------------Factory_rtf_constructor

DEFINE_GUID( CLSID_Factory_rtf_constructor, 0x6EB42D34, 0xA6BF, 0x11D0, 0x83, 0x81, 0x00, 0xA0, 0xC9, 0x1E, 0xF7, 0xB9 );
DEFINE_GUID(  IID_Ifactory_rtf_constructor, 0x6EB42D33, 0x1BAD, 0x11d1, 0x9A, 0x05, 0x00, 0x60, 0x97, 0x36, 0x92, 0xFD );

struct Ifactory_rtf_constructor : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ifactory_rtf_constructor* )           { return IID_Ifactory_rtf_constructor; }

    virtual STDMETHODIMP        Put                     ( VARIANT* text, VARIANT_BOOL raw )     = 0;
    virtual STDMETHODIMP        Put_formatted           ( VARIANT* text, enum Ausgabeformat format, BSTR debug ) = 0;
    virtual STDMETHODIMP        Embed                   ( BSTR filename, Embed_flags )          = 0;
    virtual STDMETHODIMP        Rtf_start               ()                                      = 0;
    virtual STDMETHODIMP        Rtf_finish              ()                                      = 0;

    virtual STDMETHODIMP        Rtf_prop                ( int code, int value )                 = 0;
    virtual STDMETHODIMP        Rtf_put                 ( int begin_ptr, int end_ptr )          = 0;
    virtual STDMETHODIMP        Rtf_open                ( int entity_ptr )                      = 0;
    virtual STDMETHODIMP        Rtf_close               ()                                      = 0;
};

//-------------------------------------------------------------------------------Ifactory_processor

DEFINE_GUID( CLSID_Factory_processor     , 0x9F716A41, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID(  IID_Ifactory_processor     , 0x9F716A40, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

HRESULT create_factory_processor( const CLSID&, IUnknown** )  COM_LINKAGE;

struct Ifactory_processor : IDispatch
{
    friend inline const GUID& __uuidof_                 ( Ifactory_processor * )                  { return IID_Ifactory_processor; }

    virtual STDMETHODIMP    put_Param                   ( BSTR parameter )                      = 0;
    virtual STDMETHODIMP        Make_script             ( BSTR rtf_input, BSTR* script )        = 0;
    virtual STDMETHODIMP        Init_engine             ()                                      = 0;
    virtual STDMETHODIMP        Close_engine            ()                                      = 0;
    virtual STDMETHODIMP    get_Global_object           ( IDispatch** )                         = 0;
    virtual STDMETHODIMP        Add_obj                 ( IDispatch* obj, BSTR name, Script_item_flags flags = sif_isvisible ) = 0;
    virtual STDMETHODIMP        Parse                   ( BSTR script, Scripttext_flags scripttext_isvisible, VARIANT* ) = 0;
    virtual STDMETHODIMP        Start_engine            ()                                      = 0;
    virtual STDMETHODIMP    get_Script                  ( IUnknown** )                          = 0;
    virtual STDMETHODIMP    get_Idispatch               ( BSTR name, IDispatch** )              = 0;
    virtual STDMETHODIMP        Name_exists             ( BSTR sub_name, VARIANT_BOOL* )        = 0;
    virtual STDMETHODIMP        Preprocess              ( BSTR script, BSTR* result_script )    = 0;
    virtual STDMETHODIMP    get_Error_filename          ( BSTR* filename )                      = 0;
    virtual STDMETHODIMP    get_Error_document          ( BSTR* doc )                           = 0;
    virtual STDMETHODIMP    put_Language                ( BSTR language )                       = 0;
    virtual STDMETHODIMP    get_Language                ( BSTR* language )                      = 0;
    virtual STDMETHODIMP    put_Template_filename       ( BSTR filename )                       = 0;
    virtual STDMETHODIMP    get_Template_filename       ( BSTR* filename )                      = 0;
    virtual STDMETHODIMP    put_Head_filename           ( BSTR filename )                       = 0;
    virtual STDMETHODIMP    get_Head_filename           ( BSTR* filename )                      = 0;
    virtual STDMETHODIMP    put_Document_filename       ( BSTR filename )                       = 0;
    virtual STDMETHODIMP    get_Document_filename       ( BSTR* filename )                      = 0;
    virtual STDMETHODIMP putref_Parameters              ( Ivariables* variables )               = 0;
    virtual STDMETHODIMP    get_Parameters              ( Ivariables** variables  )             = 0;
    virtual STDMETHODIMP    put_Parameter               ( BSTR name, VARIANT* value )           = 0;
    virtual STDMETHODIMP    get_Parameter               ( BSTR name, Ivariable** result )       = 0;
    virtual STDMETHODIMP        Process                 ()                                      = 0;
    virtual STDMETHODIMP        Eval                    ( BSTR script_text, enum Scripttext_flags, VARIANT* result ) = 0;
    virtual STDMETHODIMP        Close                   ()                                      = 0;
    virtual STDMETHODIMP    put_Collect                 ( int count )                           = 0;
    virtual STDMETHODIMP    get_Collect                 ( int* count )                          = 0;
    virtual STDMETHODIMP    get_Document_head_modified  ( VARIANT_BOOL* modified )              = 0;
    virtual STDMETHODIMP    get_Document_copied         ( VARIANT_BOOL* modified )              = 0;
    virtual STDMETHODIMP        Close_output_file       ()                                      = 0;
    virtual STDMETHODIMP    get_Collected_documents_count( int* count )                         = 0;
    virtual STDMETHODIMP    get_Template_dir            ( BSTR* path )                          = 0;
    virtual STDMETHODIMP    put_Template_dir            ( BSTR path )                           = 0;
    virtual STDMETHODIMP    get_Document_dir            ( BSTR* path )                          = 0;
    virtual STDMETHODIMP    put_Document_dir            ( BSTR path )                           = 0;
    virtual STDMETHODIMP    get_Real_document_filename  ( BSTR* filename )                      = 0;
    virtual STDMETHODIMP    get_Last_step               ( BSTR* step )                          = 0;
    virtual STDMETHODIMP    get_Script_text             ( BSTR* )                               = 0;
    virtual STDMETHODIMP        Add_parameters          ()                                      = 0;
    virtual STDMETHODIMP    get_Document_written        ( VARIANT_BOOL* )                       = 0;
    virtual STDMETHODIMP    put_Db_name                 ( BSTR )                                = 0;
    virtual STDMETHODIMP    get_Db_name                 ( BSTR* )                               = 0;
    virtual STDMETHODIMP    put_Merge_documents         ( VARIANT_BOOL )                       = 0;
    virtual STDMETHODIMP    get_Merge_documents         ( VARIANT_BOOL* )                      = 0;
};

//-------------------------------------------------------------------------------------------Irerun

DEFINE_GUID( CLSID_Rerun, 0x82AD717A, 0xA075, 0x400b, 0x86, 0x73, 0xB6, 0x57, 0x8F, 0xCB, 0xD8, 0x35 );
DEFINE_GUID(  IID_Irerun, 0x5C2BFBBD, 0xE82F, 0x459c, 0x95, 0xBB, 0xCE, 0x3E, 0x3B, 0xCB, 0xE6, 0xB3 );

HRESULT create_hostware_rerun( const CLSID&, IUnknown** result );

struct Irerun : IDispatch
{
    friend inline const GUID& __uuidof_                     ( Irerun* )                             { return IID_Irerun; }

    virtual STDMETHODIMP        Open                        ( BSTR filename )                       = 0;
    virtual STDMETHODIMP        Close                       ()                                      = 0;
    virtual STDMETHODIMP        Process_next_record         ( VARIANT_BOOL* process_next_record )   = 0;
  //virtual STDMETHODIMP        Processing_was_ok           ( VARIANT_BOOL )                        = 0;
    virtual STDMETHODIMP        Set_record_ok               ( int record_number, VARIANT_BOOL ok )  = 0;
    virtual STDMETHODIMP    get_Rerunning                   ( VARIANT_BOOL* result )                = 0;
    virtual STDMETHODIMP    get_Record_number               ( int* result )                         = 0;
    virtual STDMETHODIMP    get_Ok                          ( VARIANT_BOOL* result )                = 0;
};

//-----------------------------------------------------------------------------------Iscript_object

DEFINE_GUID( CLSID_Script_object, 0x9F716A43, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );
DEFINE_GUID(  IID_Iscript_object, 0x9F716A42, 0xD1F0, 0x11CF, 0x86, 0x9D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 );

struct Iscript_object : IDispatch
{
    friend inline const GUID& __uuidof_                     ( Iscript_object* )                     { return IID_Iscript_object; }

    virtual STDMETHODIMP        Obj_close();
    virtual STDMETHODIMP    put_Obj_language                ( BSTR scripting_engine_name );
    virtual STDMETHODIMP    get_Obj_language                ( BSTR* scripting_engine_name );
    virtual STDMETHODIMP        Obj_parse                   ( BSTR script_text, enum Scripttext_flags, VARIANT* result );
    virtual STDMETHODIMP        Obj_parse_only              ( BSTR script_text, enum Scripttext_flags );
    virtual STDMETHODIMP        Obj_add_variables           ( Ivariables2* );
    virtual STDMETHODIMP        Obj_eval                    ( BSTR script_text, enum Scripttext_flags, VARIANT* result );
    virtual STDMETHODIMP        Obj_name_exists             ( BSTR sub_name, VARIANT_BOOL* );
    virtual STDMETHODIMP        Obj_add_object              ( IDispatch* object, BSTR name, enum Script_item_flags flags = sif_isvisible );
};

//-------------------------------------------------------------------------------------------------

#endif
#endif
