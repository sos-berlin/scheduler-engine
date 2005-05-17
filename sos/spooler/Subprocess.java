// $Id$

package sos.spooler;

/** Ein Subprozess ist irgendein Prozess, der mit {@link Task#create_subprocess()} gestartet werden kann.
 * 
 * 
 * @author Joacim Zschimmer
 * @version $Revision$
 */

public class Subprocess  extends Idispatch
{
    private                 Subprocess          ( long idispatch )                  { super(idispatch); }


    public void             close               ()                                  {                com_call( "close" ); }


    public void             start               ( String command_line )             {                com_call( "start", command_line ); }

    
    public void             start               ( String filename_and_arguments[] ) {                com_call( "start", filename_and_arguments ); }


    public int              pid                 ()                                  { return     int_com_call( "<pid" ); }


    public boolean          terminated          ()                                  { return boolean_com_call( "<terminated" ); }


    /** Erst aufrufen, wenn {@link #terminated()} == true. */
    public int              exit_code           ()                                  { return     int_com_call( "<exit_code" ); }


    /** Wenn ignore_error nicht gesetzt ist und der Subprozess mit exit_code != 0 endet, 
      * dann stoppt der Job mit Fehler. 
      */
    public void         set_ignore_error        ( boolean b )                       {                com_call( ">ignore_error", b ); }


    public boolean          ignore_error        ()                                  { return boolean_com_call( "<ignore_error" ); }


    /** Wenn ignore_signal nicht gesetzt ist und der Subprozess mit einem Signal endet (nur Unix),
        dann stoppt der Job mit Fehler.
        */
    public void         set_ignore_signal       ( boolean b )                       {                com_call( ">ignore_signal", b ); }


    public boolean          ignore_signal       ()                                  { return boolean_com_call( "<ignore_signal" ); }


    /** Nach Ablauf der Zeit bricht der Scheduler den Subprozess ab (Unix: mit SIGKILL) */
    public void         set_timeout             ( double seconds )                  {                com_call( ">timeout", seconds ); }


    /** Setzt eine Umgebungsvariable für den Prozess.
      * Vor start() aufzurufen. */
    public void         set_environment         ( String entry_name, String value ) {                com_call( ">environment", entry_name, value ); }
    

    /** Wartet aufs Ende des Subprocesses. 
        @return true, wenn der Subprozess geendet hat */
    public boolean          wait_for_termination( double seconds )                  { return boolean_com_call( "wait_for_termination", new Double( seconds ) ); }


    /** Wartet aufs Ende des Subprocesses. */
    public void             wait_for_termination()                                  {                com_call( "wait_for_termination" ); }


    /** Bricht den Subprocess ab. 
      * @param signal Nur unter Unix: Das Signal für kill(), 0 wird als 9 (SIGKILL, sofortiges Ende) interpretiert. 
      */
    public void             kill                ( int signal )                      {                com_call( "kill", signal ); }


    /** Bricht den Subprocess ab. */
    public void             kill                ()                                  {                com_call( "kill" ); }
}
