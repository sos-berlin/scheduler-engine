// $Id: Job_chain_node.java,v 1.4 2002/11/15 09:47:41 jz Exp $

package sos.spooler;

/**
 * Jobkettenknoten (Job, Zustand, Folgezustand, Fehlerzustand)
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.4 $
 */

public class Job_chain_node extends Idispatch
{
    private                 Job_chain_node      ( long idispatch )                  { super(idispatch); }


    public void         set_name                ( String value )                    {                        com_call( ">name", value ); }


    public String           state               ()                                  { return (String)        com_call( "<state"       ); }


    /** @return NULL, wenn es keinen nächsten Knoten gibt (Folgezustand ist nicht angegeben) */
    public Job_chain_node   next_node           ()                                  { return (Job_chain_node)com_call( "<next_node"   ); }


    /** @return NULL, wenn es keinen Fehler-Knoten gibt (Fehlerzustand ist nicht angegeben) */
    public Job_chain_node   error_node          ()                                  { return (Job_chain_node)com_call( "<error_node"  ); }


    public Job              job                 ()                                  { return (Job)           com_call( "<job"         ); }


    public String           next_state          ()                                  { return (String)        com_call( "<next_state"  ); }


    public String           error_state         ()                                  { return (String)        com_call( "<error_state" ); }
}
