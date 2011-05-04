// $Id$

// §1735


package sos.hostware;

import sos.hostware.Scripttext_flags;
import java.math.BigDecimal;


public class Factory_processor extends Idispatch
{
    public                          Factory_processor       ()                                              throws Exception  { super( "hostware.Factory_processor" ); }


    /** Immer rufen! */
    
    public void close()  throws Exception
    {
        try
        {
            close_native();
            _document_written = document_written();
            _real_document_filename = real_document_filename();
        }
        finally
        {
            com_close();
        }
    }


    public         void             close_output_file       ()                                              throws Exception  { com_call( "close_output_file" ); }

    public         String           eval                    ( String script_text, Scripttext_flags flags )  throws Exception  { return eval( script_text, flags.value ); }
    public         String           eval                    ( String script_text )                          throws Exception  { return eval( script_text, Scripttext_flags.none ); }

    public         void             parse                   ( String script, Scripttext_flags flags )       throws Exception  { parse( script, flags.value ); }
    public         void             parse                   ( String script )                               throws Exception  { parse( script, Scripttext_flags.isvisible ); }

    public  native void             process                 ()                                              throws Exception;
    
    public  native void             add_parameters          ()                                              throws Exception;


/* So könnte es umgestellt werden: (Mit java_Factory_processor.cxx vergleichen! close() ruft noch Release()!
    public                          Factory_processor       ()                                              throws Exception  { super( "hostware.Factory_processor" ); }

    public  native void             close                   ()                                              throws Exception  { com_call( "close" ); }

    public         String           eval                    ( String script_text, Scripttext_flags flags )  throws Exception  { return string_com_call( "eval", script_text, flags.value ); }
    public         String           eval                    ( String script_text )                          throws Exception  { return eval( script_text, Scripttext_flags.none ); }

    public         void             parse                   ( String script, Scripttext_flags flags )       throws Exception  { com_call( "parse", script, flags.value ); }
    public         void             parse                   ( String script )                               throws Exception  { parse( script, Scripttext_flags.isvisible ); }

    public  native void             process                 ()                                              throws Exception  { com_call( "process" ); }
    
    public  native void             add_parameters          ()                                              throws Exception  { com_call( "add_parameters" ); }
*/

    // EIGENSCHAFTEN

  //public  native int              collect                 ()                                              throws Exception;
  //public  native void         set_collect                 ( int count )                                   throws Exception;

  //public  native int              collected_documents_count()                                             throws Exception;

  //public  native boolean          document_copied         ()                                              throws Exception;

  //public  native String           document_dir            ()                                              throws Exception  { return string_com_call( "<document_dir" ); }
  //public  native void         set_document_dir            ( String path )                                 throws Exception  { com_call( ">document_dir", path ); }

    public  native String           document_filename       ()                                              throws Exception;
    public  native void         set_document_filename       ( String document_filename )                    throws Exception;

  //public  native boolean          document_head_modified  ()                                              throws Exception;

    public  native String           language                ()                                              throws Exception;
    public  native void         set_language                ( String language )                             throws Exception;

  //public  native String           last_step               ()                                              throws Exception;

    public  native String           head_filename           ()                                              throws Exception;
    public  native void         set_head_filename           ( String head_filename )                        throws Exception;

    public  native void         set_param                   ( String options )                              throws Exception;

  //public  native Variable         parameter               ( String name )                                 throws Exception;
    public  native String           parameter_as_string     ( String name )                                 throws Exception;
    public  native void         set_parameter               ( String name, String value )                   throws Exception;
    public         void         set_parameter               ( String name, boolean value )                  throws Exception  { set_parameter_bool  ( name, value ); }
    public         void         set_parameter               ( String name, int value )                      throws Exception  { set_parameter_int   ( name, value ); }
  //public         void         set_parameter               ( String name, long value )                     throws Exception  { set_parameter_long  ( name, value ); }
    public         void         set_parameter               ( String name, double value )                   throws Exception  { set_parameter_double( name, value ); }
    
    
    public         void         set_parameter               ( String name, BigDecimal value )               throws Exception
    { 
        BigDecimal cy = value.multiply( BigDecimal.valueOf( 10000 ) );

        cy.setScale(0); 

        if( value.compareTo( BigDecimal.valueOf( Long.MAX_VALUE ) ) > 0
         || value.compareTo( BigDecimal.valueOf( Long.MIN_VALUE ) ) < 0 )  throw new Exception( "Betrag zu groß für CURRENCY" );

        set_parameter_currency( name, cy.longValue() ); 
    }

