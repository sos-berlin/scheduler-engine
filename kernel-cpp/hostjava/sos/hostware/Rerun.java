// $Id: Rerun.java 11247 2005-01-11 23:22:22Z jz $

package sos.hostware;

/**
 * @author Joacim Zschimmer
 */

public class Rerun  extends Idispatch
{
  //private                     Rerun                       ( long idispatch )     throws Exception { super( idispatch ); }

    public                      Rerun                       ()                     throws Exception { super( "hostware.Rerun" ); }
        
    public                      Rerun                       ( String filename )    throws Exception         
    { 
        super( "hostware.Rerun" );  
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
    
    public                      Rerun                       ( java.io.File file )  throws Exception { this( file.toString() ); }
    
    public void                 open                        ( File filename )      throws Exception {                   com_call( "Open"                    , filename.toString() ); }
    public void                 open                        ( String filename )    throws Exception {                   com_call( "Open"                    , filename      ); }
    public void                 close                       ()                     throws Exception {                  com_close( "Close"                                   ); }
    public boolean              process_next_record         ()                     throws Exception { return    boolean_com_call( "Process_next_record"                     ); }
  //public void                 processing_was_ok           ( boolean ok )         throws Exception {                   com_call( "Processing_was_ok"       , ok            ); }
  //public void                 commit                      ()                     throws Exception {                   com_call( "Commit"                                  ); }
    public void                 set_record_ok               ( int record_number, boolean ok ) throws Exception {        com_call( "Set_record_ok"           , new Integer(record_number), new Boolean(ok) ); }
    public boolean              rerunning                   ()                                      { return    boolean_com_call( "<Rerunning"                              ); }
    public boolean              ok                          ()                                      { return    boolean_com_call( "<Ok"                                     ); }
    public int                  record_number               ()                                      { return        int_com_call( "<Record_number"                          ); }
};

