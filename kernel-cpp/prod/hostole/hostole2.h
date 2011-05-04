// hostole2.h                            (C)1997 SOS GmbH Berlin

#ifndef __HOSTOLE2_H
#define __HOSTOLE2_H

//---------------------------------------------------------------------------------------------

#include "hostole.h"

#include <map>

#include "../kram/thread_semaphore.h"
#include "../kram/sosfield.h"
#include "../file/anyfile.h"

#include "../kram/com_simple_standards.h"
#include "../kram/com_server.h"
#include "../zschimmer/mutex.h"

#define INITGUIDS


namespace sos {


extern Typelib_descr hostole_typelib;

struct Connection_point_container;
struct Hostware_dynobj;

//---------------------------------------------------------------------------------------------

//extern Typelib_descr   hostware_typelib_descr;
extern Ole_class_descr* hostware_class_ptr;
extern Ole_class_descr* hostware_file_class_ptr;
extern Ole_class_descr* hostware_dynobj_class_ptr;
extern Ole_class_descr* hostware_type_info_class_ptr;
extern Ole_class_descr* hostware_type_param_class_ptr;
extern Ole_class_descr* hostware_field_type_class_ptr;
extern Ole_class_descr* hostware_record_type_class_ptr;
extern Ole_class_descr* hostware_field_descr_class_ptr;
extern Ole_class_descr* hostware_dialogbox_class_ptr;
extern Ole_class_descr* hostware_control_class_ptr;
extern Ole_class_descr* parser_class_ptr;
extern Ole_class_descr* token_class_ptr;

//--------------------------------------------------------------------------------------Hostware

struct Hostware : Ihostware, 
                  Ihas_java_class_name,
                  z::com::Imodule_interface2,
                  Sos_ole_object
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Hostware                ()                                      : Sos_ole_object( hostware_class_ptr, static_cast<Ihostware*>( this ) ) {}
                               ~Hostware                ();


    USE_SOS_OLE_OBJECT_WITHOUT_QI

    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    HRESULT                     init                    ();

    STDMETHODIMP                Copy_file               ( BSTR, BSTR );
    STDMETHODIMP                Shell_execute_and_wait  ( BSTR file, BSTR verb, double seconds );
    STDMETHODIMP                Get_single              ( BSTR statement, SAFEARRAY* param_array, Ihostware_dynobj** record );
    STDMETHODIMP                Execute_direct          ( BSTR statement, SAFEARRAY* param_array );
    STDMETHODIMP                Letter_factory          ( BSTR param );
    STDMETHODIMP                Null                    ( VARIANT* o, short* result )         { *result = V_VT( o ) == VT_NULL; return (HRESULT)NOERROR; }
    STDMETHODIMP                Empty                   ( VARIANT* o, short* result )         { *result = V_VT( o ) == VT_EMPTY; return (HRESULT)NOERROR; }
    STDMETHODIMP                Sleep                   ( double* seconds );
    STDMETHODIMP                Remove_file             ( BSTR );
    STDMETHODIMP                Rename_file             ( BSTR, BSTR );
    STDMETHODIMP                File_as_string          ( BSTR filename, BSTR* string );
    STDMETHODIMP                As_parser_string        ( BSTR o, BSTR quote, BSTR *result );
    STDMETHODIMP                As_xml                  ( VARIANT* field_or_dynobj, BSTR options, BSTR* result );
    STDMETHODIMP                From_xml                ( BSTR xml, BSTR options, BSTR* result );
    STDMETHODIMP                As_date                 ( BSTR date, BSTR format, DATE* result );
    STDMETHODIMP                Date_as_string          ( VARIANT* date, BSTR format, BSTR* result );
    STDMETHODIMP                Get_array               ( BSTR statement, SAFEARRAY* param_array, SAFEARRAY** result );
    STDMETHODIMP                Sql_quoted              ( VARIANT value, BSTR* result );
    STDMETHODIMP                Sql_equal               ( BSTR field_name, VARIANT value, BSTR* expr_string );
    STDMETHODIMP                File_exists             ( BSTR hostware_filename, short* result );
    STDMETHODIMP                Make_path               ( BSTR path );
    STDMETHODIMP                Ghostscript             ( BSTR );
    STDMETHODIMP                Read_begin_and_end_of_file( BSTR filename, int begin_bytes, int end_bytes, SAFEARRAY** result );
    STDMETHODIMP                Check_licence           ( BSTR product_name );
    STDMETHODIMP                Convert_to              ( BSTR type_name, VARIANT* value, BSTR format, VARIANT* result );
    STDMETHODIMP                Open                    ( BSTR filename, Ihostware_file** );
    STDMETHODIMP                Remove_directory        ( BSTR, VARIANT_BOOL force );
    STDMETHODIMP                Use_version             ( BSTR );
    STDMETHODIMP            get_Used_version            ( BSTR* );
    STDMETHODIMP                Need_version            ( BSTR );
    STDMETHODIMP         putref_Java_vm__deleted        ( void* );
    STDMETHODIMP                Get_log_                ( void*** ostream, void** mutex );
    STDMETHODIMP            get_Log_indent_tls_index    ( uint* result );
    STDMETHODIMP                Get_single_value        ( BSTR filename, SAFEARRAY* param_array, VARIANT* result );
    STDMETHODIMP            get_Version                 ( BSTR* );
    STDMETHODIMP                Is_version_or_later     ( BSTR, VARIANT_BOOL* );
    STDMETHODIMP                Hex_md5_from_bytes      ( BSTR bytes, BSTR* result );
    STDMETHODIMP                File_version_info       ( BSTR filename, BSTR option, Ivariables2** result );
    STDMETHODIMP                Chdir                   ( BSTR );
    STDMETHODIMP            get_System_information      ( BSTR, VARIANT*, VARIANT* );