    public  native void         set_parameter_bool          ( String name, boolean value )                  throws Exception;
    public  native void         set_parameter_int           ( String name, int value )                      throws Exception;
  //public  native void         set_parameter_long          ( String name, long value )                     throws Exception;
    public  native void         set_parameter_double        ( String name, double value )                   throws Exception;
    public  native void         set_parameter_currency      ( String name, long value )                     throws Exception;
    
  //public         void         set_parameter_object        ( String name, Object o )                       throws Exception    { com_call( ">parameter", new Object[]{ name, o } ); }

  //public  native Variables        variables               ()                                              throws Exception;
  //public  native void         set_variables               ( Variables v )                                 throws Exception;


    public String                   last_step               ()                                              throws Exception    { return  string_com_call( "<last_step" ); }
  //public boolean                  name_exists             ( String name )                                 throws Exception    { return boolean_com_call( "name_exists", name ); }

    public String                   template_dir            ()                                              throws Exception    { return  string_com_call( "<template_dir" ); }
    public void                 set_template_dir            ( String path )                                 throws Exception    {                com_call( ">template_dir", path ); }
    public String                   document_dir            ()                                              throws Exception    { return  string_com_call( "<document_dir" ); }
    public void                 set_document_dir            ( String path )                                 throws Exception    {                com_call( ">document_dir", path ); }
    public int                      collected_documents_count()                                             throws Exception    { return     int_com_call( "<collected_documents_count" ); }
    public boolean                  document_copied         ()                                              throws Exception    { return boolean_com_call( "<document_copied" ); }
    public boolean                  document_head_modified  ()                                              throws Exception    { return boolean_com_call( "<document_head_modified" ); }
    public void                 set_collect                 ( int c )                                       throws Exception    {                com_call( ">collect", c ); }
    public int                      collect                 ()                                              throws Exception    { return     int_com_call( "<collect" ); }
  
    public  native String           template_filename       ()                                              throws Exception;
    public  native void         set_template_filename       ( String template_filename )                    throws Exception;

    public  native String           script_text             ()                                              throws Exception;
    public  native String           error_filename          ()                                              throws Exception;
    public  native String           error_document          ()                                              throws Exception;

    public  boolean                 document_written        ()                                              throws Exception    { return com_valid()? boolean_com_call( "<document_written" ) 
                                                                                                                                                    : _document_written; }
                                                                                                                                                    
    public String                   real_document_filename  ()                                              throws Exception    { return com_valid()? string_com_call( "<real_document_filename" )
                                                                                                                                                    : _real_document_filename; }
                                                                                                                                                    
    public void                 set_db_name                 ( String db_name )                              throws Exception    {                com_call( ">db_name", db_name ); }
    public String                   db_name                 ()                                              throws Exception    { return  string_com_call( "<db_name" ); }

    public void                 set_merge_documents         ( boolean b )                                   throws Exception    {                com_call( ">merge_documents", b ); }


    /** Dasselbe wie add_obj( o, name, Script_item_flags.isvisible ).
     * @see #add_obj(Object,String,Script_item_flags)
     */    
    
    public void                     add_obj                 ( Object o, String name )                       throws Exception    { add_obj( o, name, Script_item_flags.isvisible ); }
    
    
    
    /** Objekt an Skript übergeben (zurzeit nur für Spidermonkey).
     *
     */
    public void                     add_obj                 ( Object o, String name, Script_item_flags flags ) throws Exception { com_call( "add_obj", new Object[]{ o, name, new Integer( flags.value ) } ); }
    


    // PROTECTED UND PRIVATE

  //protected void                  finalize                ()                                              throws Throwable  {}    // Erstmal abgeklemmt.
  //private native void             construct               ()                                              throws Exception;
    public void                     destruct                ()                                              { com_close(); }


    public  native void             close_native            ()                                              throws Exception;
    private native String           eval                    ( String script_text, int flags )               throws Exception;
    private native void             parse                   ( String script, int flags )                    throws Exception;


    private boolean                _document_written        = false;    // Damit nach close() noch document_written() aufgerufen werden kann.
    private String                 _real_document_filename  = "";

    static
    {
        Idispatch.load_module();
    }
}
