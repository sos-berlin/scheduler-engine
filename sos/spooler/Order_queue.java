// $Id: Order_queue.java,v 1.1 2002/11/09 09:13:17 jz Exp $

package sos.spooler;

public class Order_queue extends Idispatch
{
    private                 Order_queue         ( long idispatch )                  { super(idispatch); }

    public int              length              ()                                  { return        int_com_call( "<length"             ); }
    public Order            add_order           ( Order order )                     { return (Order)    com_call( "add_order", order    ); }
}
