// $Id: Idispatch.java,v 1.4 2003/10/19 11:41:01 jz Exp $

package sos.spooler;

/**
 * Interne Klasse des Spoolers.
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.4 $
 */

public class Idispatch
{
/*
    class Name_context
    {
        private                     Name_context            ( int i )                               { value = i; }

        final Name_context          none                    = new Name_context(          0 );
        final Name_context          method                  = new Name_context( 0x00000001 );
        final Name_context          property_get            = new Name_context( 0x00000002 );
        final Name_context          method_or_property_get  = new Name_context( 0x00000003 );
        final Name_context          property_put            = new Name_context( 0x00000004 );
      //final Name_context          property_putref         = new Name_context( 0x00000008 );

        public int value = 0;
    }
*/
/*
    public class Method
    {
        public Method( Name_context context_, String name_ )
        {
            context = context_;
            name    = name_;
            id      = com_method_id( context, name );
        }

        public Variant call()
        {
            Variant[] values = new Variant[0];
            return com_call( _idispatch, id, context.value, values );
        }

        public Variant call( Variant value )
        {
            Variant[] values = new Variant[1];
            values[0] = value;
            return com_call( _idispatch, id, context.value, values );
        }

        Name_context            context;
        String                  name;
        int                     id;
    }
*/

    protected Idispatch( long idispatch )
    { 
        _idispatch = idispatch; 
    }


    final void com_clear()
    { 
        _idispatch = 0; 
    }


    Object com_call( String name )
    {
        Object[] params = new Object[0];
        return com_call( _idispatch, name, params );
    }


    boolean boolean_com_call( String name )
    {
        Object[] params = new Object[0];
        return ( (Boolean)com_call( _idispatch, name, params ) ).booleanValue();
    }


    boolean boolean_com_call( String name, Object par1 )
    {
        Object[] params = new Object[1];
        params[0] = par1;
        return ( (Boolean)com_call( _idispatch, name, params ) ).booleanValue();
    }

    int int_com_call( String name )
    {
        Object[] params = new Object[0];
        return ( (Integer)com_call( _idispatch, name, params ) ).intValue();
    }


    double double_com_call( String name )
    {
        Object[] params = new Object[0];
        return ( (Double)com_call( _idispatch, name, params ) ).doubleValue();
    }


    Object com_call( String name, int par1 )
    {
        Object[] params = new Object[1];
        params[0] = new Integer( par1 );
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, double par1 )
    {
        Object[] params = new Object[1];
        params[0] = new Double( par1 );
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, boolean par1 )
    {
        Object[] params = new Object[1];
        params[0] = new Boolean( par1 );
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1 )
    {
        Object[] params = new Object[1];
        params[0] = par1;
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1, Object par2 )
    {
        Object[] params = new Object[2];
        params[0] = par1;
        params[1] = par2;
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1, Object par2, Object par3 )
    {
        Object[] params = new Object[3];
        params[0] = par1;
        params[1] = par2;
        params[2] = par3;
        return com_call( _idispatch, name, params );
    }


    Object com_call( String name, Object par1, Object par2, Object par3, Object par4 )
    {
        Object[] params = new Object[4];
        params[0] = par1;
        params[1] = par2;
        params[2] = par3;
        params[3] = par4;
        return com_call( _idispatch, name, params );
    }


    static native Object        com_call                ( long idispatch, String name, Object[] params );

    private volatile long      _idispatch;
}