  //STDMETHODIMP                get_Exception           ( Iexception** );
  //STDMETHODIMP                put_Exception           ( Iexception* );


#ifdef SYSTEM_HAS_IDISPATCH
    STDMETHODIMP                Create_object           ( BSTR class_name, BSTR language, IDispatch** );
    STDMETHODIMP                Parse_mt940             ( BSTR, BSTR, IDispatch** );

# else
    STDMETHODIMP                Create_object           ( BSTR class_name, BSTR language, IDispatch** ) { return E_NOTIMPL; }
    STDMETHODIMP                Parse_mt940             ( BSTR, BSTR, IDispatch** )                     { return E_NOTIMPL; }
#endif

    STDMETHODIMP                Get_single_or_array_open( Any_file*, BSTR stmt, SAFEARRAY* param_array );
    virtual void               _obj_print               ( ostream* s ) const                { *s << "Hostware"; }


    // Imodule_interface
    STDMETHODIMP                module_interface_version()                                      { return 3; }
    STDMETHODIMP         putref_Com_context             ( const z::com::Com_context* );
    STDMETHODIMP            put_Log_categories          ( const BSTR );
    STDMETHODIMP                Set_stream_and_system_mutex ( ostream**, z::System_mutex* );

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                        { return zschimmer::com::String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                      { return (char*)"sos.hostware.Global"; }

    STDMETHODIMP            put_Log_context             ( zschimmer::Log_context** );
    STDMETHODIMP            get_Log_context             ( void*** );
};

//---------------------------------------------------------------------------------Hostware_file


struct Hostware_file : Ihostware_file, 
                       Ihas_java_class_name,
                       Sos_ole_object,
                       z::My_thread_only
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware_file" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Hostware_file            ();
    virtual                    ~Hostware_file            ();

    USE_SOS_OLE_OBJECT_WITHOUT_QI


    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    HRESULT                     init                    ();
    void                       _obj_print               ( ostream* s ) const;

    STDMETHODIMP                Open                    ( BSTR );
    STDMETHODIMP                Close                   ();                 // Reserviertes Wort in Visual Basic
    STDMETHODIMP                Close_file              ();
    STDMETHODIMP                Close_cursor            ();
    STDMETHODIMP                Put_line                ( BSTR );
    STDMETHODIMP                Get_line                ( BSTR* );
    STDMETHODIMP                Eof                     ( short* );
    STDMETHODIMP                Create_record           ( Ihostware_dynobj** );
    STDMETHODIMP                Create_key              ( Ihostware_dynobj** key );
    STDMETHODIMP                Get                     ( Ihostware_dynobj** object );
    STDMETHODIMP                Put                     ( VARIANT* object );
    STDMETHODIMP                Set_key                 ( VARIANT* key );
    STDMETHODIMP                Delete_key              ( VARIANT* key );
    STDMETHODIMP                Get_key                 ( VARIANT* key, Ihostware_dynobj** object );
    STDMETHODIMP                Insert                  ( VARIANT* record );
    STDMETHODIMP                Update                  ( VARIANT* );
    STDMETHODIMP                Update_direct           ( VARIANT* record );
    STDMETHODIMP                Store                   ( VARIANT* record );
    STDMETHODIMP            put_Date_format             ( BSTR format );
    STDMETHODIMP            put_Decimal_symbol          ( BSTR symbol );
    STDMETHODIMP            get_Field_name              ( LONG number, BSTR* name);
    STDMETHODIMP            get_Field_count             ( LONG* number );
    STDMETHODIMP                Prepare                 ( BSTR filename );
    STDMETHODIMP            put_Parameter               ( LONG no, VARIANT* value );
    STDMETHODIMP                Execute                 ();
    STDMETHODIMP            put_Parameters              ( SAFEARRAY* param_array );
    STDMETHODIMP            get_Type                    ( Ihostware_record_type ** type );
    STDMETHODIMP            get_Opened                  ( short* result )                       { LOG( *this << ".opened()\n" ); *result = _any_file.opened()? -1 : 0; return NOERROR; }
    STDMETHODIMP                Rewind                  ();
    STDMETHODIMP            put_Date_time_format        ( BSTR format );
    STDMETHODIMP            get_Debug_info              ( BSTR* );
    STDMETHODIMP            get_Row_count               ( int* );
    SOS_OLE_MEMBER_BOOL       ( Write_empty_as_null )
    SOS_OLE_MEMBER_BOOL       ( Read_null_as_empty )
    STDMETHODIMP                Db_open                 ( BSTR select_statement, SAFEARRAY* param_array, Ihostware_file** result );
    STDMETHODIMP                Db_get_single           ( BSTR filename, SAFEARRAY* param_array, Ihostware_dynobj** );
    STDMETHODIMP                Db_execute              ( BSTR statement, SAFEARRAY* param_array, int* result );
    STDMETHODIMP                Db_get_single_value     ( BSTR select_statement, SAFEARRAY* param_array, VARIANT* result );
    STDMETHODIMP            get_Filename                ( BSTR* result );
    STDMETHODIMP                Db_get_blob             ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, VARIANT* result );
    STDMETHODIMP                Db_get_clob             ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, VARIANT* result );
    STDMETHODIMP                Db_get_blob_or_clob     ( BSTR Table_name, BSTR Field_name, BSTR Where_clause, SAFEARRAY* param_array, bool clob, VARIANT* result );
    STDMETHODIMP                Db_get_array            ( BSTR select_statement, SAFEARRAY* param_array, SAFEARRAY** result );

    Hostware_dynobj*            get_key_data            ( const VARIANT& key );
    Hostware_dynobj*            get_record_data         ( const VARIANT& record );
    STDMETHODIMP                get_Db_name_in          ( BSTR* result );
  //Hostware_dynobj*            variant_as_dynobj       ( Record_type*, const VARIANT& );
    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                        { return zschimmer::com::String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                      { return (char*)"sos.hostware.File"; }

//protected:
    Fill_zero                  _zero_;
    Sos_string                 _filename_prefix;
    Any_file                   _any_file;
    Bool                       _opened;
    Sos_ptr<Record_type>       _type;
    Sos_ptr<Field_descr>       _key_descr;
    Dynamic_area               _buffer;
    int4                       _buffer_size;
    Text_format                _text_format;
    Sos_simple_array<Dyn_obj>  _param_values;           // für set_parameters()
};

