// $Id$

import java.math.BigDecimal;
import java.io.*;
import sos.hostware.Factory_processor;


public class Hostjava_test
{
    class Factory extends Thread
    {
        String name;
        int count;
        
        Factory( String name, int count ) { this.name = name; this.count = count; }
        
        public void run()
        {
          try
          {                    
            Factory_processor processor = new Factory_processor();
            processor.set_collect( 3 );
//System.err.print( "Bitte Eingabe drücken!\n" );
//int c = System.in.read();

            try
            {
                for( int i = 1; i <= count; i++ )
                {
                  //processor.set_db_name( "jdbc -class=oracle.jdbc.driver.OracleDriver jdbc:oracle:thin:@8of9.sos:1521:test -user=aok -password=aok" );
                    processor.set_language( "PerlScript" );
                  //processor.set_language( "VBScript" );              
                  //processor.set_language( "javascript" );
                    processor.set_template_filename( "/home/joacim/tmp/Bestellung.Perl.rtf" );
                  //processor.set_template_filename( "l:/prod/hostjava/maz1.rtf" );
                  //processor.set_document_filename( "/home/joacim/tmp/" + name );
                  //processor.set_template_filename( "/home/joacim/tmp/vorlage.rtf" );
                  //processor.set_template_filename( "s:/tmp/vorlage.rtf" );
                    if( processor.document_written() )  System.err.print( "real_document_filename=" + processor.real_document_filename() + "\n" );
                  //processor.set_document_filename( "doc.rtf" );
                  //processor.set_template_filename( "s:/test/factory/vorlage.rtf" );
                  //processor.set_template_filename( "c:/tmp/ISA_Fami_AngabenStiefkinderEnkel.rtf" );
                    processor.set_document_filename( "/tmp/test.###." + name );
                  //processor.set_param( "-rtf-single-lines" );
                    processor.set_parameter( "variable", "VARIABLENWERT" );
                    processor.set_parameter( "bool", true );
                    processor.set_parameter( "int", (int)4711 );
                    processor.set_parameter( "cy", new BigDecimal( "1234.5678" ) );
                    processor.set_parameter( "double", 47.11 );
                    processor.add_parameters();
/*
                  //processor.parse( "a=1" );
                  //processor.parse( "sub to_euro{ $_ / 1.95; }" );
                  //processor.parse( "$a=\"A\";" );
                  //processor.set_parameter( "rvnr", "RVNR1234" );
                    processor.set_collect( 7 );
                    System.err.print( "collect=" + processor.collect() + "\n" );
                    processor.set_template_dir( "c:/tmp" );
                    System.err.print( "template_dir=" + processor.template_dir() + "\n" );
                    processor.set_document_dir( "c:/TMP" );
                    System.err.print( "template_dir=" + processor.document_dir() + "\n" );
*/
                  //System.err.print( "script_text=" + processor.script_text() );

                    try
                    {
                        processor.set_merge_documents( true );
                        processor.process();
                        
                        //processor.set_language( "javascript" );
                        //processor.set_template_filename( "l:/tmp/vorlage.rtf" );
                        //processor.set_parameter( "variable", "VARIABLENWERT 222" );
                        //processor.process();
                        
                        processor.close_output_file();
                        
                        //processor.close();
                        if( processor.document_written() )  System.err.print( "real_document_filename=" + processor.real_document_filename() + "\n" );
                        
                        //System.err.print( "collected_documents_count=" + processor.collected_documents_count() + "\n" );
                        //System.err.print( "document_copied=" + processor.document_copied() + "\n" );
                        //System.err.print( "document_head_modified=" + processor.document_head_modified() + "\n" );
                        
                        //throw new Exception("XXX");
                    }
                    catch( Exception x )
                    {
                        System.err.print( x + "   last_step=" + processor.last_step() + "  template=" + processor.template_filename() + "  error_template=" + processor.error_filename()  + "  output=" + name + "\n" );

                        FileOutputStream f = new FileOutputStream( "/tmp/fehler.rtf" );
                        PrintStream ps = new PrintStream(f);
                        ps.print( processor.error_document() );
                        ps.close();
                        f.close();
                    }
                    
                    System.err.print( i + " ");
                    //System.gc();
                }            
            }
            catch( Throwable x )
            {
                System.err.print( x.toString() );
            }
            finally
            {
                processor.close();
            }
        
            System.err.print( name + " fertig.\n" );
          }
          catch( Exception x ) { System.err.print( "FEHLER  " + x + "\n" ); }
        }
        
    }
    


