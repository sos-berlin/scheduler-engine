// $Id

package sos.spooler;

/**
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision: 1.3 $
 */

public class Spooler_program
{
    public                  Spooler_program         ( String parameters )               { construct( parameters ); }
    public                  Spooler_program         ( String parameters[] )             { construct_argv( parameters ); }

    private native void     construct               ( String parameters );
    private native void     construct_argv          ( String[] parameters );

    public static int main( String[] parameters )
    {
        new Spooler_program( "" + parameters );
        return 0;
    }

    static
    {
        System.loadLibrary( "scheduler" );
    }
}
