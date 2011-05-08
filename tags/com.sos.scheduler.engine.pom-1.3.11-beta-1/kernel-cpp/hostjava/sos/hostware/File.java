package sos.hostware;

/**
 * Java-Klasse für COM-Klasse hostware.File.
 *
 *
 *
 * @author Joacim Zschimmer
 */

public class File  extends Idispatch
{
    public                      File                        ()                          throws Exception    { super( "hostware.File" ); }

    /** Öffnet die Datei mit {@link #open(String)} */    
    public                      File                        ( String filename )         throws Exception            
    { 
        super( "hostware.File" );  
        boolean ok = false; 
        
        try 
        { 
            open( filename ); 
            ok = true; 
        } 
        finally 
        { 
            if( !ok )  com_close(); 
        } 
    }
    
    /** Öffnet die Datei mit {@link #open(File)} */    
    public                      File                        ( java.io.File filename )   throws Exception    { this( filename.toString() ); }
    
    /** Diese Methode oder {@link #close()} sollte finally gerufen werden, um die C++-Ressource freizugeben. */
    public void                 destruct                    ()                                              {                   com_close(); }
    
    public void                 open                        ( String filename )         throws Exception    {                   com_call( "open"            , filename      ); }

    public void                 open                        ( java.io.File filename )   throws Exception    {                   com_call( "open"            , filename.toString() ); }

    /** Schließt die evtl. offene Datei und gibt die C++-Ressourcen frei. */
    public void                 close                       ()                          throws Exception    { if( com_valid() )  try {             com_call( "close" ); }  finally { com_close(); } }
    
    /** Liefert eine Zeile. Bei einer Textdatei sind CR und LF am Ende entfernt. */
    public String               get_line                    ()                          throws Exception    { return    (String)com_call( "get_line"                        ); }
    
    /** Schreibt eine Zeile. Bei einer Textdatei werden CR (nur Windows) und LF angehängt. */
    public void                 put_line                    ( String line )             throws Exception    {                   com_call( "put_line"        , line          ); }

  //public Record               create_record               ()                          throws Exception    { return    (Record)com_call( "create_record"                   ); }
  //public Record               create_key                  ()                          throws Exception    { return    (Record)com_call( "create_key"                      ); }
  
    public Record               get                         ()                          throws Exception    { return    (Record)com_call( "get"                             ); }
  //public Record               get                         ()                          throws Exception    { return    (Record)com_call( 8, dispatch_method                ); }
  //public void                 put                         ( Record record )           throws Exception    {                   com_call( "put"             , record        ); }
    public void                 put                         ( String line )             throws Exception    { put_line( line ); }
  //public void                 set_key                     ( Record key )              throws Exception    {                   com_call( "set_key"         , key           ); }
  //public void                 delete_key                  ( Record key )              throws Exception    {                   com_call( "delete_key"      , key           ); }
  //public Record               get_key                     ( Record key )              throws Exception    { return    (Record)com_call( "get_key"         , key           ); }
  //public void                 insert                      ( Record record )           throws Exception    {                   com_call( "insert"          , record        ); }
  //public void                 update                      ( Record record )           throws Exception    {                   com_call( "update"          , record        ); }
  //public void                 update_direct               ( Record record )           throws Exception    {                   com_call( "update_direct"   , record        ); }
  //public void                 store                       ( Record record )           throws Exception    {                   com_call( "store"           , record        ); }
  
    /** True, wenn nächstes {@link #get_line()} oder {@link #get()} eine EOF-Exception liefern würden */
    public boolean              eof                         ()                          throws Exception    { return    boolean_com_call( "eof"                             ); }
    
  //public void             set_date_format                 ( String format )           throws Exception    {                   com_call( ">date_format"    , format        ); }
  //public void             set_decimal_symbol              ( String decimal_symbol );
  
    /** Liefert den Spaltennamen des angegebenen Spalte. Die Nummerierung begint mit 0. */
    public String               field_name                  ( int nr )                  throws Exception    { return    (String)com_call( "field_name"      , nr            ); }
    
    /** Liefert die Anzahl der Spalten */
    public int                  field_count                 ()                          throws Exception    { return        int_com_call( "<field_count"                    ); }
    
  //public String               prepare                     ( String filename )         throws Exception    {                   com_call( "prepare"         , filename      ); }
  //public void             set_parameter                   ( int nr, Variant value )   throws Exception    {                   com_call( ">parameter"      , nr, value     ); }
  //public void                 execute                     ()                          throws Exception    {                   com_call( "execute"                         ); }
  //public void                 set_parameters              ( Variant[] param_array )   
  //public Type                 type                        ();
  
    /** True, wenn Datei geöffnet ist. */
    public boolean              opened                      ()                          throws Exception    { return    boolean_com_call( "<opened"                         ); }
    
    /** Nächstes {@link #get_line()} oder {@link #get()} lesen den ersten Satz der Datei */
    public void                 rewind                      ()                          throws Exception    {                   com_call( "rewind"                          ); }
    
  //public void             set_date_time_format            ( String format );
  //public String               debug_info                  ();
  //public void             set_write_empty_as_null         ( boolean b );
  //public void                 close_cursor                ();
  
    /** Voreinstellung für {@link sos.hostware.Record#read_null_as_empty()} für mit {@link #get()} gelesene Records. */
    public void                 read_null_as_empty          ()                          throws Exception    {                   com_call( ">read_null_as_empty", true       ); }

    static
    {
        Idispatch.load_module();
    }
};
