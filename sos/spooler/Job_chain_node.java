// $Id: Job_chain_node.java,v 1.6 2004/07/13 11:28:06 jz Exp $

package sos.spooler;

/**
 * Ein Jobkettenknoten beschreibt eine Stelle in einer Jobkette ({@link Job_chain}). 
 * Einem Jobkettenknoten sind zugeordnet: ein Zustand, ein Job, ein Folgezustand und ein Fehlerzustand.
 * <p>
 * Ein Jobkettenknoten wird mit {@link Job_chain#add_job(String,String,String,String)} oder mit {@link Job_chain#add_end_state(String)}
 * erzeugt.    
 *
 * @see Job_chain#node(String)
 * @see Order#job_chain_node()
 * @author Joacim Zschimmer
 * @version $Revision: 1.6 $
 */

public class Job_chain_node extends Idispatch
{
    private                 Job_chain_node      ( long idispatch )                  { super(idispatch); }


  //public void         set_name                ( String value )                    {                        com_call( ">name", value ); }


    
    /** Zustand, für den dieser Jobkettenknoten gilt.
     * 
     * <p><br/><b>Beispiel</b>
     * <pre>
     *     Job_chain_node job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "state=" + job_chain_node.state() );                      // "state=100"
     * </pre>
     *
     * <p><br/><b>Beispiel in JavaScript</b>
     * <pre>
     *     var job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "state=" + job_chain_node.state );                        // "state=100"
     * </pre>
     */

    public String           state               ()                                  { return (String)        com_call( "<state"       ); }


    
    /** Liefert den nächsten Knoten in der Jobkette für den Erfolgsfall.
     * 
     * <p><br/><b>Beispiel</b>
     * <pre>
     *     Job_chain_node job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "next state=" + job_chain_node.next_node().state() );     // "state=200"
     * </pre>
     *
     * <p><br/><b>Beispiel in JavaScript</b>
     * <pre>
     *     var job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "next state=" + job_chain_node.next_node.state );         // "state=200"
     * </pre>
     *  
     * @return null, wenn es keinen nächsten Knoten gibt (Folgezustand ist nicht angegeben) 
     */
    public Job_chain_node   next_node           ()                                  { return (Job_chain_node)com_call( "<next_node"   ); }


    
    /** Liefert den nächsten Knoten in der Jobkette für den Fehlerfall.
     *  
     * <p><br/><b>Beispiel</b>
     * <pre>
     *     Job_chain_node job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "error state=" + job_chain_node.error_node().state() );   // "state=999"
     * </pre>
     *
     * <p><br/><b>Beispiel in JavaScript</b>
     * <pre>
     *     var job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "error state=" + job_chain_node.error_node.state );       // "state=999"
     * </pre>
     * 
     * @return null, wenn es keinen Fehler-Knoten gibt (Fehlerzustand ist nicht angegeben) 
     */
    public Job_chain_node   error_node          ()                                  { return (Job_chain_node)com_call( "<error_node"  ); }


    /** Liefert den Job, der dem Knoten zugeordnet ist.
     * 
     * <p><br/><b>Beispiel</b>
     * <pre>
     *     Job_chain_node job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "job=" + job_chain_node.job().name() );                   // "job=job_100"
     * </pre>
     *
     * <p><br/><b>Beispiel in JavaScript</b>
     * <pre>
     *     var job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "job=" + job_chain_node.job.name );                       // "job=job_100"
     * </pre>
     */
    public Job              job                 ()                                  { return (Job)           com_call( "<job"         ); }


    /** Liefert den nächsten Zustand in der Jobkette für den Erfolgsfall.
     * 
     * <p><br/><b>Beispiel</b>
     * <pre>
     *     Job_chain_node job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "next state=" + job_chain_node.next_state() );            // "state=200"
     * </pre>
     *
     * <p><br/><b>Beispiel in JavaScript</b>
     * <pre>
     *     var job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "next_state=" + job_chain_node.next_state );              // "state=200"
     * </pre>
     */
    public String           next_state          ()                                  { return (String)        com_call( "<next_state"  ); }


    /** Liefert den nächsten Zustand in der Jobkette für den Fehlerfall.
     *  
     * <p><br/><b>Beispiel</b>
     * <pre>
     *     Job_chain_node job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "error state=" + job_chain_node.error_node().state() );   // "error state=999"
     * </pre>
     *
     * <p><br/><b>Beispiel in JavaScript</b>
     * <pre>
     *     var job_chain_node = spooler.job_chain( "Jobkette" ).node( 100 );                                                  
     *     spooler_log.debug( "error state=" + job_chain_node.error_node.state );       // "error state=999"
     * </pre>
     *  
     */
    public String           error_state         ()                                  { return (String)        com_call( "<error_state" ); }
}
