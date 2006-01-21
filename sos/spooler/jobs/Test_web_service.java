// $Id$
/**
 * 
 * @author Joacim Zschimmer
 * @version $Revision$  $Date$
 * Created on 19.01.2006
 *
 */
package sos.spooler.jobs;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLConnection;



/** 
 * @author Joacim Zschimmer
 * @version $Revision: 3946 $
 */


public class Test_web_service  extends sos.spooler.Job_impl
{
    //private String              current_url_string = null;
    public final String encoding = "iso-8859-1";
            

    //---------------------------------------------------------------------------------------------
    
    static String string_from_reader( Reader reader )  throws IOException
    {
        StringBuffer result = new StringBuffer( 10000 );
        char[]       buffer = new char[ 1024 ];
        
        while(true)
        {
            int len = reader.read( buffer );
            if( len <= 0 )  break;
            result.append( buffer, 0, len );
        }
        
        return result.toString();
    }
    
    //---------------------------------------------------------------------------------------------
    
    public Test_web_service()  throws Exception
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
        URL url = new URL( "http://localhost:" + spooler.tcp_port() + "/webdienst" );
            

        HttpURLConnection http_connection = null;
        
        try
        {
            URLConnection connection = url.openConnection();
            if( !( connection instanceof HttpURLConnection ) )  throw new Exception( "Nur HTTP-URLs werden akzeptiert: url=" + url );

            http_connection = (HttpURLConnection)connection;
            http_connection.setRequestMethod( "POST" );
            http_connection.setRequestProperty( "Content-Type", "text/xml; charset=" + encoding );  //TODO scheduler_http.cxx muss charset verstehen!
            http_connection.setDoOutput( true );
            http_connection.connect();

            
            OutputStream        output_stream = http_connection.getOutputStream();
            OutputStreamWriter  writer        = new OutputStreamWriter( output_stream, encoding );

            writer.write( "<?xml version='1.0'?>\n" );
            writer.write( "<my_request>bla</my_request>\n" );
            writer.flush();
            output_stream.flush();
            

            int response_code = http_connection.getResponseCode();
            
            if( response_code != HttpURLConnection.HTTP_OK 
             && response_code != HttpURLConnection.HTTP_ACCEPTED )
                throw new Exception( http_connection.getResponseMessage() );
            
            //Object response = http_connection.getContent();
            spooler_log.info( string_from_reader( new InputStreamReader( http_connection.getInputStream(), encoding ) ) );        
        }
        finally
        {
            if( http_connection != null )  http_connection.disconnect();
        }

        return false;
    }
    
    //---------------------------------------------------------------------------------------------
}
