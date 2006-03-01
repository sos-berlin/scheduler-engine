// $Id: Order.java 4051 2006-01-18 19:05:50Z jz $

package sos.spooler;

/** 
 * @author Joacim Zschimmer
 * @version $Revision: 4051 $
 */



public class Web_service_operation  extends Idispatch
{
    private                     Web_service_operation   ( long idispatch )          { super(idispatch); }

    
    public Web_service          web_service             ()                          { return (Web_service         )   com_call( "<web_service"          ); }
    public Web_service_request  web_service_request     ()                          { return (Web_service_request )   com_call( "<web_service_request"  ); }
    public Web_service_response web_service_response    ()                          { return (Web_service_response)   com_call( "<web_service_response" ); }
}
