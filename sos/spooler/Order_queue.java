// $Id: Order_queue.java,v 1.2 2002/11/13 21:20:49 jz Exp $

/**
 * Auftragsliste eines Jobs.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.2 $
 */

package sos.spooler;

public class Order_queue extends Idispatch
{
    private                 Order_queue         ( long idispatch )                  { super(idispatch); }

    public int              length              ()                                  { return        int_com_call( "<length"             ); }
    public Order            add_order           ( Order order )                     { return (Order)    com_call( "add_order", order    ); }
}
