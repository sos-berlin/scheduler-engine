// $Id

package sos.spooler;

/**
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.4 $
 */

public class Spooler_program
{
    public                  Spooler_program         ( String parameters )               { construct( parameters ); }
    public                  Spooler_program         ( String parameters[] )             { construct_argv( parameters ); }

    private native void     construct               ( String parameters );
    private native void     construct_argv          ( String[] parameters );

    public static void main( String[] parameters )
    {
        String[] p = new String[ parameters.length + 1 ];

        p[0] = "scheduler";
        for( int i = 0; i < parameters.length; i++ )  p[i+1] = parameters[i];

        new Spooler_program( p );
    }

    static
    {
        System.loadLibrary( "scheduler" );
    }
}
