// $Id$

package sos.spooler;

/** Ein Auftrag zur Verarbeitung durch einen Job.
 * 
 * <p>
 * Ein Auftrag, der in der Auftragswarteschlange eines Jobs steht, veranlasst diesen, einen Jobschritt 
 * (also {@link Job_impl#spooler_process()}) durchzuführen.
 *
 * <p><br/><b>Beispiel</b>
 * <pre>
 *     Order order = spooler.create_order();
 *     order.set_id   ( "10001" );
 *     order.set_title( "Auftrag für Datensatz 10001" );
 *     order.set_state( "100" );
 *     spooler.job_chain( "jobkette1" ).add_order( order );
 * </pre>
 *
 * <p><br/><b>Beispiel (Javascript)</b>
 * <pre>
 *     var order = spooler.create_order();
 *     order.id    = "10001";
 *     order.title = "Auftrag für Datensatz 10001";
 *     order.state = 100;
 *     spooler.job_chain( "jobkette1" ).add_order( order );
 * </pre>
 *
 * @see Spooler#create_order()
 * @see Spooler#job_chain(String)
 * @see Task#order() 
 *
 * @author Joacim Zschimmer
 * @version $Revision$
 */

public class Order extends Idispatch
{
    private                 Order               ( long idispatch )                  { super(idispatch); }

    
    
    /** Stellt die Kennung des Auftrags ein.
     * 
     * <p>
     * Jeder Auftrag hat eine (innerhalb der Jobkette oder der Auftragswarteschlange des Jobs eindeutige) Kennung.
     * Diese Kennung sollten den zu verarbeitenden Daten entsprechen.
     * Überlicherweise wird der Schlüssel eines Datenbanksatzes verwendet.  
     * <p>
     * Ohne diesen Aufruf vergibt der Aufruf {@link Job_chain#add_order(Order)} bzw. {@link Order_queue#add_order(Order)} eine Kennung.
     *  
     * @param value Die Kennung
     */
    
    public void         set_id                  ( String value )                    {                   com_call( ">id", value          ); }
    
    
    
    /** Liefert die Kennung des Auftrags. */
    public String           id                  ()                                  { return (String)   com_call( "<id"                 ); }
    
    
    
    /** Der Titel ist ein Klartext, der den Auftrag bezeichnet. */ 
    public void         set_title               ( String value )                    {                   com_call( ">title", value       ); }

    
    
    /** Der Titel ist ein Klartext, der den Auftrag bezeichnet. */ 
    public String           title               ()                                  { return (String)   com_call( "<title"              ); }
    
    
    
    /** Aufträge mit höherer Priorität werden zuerst verarbeitet. */
    public void         set_priority            ( int value )                       {                   com_call( ">priority", value    ); }
    

    
    /** Aufträge mit höherer Priorität werden zuerst abgearbeitet. */
    public String           priority            ()                                  { return (String)   com_call( "<priority"           ); }
    
    
    
    /** Liefert die Jobkette, in der der Auftrag enthalten ist, oder null. */
    public Job_chain        job_chain           ()                                  { return (Job_chain)com_call( "<job_chain"          ); }

    
    
    /** Liefert den Jobkettenknoten, der dem Zustand des Auftrags entspricht, oder null, wenn der Auftrag nicht in einer Jobkette ist. */
    public Job_chain_node   job_chain_node      ()                                  { return (Job_chain_node)com_call( "<job_chain_node" ); }

    
    /** Stellt den Zustand des Auftrags auf den Zustand ein, für den in der Jobkette der angegebene Job eingestellt ist.
     * 
     * <p>
     * Besser ist der Aufruf von {@link #set_state(String)}.
     */
    public void         set_job                 ( Job job )                         {                   com_call( ">job", job           ); }

    
    
    /** Stellt den Zustand des Auftrags auf den Zustand ein, der dem Job in Jobkette entspricht.
     * 
     * <p>
     * Besser ist der Aufruf von {@link #set_state(String)}.
     */
    public void         set_job                 ( String job_name )                 {                   com_call( ">job", job_name      ); }
    
    
    
    /**
     * Liefert den Job, in dessen Auftragswarteschlange sich der Auftrag befindet, oder null.
     */
    public Job              job                 ()                                  { return (Job)      com_call( "<job"                ); }
    
    
    
    /**
     * Stellt den Zustand des Auftrags ein.
     * 
     * <p>
     * Wenn der Auftrag in einer Jobkette ist, wird der Auftrag an die entsprechende Stelle der Jobkette verschoben.
     * @param value
     */
    public void         set_state               ( String value )                    {                   com_call( ">state", value       ); }
    
    
    
    /**
     * Liefert den Zustand des Auftrags.
     */
    public String           state               ()                                  { return (String)   com_call( "<state"              ); }

    
    
    /** Stellt einen Klartext ein, der den Zustand des Auftrags beschreibet. */
    public void         set_state_text          ( String value )                    {                   com_call( ">state_text", value  ); }
    
    
    
    /** Liefert den mit set_state_text() eingestellten Text. */
    public String           state_text          ()                                  { return (String)   com_call( "<state_text"         ); }

    
    
    /** Die Nutzlast, also Parameter des Auftrags.
     * Neben der Auftragskennung (id), die den Auftrag identifiziert, können hier zusätzliche
     * Angaben gemacht werden. 
     * 
     * @param payload Ein String oder ein {@link Variable_set}.
     * 
     * @see #set_id(String)
     * @see Spooler#create_variable_set()
     */
    public void         set_payload             ( Object payload )                  {                   com_call( ">payload", payload   ); }

    
    
    /** Liefert den mit set_payload() eingestellten Wert. */
    public Object           payload             ()                                  { return            com_call( "<payload"            ); }
    
    
    
    /** Prüft den COM-Typ der Nutzlast.
     * 
     * @param name "Spooler.Variable_set", "Hostware.Dyn_obj" oder "Hostware.Record".  
     * @return true, wenn die Nutzlast vom angegebenen COM-Typ ist. 
     */
    public boolean          payload_is_type     ( String name )                     { return    boolean_com_call( "payload_is_type"     ); }
    
    
    
    /** Liefert die &lt;run_time> (zur periodischen Wiederholung des Auftrags).
     * 
     */
    public Run_time         run_time            ()                                  { return (Run_time) com_call( "<run_time"           ); }

    public void             remove_from_job_chain()                                 {                   com_call( "remove_from_job_chain" ); }
}
