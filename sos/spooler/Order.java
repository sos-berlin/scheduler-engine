// $Id: Order.java,v 1.4 2004/07/12 17:59:49 jz Exp $

/**
 * Auftrag
 * 
 * <p>
 * Ein Auftrag, der in der Auftragswarteschlange eines Jobs steht, veranlasst diesen, einen Jobschritt 
 * (also {@link Job_impl#spooler_process()}) durchzuführen.
 * <p>
 * Ein Auftrag, der in einer Jobkette steht, wird anschließend dem nächsten Job der Jobkette zugestellt.
 * 
 *
 * @author Joacim Zschimmer
 * @version $Revision: 1.4 $
 */

package sos.spooler;

public class Order extends Idispatch
{
    private                 Order               ( long idispatch )                  { super(idispatch); }

    
    
    /**
     * Stellt die Kennung des Auftrags ein.
     * 
     * <p>
     * Jeder Auftrag hat eine (innerhalb der Jobkette oder der Auftragswarteschlange des Jobs eindeutige) Kennung.
     * <p>
     * Ohne diesen Aufruf vergibt die Methode {@link Job_chain#add_order(Order)} bzw. {@link Order_queue#add_order(Order)} eine Kennung.
     *  
     * @param value Die Kennung
     */
    
    public void         set_id                  ( String value )                    {                   com_call( ">id", value          ); }
    
    
    
    /** Liefert die Kennung des Auftrags. */
    public String           id                  ()                                  { return (String)   com_call( "<id"                 ); }
    
    
    
    /** Der Titel ist ein Klartext für die Protokollierung. */ 
    public void         set_title               ( String value )                    {                   com_call( ">title", value       ); }

    
    
    /** Der Titel ist ein Klartext für die Protokollierung. */
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

    
    
    /**
     * Stellt einen Text für die Protokollierung ein.
     */
    public void         set_state_text          ( String value )                    {                   com_call( ">state_text", value  ); }
    
    
    
    /**
     * Liefert den mit set_state_text() eingestellten Text.
     */
    public String           state_text          ()                                  { return (String)   com_call( "<state_text"         ); }

    
    
    /**
     * Die Nutzlast, also Parameter des Auftrags.
     * 
     * @param payload Ein String, ein {@link Variable_set} oder ein Hostware.Dyn_obj.
     * @see Spooler#create_variable_set()
     */
    public void         set_payload             ( Object payload )                  {                   com_call( ">payload", payload   ); }

    
    
    /**
     * Die Nutzlast, also Parameter des Auftrags.
     */
    public Object           payload             ()                                  { return            com_call( "<payload"            ); }
    
    
    
    /** 
     * Prüft den COM-Typ der Nutzlast.
     * 
     * @param name "Spooler.Variable_set", "Hostware.Dyn_obj" oder "Hostware.Record".  
     * @return true, wenn die Nutzlast vom angegebenen COM-Typ ist. 
     */
    public boolean          payload_is_type     ( String name )                     { return    boolean_com_call( "payload_is_type"     ); }
}
