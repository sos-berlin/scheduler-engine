// $Id: spooler_idl.h,v 1.6 2002/11/24 15:32:59 jz Exp $


/*  Ersatz für spooler.odl für Systeme ohne COM. 
    2002 Zschimmer GmbH
*/


#ifndef __SPOOLER_IDL_H
#define __SPOOLER_IDL_H

namespace spooler_com {

//-------------------------------------------------------------------------------------------------

DEFINE_GUID( LIBID_hostWare, 0xED57B226, 0xCD4F, 0x490a, 0xB4, 0x7E, 0xE7, 0xD4, 0x4A, 0x89, 0x52, 0x73 );

//-------------------------------------------------------------------------------------------------

struct IXMLDOMDocument;

struct Ivariable_set;
struct Ilog;
struct Itask;
struct Ithread;
struct Imail;
struct Ijob_chain;
struct Iorder_queue;
struct Iorder;

//--------------------------------------------------------------------------Has_java_class_name

DEFINE_GUID(  IID_Ihas_java_class_name, 0x748E665E, 0x6252, 0x418e, 0x88, 0x7A, 0x55, 0xB1, 0x1F, 0xD8, 0x28, 0x70 );

struct Ihas_java_class_name : IUnknown
{
    DEFINE_UUIDOF( Ihas_java_class_name )

    HRESULT             get_java_class_name             ( BSTR* result );
};

//----------------------------------------------------------------------------------------Error

DEFINE_GUID(  IID_Ierror, 0x5BF4BD80, 0xA437, 0x46df, 0x86, 0xAB, 0x05, 0xE5, 0xE9, 0xE1, 0xC4, 0xE2 );
DEFINE_GUID( CLSID_Error, 0x5B4463E8, 0x4DAE, 0x4198, 0x91, 0xC2, 0x16, 0x04, 0x11, 0x36, 0x9D, 0xF1 );

struct Ierror : IDispatch
{
    DEFINE_UUIDOF( Ierror )

    HRESULT             get_is_error                    ( VARIANT_BOOL* result );
    HRESULT             get_code                        ( BSTR* code );
    HRESULT             get_text                        ( BSTR* text );
};

//------------------------------------------------------------------------------------Ivariable

DEFINE_GUID(  IID_Ivariable,  0x2B9F8928, 0x6722, 0x40a3, 0xB6, 0xEC, 0x24, 0x55, 0xF0, 0xA2, 0x53, 0xF7 );
DEFINE_GUID( CLSID_Variable,  0xE5F145CE, 0x7AB6, 0x4204, 0x93, 0x06, 0xFE, 0xCD, 0x4B, 0xF7, 0xA6, 0x19 );

struct Ivariable : IDispatch
{
    DEFINE_UUIDOF( Ivariable )

    HRESULT             put_value                       ( VARIANT* value );
    HRESULT             get_value                       ( VARIANT* value );
  //HRESULT                 dim                         ( int size );
    HRESULT             get_name                        ( BSTR* name );
    HRESULT                 Clone                       ( Ivariable** );
};

//---------------------------------------------------------------------------------Variable_set

DEFINE_GUID(  IID_Ivariable_set, 0x5F028EDD, 0x2195, 0x4bcc, 0xB6, 0x3F, 0xFC, 0x88, 0xB7, 0x3A, 0x47, 0x12 );
DEFINE_GUID( CLSID_Variable_set, 0x818150DA, 0xCD01, 0x4db3, 0x97, 0x2B, 0x43, 0x3A, 0x9D, 0x62, 0x37, 0xF3 );

struct Ivariable_set : IDispatch
{
    DEFINE_UUIDOF( Ivariable_set )

    HRESULT                 set_var                     ( VARIANT* value );
    HRESULT             put_var                         ( VARIANT* value );
    HRESULT             get_var                         ( VARIANT* value );
    HRESULT             get_count                       ( int* value );
    HRESULT             get_dom                         ( IXMLDOMDocument** xml_document );
    HRESULT                 Clone                       ( Ivariable_set** result );
    HRESULT                 merge                       ( Ivariable_set* other);
    HRESULT                 _NewEnum                    ( IUnknown** enumerator );    
};

//----------------------------------------------------------------------Variable_set_enumerator

DEFINE_GUID(  IID_Ivariable_set_enumerator, 0x4FA28F03, 0x7970, 0x43cf, 0xBA, 0xFF, 0x5F, 0x2F, 0x0A, 0xF8, 0xB1, 0xB6 );
DEFINE_GUID( CLSID_Variable_set_enumerator, 0xEFAC6F36, 0x563F, 0x42a7, 0x86, 0xD7, 0x17, 0x0A, 0x1A, 0x18, 0xD1, 0xD5 );

struct Ivariable_set_enumerator : IEnumVARIANT
{ 
    DEFINE_UUIDOF( Ivariable_set_enumerator )

