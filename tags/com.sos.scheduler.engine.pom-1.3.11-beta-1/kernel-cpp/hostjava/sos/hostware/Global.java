package sos.hostware;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2005
 * Organisation: SOS
 * @author Joacim Zschimmer, http://zschimmer.com
 * @version $Revision$
 */


import java.util.Date;


public class Global  extends Idispatch
{
    public                      Global                      ()                                  throws Exception    { super( "hostware.Global" ); }
    
    public Object               system_information          ( String what )                     throws Exception    { return com_call( "<system_information", what            ); }
    public Object               system_information          ( String what, String parameter )   throws Exception    { return com_call( "<system_information", what, parameter ); }


    public long long_system_information( String what )
    throws 
        Exception    
    { 
        return long_system_information( what, "" );
    }

    
    public long long_system_information( String what, String parameter )
    throws 
        Exception    
    { 
        Object result = com_call( "<system_information", what, parameter );
        
        return ((Number)result).longValue();
    }

    
    public void check_licence( String component_name )
    throws 
        Exception    
    { 
        com_call( "check_licence", component_name );
    }

    
/*
    public native void          copy_file                   ( String source_file, String dest_file );
    public native void          shell_execute_and_wait      ( String file, String verb, double seconds );   // seconds < 0: Keine Zeitbeschränkung

  //    [vararg,helpstring( "Den ersten Datensatz lesen. SQL-Parameter können angegeben werden." )]
  //public native void          get_single( String filename, [in] SAFEARRAY(VARIANT) param_array, [retval,out] Ihostware_dynobj** record );

  //    [vararg,helpstring( "Anweisung mit Parametern ausführen" )]
  //public native void          execute_direct( String statement, [in] SAFEARRAY(VARIANT) param_array );

  //public native void          null                        ( [in] VARIANT* o, [retval,out] VARIANT_BOOL* result );
  //public native void          empty                       ( [in] VARIANT* o, [retval,out] VARIANT_BOOL* result );
    public native void          sleep                       ( double seconds );
    public native void          remove_file                 ( String filename );
    public native void          rename_file                 ( String old_filename, String new_filename );
    public native String        file_as_string              ( String filename );
    public native String        as_parser_string            ( String o, String quote );  // Baut einen von Hostware_parser erkennbaren String zusammen
    public native String        as_xml                      ( String text, String options );
    public native String        as_xml                      ( Record record, String options );
    public native Date          as_date                     ( String date, String format );
    public        Date          as_date                     ( String date )                         { return as_date(date,""); }
    public native String        date_as_string              ( Date date, String format );
    public        String        date_as_string              ( Date date )                           { return date_as_string(date,""); }

  //    [vararg,helpstring( "Alle Datensätze als Array lesen. SQL-Parameter können angegeben werden." )]
  //public native void          get_array( String filename, [in] SAFEARRAY(VARIANT) param_array, [retval,out] SAFEARRAY(VARIANT)* result );

    public native String        from_xml                    ( String xml_string, String options );
  //public native void          create_object               ( String script_text_or_class_name, [in,defaultvalue("")] BSTR language, [out,retval] IDispatch** );
    public native String        sql_quoted                  ( Variant value );
    public native String        sql_equal                   ( String field_name, Variant value );
    public native boolean       file_exists                 ( String hostware_filename );
    public native void          make_path                   ( String path );
    public native void          ghostscript                 ( String parameters );
    public native String[]      read_begin_and_end_of_file  ( String filename, int begin_bytes, int end_bytes );
  //public native Dom_document  parse_mt940                 ( String source_text );
*/  
};


