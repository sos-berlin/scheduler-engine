// $Id: spooler_task.cxx 3714 2005-06-16 08:30:05Z jz $


#include "spooler.h"
//#include "spooler_files.h"

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

string Scheduler_event::name_of_event_code( Event_code event_code )
{
    switch( event_code )
    {
        case evt_none:              return "none";
        case evt_task_state:        return "task_state";
        case evt_disk_full:         return "disk_full";
        case evt_database_error:    return "database_error";
        case evt_task_start_error:  return "task_start_error";
        default:                    return S() << "Event_code(" << event_code << ")";
    }
}

//-----------------------------------------------------------------Scheduler_event::Scheduler_event

Scheduler_event::Scheduler_event( Event_code event_code, Log_level severity, Scheduler_object* object )
:
    _event_code(event_code),
    _severity(severity),
    _object(object),
    _spooler(object->_spooler)
{
}

//-----------------------------------------------------------------------Scheduler_event::send_mail

xml::Document_ptr Scheduler_event::dom()
{
    xml::Document_ptr event_dom;

    event_dom.create();
    event_dom.appendChild( event_dom.createProcessingInstruction( "xml", "version=\"1.0\"" ) );

    xml::Element_ptr scheduler_event_element = event_dom.createElement( "scheduler_event" );
    event_dom.appendChild( scheduler_event_element  );

    scheduler_event_element.setAttribute( "time"    , Time::now().xml_value() );
    scheduler_event_element.setAttribute( "event"   , name_of_event_code( _event_code ) );
    scheduler_event_element.setAttribute( "severity", name_of_log_level( _severity ) );

    switch( _object->scheduler_type_code() )
    {
        case Scheduler_object::type_scheduler:
            break;

        case Scheduler_object::type_job:
        {
            scheduler_event_element.setAttribute( "job"     , static_cast<Job*>( +_object )->name() );
            break;
        }

        case Scheduler_object::type_task:
        {
            scheduler_event_element.setAttribute( "task"    , static_cast<Task*>( +_object )->id() );
            scheduler_event_element.setAttribute( "job"     , static_cast<Task*>( +_object )->job()->name() );
            break;
        }

        case Scheduler_object::type_order:     
            break;

        case Scheduler_object::type_job_chain: 
            break;

        default:                              
            break;
    }

    if( _error )
    append_error_element( scheduler_event_element, _error );

    scheduler_event_element.appendChild( _spooler->state_dom_element( event_dom, show_standard ) );


    if( _mail ) 
    {
        scheduler_event_element.appendChild( _mail->dom_element( event_dom ) );
    }

    return event_dom;
}

//------------------------------------------------------------------------Scheduler_event::mail_dom

xml::Document_ptr Scheduler_event::mail_dom( const xml::Document_ptr& event_dom_ )
{
    xml::Document_ptr event_dom = event_dom_? event_dom_ : dom();


    // STYLESHEET AUSFÜHREN

    xml::Document_ptr    mail_dom;
    ptr<Xslt_stylesheet> stylesheet;

    try
    {
        stylesheet = _mail->xslt_stylesheet();
        if( stylesheet )  mail_dom = stylesheet->apply( event_dom );
    }
    catch( exception& x )
    {
        _object->log()->warn( S() << "XSLT-Stylesheet: " << x );
    }    


    try
    {
        stylesheet = _object->mail_xslt_stylesheet();
        if( stylesheet )  mail_dom = stylesheet->apply( event_dom );
    }
    catch( exception& x )
    {
        _object->log()->warn( S() << "XSLT-Stylesheet: " << x );
    }    


    Z_LOG( mail_dom.xml() );

    if( !mail_dom  ||  mail_dom.select_nodes( "/mail/@*" ).count() == 0 )        // Kein <mail> oder <mail> ohne Attribute?
    {
        try
        {
            int STYLESHEET_LOAD_FILE_ERSETZEN;
            stylesheet = Z_NEW( Xslt_stylesheet );
            stylesheet->load_file( "s:/prod/scheduler/mail.xsl" );
            //stylesheet.load_xml( default_mail_xslt_stylesheet_xsl );        // spooler_http_files.cxx ==> spooler_files.cxx, spooler_files.h
            mail_dom = stylesheet->apply( event_dom );
        }
        catch( exception& x )
        {
            _object->log()->warn( S() << "Internes XSLT-Stylesheet: " << x );
        }
    }

    return mail_dom;
}

//-----------------------------------------------------------------------Scheduler_event::send_mail

int Scheduler_event::send_mail( const xml::Document_ptr& mail_dom_ )
{
    xml::Document_ptr mail_dom = mail_dom_? mail_dom_ : this->mail_dom();

    // MAIL SENDEN

    if( mail_dom )
    {
        Com_mail mail ( _spooler );
        mail.set_dom( mail_dom.select_node( "/mail" ) );
        return mail.send();
    }

    return 0; //?
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