    HRESULT                 Next                    ( unsigned long celt, VARIANT* rgvar, unsigned long* pceltFetched );
    HRESULT                 Skip                    ( unsigned long celt );
    HRESULT                 Reset                   ();
    HRESULT                 Clone                   ( IEnumVARIANT** ppenum );
};

//-----------------------------------------------------------------------------------Object_set

DEFINE_GUID(  IID_Iobject_set,  0xFB6288E5, 0x3391, 0x4c9d, 0xA1, 0x0E, 0x04, 0xE1, 0xCC, 0x16, 0x29, 0x00 );
DEFINE_GUID( CLSID_Object_set,  0x9A38442F, 0x9816, 0x49b9, 0xAB, 0x67, 0x6B, 0x35, 0x2C, 0xF8, 0x4F, 0x66 );

struct Iobject_set : IDispatch
{
    DEFINE_UUIDOF( Iobject_set )

    HRESULT             get_low_level                   ( int* level );
    HRESULT             get_high_level                  ( int* level );
};

//------------------------------------------------------------------------------------------Job

DEFINE_GUID( IID_Ijob,  0x4DF2E8E8, 0x52DA, 0x454f, 0x8A, 0xDE, 0x3B, 0x5D, 0x4F, 0xE7, 0xF7, 0x3B );
DEFINE_GUID( CLSID_Job,  0x15ECAE84, 0x4142, 0x43e6, 0xA3, 0x8C, 0xC9, 0x0A, 0x29, 0x90, 0x97, 0xB4 );

struct Ijob : IDispatch
{
    DEFINE_UUIDOF( Ijob )

    HRESULT                 start_when_directory_changed( BSTR filename_pattern );
    HRESULT                 clear_when_directory_changed();
  //HRESULT                 start_on_signal             ();
  //HRESULT                 start_after_timeout         ( Itask** );
    HRESULT                 start                       ( Itask** );
    HRESULT             get_thread                      ( Ithread** result );
  //HRESULT             put_include_path                ( BSTR include_path );
    HRESULT             get_include_path                ( BSTR* include_path );
    HRESULT             get_name                        ( BSTR* name );
    HRESULT                 wake                        ();
    HRESULT             put_state_text                  ( BSTR state_text );
    HRESULT             get_title                       ( BSTR* title );
    HRESULT             put_delay_after_error           ( VARIANT* time );
    HRESULT             get_order_queue                 ( Iorder_queue** result );
};

//-----------------------------------------------------------------------------------------Task

DEFINE_GUID( IID_Itask,  0x65E311F1, 0x04BF, 0x4e34, 0xA8, 0x47, 0xBB, 0xF0, 0xB0, 0xAC, 0x6D, 0xC6 );
DEFINE_GUID( CLSID_Task,   0x00BB25C8, 0x812A, 0x4200, 0xA6, 0xF9, 0x1A, 0xE8, 0xE2, 0x65, 0x04, 0x74 );

struct Itask : IDispatch
{
    DEFINE_UUIDOF( Itask )

    HRESULT             get_object_set                  ( Iobject_set** result );
    HRESULT             put_error                       ( VARIANT* error_text );       // Für PerlScript, das einen Fehlertext nicht durchreicht
    HRESULT             get_error                       ( Ierror** result );
    HRESULT             get_job                         ( Ijob** job );
    HRESULT             get_params                      ( Ivariable_set** parameters );
    HRESULT             put_result                      ( VARIANT* value );
    HRESULT             get_result                      ( VARIANT* value );
    HRESULT                 wait_until_terminated       ( VARIANT_BOOL* ok );
    HRESULT             put_repeat                      ( double seconds );
    HRESULT             get_thread                      ( Ithread** result );
    HRESULT                 end                         ();
    HRESULT             put_history_field               ( VARIANT* value );
    HRESULT             get_id                          ( int* result );
    HRESULT             put_delay_spooler_process       ( VARIANT* seconds );
    HRESULT             put_close_engine                ( VARIANT_BOOL close_after_task );
    HRESULT             get_order                       ( Iorder** result );
};

//---------------------------------------------------------------------------------------Thread

DEFINE_GUID( IID_Ithread,  0x0FA3AC14, 0x01EC, 0x4c8f, 0x81, 0xD9, 0x7C, 0xAB, 0xF8, 0xA7, 0x6B, 0x43 );
DEFINE_GUID( CLSID_Thread, 0x4E70F30E, 0xD446, 0x42b1, 0xB4, 0xF0, 0x7A, 0x53, 0x11, 0x3D, 0x87, 0xC7 );

struct Ithread : IDispatch
{
    DEFINE_UUIDOF( Ithread )

