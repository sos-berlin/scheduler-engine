// $Id$

package sos;


//---------------------------------------------------------------------------------Scripttext_flags

enum Scripttext_flags
{
    scripttext_none         = 0,
    scripttext_isvisible    = 0x00000002,
    scripttext_isexpression = 0x00000020,
    scripttext_ispersistent = 0x00000040
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

//------------------------------------------------------------------------------------------Variant

public class Variant
{
};

//-------------------------------------------------------------------------------------------Record
/*

public class Record  // Dynobj
{
    public native void          obj_add_field               ( String name, Type type );
    public native void          obj_add_field               ( String name, String type );
    public native int           obj_field_count             ();
    public native String        obj_field_name              ( int field_nr );
    public native void          obj_set_write_empty_as_null ( bool b );
    public native int           obj_field_index             ( String name );
    public native Record        clone                       ();
    public native Variant       obj_field                   ( int    nr );
    public native Variant       obj_field                   ( String name );
    public native void          obj_set_field               ( int    nr  , Variant value );
    public native void          obj_set_field               ( String name, Variant value );
    public native String        obj_xml                     ( String options = "" );
    public native void          obj_set_xml                 ( String options, String xml );
    public native Type          obj_type                    ();
};

*/
//----------------------------------------------------------------------------------------Type_info
/*
public class Type_info
{
    public native String        name                        ();
};
*/
//---------------------------------------------------------------------------------------Type_param
/*

public class Type_param
{
        [propget] HRESULT std_type      ( [out,retval] Hostware_std_type* o );
        [propget] HRESULT size          ( [out,retval] long* o );
        [propget] HRESULT display_size  ( [out,retval] long* o );
        [propget] HRESULT precision     ( [out,retval] long* o );
        [propget] HRESULT radix         ( [out,retval] long* o );
        [propget] HRESULT scale         ( [out,retval] VARIANT* o );     // long oder NULL
        [propget] HRESULT usigned       ( [out,retval] VARIANT_BOOL* o );
        [propget] HRESULT info          ( [out,retval] Ihostware_type_info** o );
};

*/
//---------------------------------------------------------------------------------------Field_type
/*
public class Field_type
{
        [propget] HRESULT field_size    ( [out,retval] long* o );
        [propget] HRESULT info          ( [out,retval] Ihostware_type_info** o );
        [propget] HRESULT param         ( [out,retval] Ihostware_type_param** o );
};
*/
//--------------------------------------------------------------------------------------Record_type
/*
public class Record_Type
{
    public native void          name( [out,retval] BSTR* name );
    public native void          field_count( [out,retval] long* number );
    public native void          field_descr( [in] long i, [out,retval] Ihostware_field_descr** o );
    public native void          add_field_descr( [in] Ihostware_field_descr* o );
    public native void          add_field( [in] BSTR name, [in] BSTR type );
    public native void          param( [out,retval] Ihostware_type_param** o );
};
*/
//--------------------------------------------------------------------------------------Field_descr
/*
public class Field_descr
{
    public native void          name( [out,retval] BSTR* name );
    public native void          type( [out,retval] Ihostware_field_type** type );
    public native void          offset( [out,retval] long* o );
    public native void          remark( [out,retval] BSTR* o );
};
*/
//---------------------------------------------------------------------------------------------File
/*
public class File    
{
    public native void          open( [in] BSTR filename );
    public native void          close( void );
    public native void          get_line( [out,retval] BSTR* buffer );
    public native void          put_line( [in] BSTR record );
    public native void          create_record( [out,retval] Ihostware_dynobj** record );
    public native void          create_key( [out,retval] Ihostware_dynobj** key );
    public native void          get( [out,retval] Ihostware_dynobj** object );
    public native void          put( [in] VARIANT* object );
    public native void          set_key( [in] VARIANT* key );
    public native void          delete_key( [in] VARIANT* key );
    public native void          get_key( [in] VARIANT* key, [out,retval] Ihostware_dynobj** object );
    public native void          insert( [in] VARIANT* record );
    public native void          update( [in] VARIANT* record );
    public native void          update_direct( [in] VARIANT* record );
    public native void          store( [in] VARIANT* record );
    public native void          eof( [out,retval] VARIANT_BOOL* eof );
    public native void          set_date_format( [in] BSTR format );
    public native void          set_decimal_symbol( [in] BSTR decimal_symbol );
    public native void          field_name( [in] long number, [retval,out] BSTR* name );
    public native void          field_count( [retval,out] long* number );
    public native void          prepare( [in] BSTR filename );
    public native void          set_parameter( [in] long no, [in] VARIANT* value );
    public native void          execute();
    public native void          set_parameters( [in] SAFEARRAY(VARIANT) param_array );
    public native void          type( [out,retval] Ihostware_record_type** type );
    public native void          opened( [out,retval] VARIANT_BOOL* result );
    public native void          rewind();
    public native void          set_date_time_format( [in] BSTR format );
    public native void          debug_info( [out,retval] BSTR* text );
    public native void          set_write_empty_as_null( [in] VARIANT_BOOL b );
    public native void          close_cursor( void );
};
*/
//------------------------------------------------------------------------------------Script_object
/*
public class Script_object
{
    public native void          obj_close();
    public native void          obj_set_language( [in] BSTR scripting_engine_name );
    public native void          obj_language( [out,retval] BSTR* scripting_engine_name );
    public native void          obj_parse( [in] BSTR script_text, [in,defaultvalue(scripttext_isvisible)] enum Scripttext_flags, [out,retval] VARIANT* result );
    public native void          obj_eval( [in] BSTR script_text, [in,defaultvalue(0)] enum Scripttext_flags, [out,retval] VARIANT* result );
    public native void          obj_name_exists( [in] BSTR sub_name, [out,retval] VARIANT_BOOL* );
};
*/
//-----------------------------------------------------------------------------------------Hostware
/*
public class Hostware
{
    public native void          copy_file( [in] BSTR source_file, [in] BSTR dest_file );
    public native void          shell_execute_and_wait( [in] BSTR file, [in] BSTR verb, [in] double seconds );   // seconds < 0: Keine Zeitbeschränkung

        [vararg,helpstring( "Den ersten Datensatz lesen. SQL-Parameter können angegeben werden." )] 
    public native void          get_single( [in] BSTR filename, [in] SAFEARRAY(VARIANT) param_array, [retval,out] Ihostware_dynobj** record );

        [vararg,helpstring( "Anweisung mit Parametern ausführen" )]
    public native void          execute_direct( [in] BSTR statement, [in] SAFEARRAY(VARIANT) param_array );

    public native void          null( [in] VARIANT* o, [retval,out] VARIANT_BOOL* result );
    public native void          empty( [in] VARIANT* o, [retval,out] VARIANT_BOOL* result );
    public native void          sleep( [in] double seconds );
    public native void          remove_file( [in] BSTR filename );
    public native void          rename_file( [in] BSTR old_filename, [in] BSTR new_filename );
    public native void          file_as_string( [in] BSTR filename, [out,retval] BSTR* string );
    public native void          as_parser_string( [in] BSTR o, [in] BSTR quote, [out,retval] BSTR* result );  // Baut einen von Hostware_parser erkennbaren String zusammen
    public native void          as_xml( [in] VARIANT* field_or_dynobj, [in,defaultvalue("")] BSTR options, [out,retval] BSTR* result );
    public native void          as_date( [in] BSTR date, [in,defaultvalue("")] BSTR format, [out,retval] DATE* result );
    public native void          date_as_string( [in] DATE date, [in,defaultvalue("")] BSTR format, [out,retval] BSTR* result );

        [vararg,helpstring( "Alle Datensätze als Array lesen. SQL-Parameter können angegeben werden." )] 
    public native void          get_array( [in] BSTR filename, [in] SAFEARRAY(VARIANT) param_array, [retval,out] SAFEARRAY(VARIANT)* result );

    public native void          from_xml( [in] BSTR xml_string, [in,defaultvalue("")] BSTR options, [out,retval] BSTR* result );
    public native void          create_object( [in] BSTR script_text_or_class_name, [in,defaultvalue("")] BSTR language, [out,retval] IDispatch** );
    public native void          sql_quoted( [in] VARIANT value, [out,retval] BSTR* result );
    public native void          sql_equal( [in] BSTR field_name, [in] VARIANT value, [out,retval] BSTR* expr_string );
    public native void          file_exists( [in] BSTR hostware_filename, [out,retval] VARIANT_BOOL* result );
    public native void          make_path( [in] BSTR path );
    public native void          ghostscript( [in] BSTR parameters );
    public native void          read_begin_and_end_of_file( [in] BSTR filename, [in] int begin_bytes, [in] int end_bytes, [out,retval] SAFEARRAY(VARIANT)* result );
    public native void          parse_mt940( [in] BSTR source_text, [out,retval] IDispatch** dom_document );
};
*/
//-----------------------------------------------------------------------------------------Variable
/*
public class Variable
{
    public native void          set_value                   ( [in,optional] VARIANT* index, [in] VARIANT* value );
    public native void          value                       ( [in,optional] VARIANT* index, [out,retval] VARIANT* value );
    public native void          dim                         ( [in] int size );
    public native void          name                        ( [out,retval] BSTR* name );
};
*/
//----------------------------------------------------------------------------------------Variables
/*
public class Variables
{
    public native void          set_var                     ( [in] BSTR name, [in] VARIANT* value );
    public native void          set_value                   ( [in] BSTR name, [in] VARIANT* value );
    public native void          value                       ( [in] BSTR name, [out,retval] Ivariable** result );
    public native void          count                       ( [out,retval] int* value );

    public native void          clone                       ( [out,retval] Ivariables** result );

        [id(DISPID_NEWENUM),propget,restricted]
    public native void          _NewEnum                    ( [out,retval] IUnknown** enumerator );    
};
*/
//----------------------------------------------------------Hostware_variables_enumerator
/*
    interface Ivariables_enumerator : IEnumVARIANT
    { 
    public native void          Next                    ( unsigned long celt, VARIANT* rgvar, unsigned long* pceltFetched );
    public native void          Skip                    ( unsigned long celt );
    public native void          Reset                   ();
    public native void          Clone                   ( Ivariables_enumerator** ppenum );
    }
*/
//--------------------------------------------------------------------------------Factory_processor

public class Factory_processor
{
    public                          Factory_processor       ()                                      { construct(); }

