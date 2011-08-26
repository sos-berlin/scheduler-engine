// $Id: hostole_ptr.h 12688 2007-03-23 08:56:43Z jz $

// Hostole für zschimmer.h, frei von sos.h, mit HRESULT-Prüfung und Exceptions

#ifndef __HOSTOLE_PTR_H
#define __HOSTOLE_PTR_H


#include "../zschimmer/z_com.h"
#include "../hostole/hostole.h"


#ifdef _WIN32
#   pragma warning( push )
#   pragma warning( disable: 4800 )     // Variable wird auf booleschen Wert ('True' oder 'False') gesetzt (Auswirkungen auf Leistungsverhalten moeglich)
#   pragma auto_inline( off )
#endif


namespace sos {
namespace hostole {

//------------------------------------------------------------------------------------throw_hostole

void throw_hostole( HRESULT hr, const char* function );

//-------------------------------------------------------------------------------------------X_CALL

#define X_CALL( CALL, PARAMS )                                                                  \
    do {                                                                                        \
        HRESULT hr = _ptr->CALL PARAMS;                                                         \
        if( FAILED(hr) )  throw_hostole( hr, CLASSNAME "." #CALL );                             \
    } while(0)

//------------------------------------------------------------------------------------------X_CALL_

#define X_CALL_( RESULTTYPE, CALL, PARAMS )                                                     \
    do {                                                                                        \
        RESULTTYPE result;                                                                      \
        HRESULT hr = _ptr->CALL PARAMS;                                                         \
        if( FAILED(hr) )  throw_hostole( hr, CLASSNAME "." #CALL );                             \
        return result;                                                                          \
    } while(0)

//----------------------------------------------------------------------------------hostole_class<>
/*
template< class CLASS, class HOSTOLE_INTERFACE >
struct hostole_class : zschimmer::ptr<HOSTOLE_INTERFACE>
{
    typedef zschimmer::com::Variant  Variant;
    typedef zschimmer::com::Bstr     Bstr;



                                Record              ()                                      {}
                                Record              ( Ihostware_dynobj* o )                 : zschimmer::ptr<Ihostware_dynobj>(o) {}

    Record&                     operator =          ( Ihostware_dynobj* o )                         { zschimmer::ptr<Ihostware_dynobj>::operator = ( o );  return *this; }
};
*/
//-------------------------------------------------------------------------------------------Record
#define CLASSNAME "Record"

struct Record : zschimmer::ptr<Ihostware_dynobj>
{
    typedef zschimmer::com::Variant  Variant;
    typedef zschimmer::com::Bstr     Bstr;



                                Record              ()                                      {}
                                Record              ( Ihostware_dynobj* o )                 : zschimmer::ptr<Ihostware_dynobj>(o) {}

    Record&                     operator =          ( Ihostware_dynobj* o )                 { zschimmer::ptr<Ihostware_dynobj>::operator = ( o );  return *this; }

    Record                      clone               ()                                  const { X_CALL_( Record       , Clone                     , ( result.pp() ) ); }

    void                        add_field           ( BSTR name_bstr, VARIANT* type_vt )const { X_CALL (                Obj_add_field             , ( name_bstr, type_vt ) ); }

    int                         field_count         ()                                  const { X_CALL_( int          , get_Obj_field_count       , ( &result ) ); }

    void                    get_field_name          ( int field_no, BSTR* result )      const { X_CALL (                get_Obj_field_name        , ( field_no, result ) ); }
    string                      field_name          ( int field_no )                    const { Bstr result;  get_field_name( field_no, &result._bstr );  return string_from_bstr( result ); }

    int                         field_index         ( BSTR name_bstr )                  const { X_CALL_( int          , get_Obj_field_index       , ( name_bstr, &result ) ); }

    void                    get_field               ( const VARIANT& name, VARIANT* result)const{X_CALL (               Obj_field                 , ( const_cast<VARIANT*>( &name ), result ) ); }
    void                    get_field               ( const Variant& name, VARIANT* result)const{get_field( static_cast<const VARIANT&>( name ), result ); }
    Variant                     field               ( const VARIANT& name )             const { X_CALL_( Variant      , Obj_field                 , ( const_cast<VARIANT*>( &name ), &result ) ); }
    Variant                     field               ( const Variant& name )             const { return field( static_cast<const VARIANT&>( name ) ); }

    Variant                     operator[]          ( const VARIANT& name )             const { return field( name ); }
    Variant                     operator[]          ( const string& name )              const { return field( name ); }
    Variant                     operator[]          ( const char* name )                const { return field( Variant( name ) ); }
    Variant                     operator[]          ( int nr )                          const { return field( Variant( nr ) ); }

    bool                        is_null             ( const string& name )              const { return as_int( name.c_str() ); }
    bool                        is_null             ( const char* name )                const;
    bool                        is_null             ( int nr )                          const;

    int                         as_int              ( const string& name )              const { return as_int( name.c_str() ); }
    int                         as_int              ( const char* name )                const;
    int                         as_int              ( int nr )                          const;

    int                         as_int              ( const string& name, int value_for_null )   const { return as_int( name.c_str(), value_for_null ); }
    int                         as_int              ( const char* name  , int value_for_null )   const;
    int                         as_int              ( int nr            , int value_for_null )   const;

    bool                        as_bool             ( const string& name )              const { return as_bool( name.c_str() ); }
    bool                        as_bool             ( const char* name )                const;
    bool                        as_bool             ( int nr )                          const;

    string                      as_string           ( const string& name )              const { return as_string( name.c_str() ); }
    string                      as_string           ( const char* name )                const;
    string                      as_string           ( int nr )                          const;
/*
    string                      as_string           ( const string& name, const string& deflt )   const { return as_string( name.c_str() ); }
    string                      as_string           ( const char* name, const char* deflt );
    string                      as_string           ( int nr, const char* delft );
*/
    void                        set_field           ( const VARIANT& name, const VARIANT& value )  const { X_CALL (           Obj_set_field             , ( const_cast<VARIANT*>( &name ), const_cast<VARIANT*>( &value ) ) ); }
    void                        set_field           ( const Variant& name, const VARIANT& value )  const { X_CALL (           Obj_set_field             , ( const_cast<Variant*>( &name ), const_cast<VARIANT*>( &value ) ) ); }
    void                        set_field           ( const string& name , const VARIANT& value )  const { set_field( Variant(name), value ); }

    void                    get_xml                 ( BSTR options, BSTR* result )      const { X_CALL (                get_Obj_xml               , ( options, result ) ); }
    string                      xml                 ( const string& options = "" )      const { Bstr result;  get_xml( Bstr(options), &result._bstr );  return string_from_bstr(result); }
    void                    set_xml                 ( BSTR options, BSTR xml )          const { X_CALL (                put_Obj_xml               , ( options, xml ) ); }
  //Ihostware_record_type_ptr get_type              ()                                  const { X_CALL_( Ihostware_record_type_ptr,Ihostware_dynobj,get_obj_type              , ( &result ) ); }
  //void                    get_write_empty_as_null ( VARIANT_BOOL* b )                 const { X_CALL (                get_Obj_write_empty_as_null, ( b ) ); }
    void                    set_write_empty_as_null ( VARIANT_BOOL b )                  const { X_CALL (                put_Obj_write_empty_as_null, ( b ) ); }
                                                                                        
  private:
    void*                   operator ->             ();                                     // gesperrt
};

#undef CLASSNAME
//---------------------------------------------------------------------------------------------File
#define CLASSNAME "File"

struct File : zschimmer::ptr<Ihostware_file>
{
    typedef zschimmer::com::Bstr    Bstr;
    typedef zschimmer::com::Variant Variant;
    typedef zschimmer::com::Locked_safearray<Variant> Locked_safearray;


                            File                    ()                                          {}
                            File                    ( Ihostware_file* o )                       : zschimmer::ptr<Ihostware_file>(o) {}
                          //File                    ( const CLSID& clsid )                      { HRESULT hr = CoCreateInstance(clsid);  if( FAILED(hr) )  throw_hostole( hr, "File::CoCreateInstance" ); }
                            File                    ( const Bstr&   filename )                  { open( filename ); ; }
                            File                    ( const string& filename )                  { open( filename ); ; }
                            File                    ( const char*   filename )                  { open( filename ); ; }
                           ~File                    ()                                          {}

    File&                   operator =              ( Ihostware_file* o )                       { zschimmer::ptr<Ihostware_file>::operator = ( o );  return *this; }

    void                    create_instance         ()                                          { zschimmer::ptr<Ihostware_file>::create_instance( CLSID_File ); }

    void                    open                    ( BSTR filename )                           { if( !_ptr )  create_instance();
                                                                                                  X_CALL (                Open                      , ( filename ) ); }

    void                    open                    ( const string& filename )                  { open( Bstr( filename ) ); }

    void                    open                    ( const char*   filename )                  { open( Bstr( filename ) ); }

    void                    close                   ()                                    const { X_CALL (                Close                     , () ); }

    void                    close_file              ()                                    const { X_CALL (                Close_file                , () ); }
    
    void                    close_cursor            ()                                    const { X_CALL (                Close_cursor              , () ); }
    
    void                    put_line                ( BSTR line )                         const { X_CALL (                Put_line                  , ( line ) ); }
    void                    put_line                ( const string& line )                const { put_line( Bstr(line) ); }
    
    void                    get_line                ( BSTR* result )                      const { X_CALL (                Get_line                  , ( result ) ); }
    Bstr                    get_line                ()                                    const { Bstr result;  get_line( &result._bstr );  return result; }
    
    bool                    eof                     ()                                    const { X_CALL_( VARIANT_BOOL , Eof                       , ( &result ) ); }
    
    Record                  create_record           ()                                    const { X_CALL_( Record       , Create_record             , ( result.pp() ) ); }
    
    Record                  create_key              ()                                    const { X_CALL_( Record       , Create_key                , ( result.pp() ) ); }
    
    Record                  get                     ()                                    const { X_CALL_( Record       , Get                       , ( result.pp() ) ); }
    
    void                    put                     ( const VARIANT& object )             const { X_CALL (                Put                       , ( const_cast<VARIANT*>( &object ) ) ); }
    
    void                    set_key                 ( const VARIANT& key )                const { X_CALL (                Set_key                   , ( const_cast<VARIANT*>( &key ) ) ); }
    
    void                    delete_key              ( const VARIANT& key )                const { X_CALL (                Delete_key                , ( const_cast<VARIANT*>( &key ) ) ); }
    Record                  get_key                 ( const VARIANT& key )                const { X_CALL_( Record       , Get_key                   , ( const_cast<VARIANT*>( &key ), result.pp() ) ); }
    void                    insert                  ( const VARIANT& record )             const { X_CALL (                Insert                    , ( const_cast<VARIANT*>( &record ) ) ); }
    void                    update                  ( const VARIANT& record )             const { X_CALL (                Update                    , ( const_cast<VARIANT*>( &record ) ) ); }
    void                    update_direct           ( const VARIANT& record )             const { X_CALL (                Update_direct             , ( const_cast<VARIANT*>( &record ) ) ); }
    void                    store                   ( const VARIANT& record )             const { X_CALL (                Store                     , ( const_cast<VARIANT*>( &record ) ) ); }
    void                set_date_format             ( BSTR format )                       const { X_CALL (                put_Date_format           , ( format ) ); }
    void                set_decimal_symbol          ( BSTR symbol )                       const { X_CALL (                put_Decimal_symbol        , ( symbol ) ); }
    void                get_field_name              ( LONG number, BSTR* result )         const { X_CALL (                get_Field_name            , ( number, result ) ); }
    Bstr                    field_name              ( LONG number )                       const { X_CALL_( Bstr         , get_Field_name            , ( number, &result._bstr ) ); }
    LONG                    field_count             ()                                    const { X_CALL_( LONG         , get_Field_count           , ( &result ) ); }
    void                    prepare                 ( BSTR filename )                     const { X_CALL (                Prepare                   , ( filename ) ); }
    void                set_parameter               ( LONG no, const VARIANT& value )     const { X_CALL (                put_Parameter             , ( no, const_cast<VARIANT*>( &value ) ) ); }
    void                    execute                 ()                                    const { X_CALL (                Execute                   , () ); }
    void                set_parameters              ( const SAFEARRAY& param_array )      const { X_CALL (                put_Parameters            , ( const_cast<SAFEARRAY*>( &param_array ) ) ); }
  //void                    type                    ( Ihostware_record_type ** type )     const { X_CALL (                get_Type                  , ( type ) ); }
    bool                    opened                  ()                                    const { X_CALL_( VARIANT_BOOL , get_Opened                , ( &result ) ); }
    void                    rewind                  ()                                    const { X_CALL (                Rewind                    , () ); }
    void                set_date_time_format        ( BSTR format )                       const { X_CALL (                put_Date_time_format      , ( format ) ); }
    Bstr                    debug_info              ()                                    const { X_CALL_( Bstr         , get_Debug_info            , ( &result._bstr ) ); }
  //void                get_write_empty_as_null     ( VARIANT_BOOL* b )                   const { X_CALL (                get_Write_empty_as_null   , ( b ) ); }
    void                set_write_empty_as_null     ( VARIANT_BOOL b )                    const { X_CALL (                put_Write_empty_as_null   , ( b ) ); }
    int                     row_count               ()                                    const { X_CALL_( int          , get_Row_count             , ( &result ) ); }

    File                    db_open                 ( const string& stmt )                const { X_CALL_( File         , Db_open                   , ( Bstr(stmt), NULL, result.pp() ) ); }
    File                    db_open                 ( const string& stmt, const Variant& par1 )
                                                                                          const { Locked_safearray params ( 1 );  params[0] = &par1; 
                                                                                                  X_CALL_( File         , Db_open                   , ( Bstr(stmt), params.safearray_ptr(), result.pp() ) ); }
    File                    db_open                 ( const string& stmt, const Variant& par1, const Variant& par2 )
                                                                                          const { Locked_safearray params ( 2 );  params[0] = &par1;  params[1] = par2;
                                                                                                  X_CALL_( File         , Db_open                   , ( Bstr(stmt), params.safearray_ptr(), result.pp() ) ); }

    Record                  db_get_single           ( const string& stmt )                const { X_CALL_( Record       , Db_get_single             , ( Bstr(stmt), NULL, result.pp() ) ); }
    Record                  db_get_single           ( const string& stmt, const Variant& par1 )
                                                                                          const { Locked_safearray params ( 1 );  params[0] = &par1;
                                                                                                  X_CALL_( Record       , Db_get_single             , ( Bstr(stmt), NULL, result.pp() ) ); }


  private:
    void*                   operator ->             ();
};

#undef CLASSNAME
//-----------------------------------------------------------------------------------------Hostware
#define CLASSNAME "Hostware"

struct Hostware : zschimmer::ptr<Ihostware>
{
    typedef zschimmer::com::Bstr    Bstr;
    typedef zschimmer::com::Variant Variant;



                            Hostware                ()                                          {}
                            Hostware                ( Ihostware* o )                            : zschimmer::ptr<Ihostware>(o) {}

    Hostware&               operator =              ( Ihostware* o )                                       { zschimmer::ptr<Ihostware>::operator = ( o );  return *this; }

    void                    create_instance         ()                                                     { zschimmer::ptr<Ihostware>::create_instance( CLSID_Global ); }
    HRESULT                 Create_instance         ()                                                     { return zschimmer::ptr<Ihostware>::CoCreateInstance( CLSID_Global ); }
    void                    copy_file               ( BSTR a, BSTR b )                               const { X_CALL (                        Copy_file              , (a,b) ); }
    void                    shell_execute_and_wait  ( BSTR file, BSTR verb, double seconds )         const { X_CALL (                        Shell_execute_and_wait , ( file, verb, seconds ) ); }
    
    Record                  get_single              ( BSTR stmt, SAFEARRAY* param_array = NULL )     const { X_CALL_( Record               , Get_single             , ( stmt, param_array, result.pp() ) ); }
    Record                  get_single              ( const string& stmt, SAFEARRAY* param_array=NULL )const{ Bstr stmt_bstr = stmt;
                                                                                                             X_CALL_( Record               , Get_single             , ( stmt_bstr, param_array, result.pp() ) ); }

    Variant                 get_single_value        ( BSTR          stmt, SAFEARRAY* param_array = NULL ) const { X_CALL_( Variant              , Get_single_value       , (      stmt , param_array, &result ) ); }
    Variant                 get_single_value        ( const string& stmt, SAFEARRAY* param_array = NULL ) const { return get_single_value( Bstr(stmt), param_array ); }
    void                    get_single_value        ( const string& stmt, VARIANT* result               ) const { X_CALL (                        Get_single_value       , ( Bstr(stmt), NULL       ,  result ) ); }

    void                    execute_direct          ( BSTR stmt, SAFEARRAY* param_array = NULL )     const { X_CALL (                 Execute_direct         , ( stmt, param_array ) ); }
  //void                    letter_factory          ( BSTR param );
    bool                    null                    ( VARIANT* o )                                   const { X_CALL_( VARIANT_BOOL  , Null                   , ( o, &result ) ); }
    bool                    empty                   ( VARIANT* o )                                   const { X_CALL_( VARIANT_BOOL  , Empty                  , ( o, &result ) ); }
    void                    sleep                   ( double seconds )                               const { X_CALL (                 Sleep                  , ( &seconds ) ); }
    void                    remove_file             ( BSTR filename )                                const { X_CALL (                 Remove_file            , ( filename ) ); }
    void                    rename_file             ( BSTR old_filename, BSTR new_filename )         const { X_CALL (                 Rename_file            , ( old_filename, new_filename ) ); }

    void                    file_as_string          ( BSTR filename, BSTR* result )                  const { X_CALL (                 File_as_string         , ( filename, result ) ); }
    string                  file_as_string          ( const string& filename )                       const { Bstr result; file_as_string( Bstr(filename), &result._bstr );  return string_from_bstr(result); }

    void                    file_as_string_or_empty ( BSTR filename, BSTR* result )                  const;
    string                  file_as_string_or_empty ( const string& filename )                       const;

    void                    as_parser_string        ( BSTR o, BSTR quote, BSTR *result )             const { X_CALL (                 As_parser_string       , ( o, quote, result ) ); }
    Bstr                    as_parser_string        ( BSTR o, BSTR quote )                           const { X_CALL_( Bstr          , As_parser_string       , ( o, quote, &result._bstr ) ); }
    
    void                    as_xml                  ( VARIANT* field_or_dynobj, BSTR options, BSTR* result ) const { X_CALL (         As_xml                 , ( field_or_dynobj, options, result ) ); }
    Bstr                    as_xml                  ( VARIANT* field_or_dynobj, BSTR options )       const { X_CALL_( Bstr          , As_xml                 , ( field_or_dynobj, options, &result._bstr ) ); }
    
    void                    from_xml                ( BSTR xml, BSTR options, BSTR* result )         const { X_CALL (                 From_xml               , ( xml, options, result ) ); }
    Bstr                    from_xml                ( BSTR xml, BSTR options )                       const { X_CALL_( Bstr          , From_xml               , ( xml, options, &result._bstr ) ); }
    
    DATE                    as_date                 ( BSTR date, BSTR format )                       const { X_CALL_( DATE          , As_date                , ( date, format, &result ) ); }
    
    void                    date_as_string          ( VARIANT* date, BSTR format, BSTR* result )     const { X_CALL (                 Date_as_string         , ( date, format, result ) ); }
    Bstr                    date_as_string          ( VARIANT* date, BSTR format )                   const { X_CALL_( Bstr          , Date_as_string         , ( date, format, &result._bstr ) ); }
    
    SAFEARRAY*              get_array               ( BSTR stmt, SAFEARRAY* param_array )            const { X_CALL_( SAFEARRAY*    , Get_array              , ( stmt, param_array, &result ) ); }
    
    void                    sql_quoted              ( VARIANT value, BSTR* result )                  const { X_CALL (                 Sql_quoted             , ( value, result ) ); }
    Bstr                    sql_quoted              ( VARIANT value )                                const { X_CALL_( Bstr          , Sql_quoted             , ( value, &result._bstr ) ); }
    
    void                    sql_equal               ( BSTR field_name, VARIANT value, BSTR* result ) const { X_CALL (                 Sql_equal              , ( field_name, value, result ) ); }
    Bstr                    sql_equal               ( BSTR field_name, VARIANT value )               const { X_CALL_( Bstr          , Sql_equal              , ( field_name, value, &result._bstr ) ); }
    
    bool                    file_exists             ( BSTR hostware_filename )                       const { X_CALL_( VARIANT_BOOL  , File_exists            , ( hostware_filename, &result ) ); }
    
    void                    make_path               ( BSTR path )                                    const { X_CALL (                 Make_path              , ( path ) ); }
    
    void                    ghostscript             ( BSTR cmd )                                     const { X_CALL (                 Ghostscript            , ( cmd ) ); }
    
    SAFEARRAY*              read_begin_and_end_of_file( BSTR filename, int begin_bytes, int end_bytes ) const { X_CALL_( SAFEARRAY* , Read_begin_and_end_of_file, ( filename, begin_bytes, end_bytes, &result ) ); }
    
    void                    check_licence           ( BSTR product_name )                            const { X_CALL (                 Check_licence          , ( product_name ) ); }

    void                    use_version             ( BSTR version )                                 const { X_CALL (                 Use_version            , ( version ) ); }
    void                    use_version             ( const string& version )                        const { use_version( Bstr( version ) ); }
    void                    use_version             ( const char* version )                          const { use_version( Bstr( version ) ); }

    void                    need_version            ( BSTR version )                                 const { X_CALL (                 Need_version           , ( version ) ); }
    void                    need_version            ( const string& version )                        const { need_version( Bstr(version) ); }
    void                    need_version            ( const char*   version )                        const { need_version( Bstr(version) ); }

    bool                    is_version_or_later     ( BSTR version )                                 const { X_CALL_( VARIANT_BOOL  , Is_version_or_later    , ( version, &result ) ); }
    bool                    is_version_or_later     ( const string& version )                        const { return is_version_or_later( Bstr(version) ); }
    bool                    is_version_or_later     ( const char* version )                          const { return is_version_or_later( Bstr(version) ); }

    void                    use_log                 ()                                               const;

    void                    hex_md5_from_bytes      ( BSTR byte_string, BSTR* result )               const { X_CALL (                 Hex_md5_from_bytes     , ( byte_string, result ) ); }
    string                  hex_md5_from_bytes      ( const string& byte_string )                    const { Bstr result;  hex_md5_from_bytes( Bstr( byte_string ), &result._bstr );  return string_from_bstr( result ); }

#ifdef JNI_TRUE
  //void                set_java_vm                 ( JavaVM* java_vm )                              const { X_CALL (          putref_Java_vm                , ( java_vm ) ); }
#endif
    void                set_com_context             ( z::com::Com_context const* )                   const;
    void                set_com_context             ()                                               const;

#ifdef SYSTEM_HAS_IDISPATCH
  //void                    create_object           ( BSTR class_name, BSTR language, IDispatch** result ) { X_CALL (                        create_object, ( class_name, language, result ) ); }
  //void                    parse_mt940             ( BSTR text, BSTR options, IDispatch** xml_result )    { X_CALL (                        parse_mt940, ( text, options, xml_result ) ); }
#endif

  private:
    void*                   operator ->             ();
};

#undef CLASSNAME
//----------------------------------------------------------------------------------------Variables
#define CLASSNAME "Variables"

struct Variables : zschimmer::ptr<Ivariables2>
{
    typedef zschimmer::com::Bstr    Bstr;
    typedef zschimmer::com::Variant Variant;


                            Variables               ()                                          {}
                            Variables               ( Ivariables2* o )                          : zschimmer::ptr<Ivariables2>(o) {}

    Variables&              operator =              ( Ivariables2* o )                          { zschimmer::ptr<Ivariables2>::operator = ( o );  return *this; }

    void                    create_instance         ()                                          { zschimmer::ptr<Ivariables2>::create_instance( CLSID_Variables2 ); }

    Variables               clone                   ()                                          const { zschimmer::ptr<Ivariables2> result; 
                                                                                                        X_CALL (                        Obj_clone                 , ( result.pp() ) ); 
                                                                                                        return +result; }

    void                    set                     ( BSTR name         , const VARIANT& value )const { X_CALL (                        put_Obj_value             , ( name, const_cast<VARIANT*>( &value ) ) ); }
    void                    set                     ( const string& name, const VARIANT& value )const { set( Bstr(name), value ); }
    void                    set                     ( const char* name  , const VARIANT& value )const { set( Bstr(name), value ); }

    void                    get                     ( BSTR name, VARIANT* result )              const { X_CALL (                        get_Obj_value             , ( name, result ) ); }
    void                    get                     ( const string& name, VARIANT* result )     const { get( Bstr( name ), result ); }
    Variant                 get                     ( const string& name )                      const { Variant result; get( name, &result ); return result;  }

    void                set_xml                     ( BSTR xml )                                const { X_CALL (                        put_Obj_xml               , ( xml ) ); }
    void                set_xml                     ( const string& xml )                       const { set_xml( Bstr( xml ) ); }

    void                get_xml                     ( BSTR* result )                            const { X_CALL (                        get_Obj_xml               , ( result ) ); }
    string                  xml                     ()                                          const { Bstr result; get_xml( &result ); return string_from_bstr(result); }



  private:
    void*                   operator ->             ();
};

#undef CLASSNAME
//--------------------------------------------------------------------------------Factory_processor
#define CLASSNAME "Factory_processor"

struct Factory_processor : zschimmer::ptr<Ifactory_processor>
{
    typedef zschimmer::com::Bstr    Bstr;
    typedef zschimmer::com::Variant Variant;


                            Factory_processor       ()                                                {}
                            Factory_processor       ( Ifactory_processor* o )                         : zschimmer::ptr<Ifactory_processor>(o) {}
                           ~Factory_processor       ()                                                { if( _ptr )  _ptr->Close(); }

    Factory_processor&      operator =              ( Ifactory_processor* o )                         { zschimmer::ptr<Ifactory_processor>::operator = ( o );  return *this; }

    void                    create_instance         ()                                                { zschimmer::ptr<Ifactory_processor>::create_instance( CLSID_Factory_processor ); }

    void                set_param                   ( const string& param )                     const { X_CALL (                        put_Param                 , ( Bstr(param) ) ); }
    void                set_param                   ( const string& name, const string& value ) const { set_param( name + "=" + zschimmer::quoted_string( value ) ); }

    void                set_language                ( BSTR language )                           const { X_CALL (                        put_Language              , ( language ) ); }
    void                set_language                ( const string& language )                  const { set_language( Bstr(language) ); }

    string                  language                ()                                          const { Bstr result;
                                                                                                        X_CALL (                        get_Language              , ( &result ) );  
                                                                                                        return result.as_string(); }

    void                set_head_filename           ( BSTR filename )                           const { X_CALL (                        put_Head_filename         , ( filename ) ); }
    void                set_head_filename           ( const string& filename )                  const { set_head_filename( Bstr(filename) ); }

    void                set_template_filename       ( BSTR filename )                           const { X_CALL (                        put_Template_filename     , ( filename ) ); }
    void                set_template_filename       ( const string& filename )                  const { set_template_filename( Bstr(filename) ); }

    void                set_document_filename       ( BSTR filename )                           const { X_CALL (                        put_Document_filename     , ( filename ) ); }
    void                set_document_filename       ( const string& filename )                  const { set_document_filename( Bstr(filename) ); }

    void                set_parameters              ( Ivariables* parameters )                  const { X_CALL (                        putref_Parameters         , ( parameters ) ); }
    void                set_parameters              ( Ivariables2* parameters )                 const;

    void                set_parameter               ( BSTR name, const VARIANT& value )         const { X_CALL (                        put_Parameter             , ( name , const_cast<VARIANT*>( &value ) ) ); }
    void                set_parameter               ( const string& name, const Variant& value )const { set_parameter( Bstr(name), value ); }
    void                set_parameter               ( const string& name, const string& value ) const { set_parameter( Bstr(name), Variant(value) ); }
    void                set_parameter               ( const char*   name, const string& value ) const { set_parameter( Bstr(name), Variant(value) ); }

    void                set_collect                 ( int collect )                             const { X_CALL (                        put_Collect               , ( collect ) ); }
    int                     collect                 ()                                          const { X_CALL_( int                  , get_Collect               , ( &result ) ); }

    void                    init_engine             ()                                          const { X_CALL (                        Init_engine               , () ); }

    bool                    name_exists             ( BSTR name )                               const { VARIANT_BOOL result;
                                                                                                        X_CALL (                        Name_exists               , ( name, &result ) );
                                                                                                        return result != 0; }

    bool                    name_exists             ( const string& name )                      const { return name_exists( Bstr(name) ); }

    void                    preprocess              ( BSTR script, BSTR* result )               const { X_CALL (                        Preprocess                , ( script, result ) ); }
    string                  preprocess              ( const string& script )                    const { Bstr result;  preprocess( Bstr(script), &result );  return string_from_bstr(result); }

    void                    add_obj                 ( IDispatch* obj, BSTR name, Script_item_flags flags = sif_isvisible )
                                                                                                const { X_CALL (                        Add_obj                   , ( obj, name, flags ) ); }
    void                    add_obj                 ( IDispatch* obj, const string& name, Script_item_flags flags = sif_isvisible )
                                                                                                const { add_obj( obj, Bstr(name), flags ); }

    void                    parse                   ( BSTR text, Scripttext_flags flags, VARIANT* result )
                                                                                                const { X_CALL (                        Parse                     , ( text, flags, result ) ); }

    Variant                 parse                   ( const string& text, Scripttext_flags flags = scripttext_isvisible )
                                                                                                const { Variant result;  parse( Bstr(text), flags, &result );  return result; }

    void                    add_parameters          ()                                          const { X_CALL (                        Add_parameters            , () ); }
    void                    process                 ()                                          const { X_CALL (                        Process                   , () ); }
    void                    close_output_file       ()                                          const { X_CALL (                        Close_output_file         , () ); }
    void                    close                   ()                                          const { X_CALL (                        Close                     , () ); }

    // Ab Hostware 1.6.110:
    void                set_merge_documents         ( bool b )                                  const { X_CALL (                        put_Merge_documents       , ( b ) ); }

  private:
    void*                   operator ->             ();
};

#undef CLASSNAME
//--------------------------------------------------------------------------------Script_object
#define CLASSNAME "Script_object"

struct Script_object : zschimmer::ptr<Iscript_object>
{
    typedef zschimmer::com::Bstr    Bstr;
    typedef zschimmer::com::Variant Variant;


                            Script_object           ()                                                {}
                            Script_object           ( Iscript_object* o )                             : zschimmer::ptr<Iscript_object>(o) {}

    Script_object&          operator =              ( Iscript_object* o )                             { zschimmer::ptr<Iscript_object>::operator = ( o );  return *this; }

    void                    create_instance         ()                                                { zschimmer::ptr<Iscript_object>::create_instance( CLSID_Script_object ); }

    void                    close                   ()                                          const { X_CALL (                        Obj_close                 , () ); }

    void                set_language                ( BSTR language )                           const { X_CALL (                        put_Obj_language          , ( language ) ); }
    void                set_language                ( const string& language )                  const { set_language( Bstr(language) ); }

  //string                  language                ()                                          const { Bstr result;
  //                                                                                                    X_CALL (                        get_Language              , ( &result ) );  
  //                                                                                                    return result.as_string(); }

    void                    add_object              ( IDispatch* obj, BSTR name, Script_item_flags flags = sif_isvisible )
                                                                                                const { X_CALL (                        Obj_add_object            , ( obj, name, flags ) ); }
    void                    add_object              ( IDispatch* obj, const string& name, Script_item_flags flags = sif_isvisible )
                                                                                                const { add_object( obj, Bstr(name), flags ); }

    void                    parse                   ( BSTR text, Scripttext_flags flags, VARIANT* result )
                                                                                                const { X_CALL (                        Obj_parse                 , ( text, flags, result ) ); }

    Variant                 parse                   ( const string& text, Scripttext_flags flags = scripttext_isvisible )
                                                                                                const { Variant result;  parse( Bstr(text), flags, &result );  return result; }

    void                    parse_only              ( BSTR text, Scripttext_flags flags = scripttext_isvisible )
                                                                                                const { X_CALL (                        Obj_parse_only            , ( text, flags ) ); }

    void                    parse_only              ( const string& text, Scripttext_flags flags = scripttext_isvisible ) const  { parse_only( Bstr(text), flags ); }

    bool                    name_exists             ( BSTR name )                               const { VARIANT_BOOL result;   X_CALL ( Obj_name_exists           , ( name, &result ) );  return result != 0; }
    bool                    name_exists             ( const string& name )                      const { return name_exists( Bstr(name) ); }

    void                    add_variables           ( Ivariables2* variables )                  const { X_CALL (                        Obj_add_variables         , ( variables ) ); }

  private:
    void*                   operator ->             ();
};

#undef CLASSNAME
//-------------------------------------------------------------------------------------------------

} //namespace hostware
} //namespace sos

//-------------------------------------------------------------------------------------------------

#undef X_CALL_
#undef X_CALL

#ifdef _WIN32
#   pragma warning( pop )
#   pragma auto_inline( on )
#endif

#endif
