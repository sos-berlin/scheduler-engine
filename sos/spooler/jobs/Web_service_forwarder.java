// $Id: Task.java 3946 2005-09-26 08:52:01Z jz $

package sos.spooler.jobs;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;

import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMResult;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.stream.StreamSource;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.Text;

import org.apache.xerces.parsers.DOMParser;         // xercesImpl.jar
import org.apache.xml.serialize.OutputFormat;
import org.apache.xml.serialize.XMLSerializer;      // xercesImpl.jar

import sos.spooler.Order;
import sos.spooler.Web_service;



/** 
 * @author Joacim Zschimmer
 * @version $Revision: 3946 $
 */


public class Web_service_forwarder  extends sos.spooler.Job_impl
{
    //private String              current_url_string = null;
    public final String encoding = "iso-8859-1";
            
    TransformerFactory transformer_factory = TransformerFactory.newInstance();
    
    //---------------------------------------------------------------------------------------------
    
    class Order_exception  extends Exception
    {
        Order_exception( String message ) 
        {
            super( message );
        }
    }
    
    //---------------------------------------------------------------------------------------------
    
    class Process
    {
        Order       order       = spooler_task.order();
        Web_service web_service = order.web_service();
        
        //-----------------------------------------------------------------------------------------
        
        Process()  throws Exception
        {
        }
        
        //-----------------------------------------------------------------------------------------
        
        boolean process()  throws Exception
        {
            /*
            Object payload = order.payload();
            
            if( !( payload instanceof String ) )  
            {
                spooler_log.warn( "Order.payload ist kein String" );
                return false;
            }
            */
            return execute_service( transform_order( dom_from_xml( order.xml() ) ) );
        }
        
        //-----------------------------------------------------------------------------------------
        
        private Document dom_from_xml( String xml )  throws Exception
        {
            DOMParser parser = new DOMParser();
            //parser.setErrorHandler();
            parser.parse( new org.xml.sax.InputSource( new StringReader( xml ) ) );
                
            return parser.getDocument();
        }

        //-----------------------------------------------------------------------------------------
        
        private Document transform_order( Document order_document )  throws Exception
        {
            File stylesheet_file = new File( web_service.transform_xslt_stylesheet_path() );
            Transformer transformer = transformer_factory.newTransformer( new StreamSource(  ) );
            
            Document result = new Document();
            transformer.transform( new DOMSource( order_document ), new DOMResult( result ) );
            
            return result;
        }
        
        //-----------------------------------------------------------------------------------------
        
        private boolean execute_service( Document service_request_document )  throws Exception
        {
            Element service_request_element = service_request_document.getDocumentElement();
            
            if( !service_request_element.getNodeName().equals( "service_request" ) )
                throw new Order_exception( "Forward_xslt_stylesheet liefert nicht <service_request>" );
    
            
            URL url = new URL( service_request_element.getAttribute( "url" ) );
    
    
    
    
    
            
            HttpURLConnection http_connection = null;
            
            try
            {
                http_connection = (HttpURLConnection)url.openConnection();
                //http_connection.setMethod( "POST" );
                //? http_connection.setDoOutput( true );
                http_connection.connect();
    
                
                OutputStream        output_stream = http_connection.getOutputStream();
                OutputStreamWriter  writer        = new OutputStreamWriter( output_stream, encoding );
                Element             content_element  = null;
                
                for( Node node = service_request_element.getFirstChild(); node != null; node = content_element.getNextSibling() )
                {
                    if( node.getNodeType() == Node.ELEMENT_NODE  &&  node.getNodeName().equals( "content" ) ) 
                    {
                        content_element = (Element)node;
                        break;
                    }
                }
                
                if( content_element == null )  throw new Order_exception( "<service_request> ohne <content>" );
                
                
                // Inhalt von <content> schreiben
                
                Node data_node = content_element.getFirstChild();
                
                if( data_node == null )  throw new Order_exception( "<content> ist leer" );
                
                if( data_node.getNodeType() == Node.TEXT_NODE 
                 || data_node.getNodeType() == Node.CDATA_SECTION_NODE )
                {
                    writer.write( data_node.getTextContent() );
                }
                else
                if( data_node.getNodeType() == Node.ELEMENT_NODE )
                {
                    new XMLSerializer( output_stream, new OutputFormat( service_request_document, encoding, false ) )
                    .serialize( (Element)data_node );
                }
                else
                    throw new Order_exception( "<content> enthält kein Element und keinen Text" );
                
                if( data_node.getNextSibling() != null )  throw new Order_exception( "<content> enthält mehr als einen Knoten" );
                    
                
                writer.flush();
                output_stream.flush();
                
    
                int response_code = http_connection.getResponseCode();
                
                if( response_code != HttpURLConnection.HTTP_OK 
                 && response_code != HttpURLConnection.HTTP_ACCEPTED )
                {
                    spooler_log.warn( http_connection.getResponseMessage() );
                    //http_connection.getErrorStream()
                    order.setback();    
                }
                
                //Object response = http_connection.getContent();
                InputStreamReader reader = new InputStreamReader( http_connection.getInputStream(), encoding );    
    
                char[] buffer = new char[ 1024 ];
                StringBuffer response = new StringBuffer();
                while(true)
                {
                    int len = reader.read( buffer );
                    if( len == 0 )  break;
                    response.append( buffer, 0, len );
                }
                spooler_log.info( response.toString() );        
            }
            finally
            {
                if( http_connection != null )  http_connection.disconnect();
            }
            
            return true;
        }
        
        //-----------------------------------------------------------------------------------------
        
    } //class
    
    //---------------------------------------------------------------------------------------------
    
    public Web_service_forwarder()  throws Exception
    {
    }
    
    //---------------------------------------------------------------------------------------------
    
    public boolean spooler_open()  throws Exception
    {
        return true;
    }
    
    //---------------------------------------------------------------------------------------------
    
    public void spooler_close()
    {
        //if( http_connection != null )
        //{
        //    http_connection.disconnect();
        //}
    }
    
    //---------------------------------------------------------------------------------------------

    public boolean spooler_process()  throws Exception
    {
        try
        {
            return new Process().process();
        }
        catch( Order_exception x )
        {
            spooler_log.warn( x.toString() );
            spooler_task.order().setback();
            return false;
        }
    }
    
    //---------------------------------------------------------------------------------------------
}