    protected void                  finalize                () throws Throwable                     { destruct();  super.finalize(); }
    private native void             construct               ();
    private native void             destruct                ();

    public native void              close                   ();

    public native void          set_language                ( String language);
    public native String            language                ();
    public native void          set_template_filename       ( String template_filename );
    public native String            template_filename       ();
    public native void          set_head_filename           ( String head_filename );
    public native String            head_filename           ();
    public native void          set_document_filename       ( String document_filename );
    public native String            document_filename       ();

    //HRESULT parameters( [in] Ivariables* variables );
    //HRESULT parameters( [out,retval] Ivariables** variables  );

    public native void          set_parameter               ( String name, String value );
    public native String            parameter               ( String name );

    public native void              process                 ();

    public native Variant           eval                    ( String script_text, Scripttext_flags = scripttext_none );
    public native void              put_collect             ( int count );
    public native int               collect                 ();
    public native bool              document_head_modified  ();
    public native bool              document_copied         ();
    public native void              close_output_file       ();
    public native int               collected_documents_count();
    public native String            template_dir            ();
    public native void          set_template_dir            ( String path );
    public native String            document_dir            ();
    public native void          set_document_dir            ( String path );
    public native String            real_document_filename  ();
    public native String            last_step               ();


    private int                     my_data;

