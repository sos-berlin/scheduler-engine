// $Id: Job_chain_node.java,v 1.5 2004/07/12 17:59:49 jz Exp $

package sos.spooler;

/**
 * Ein Jobkettenknoten beschreibt eine Stelle in einer Jobkette ({@link Job_chain}). 
 * Einem Jobkettenknoten sind zugeordnet: ein Zustand, ein Job, ein Folgezustand und ein Fehlerzustand.
 * <p>
 * Ein Jobkettenknoten wird mit {@link Job_chain#add_job(String,String,String,String)} oder mit {@link Job_chain#add_end_state(String)}
 * erzeugt.    
 *
 * @author Joacim Zschimmer
 * @version $Revision: 1.5 $
 */

public class Job_chain_node extends Idispatch
{
    private                 Job_chain_node      ( long idispatch )                  { super(idispatch); }


  //public void         set_name                ( String value )                    {                        com_call( ">name", value ); }


    /** @return Zustand, für den dieser Jobkettenknoten gilt.
     */

    public String           state               ()                                  { return (String)        com_call( "<state"       ); }


    /** @return null, wenn es keinen nächsten Knoten gibt (Folgezustand ist nicht angegeben) */
    public Job_chain_node   next_node           ()                                  { return (Job_chain_node)com_call( "<next_node"   ); }


    /** @return null, wenn es keinen Fehler-Knoten gibt (Fehlerzustand ist nicht angegeben) */
    public Job_chain_node   error_node          ()                                  { return (Job_chain_node)com_call( "<error_node"  ); }


    public Job              job                 ()                                  { return (Job)           com_call( "<job"         ); }


    public String           next_state          ()                                  { return (String)        com_call( "<next_state"  ); }


    public String           error_state         ()                                  { return (String)        com_call( "<error_state" ); }
}