    HRESULT             get_log                     ( Ilog** log );
    HRESULT             get_script                  ( IDispatch** script_object );
  //HRESULT             put_include_path            ( BSTR include_path );
    HRESULT             get_include_path            ( BSTR* include_path );
    HRESULT             get_name                    ( BSTR* name );
};
//--------------------------------------------------------------------------------------Spooler

DEFINE_GUID( IID_Ispooler,  0x3D8FF20C, 0x5CFD, 0x4b70, 0x9A, 0x2D, 0xB5, 0x1A, 0xB3, 0xDC, 0xB9, 0x8D );
DEFINE_GUID( CLSID_Spooler, 0x87605BDB, 0x42C7, 0x43e7, 0xB3, 0x11, 0xD5, 0x68, 0xF8, 0x6D, 0x78, 0xB5 );

struct Ispooler : IDispatch
{
    DEFINE_UUIDOF( Ispooler )

    HRESULT             get_log                     ( Ilog** log );
    HRESULT             get_id                      ( BSTR* spooler_id );
    HRESULT             get_param                   ( BSTR* spooler_param );
    HRESULT             get_script                  ( IDispatch** script_object );
    HRESULT             get_job                     ( Ijob** job );
    HRESULT                 create_variable_set     ( Ivariable_set** result );
  //HRESULT             put_include_path            ( BSTR include_path );
    HRESULT             get_include_path            ( BSTR* include_path );
    HRESULT             get_log_dir                 ( BSTR* directory );
    HRESULT                 let_run_terminate_and_restart();
    HRESULT             get_variables               ( Ivariable_set** );
    HRESULT             put_var                     ( VARIANT* value );
    HRESULT             get_var                     ( VARIANT* value );
    HRESULT             get_db_name                 ( BSTR* filename );
    HRESULT                 create_job_chain        ( Ijob_chain** result );
    HRESULT                 add_job_chain           ( Ijob_chain* job_chain );
    HRESULT             get_job_chain               ( Ijob_chain** result );
    HRESULT                 create_order            ( Iorder** result );
    HRESULT             get_is_service              ( VARIANT_BOOL* result );
};

//------------------------------------------------------------------------------------Log_level

enum Log_level
{
    log_debug9 = -9,
    log_debug8 = -8,
    log_debug7 = -7,
    log_debug6 = -6,
    log_debug5 = -5,
    log_debug4 = -4,
    log_debug3 = -3,
    log_debug2 = -2,
    log_debug1 = -1,
    log_debug  = -1,
    log_info   =  0, 
    log_warn   =  1, 
    log_error  =  2,
  //log_fatal  =  3
};

//------------------------------------------------------------------------------------------Log

DEFINE_GUID( IID_Ilog,  0x3B6C8A62, 0xB511, 0x445d, 0xA2, 0xA2, 0xE8, 0x52, 0xBC, 0x2E, 0x05, 0xA0 );
DEFINE_GUID( CLSID_Log, 0x032974D7, 0x8668, 0x41c4, 0xA2, 0x04, 0x3E, 0x89, 0x3E, 0x06, 0x84, 0x6F );

struct Ilog : IDispatch
{
    DEFINE_UUIDOF( Ilog )

    HRESULT                 debug9                  ( BSTR line );
    HRESULT                 debug8                  ( BSTR line );
    HRESULT                 debug7                  ( BSTR line );
    HRESULT                 debug6                  ( BSTR line );
    HRESULT                 debug5                  ( BSTR line );
    HRESULT                 debug4                  ( BSTR line );
    HRESULT                 debug3                  ( BSTR line );
    HRESULT                 debug2                  ( BSTR line );
    HRESULT                 debug1                  ( BSTR line );
    
