// $Id: Job_chain_node.java,v 1.1 2002/11/09 09:13:17 jz Exp $

package sos.spooler;

public class Job_chain_node extends Idispatch
{
    private                 Job_chain_node      ( long idispatch )                  { super(idispatch); }

    public void         set_name                ( String value )                    {                        com_call( ">name", value ); }
    public String           state               ()                                  { return (String)        com_call( "<state"       ); }
    public Job_chain_node   next_node           ()                                  { return (Job_chain_node)com_call( "<next_node"   ); }
    public Job_chain_node   error_node          ()                                  { return (Job_chain_node)com_call( "<error_node"  ); }
    public Job              job                 ()                                  { return (Job)           com_call( "<job"         ); }
}