//-------------------------------------------------------------------------------Hostware_dynobj

struct Hostware_dynobj : Ihostware_dynobj,
                         Ihas_java_class_name,
                         Sos_ole_object,
                         z::My_thread_only
{
    void*                       operator new            ( size_t size )                         { return sos_alloc( size, "Hostware_dynobj" ); }
    void                        operator delete         ( void* ptr )                           { sos_free( ptr ); }


                                Hostware_dynobj         ( Record_type* = NULL, Hostware_file* = NULL );
                                Hostware_dynobj         ( const Hostware_dynobj& );
    virtual                    ~Hostware_dynobj         ();


    Hostware_dynobj&            operator =              ( const Hostware_dynobj& );

    STDMETHODIMP                Clone                   ( Ihostware_dynobj** );

    void                       _obj_print               ( ostream* s ) const;

    USE_SOS_OLE_OBJECT_ADDREF_RELEASE   
    USE_SOS_OLE_OBJECT_GETTYPEINFO      

#ifdef Z_COM
    const static ::zschimmer::com::Com_method _methods[];
#endif

    STDMETHODIMP                QueryInterface          ( const IID&, void** );


    STDMETHODIMP                GetIDsOfNames           ( const IID&, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHODIMP                Invoke                  ( DISPID, const IID&, LCID, unsigned short,
                                                          DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT* );


    void                        assign                  ( const VARIANT& );
  //STDMETHODIMP                get_obj_value           ( VARIANT* );
    STDMETHODIMP                Obj_add_field           ( BSTR name_bstr, VARIANT* type_vt );
    STDMETHODIMP            get_Obj_field_count     ( int* field_count );
    STDMETHODIMP            get_Obj_field_name      ( int field_no, BSTR* name );
    STDMETHODIMP            get_Obj_field_index     ( BSTR name_bstr, int* field_no );
    STDMETHODIMP                Obj_field               ( VARIANT* name, VARIANT* result );
    STDMETHODIMP                Obj_set_field           ( VARIANT* name, VARIANT* result );
    STDMETHODIMP            get_Obj_xml             ( BSTR options, BSTR* result );
    STDMETHODIMP            put_Obj_xml             ( BSTR options, BSTR xml );
    STDMETHODIMP            get_Obj_type            ( Ihostware_record_type** );


    SOS_OLE_MEMBER_BOOL       ( Obj_write_empty_as_null )
    SOS_OLE_MEMBER_BOOL       ( Obj_read_null_as_empty )

    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                        { return zschimmer::com::String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                      { return (char*)"sos.hostware.Record"; }

  //STDMETHODIMP                value                   ( BSTR name, VARIANT* value );

    void                        construct               ();
    void                        finish                  ();     // Nachbearbeitet _type und _record
    Field_descr*                field                   ( VARIANT* field_name );


    Sos_ptr<Field_type>        _type;
    Dynamic_area               _record;
    Dynamic_area               _value_buffer;
    Text_format                _text_format;
};

//-------------------------------------------------------------------------------------------------

//Iexception                      make_iexception         ( const Xc& x );

//-------------------------------------------------------------------------------------------------

} //namespace sos

#endif
