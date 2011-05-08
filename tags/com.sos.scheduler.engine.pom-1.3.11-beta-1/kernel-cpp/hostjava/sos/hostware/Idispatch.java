// $Id$


package sos.hostware;


public class Idispatch
{
    Idispatch( String name )  //throws Exception
    { 
        _idispatch = com_construct( name ); 
    }
    
    
    Idispatch( long idispatch )  //throws Exception
    { 
        _idispatch = idispatch; 
    }


    Idispatch()
    {
        _idispatch = 0;
    }
    
    protected void finalize()  throws Throwable
    {
        try
        {
            com_close();
        }
        finally
        {
            super.finalize();
        }
    }

    private final void com_clear()
    { 
        _idispatch = 0; 
    }
    
    
    public void destruct()
    {
        try
        {
            com_close();
        }
        catch( Exception x ) 
        {
            // Fehler ignorieren wir, damit finalizer ungestört arbeiten können.
        }
    }


    final void com_close()  //throws Exception
    { 
        if( _idispatch != 0 )
        {
            long i = _idispatch;
            _idispatch = 0; 
            com_release( i );
        }
    }


    final void com_close( String method )  //throws Exception
    {
        if( _idispatch != 0 )
        {
            try
            {
                com_call( method );
            }
            finally
            {
                com_close();
            }
        }        
    }
    

    Object com_call( int dispid, int dispatch_context )  //throws Exception
    {
        return com_call( _idispatch, dispid, dispatch_context, null );
    }


    Object com_call( String name )  //throws Exception
    {
        return com_call( _idispatch, name, null );
    }


    boolean boolean_com_call( String name )  //throws Exception
    {
        return boolean_com_call( _idispatch, name, null );
    }


    int int_com_call( String name )  //throws Exception
    {
        return ( (Integer)com_call( _idispatch, name, null ) ).intValue();
    }


    int int_com_call( String name, Object par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = par1;
        return ( (Integer)com_call( _idispatch, name, params ) ).intValue();
    }


    double double_com_call( String name )  //throws Exception
    {
        return ( (Double)com_call( _idispatch, name, null ) ).doubleValue();
    }


    String string_com_call( String name )  //throws Exception
    {
        return string_com_call( _idispatch, name, null );
    }


    String string_com_call( String name, Object par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = par1;
        return string_com_call( _idispatch, name, params );
    }


    String string_com_call( String name, int par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = new Integer( par1 );
        return string_com_call( _idispatch, name, params );
    }


    String string_com_call( String name, String par1 )  //throws Exception
    {
        return string_com_call_string( _idispatch, name, par1 );
    }


    Object com_call( String name, int par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = new Integer( par1 );
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, double par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = new Double( par1 );
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, boolean par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = new Boolean( par1 );
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object[] parameters )  //throws Exception
    {
        return com_call( _idispatch, name, parameters );
    }


    Object com_call( String name, Object par1 )  //throws Exception
    {
        Object[] params = new Object[1];
        params[0] = par1;
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1, Object par2 )  //throws Exception
    {
        Object[] params = new Object[2];
        params[0] = par1;
        params[1] = par2;
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1, Object par2, Object par3 )  //throws Exception
    {
        Object[] params = new Object[3];
        params[0] = par1;
        params[1] = par2;
        params[2] = par3;
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1, Object par2, Object par3, Object par4 )  //throws Exception
    {
        Object[] params = new Object[4];
        params[0] = par1;
        params[1] = par2;
        params[2] = par3;
        params[3] = par4;
        return com_call( _idispatch, name, params );
    }


    boolean com_valid()
    {
        return _idispatch != 0;
    }

    private static native Object            com_call           ( long idispatch, String name, Object[] params );
    private static native boolean   boolean_com_call           ( long idispatch, String name, Object[] params );
    private static native String     string_com_call           ( long idispatch, String name, Object[] params );
    private static native String     string_com_call_string    ( long idispatch, String name, String par1 );

    private static        Object            com_call           ( long idispatch, int dispid, int dispatch_context, Object[] params )  { return         com_call_id       ( idispatch, dispid, dispatch_context, params ); }
    private static        boolean   boolean_com_call           ( long idispatch, int dispid, int dispatch_context, Object[] params )  { return boolean_com_call_id       ( idispatch, dispid, dispatch_context, params ); }
    private static        String     string_com_call           ( long idispatch, int dispid, int dispatch_context, Object[] params )  { return  string_com_call_id       ( idispatch, dispid, dispatch_context, params ); }
    private static        String     string_com_call_string    ( long idispatch, int dispid, int dispatch_context, String par1     )  { return  string_com_call_id_string( idispatch, dispid, dispatch_context, par1   ); }

    private static native long              com_construct      ( String class_name );
    private static native long              com_release        ( long idispatch );
    private static native Object            com_call_id        ( long idispatch, int dispid, int dispatch_context, Object[] params );
    private static native boolean   boolean_com_call_id        ( long idispatch, int dispid, int dispatch_context, Object[] params );
    private static native String     string_com_call_id        ( long idispatch, int dispid, int dispatch_context, Object[] params );
    private static native String     string_com_call_id_string ( long idispatch, int dispid, int dispatch_context, String par1 );
    private static native int               com_get_dispid     ( long idispatch, String name );
  //public  static native long              com_object_count   ();

    private volatile long          _idispatch;
    
    
    static final int dispatch_method         = 1;
    static final int dispatch_propertyget    = 2;
    static final int dispatch_propertyput    = 4;
    static final int dispatch_propertyputref = 8;

    //---------------------------------------------------------------------------------------------

    static boolean is_module_loaded()
    {
        return java.lang.System.getProperty( "os.name" ).equals( "HP-UX" );

        /*    
        try
        {
            Idispatch.class.getMethod( "com_construct", new Class[]{ String.class } );
            return true;
        }
        catch( NoSuchMethodException x )
        {
            return false;
        }
        */
    }

    //---------------------------------------------------------------------------------------------

    static void load_module()
    {
        if( !is_module_loaded() )
        {
            System.loadLibrary( "hostjava" );
        }
    }    

    //---------------------------------------------------------------------------------------------

    static
    {
        Idispatch.load_module();
    }
};
