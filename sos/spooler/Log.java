// $Id: Log.java,v 1.11 2004/12/28 11:40:02 jz Exp $

package sos.spooler;

/** Zum Schreiben eines Protokolls.
 *
 * @see Job_impl#spooler_log
 * @see Spooler#log()
 * @author Joacim Zschimmer
 * @version $Revision: 1.11 $
 */

public class Log extends Idispatch
{
    private                     Log                     ( long idispatch )                          { super(idispatch); }


    /** Eine Fehlermeldung (Gewicht 2). */
    public void                 error                   ( String line )                             { com_call( "error" , line ); }

    
    /** Eine Warnung (Gewicht 1). */
    public void                 warn                    ( String line )                             { com_call( "warn"  , line ); }

    
    /** Eine Informationsmeldung (Gewicht 0). */
    public void                 info                    ( String line )                             { com_call( "info"  , line ); }

    
    /** Eine Debugmeldung (Gewicht -1). */
    public void                 debug                   ( String line )                             { com_call( "debug" , line ); }

    
    /** Eine Debugmeldung (Gewicht -1). */
    public void                 debug1                  ( String line )                             { com_call( "debug1", line ); }

    
    /** Eine Debugmeldung (Gewicht -2). */
    public void                 debug2                  ( String line )                             { com_call( "debug2", line ); }

    
    /** Eine Debugmeldung (Gewicht -3). */
    public void                 debug3                  ( String line )                             { com_call( "debug3", line ); }

    
    /** Eine Debugmeldung (Gewicht -4). */
    public void                 debug4                  ( String line )                             { com_call( "debug4", line ); }

    
    /** Eine Debugmeldung (Gewicht -5). */
    public void                 debug5                  ( String line )                             { com_call( "debug5", line ); }

    
    /** Eine Debugmeldung (Gewicht -6). */
    public void                 debug6                  ( String line )                             { com_call( "debug6", line ); }

    
    /** Eine Debugmeldung (Gewicht -7). */
    public void                 debug7                  ( String line )                             { com_call( "debug7", line ); }

    
    /** Eine Debugmeldung (Gewicht -8). */
    public void                 debug8                  ( String line )                             { com_call( "debug8", line ); }

    
    /** Eine Debugmeldung (Gewicht -9). */
    public void                 debug9                  ( String line )                             { com_call( "debug9", line ); }
    
    /** Eine Meldung mit dem angegebenen Gewicht (level).
     * 
     * @param level Gewicht von -9 (debug9) bis 2 (error).
     */
    public void                 log                     ( int level, String line )                  { com_call( "log"   , new Integer(level), line ); }

    
    /** Liefert das Mail-Objekt. */
    public Mail                 mail                    ()                                          { return (Mail)     com_call( "<mail"                   ); }

    
    
    /** Stellt ein, ob das Protokoll nach einem Fehler versendet werden soll.
     * 
     * @see #mail_on_error()
     */
    public void             set_mail_on_error           ( boolean value )                           {                   com_call( ">mail_on_error", value   ); }

    
    
    /** Liefert die Angabe von {@link #set_mail_on_error(boolean)}. */
    public boolean              mail_on_error           ()                                          { return    boolean_com_call( "<mail_on_error"          ); }

    
    
    /** Stellt ein, ob das Protokoll nach einem erfolgreichen Lauf versendet werden soll. 
     * 
     * @see #mail_on_success()
     */
    public void             set_mail_on_success         ( boolean value )                           {                   com_call( ">mail_on_success", value ); }

    
    
    /** Liefert die Angabe von {@link #set_mail_on_success(boolean)}. */
    public boolean              mail_on_success         ()                                          { return    boolean_com_call( "<mail_on_success"        ); }

    
    
    /** Stellt ein, ob das Protokoll nach wenigstens einem Jobschritt versendet werden soll. 
     * 
     * @see #mail_on_process()
     */
    public void             set_mail_on_process         ( int steps )                               {                   com_call( ">mail_on_process", steps ); }

    
    
    /** Liefert die Angabe von {@link #set_mail_on_process(int)}. */
    public int                  mail_on_process         ()                                          { return        int_com_call( "<mail_on_process"        ); }

    
    /** Mindestgewicht der Log-Ausgaben setzen.
     * 
     * @param level Gewicht, das eine Log-Ausgabe haben muss, um im Protokoll zu erscheinen.
     * @see #level()
     */
    public void             set_level                   ( int level )                               {                   com_call( ">level", level           ); }
    
    
    
    /** Liefert das Mindestgewicht der Log-Ausgaben. 
     * 
     * Ausgaben und diesem Gewicht werden unterdrückt.
     * 
     * @see #set_level(int)
     */
    public int                  level                   ()                                          { return        int_com_call( "<level"                  ); }

    
    
    /** Dateiname des Protokolls. 
     */
    public String               filename                ()                                          { return (String)   com_call( "<filename"               ); }

    
    
    /** Setzt den Dateinamen fürs Protokoll.
     * 
     * Wenn das Protokoll geschlossen wird, schreibt der Scheduler es in diese Datei.
     * 
     * @see #new_filename()
     * @see #filename()
     */
    public void             set_new_filename            ( String filename )                         {                   com_call( ">new_filename", filename ); }

    
    
    /**
     * Liefert den mit {@link #set_new_filename(String)} gesetzten Dateiname.
     * 
     * @see #filename()
     */
    public String               new_filename            ()                                          { return (String)   com_call( "<new_filename"           ); }


    /** @deprecated */
    public void             set_collect_within          ( double time )                             {                   com_call( ">collect_within", time   ); }

    
    /** @deprecated */
    public void             set_collect_within          ( String time )                             {                   com_call( ">collect_within", time   ); }

    
    /** @deprecated */
    public double               collect_within          ()                                          { return     double_com_call( "<collect_within"         ); }

    
    /** @deprecated */
    public void             set_collect_max             ( double time )                             {                   com_call( ">collect_max", time      ); }

    
    /** @deprecated */
    public void             set_collect_max             ( String time )                             {                   com_call( ">collect_max", time      ); }

    
    /** @deprecated */
    public double               collect_max             ()                                          { return     double_com_call( "<collect_max"            ); }

    
    public void             set_mail_it                 ( boolean mail_it )                         {                   com_call( ">mail_it"    , mail_it   ); }
    
    
    /** Liefert die zuletzt mit Gewicht 2 (error) ausgegebene Zeile. 
     * Oder einen Leerstring. */
    public String               last_error_line         ()                                          { return (String)   com_call( "<last_error_line"        ); }
}
