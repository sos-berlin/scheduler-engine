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


    /** Verhindert, dass der Job stoppt, wenn der Subprozess bei Task-Ende mit exit_code <> 0 geendet hat.
      * <p/>
      *  Wenn das Ende des Subprocesses nicht mit {@link #wait_for_termination()} oder {@link #wait_for_termination(double)} 
      * abgewartet worden ist, tut das der Scheduler bei Task-Ende.
      * Wenn dann der Subprocess mit {@link #exit_code()} != 0 endet, stoppt der Job mit Fehler.
      * <p/>
      * Mit set_ignore_error( true ) kann das verhindert werden.
      * {@link #wait_for_termination()} 
      */
    public void         set_ignore_error        ( boolean b )                       {                com_call( ">ignore_error", b ); }


    /**
      * @see #set_ignore_error(boolean)
      */
    public boolean          ignore_error        ()                                  { return boolean_com_call( "<ignore_error" ); }


    /** Verhindert, dass der Job stoppt, wenn der Subprozess bei Task-Ende mit Signal (nur Unix) geendet hat.
      * <p/>
      * Wenn das Ende des Subprocesses nicht mit {@link #wait_for_termination()} oder {@link #wait_for_termination(double)} 
      * abgewartet worden ist, tut das der Scheduler bei Task-Ende.
      * Wenn dann der Subprocess mit Signal endet, stoppt der Job mit Fehler.
      * <p/>
      * Mit set_ignore_signal(true) kann das verhindert werden.
      * {@link #wait_for_termination()} 
      */
    public void         set_ignore_signal       ( boolean b )                       {                com_call( ">ignore_signal", b ); }


    /**
      * @see #set_ignore_signal(boolean)
      */
    public boolean          ignore_signal       ()                                  { return boolean_com_call( "<ignore_signal" ); }


    /** Nach Ablauf der Zeit bricht der Scheduler den Subprozess ab (Unix: mit SIGKILL) */
    public void         set_timeout             ( double seconds )                  {                com_call( ">timeout", seconds ); }


    /** Setzt eine Umgebungsvariable für den Prozess.
      * Vor start() aufzurufen. */
    public void         set_environment         ( String entry_name, String value ) {                com_call( ">environment", entry_name, value ); }
    

    /** Wartet aufs Ende des Subprocesses. 
      * @return true, wenn der Subprozess geendet hat 
      * @see #wait_for_termination()
      */
    public boolean          wait_for_termination( double seconds )                  { return boolean_com_call( "wait_for_termination", new Double( seconds ) ); }


    /** Wartet aufs Ende des Subprocesses. 
      * @see #wait_for_termination(double)
      */
    public void             wait_for_termination()                                  {                com_call( "wait_for_termination" ); }


    /** Bricht den Subprocess ab. 
      * @param signal Nur unter Unix: Das Signal für kill(), 0 wird als 9 (SIGKILL, sofortiges Ende) interpretiert. 
      */
    public void             kill                ( int signal )                      {                com_call( "kill", signal ); }


    /** Bricht den Subprocess ab. */
    public void             kill                ()                                  {                com_call( "kill" ); }
}
