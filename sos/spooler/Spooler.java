// $Id: Spooler.java,v 1.12 2004/11/08 13:39:13 jz Exp $

package sos.spooler;

/** Das allgemeine Scheduler-Objekt.
 * 
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.12 $
 */

public class Spooler extends Idispatch
{
    private                 Spooler             ( long idispatch )                  { super(idispatch); }

    /** @return Wert der Kommandozeilenoption -id= beim Start des Schedulers */
    public String           id                  ()                                  { return (String)       com_call( "<id"                             ); }
    
    /** @return Wert der Kommandozeilenoption -param= beim Start des Schedulers */
    public String           param               ()                                  { return (String)       com_call( "<param"                          ); }
    
    /** @return Liefert das Arbeitsverzeichnis (als absoluten Pfad) beim Start des Schedulers. 
     * Wenn die Kommandozeilenoption -cd= benutzt worden ist, ist es das damit angegebene Verzeichnis. */
    public String           directory           ()                                  { return (String)       com_call( "<directory"                      ); }
    
    /** @return Liefert die Einstellung include_path (Kommandozeilen-Option <code>-include=</code>). */
    public String           include_path        ()                                  { return (String)       com_call( "<include_path"                   ); }
    
    /** @return Wert der Kommandozeilenoption -log-dir= beim Start des Schedulers. 
      * Das ist das Verzeichnis, in dem der Scheduler die Protokolle und Historiendateien ablegt. */
    public String           log_dir             ()                                  { return (String)       com_call( "<log_dir"                        ); }
    
    /** @return Liefert den Hostware-Datenbank-Dateinamen (für Historie usw.) */
    public String           db_name             ()                                  { return (String)       com_call( "<db_name"                        ); }
    
    /** @return Liefert true, genau dann wenn der Scheduler als Dienst (in Windows) oder als Daemon (in Unix) läuft. */
    public boolean          is_service          ()                                  { return        boolean_com_call( "<is_service"                     ); }
    
    /** @return Liefert den Namen des Rechners, auf dem der Scheduler läuft. */
    public String           hostname            ()                                  { return (String)       com_call( "<hostname"                       ); }

    
    /** @return Die scheduler-weiten Variablen als {@link Variable_set} */
    public Variable_set     variables           ()                                  { return (Variable_set) com_call( "<variables"                      ); }
    
    /** Setzt eine scheduler-weite Variable. */
    public void         set_var                 ( String name, String value )       {                       com_call( ">var", name, value               ); }
    
    /** @return Liefert den Wert einer scheduler-weiten Variablen. */
    public String           var                 ( String name )                     { return (String)       com_call( "<var", name                      ); }
    
    /** @return Eine neue Variablenmenge ({@link Variable_set}).
     * @see Job#start(Variable_set)
     */
    public Variable_set     create_variable_set ()                                  { return (Variable_set) com_call( "create_variable_set"             ); }
    

    /** Liefert das {@link Log} des Schedulers. Normalerweise sollte man aber die Variable {@link Job_impl#spooler_log} verwenden.  
     */
    public Log              log                 ()                                  { return (Log)          com_call( "<log"                            ); }
    
  //public IDispatch        script              ()                                  { return (Idispatch)    com_call( "<script"                         ); }
    
    /** @return Der gewünschte {@link Job} */
    public Job              job                 ( String job_name )                 { return (Job)          com_call( "<job", job_name                  ); }
    
    
    /** @return Liefert eine neue {@link Job_chain}. 
      * Diese Jobkette kann, nachdem sie mit Jobs gefüllt worden ist, mit {@link #add_job_chain(Job_chain)} dem Scheduler hinzugefügt werden. */
    public Job_chain        create_job_chain    ()                                  { return (Job_chain)    com_call( "create_job_chain"                ); }
    
    
    public void             add_job_chain       ( Job_chain job_chain )             {                       com_call( "add_job_chain", job_chain        ); }
    
    /** @return Liefert die gewünschte Jobkette ({@link Job_chain}) */
    public Job_chain        job_chain           ( String name )                     { return (Job_chain)    com_call( "<job_chain", name                ); }
    
    /** @return Liefert true genau dann, wenn die Jobkette vorhanden (also mit {@link #add_job_chain(Job_chain)} hingefügt worden) ist. */
    public boolean          job_chain_exists    ( String job_chain )                { return        boolean_com_call( "job_chain_exists", job_chain     ); }
    
    
    /** @return Erzeugt eine neue {@link Order}. 
     * Dieser Auftrag kann mit {@link Job_chain#add_order(Order)} einer Jobkette 
     * oder mit {@link Order_queue#add_order(Order)} direkt einem Job übergeben werden. */
    public Order            create_order        ()                                  { return (Order)        com_call( "create_order"                    ); }
    
    
    /** Sobald alle Aufträge abgerbeitet sind, beendet der Scheduler alle Jobs (durch Aufruf von {@link Job_impl#spooler_close}) und dann sich selbst.
     * Ein neuer Scheduler mit unveränderten Kommandozeilenparametern wird gestartet. */
    public void             let_run_terminate_and_restart()                         {                       com_call( "let_run_terminate_and_restart"   ); }
    
    /** Bricht den Scheduler augenblicklich ab. Kein Job hat Gelegenheit, darauf zu reagieren. */
    public void             abort_immediately   ()                                  {                       com_call( "abort_immediately"               ); }
    
    /** Bricht den Scheduler augenblicklich ab. Kein Job hat Gelegenheit, darauf zu reagieren.
     * Anschließend wird der Scheduler erneut mit den gleichen Kommandozeilenparametern gestartet*/
    public void             abort_immediately_and_restart()                         {                       com_call( "abort_immediately_and_restart"   ); }
    
    /** Name der Datenbanktabelle für die internen Variablen des Schedulers */
    public String           db_variables_table_name  ()                             { return (String)       com_call( "<db_variables_table_name" ); }

    /** Name der Datenbanktabelle für die Tasks */
    public String           db_tasks_table_name      ()                             { return (String)       com_call( "<db_tasks_table_name" ); }

    /** Name der Datenbanktabelle für die Aufträge */
    public String           db_orders_table_name     ()                             { return (String)       com_call( "<db_orders_table_name" ); }

    /** Name der Datenbanktabelle für die Historie */
    public String           db_history_table_name    ()                             { return (String)       com_call( "<db_history_table_name" ); }

    /** Name der Datenbanktabelle für die Auftragshistorie */
    public String           db_order_history_table_name()                           { return (String)       com_call( "<db_order_history_table_name" ); }
}
