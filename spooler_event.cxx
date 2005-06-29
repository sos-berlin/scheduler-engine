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
        case evt_unknown:           return "unknown";
        case evt_scheduler_started: return "scheduler_started";
        case evt_task_ended:        return "task_ended";
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
    _object_iunknown( object->_my_iunknown ),
    _spooler(object->_spooler)
{
    assert( _object_iunknown );
}

//-----------------------------------------------------------------------Scheduler_event::send_mail

xml::Document_ptr Scheduler_event::dom()
{
    xml::Document_ptr event_dom;

    event_dom.create();
    event_dom.appendChild( event_dom.createProcessingInstruction( "xml", "version=\"1.0\"" ) );

    xml::Element_ptr scheduler_event_element = event_dom.createElement( "scheduler_event" );
    event_dom.appendChild( scheduler_event_element  );

    scheduler_event_element.setAttribute( "time"        , Time::now().xml_value() );
    scheduler_event_element.setAttribute( "event"       , name_of_event_code( _event_code ) );
    scheduler_event_element.setAttribute( "severity"    , name_of_log_level( _severity ) );
    scheduler_event_element.setAttribute( "severity_value", (int)_severity );
    scheduler_event_element.setAttribute( "object_type" , Scheduler_object::name_of_type_code( _object->scheduler_type_code() ) );
    scheduler_event_element.setAttribute( "log_file"    , _log_path );

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

    Z_LOG2( "joacim", event_dom.xml( true ) );

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


/*
    if( !mail_dom  ||  !mail_dom.has_node( "/mail/header" ) )
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
*/
    Z_LOG2( "joacim", mail_dom.xml( true ) );

    if( !mail_dom.has_node( "/mail/header" ) )
    {
        mail_dom.create();
        mail_dom.appendChild( event_dom.select_node( "/scheduler_event/mail" ).cloneNode(true) );
    }

    return mail_dom;
}

//-----------------------------------------------------------------------Scheduler_event::send_mail

int Scheduler_event::send_mail( const xml::Document_ptr& mail_dom_ )
{
    xml::Document_ptr mail_dom = mail_dom_? mail_dom_ : this->mail_dom();

    // MAIL SENDEN

    if( mail_dom.has_node( "/mail/header" ) )
    {
        Com_mail mail ( _spooler );
        mail.init();
        if( _mail->smtp() != "" )  mail.set_smtp( _mail->smtp() );
        mail.set_dom( mail_dom.select_node( "/mail" ) );
        return mail.send();
    }
    else
    if( _mail )
    {
        return _mail->send();
    }

    return 0; //?
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