    HRESULT                 debug                   ( BSTR line );
    
    HRESULT                 info                    ( BSTR line );
    HRESULT                 msg                     ( BSTR line );     // Zur Kompatibilität, wie info()
    HRESULT                 warn                    ( BSTR line );
    HRESULT                 error                   ( BSTR line );
    HRESULT                 log                     ( BSTR line );

    HRESULT             get_mail                    ( Imail** mail );

    HRESULT             put_mail_on_error           ( VARIANT_BOOL mail_on_error );
    HRESULT             get_mail_on_error           ( VARIANT_BOOL* mail_on_error );

    HRESULT             put_mail_on_success         ( VARIANT_BOOL mail_on_success );
    HRESULT             get_mail_on_success         ( VARIANT_BOOL* mail_on_success );

    HRESULT             put_mail_on_process         ( int level );
    HRESULT             get_mail_on_process         ( int* level );

    HRESULT             put_level                   ( int level );
    HRESULT             get_level                   ( int* level );

    HRESULT             get_filename                ( BSTR* filename );

    HRESULT             put_new_filename            ( BSTR filename );
    HRESULT             get_new_filename            ( BSTR* filename );

    HRESULT             put_collect_within          ( VARIANT* time );
    HRESULT             get_collect_within          ( double* time );

    HRESULT             put_collect_max             ( VARIANT* time );
    HRESULT             get_collect_max             ( double* time );
};

//--------------------------------------------------------------------------------------Context

DEFINE_GUID( IID_Icontext,  0x51905432, 0xB068, 0x4124, 0xB9, 0x72, 0x26, 0x0E, 0xED, 0x6C, 0xAD, 0x16 );
DEFINE_GUID( CLSID_Context, 0x47399CB4, 0xB7A4, 0x40f9, 0xA9, 0xAE, 0x7A, 0xE4, 0xCB, 0xAE, 0x90, 0xF5 );

struct Icontext : IDispatch
{
    DEFINE_UUIDOF( Icontext )

    HRESULT             get_log                     ( Ilog** log );

    HRESULT             get_spooler                 ( Ispooler** spooler );

    HRESULT             get_thread                  ( Ithread** thread );

    HRESULT             get_job                     ( Ijob** job );

    HRESULT             get_task                    ( Itask** task );
};

//-----------------------------------------------------------------------------------------Mail

DEFINE_GUID( IID_Imail,  0x736AD9FC, 0x350B, 0x4ee0, 0xBF, 0x82, 0xB5, 0xCB, 0x2C, 0xFA, 0x0E, 0x3B );
DEFINE_GUID( CLSID_mail, 0xD5F4C5B5, 0x4CF7, 0x4ca7, 0x84, 0xC0, 0xE1, 0x69, 0x6D, 0xA5, 0x6D, 0x1D );

struct Imail : IDispatch
{
    DEFINE_UUIDOF( Imail )

    HRESULT             put_to                      ( BSTR receipient );
    HRESULT             get_to                      ( BSTR* receipient );

    HRESULT             put_from                    ( BSTR from );

    HRESULT             get_from                    ( BSTR* from );

  //
  //HRESULT             put_reply_to                ( BSTR receipient );

    HRESULT             put_cc                      ( BSTR receipients );
    HRESULT             get_cc                      ( BSTR* receipients );

    HRESULT             put_bcc                     ( BSTR receipients );
    HRESULT             get_bcc                     ( BSTR* receipients );

    HRESULT             put_subject                 ( BSTR subject );
    HRESULT             get_subject                 ( BSTR* subject );

    HRESULT             put_body                    ( BSTR body );
    HRESULT             get_body                    ( BSTR* body );

    HRESULT                 add_file                ( BSTR encoding );

    HRESULT             put_smtp                    ( BSTR hostname );
    HRESULT             get_smtp                    ( BSTR* host );

    HRESULT             put_queue_dir               ( BSTR directory );
    HRESULT             get_queue_dir               ( BSTR* directory );

    HRESULT                 add_header_field        ( BSTR value );

