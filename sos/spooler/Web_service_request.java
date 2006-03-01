// $Id: Order.java 4051 2006-01-18 19:05:50Z jz $

package sos.spooler;

/** 
 * @author Joacim Zschimmer
 * @version $Revision: 4051 $
 */



public class Web_service_request  extends Idispatch
{
    private                 Web_service_request      ( long idispatch )                  { super(idispatch); }

    
    public String           url                      ()                                  { return (String)   com_call( "<Url" ); }
    public String           header                   ( String name )                     { return (String)   com_call( "<Header", name ); }
    public String           string_content           ()                                  { return (String)   com_call( "<String_content" ); }
    public byte[]           binary_content           ()                                  { return (byte[])   com_call( "<Binary_content" ); }
}