    public static void main( String[] args ) throws Throwable
    {
        new Hostjava_test().run( args );
        System.err.print( "FERTIG.\n" );
    }
    
    
    void run( String[] args )  throws Exception
    {
      switch( 4 )
      {
        case 1:
        {
            sos.hostware.Rerun rerun = new sos.hostware.Rerun();
            sos.hostware.File  file  = new sos.hostware.File();
            
            try
            {
                rerun.open( "/tmp/my_rerun_file" );            
                if( rerun.rerunning() )  System.out.print( "Rerunning\n" );
                
                file.open( "-in select a, 'text' t loop a between 1 and 10" );
                System.err.print( "eof=" + file.eof() + "\n" );                 
                
                while( !file.eof() )
                {
                    sos.hostware.Record record = file.get();
                    
                    //System.err.print( "field_exists(a)=" + record.field_exists("a") + "\n" );
                    //System.err.print( "field_exists(q)=" + record.field_exists("q") + "\n" );
                
                    if( rerun.process_next_record() )
                    {
                        System.out.print( rerun.record_number()  );
                        for( int i = 0; i < record.field_count(); i++ ) System.out.print( record.field_name(i) + "=" + record.string(i) + " " );
                        System.out.print( "\n" );
                        rerun.set_record_ok( rerun.record_number(), rerun.record_number() != 2 && rerun.record_number() != 3  ); 
                    }
                }
                
                if( rerun.ok() )  System.out.print( "Verarbeitung ok\n" );

                file.close();
                rerun.close();
            }
            finally
            {
                rerun.destruct();
                file.destruct();
            }
            
            break;
        }
            
        case 2:
        {
            sos.hostware.File file = new sos.hostware.File( "-out file /tmp/4" );
            
            try
            {
                file.put_line( "eins" );
                file.put_line( "zwei" );
                file.close();
                
                int s = 0;
                file = new sos.hostware.File( "-in select a, null n, 'text text text text text text text text text text text text text text text' t loop a between 1 and 100000" );
                file.read_null_as_empty();                
                
                while( !file.eof() )
                {
                    sos.hostware.Record record = file.get();
                    record.read_null_as_empty();
                    int len = record.string("n").length();
                    //for( int i = 0; i < record.field_count(); i++ )
                    //{
                    //    System.err.print( record.field_name(i) + "=" + record.string(i) + " " );
                    //}
                    //System.err.print( "\n" );
                    //System.err.print( record.xml() + "\n" );
                    //String a = record.string( "a" );  s += a.length();
                    //String t = record.string( "t" );  s += t.length();
                    //for( int i = 1; i <= 10; i++ )  {  String t = record.string( "t" );  s += t.length(); }
                    //System.err.print( "i=" + record.string("i") + "  t=" + record.string("t") + "\n" );
                    record.destruct();
                }
            }
            finally
            {
                file.close();
            }
            
            break;
        }
    
    
        case 3:
        {
            sos.hostware.Word_application word = new sos.hostware.Word_application();

            try
            {
                word.load();
                word.print( "c:/tmp/doc.rtf", "-printer='Kyocera FS-800' -pages='2'" );
            }
            finally
            {
                word.close();
            }
            
            break;
        }
     
       

        case 4:
        {                  
            Factory[] factories = new Factory[ 1 ];
            for( int i = 0; i < factories.length; i++ )  factories[i] = new Factory( "doc" + i + ".rtf", 1 );
            for( int i = 0; i < factories.length; i++ )  factories[i].start();
            for( int i = 0; i < factories.length; i++ )  factories[i].join();
            
            break;
        }      
        
        
        case 5:
        {         
            new sos.hostware.File( "-in /dev/null" );  // Um Hostole.log zu aktivieren
            
          //sos.hostware.Process_with_pid process = new sos.hostware.Process_with_pid( "cmd.exe /c pause" );
          //sos.hostware.Process_with_pid process = new sos.hostware.Process_with_pid( new String[]{ "cmd.exe", "/c", "pause Test" } );
          //sos.hostware.Process_with_pid process = new sos.hostware.Process_with_pid( new String[]{ "/bin/sh", "-c", "echo hallo2 && sleep 7; exit 7" } );
            sos.hostware.Process_with_pid process = args.length == 1? new sos.hostware.Process_with_pid( args[0] ) 
                                                                    : new sos.hostware.Process_with_pid( args );
            System.err.print( "pid=" + process.pid() + " " );
           //java.lang.Thread.sleep( 20000 ); process.try_kill_pid( process.pid() );
            //process.destroy();
            process.waitFor();
            System.err.print( "exitValue=" + process.exitValue() + "\n" );
            process.close();
            break;
        }
      }        
    }
}
