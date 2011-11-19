// $Id: Order.java 4051 2006-01-18 19:05:50Z jz $

package sos.spooler;

/** 
 * @author Joacim Zschimmer
 * @version $Revision: 4051 $
 */



public class Web_service_response  extends Idispatch
{
    private                 Web_service_response    ( long idispatch )             { super(idispatch); }

    
    public String           header                  ( String name )                { return (String)   com_call( "<Header", name ); }
    public void         set_status_code             ( int code )                   {                   com_call( ">status_code", code ); }
    public void         set_header                  ( String name, String value )  {                   com_call( ">Header", name, value ); }
    public void         set_content_type            ( String content_type )        {                   com_call( ">content_type", content_type ); }
    public void         set_charset_name            ( String charset_name )        {                   com_call( ">charset_name", charset_name ); }
    public void         set_string_content          ( String content )             {                   com_call( ">string_content", content ); }
    public void         set_binary_content          ( byte[] content )             {                   com_call( ">binary_content", content ); }
    public void              send                   ()                             {                   com_call( "send"         ); }
}
