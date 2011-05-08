// $Id: Variable_set.java 4558 2006-10-04 13:55:00Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

package sos.spooler;

/** 
 * @author Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
 * @version $Revision: 4558 $
 */

public class Supervisor_client extends Idispatch
{
    private                 Supervisor_client   ( long idispatch )                  { super(idispatch); }

    public String           hostname            ()                                  { return (String)       com_call( "<hostname" ); }
    public int              tcp_port            ()                                  { return            int_com_call( "<tcp_port" ); }
}
