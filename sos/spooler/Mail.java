// $Id: Mail.java,v 1.1 2002/11/09 09:13:17 jz Exp $

package sos.spooler;

/**
 * Überschrift:
 * Beschreibung:
 * Copyright:     Copyright (c) 2002, SOS GmbH Berlin
 * Organisation:  SOS GmbH Berlin
 * @author Joacim Zschimmer, Zschimmer GmbH
 * @version 1.0
 */

public class Mail extends Idispatch
{
    private             Mail                ( long idispatch )                              { super(idispatch); }

    public void     set_to                  ( String receipients )                          {                   com_call( ">to", receipients        ); }
    public String       to                  ()                                              { return (String)   com_call( "<to"                     ); }

    public void     set_from                ( String from )                                 {                   com_call( ">from", from             ); }
    public String       from                ()                                              { return (String)   com_call( "<from"                   ); }

    public void     set_cc                  ( String receipients )                          {                   com_call( ">cc", receipients        ); }
    public String       cc                  ()                                              { return (String)   com_call( "<cc"                     ); }

    public void     set_bcc                 ( String receipients )                          {                   com_call( ">bcc", receipients       ); }
    public String       bcc                 ()                                              { return (String)   com_call( "<bcc"                    ); }

    public void     set_subject             ( String text )                                 {                   com_call( ">subject", text          ); }
    public String       subject             ()                                              { return (String)   com_call( "<subject"                ); }

    public void     set_body                ( String text )                                 {                   com_call( ">body", text             ); }
    public String       body                ()                                              { return (String)   com_call( "<body"                   ); }

    public void         add_file            ( String real_filename, String mail_filename, 
                                              String content_type, String encoding )        {                   com_call( "add_file", real_filename, mail_filename,
                                                                                                                                      content_type, encoding ); }

    public void         add_file            ( String real_filename, String mail_filename, 
                                              String content_type )                         {                   com_call( "add_file", real_filename, mail_filename, content_type ); }

    public void         add_file            ( String real_filename, String mail_filename )  {                   com_call( "add_file", real_filename, mail_filename ); }

    public void         add_file            ( String real_filename )                        {                   com_call( "add_file", real_filename ); }

    public void     set_smtp                ( String hostname )                             {                   com_call( ">smtp", hostname         ); }
    public String       smtp                ()                                              { return (String)   com_call( "<smtp"                   ); }

    public void     set_queue_dir           ( String directory )                            {                   com_call( ">queue_dir", directory   ); }
    public String       queue_dir           ()                                              { return (String)   com_call( "<queue_dir"              ); }

    public void         add_header_field    ( String field_name, String value )             {                   com_call( "add_header_field", field_name, value ); }

    public int          dequeue             ()                                              { return        int_com_call( "dequeue"                 ); }
}
