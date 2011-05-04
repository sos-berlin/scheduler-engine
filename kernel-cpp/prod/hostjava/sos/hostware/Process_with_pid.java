// $Id$

package sos.hostware;

import java.lang.*;
import java.io.*;


/**
 * @author Joacim Zschimmer
 */

public class Process_with_pid  extends java.lang.Process
{
    public Process_with_pid()
    { 
        _reference = construct();
    }

    public Process_with_pid( String command_line )  throws Exception
    { 
        _reference = construct();
        start( _reference, command_line );
    }


    public Process_with_pid( String[] program_and_parameters )  throws Exception
    { 
        _reference = construct();
        start_array( _reference, program_and_parameters );
    }


    
    protected void finalize()  throws Throwable
    {
        try
        {
            close();
        }
        finally
        {
            super.finalize();
        }
    }


    
    public void close()
    {
        if( _reference != 0 )
        {
            try
            {
                close( _reference );
            }
            finally
            {
                _reference = 0;
            }
        }
    }


    public void start( String command_line )
    {
        start( _reference, command_line );
    }
    
    public void set_own_process_group( boolean b )
    {
        set_own_process_group( _reference, b );
    }
    

    public void                 destroy                     ()                          {        destroy   ( _reference ); }
    public int                  waitFor                     ()                          { return waitFor   ( _reference ); }
    public boolean              terminated                  ()                          { return terminated( _reference ); }
    public int                  exitValue                   ()                          { return exitValue ( _reference ); }
    public int                  pid                         ()                          { return pid       ( _reference ); }

    public        InputStream   getErrorStream              ()                          { throw new Error( "getErrorStream() ist nicht implementiert" ); }      
    public        InputStream   getInputStream              ()                          { throw new Error( "getInputStream() ist nicht implementiert" ); }      
    public        OutputStream  getOutputStream             ()                          { throw new Error( "getOutputStream() ist nicht implementiert" ); }      



    private native long         construct                   ();
    private native void         destroy                     ( long reference );
    public  native void         start                       ( long reference, String command_line );
    private native void         start_array                 ( long reference, String[] program_and_parameters );
    private native void         close                       ( long reference );
    private native int          waitFor                     ( long reference );
    private native boolean      terminated                  ( long reference );
    private native int          exitValue                   ( long reference );
    private native int          pid                         ( long reference );
    public  static native void  kill_pid                    ( int pid );
    public  static native boolean try_kill_pid              ( int pid );
    private native void     set_own_process_group           ( long reference, boolean b );
    
    private long               _reference                   = 0;


    static
    {
        Idispatch.load_module();
    }
};
