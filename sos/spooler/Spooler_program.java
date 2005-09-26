// $Id

package sos.spooler;

/**
 * Applikationsklasse zum Start im Java-Debugger.
 *
 * Nur für Windows.
 * Die Klasse erwartet den Scheduler als nachladbares Modul in der Datei <code>scheduler.dll</code>,
 * deren Verzeichnis in <code>java.library.path</code> 
 * oder in der Umgebungsvariable <code>PATH</code> aufgeführt sein muss.
 *
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version $Revision$
 */

public class Spooler_program
{
    /** Start des Schedulers mit Kommandozeile als String.
      * Der erste Parameter bezeichnet den Pfad der zu der <code>scheduler.dll</code> passenden <code>scheduler.exe</code>.
     */
    
    public                  Spooler_program         ( String parameters )               { construct( parameters ); }


    /** Start des Schedulers mit Kommandozeile als Array.
      * Der erste Parameter bezeichnet den Pfad der zu der <code>scheduler.dll</code> passenden <code>scheduler.exe</code>.
     */
    
    public                  Spooler_program         ( String parameters[] )             { construct_argv( parameters ); }

    private native void     construct               ( String parameters );
    private native void     construct_argv          ( String[] parameters );


    /**
     * Die Parameter werden dem Scheduler übergeben wie beim Aufruf der normalen Variante über die Kommandozeile.
     */
     
    public static void main( String[] parameters )
    {
        String[] p = new String[ parameters.length + 1 ];

        p[0] = "scheduler.exe";
        for( int i = 0; i < parameters.length; i++ )  p[i+1] = parameters[i];

        new Spooler_program( p );
    }

    static
    {
        System.loadLibrary( "scheduler" );
    }
}