    HRESULT                 dequeue                 ( int* count );
    HRESULT             get_dequeue_log             ( BSTR* );
};

//--------------------------------------------------------------------------------Job_chain_node

DEFINE_GUID( IID_Ijob_chain_node,  0x5CC81C2F, 0xF015, 0x4591, 0xB9, 0x73, 0xB8, 0xB4, 0x96, 0x98, 0x09, 0xC4 );
DEFINE_GUID( CLSID_Job_chain_node, 0xF22B96E3, 0x475A, 0x456e, 0x9C, 0xAE, 0x8C, 0xB9, 0x88, 0x10, 0xB8, 0xDB );

struct Ijob_chain_node : IDispatch
{
    DEFINE_UUIDOF( Ijob_chain_node )

    HRESULT             get_state                   ( VARIANT* result );

    HRESULT             get_next_node               ( Ijob_chain_node** result );

    HRESULT             get_error_node              ( Ijob_chain_node** result );

    HRESULT             get_job                     ( Ijob** result );
};

//-------------------------------------------------------------------------------------Job_chain

DEFINE_GUID( IID_Ijob_chain,  0xE76C561D, 0xFFD7, 0x4680, 0xAC, 0x4F, 0x9C, 0xE1, 0xB8, 0xEE, 0xE4, 0xFE );
DEFINE_GUID( CLSID_Job_chain, 0x70EC6128, 0xF7FF, 0x44e6, 0xB3, 0x52, 0x56, 0xCF, 0x12, 0x3F, 0x78, 0x27 );

struct Ijob_chain : IDispatch
{
    DEFINE_UUIDOF( Ijob_chain )

    
    HRESULT             put_name                    ( BSTR name );

    HRESULT             get_name                    ( BSTR* result );

    HRESULT             get_order_count             ( int* result );

    HRESULT                 add_job                 ( VARIANT* jobname, 
                                                      VARIANT* input_state,
                                                      VARIANT* output_state,
                                                      VARIANT* error_state );

    HRESULT                 add_end_state           ( VARIANT* state );

    HRESULT                 add_order               ( Iorder** order );

    HRESULT             get_node                    ( Ijob_chain_node** result );

    HRESULT             get_order_queue             ( Iorder_queue** result );
};

//----------------------------------------------------------------------------------Order_queue

DEFINE_GUID( IID_Iorder_queue,  0x497E0166, 0x26FF, 0x4ea6, 0xB8, 0x46, 0x34, 0xF4, 0xBB, 0x97, 0x02, 0x1D );
DEFINE_GUID( CLSID_Order_queue, 0x65426DA4, 0x5907, 0x4630, 0x85, 0xF1, 0x22, 0x01, 0xB0, 0x72, 0x53, 0xBB );

struct Iorder_queue : IDispatch
{
    DEFINE_UUIDOF( Iorder_queue )

    HRESULT             get_length                  ( int* result );

    HRESULT                 add_order               ( Iorder** );
};

//----------------------------------------------------------------------------------------Order

DEFINE_GUID( IID_Iorder,  0xA1B7B832, 0x519C, 0x495b, 0xA1, 0xD9, 0x83, 0xAA, 0xA0, 0x16, 0x5C, 0x6D );
DEFINE_GUID( CLSID_Order, 0x8D3FAB02, 0x6DD5, 0x4831, 0x91, 0x6C, 0x46, 0x5E, 0x15, 0xE0, 0x8F, 0x35 );

struct Iorder : IDispatch
{
    DEFINE_UUIDOF( Iorder )

    HRESULT             put_id                      ( VARIANT* value );
    HRESULT             get_id                      ( VARIANT* result );

    HRESULT             put_title                   ( BSTR value );
    HRESULT             get_title                   ( BSTR* result );

    HRESULT             put_priority                ( int value );
    HRESULT             get_priority                ( int* result );

    HRESULT             get_job_chain               ( Ijob_chain** result );

    HRESULT             get_job_chain_node          ( Ijob_chain_node** result );

    HRESULT             put_job                     ( VARIANT* job_or_jobname );
    HRESULT          putref_job                     ( Ijob* job );
    HRESULT             get_job                     ( Ijob** result );

    HRESULT             put_state                   ( VARIANT* value );
    HRESULT             get_state                   ( VARIANT* result );

    HRESULT             put_state_text              ( BSTR value );
    HRESULT             get_state_text              ( BSTR* result );

    HRESULT             get_error                   ( Ierror** result );

    HRESULT             put_payload                 ( VARIANT* value );
    HRESULT          putref_payload                 ( IUnknown* value );
    HRESULT             get_payload                 ( VARIANT* result );

    HRESULT                 payload_is_type         ( VARIANT_BOOL* result );
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler_com

#endif


