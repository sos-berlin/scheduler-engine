// $Id: Job.java,v 1.6 2004/07/12 17:59:49 jz Exp $

package sos.spooler;

/**
 * 
 * @author Joacim Zschimmer
 * @version $Revision: 1.6 $
 */

public class Job extends Idispatch
{
    private                 Job                 ( long idispatch )                  { super(idispatch); }
    
    /**
     * Erzeugt eine neue Task und reiht sie in die Task-Warteschlange ein.
     */
    
    public Task             start               ()                                  { return (Task)       com_call( "start"                         ); }
    
    
    /**
     * Erzeugt eine neue Task mit Parametern und reiht sie in die Task-Warteschlange des Jobs ein.
     * 
     * <p>
     * Die Parameter stehen der Task mit {@link Task#params()} zur Verfügung.
     * <p>
     * Zwei besondere Parameter können angegeben werden:
     * <p>
     * "spooler_task_name": Gibt der Task einen Namen, der in den Statusanzeigen erscheint.
     * <p>
     * "spooler_start_after": Gibt ein Zeit in Sekunden (reelle Zahl) an, nach dessen Ablauf die Task zu starten ist. 
     * Dabei wird die Konfiguration von &lt;run_time> außer Kraft gesetzt.
     */
    
    public Task             start               ( Variable_set variables )          { return (Task)       com_call( "start", variables              ); }

    
    
    /**
     * Startet eine Task des Jobs gemäß &lt;run_time>, wenn nicht schon eine läuft und &lt;run_time> dies zulässt.
     */
    
    
    public void             wake                ()                                  {                     com_call( "wake"                          ); }
    
    
    
    /**
     * Lässt eine Task starten, sobald sich ein Verzeichnis ändert.
     * 
     * <p>
     * Wenn keine Task des Jobs läuft und sich das Verzeichnis geändert hat 
     * (eine Datei hinzukommt, umbenannt oder entfernt wird), 
     * startet der Scheduler innerhalb der &lt;run_time> eine Task.
     * <p>
     * Der Aufruf kann für verschiedene und gleiche Verzeichnisse wiederholt werden.
     * 
     * @param directory_name
     */
    
    public void             start_when_directory_changed( String directory_name )                           { com_call( "start_when_directory_changed", directory_name ); }

    
    /**
     * Lässt eine Task starten, sobald sich ein Verzeichnis ändert, mit Angabe eines Regulären Ausdrucks.
     * 
     * <p>
     * Wie {@link #start_when_directory_changed(String)}, mit der Einschränkung, dass nur Dateien beachtet werden, 
     * deren Name dem angegebenen Regulären Ausdruck entsprechen. 
     * 
     * @param directory_name
     * @param filename_pattern
     */
    
    public void             start_when_directory_changed( String directory_name, String filename_pattern )  { com_call( "start_when_directory_changed", directory_name, filename_pattern ); }

    
    /**
     * Nimmt alle Aufrufe von start_when_directory_changed() zurück.
     *
     */
    public void             clear_when_directory_changed()                          {                     com_call( "clear_when_directory_changed"  ); }

    
    
  //public Thread           thread              ()                                  { return (Thread)     com_call( "<thread"                       ); }
    
    
    
    /**
     * Dasselbe wie spooler().include_path().
     * 
     * @see Spooler#include_path()
     */
    
    public String           include_path        ()                                  { return (String)     com_call( "<include_path"                 ); }
    
    
    
    /**
     * Liefert den Jobnamen. 
     * @return Der Name des Jobs.
     */
    
    public String           name                ()                                  { return (String)     com_call( "<name"                         ); }
    
    
    
    /**
     * Setzt für die Status-Anzeige einen Text.
     * @param line Eine Textzeile
     */
    
    public void         set_state_text          ( String line )                     {                     com_call( ">state_text", line             ); }
    
    
    
    /**
     * Liefert den in der Konfiguration eingestellten Titel des Jobs.
     * <p>
     * Aus &lt;job title="...">
     * 
     */
    public String           title               ()                                  { return (String)     com_call( "<title"                        ); }

    
    /**
     * Liefert die Auftragswarteschlange des Jobs oder null.
     * @return Die {@link Order_queue} oder null, wenn der Job nicht auftragsgesteuert ist (&lt;job order="no">).
     */
    
    public Order_queue      order_queue         ()                                  { return (Order_queue)com_call( "<order_queue"                  ); }

    
    /** Stellt die Fehlertoleranz ein.
     * <p>
     * Für verschiedene Anzahlen aufeinanderfolgender Fehler kann eine Verzögerung eingestellt werden.
     * Der Job wird dann nicht gestoppt, sondern die angegebene Zeit verzögert und erneut gestartet.
     * <p>
     * Der Aufruf kann für verschiedene Anzahlen wiederholt werden. 
     * Man wird jeweils eine längere Verzögerung angeben.
     * <p>
     * Beispiel siehe {@link #set_delay_after_error(int,String)}
     * 
     * @param error_steps Anzahl der aufeinanderfolgenden Fehler, ab der die Verzögerung gilt
     * @param seconds Verzögerung als reele Zahl
     */
    
    public void         set_delay_after_error   ( int error_steps, double seconds ) {                     com_call( ">delay_after_error", new Integer(error_steps), new Double(seconds)   ); }
    
    
    /**
     * Wie {@link #set_delay_after_error(int,double)}, "HH:MM:SS" und "STOP" können angegeben werden.
     * 
     * <p>
     * Die Verzögerung kann als String "HH:MM:SS" oder "HH:MM:SS" (Stunde, Minute, Sekunde) eingestellt werden.
     * <p>
     * Statt einer Zeit kann auch "STOP" angegeben werden. 
     * Wenn der Job die angegebene Anzahl aufeinanderfolgende Fehler erreicht hat,
     * stoppt der Scheduler den Job.
     * <p>
     * Eine gute Stelle für die Aufrufe ist {@link Job_impl#spooler_init()}.
     * <p>
     * Beispiel:
     * <pre>
     *      spooler_job.set_delay_after_error(  2,  10 );
     *      spooler_job.set_delay_after_error(  5, "01:00" );
     *      spooler_job.set_delay_after_error( 10, "24:00" );
     *      spooler_job.set_delay_after_error( 20, "STOP" );
     * </pre>
     * Nach einem Fehler wiederholt der Scheduler den Job sofort.<br/>
     * Nach dem zweiten bis zum vierten Fehler verzögert der Scheduler den Job um 10 Sekunden,<br/>
     * nach dem fünften bis zum neunten Fehler um eine Minute,
     * nach dem zehnten bis zum neunzehnten um 24 Stunden,
     * nach dem zwanzigsten aufeinanderfolgenden Fehler schließlich stoppt der Job.
     *  
     * @param error_steps
     * @param hhmm_ss
     */
    
    public void         set_delay_after_error   ( int error_steps, String hhmm_ss ) {                     com_call( ">delay_after_error", new Integer(error_steps), hhmm_ss   ); }
    
    
    /**
     * Nimmt alle Aufrufe von set_delay_after_error() zurück. 
     */
    public void             clear_delay_after_error()                               {                     com_call( "clear_delay_after_error"       ); }
}
