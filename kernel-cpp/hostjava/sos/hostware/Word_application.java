// $Id$


package sos.hostware;


public class Word_application extends Idispatch
{
    public Word_application()
    { 
        super( "hostware.Word_application" );
    }


    public final void           close                       ()                                      {                   com_close(); }
    public int                  kill_all_words              ()                                      { return        int_com_call( "kill_all_words" ); }
    public void                 load                        ()                                      {                   com_call( "load" ); }
  //public Idispatch            app                         ()                                      { return (Idispatch)com_call( "<app" ); }
    public void                 print                       ( String filename, String parameters )  {                   com_call( "print", filename, parameters ); };


    static
    {
        Idispatch.load_module();
    }
};