    static
    {
        System.loadLibrary("hostjava");
    }
};

//----------------------------------------------------------------------------------Iseries_factory

public class Series_factory
{
    public native String        order_filename              ();
    public native void      set_order_filename              ( String filename );
    
    public native String        template_script_language    ();
    public native void      set_template_script_language    ( String language );
    
    public native void          start_script                ( String script );
    
    public native String        template_dir                ();
    public native void      set_template_dir                ( String path );

    public native String        template_filename           ();
    public native void      set_template_filename           ( String filename );

    public native String        head_filename               ();
    public native void      set_head_filename               ( String filename );

    public native String        document_dir                ();
    public native void      set_document_dir                ( String path );

    public native String        document_filename           ();
    public native void      set_document_filename           ( String filename );

    public native bool          on_error_continue           ();
    public native void      set_on_error_continue           ( bool cont );

  //public native void      set_processing_object_constructor( [in] IDispatch* constructor_function );

    public native Variant       context                     ();
    public native void      set_context                     ( Variant context );

  //public native void      set_output_object_constructor   ( [in] IDispatch* constructor_function );

    public native void      set_rerun_filename              ( String filename );
    public native String        rerun_filename              ();

    public native void      set_collect                     ( int count );
    public native int           collect                     ();


    public native void          open();

    public native void          close();

    public native bool          process                     ();

    public native void          process_all                 ();

    public native Factory_processor processor               ();

    public native int           record_nr                   ();

    public native bool          rerunning                   ();

    public native bool          eof                         ();
};

//---------------------------------------------------------------------------------Word_application
/*
public class Word_application    
{
    public native void          kill_all_words          ( [in,defaultvalue("")] BSTR empty, [out,retval] int* count );
    public native void          load                    ();
    public native void          app                     ( [out,retval] IDispatch** word_application_interface );
    public native void          print                   ( [in] BSTR filename, [in] BSTR parameters );
}
*/
//--------------------------------------------------------------------------------------Ghostscript
/*    
public class Ghostscript
{
    public native void          run                     ( [in] BSTR parameters );
    public native void          collect_stdout          ( [out,retval] VARIANT_BOOL* collect );
    public native void          set_collect_stdout          ( [in] VARIANT_BOOL collect );
    public native void          stdout                  ( [out,retval] BSTR* stdout_text );
    public native void          init                    ();
    public native void          close                   ();
};
*/
//-------------------------------------------------------------------------------------------------
