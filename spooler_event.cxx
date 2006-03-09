// $Id: spooler_task.cxx 3714 2005-06-16 08:30:05Z jz $


#include "spooler.h"


namespace sos {
namespace spooler {

const string mail_xpath = "( /mail | /scheduler_event/mail )";

//-------------------------------------------------------------------------------------------------

string Scheduler_event::name_of_event_code( Event_code event_code )
{
    switch( event_code )
    {
        case evt_none:                          return "none";
        case evt_unknown:                       return "unknown";
        case evt_scheduler_started:             return "scheduler_started";
        case evt_scheduler_fatal_error:         return "scheduler_fatal_error";
        case evt_scheduler_kills:               return "scheduler_kills";
        case evt_job_error:                     return "job_error";
        case evt_task_ended:                    return "task_ended";
        case evt_disk_full:                     return "disk_full";
        case evt_database_error:                return "database_error";
        case evt_database_error_switch_to_file: return "database_error_switch_to_file";
        case evt_database_error_abort:          return "database_error_abort";
        case evt_database_continue:             return "database_continue";
        case evt_task_start_error:              return "task_start_error";
        default:                                return S() << "Event_code(" << event_code << ")";
    }
}

//-----------------------------------------------------------------Scheduler_event::Scheduler_event

Scheduler_event::Scheduler_event( Event_code event_code, Log_level severity, Scheduler_object* object )
:
    _zero_(this+1),
    _spooler(object->_spooler),
    _event_code(event_code),
    _timestamp( Time::now() ),
    _severity(severity),
    _object(object),
    _object_iunknown( object->_my_iunknown )
{
    assert( _object_iunknown );
}

//-----------------------------------------------------------------------------Scheduler_event::dom

xml::Document_ptr Scheduler_event::dom()
{
    Show_what           state_show = show_standard;
    xml::Document_ptr   event_dom;

    event_dom.create();
    event_dom.appendChild( event_dom.createProcessingInstruction( "xml", "version=\"1.0\"" ) );

    xml::Element_ptr scheduler_event_element = event_dom.createElement( "scheduler_event" );
    event_dom.appendChild( scheduler_event_element  );

    scheduler_event_element.setAttribute( "time"          , _timestamp.xml_value() );
    scheduler_event_element.setAttribute( "event"         , name_of_event_code( _event_code ) );
    scheduler_event_element.setAttribute( "severity"      , name_of_log_level( _severity ) );
    scheduler_event_element.setAttribute( "severity_value", (int)_severity );

    if( _scheduler_terminates )
    scheduler_event_element.setAttribute( "scheduler_terminates", _scheduler_terminates? "yes" : "no" );

    scheduler_event_element.setAttribute( "object_type" , Scheduler_object::name_of_type_code( _object->scheduler_type_code() ) );
    scheduler_event_element.setAttribute_optional( "log_file"    , _log_path );

    if( _count )
    scheduler_event_element.setAttribute( "count", _count );


    switch( _object->scheduler_type_code() )
    {
        case Scheduler_object::type_scheduler:
        {
            state_show |= show_jobs | show_tasks;
            break;
        }

        case Scheduler_object::type_job:
        {
            Job* job = static_cast<Job*>( +_object );
            scheduler_event_element.setAttribute( "job"     , job->name() );
            state_show |= show_jobs | show_tasks;
            state_show._job_name = job->name();
            break;
        }

        case Scheduler_object::type_task:
        {
            Task* task = static_cast<Task*>( +_object );
            scheduler_event_element.setAttribute( "task"    , task->id() );
            scheduler_event_element.setAttribute( "job"     , task->job()->name() );
            state_show |= show_jobs | show_tasks;
            state_show._job_name = task->job()->name();
            state_show._task_id  = task->id();
            break;
        }

        case Scheduler_object::type_order:     
            break;

        case Scheduler_object::type_job_chain: 
            break;

        default:                              
            break;
    }


    scheduler_event_element.setAttribute_optional( "message", _message != ""? _message : _error.what() );

    if( _error )
    append_error_element( scheduler_event_element, _error );

    scheduler_event_element.appendChild( _spooler->state_dom_element( event_dom, state_show ) );


    if( _mail ) 
    {
        scheduler_event_element.appendChild( _mail->dom_element( event_dom ) );
    }

    //Z_LOG2( "joacim", event_dom.xml( true ) );

    return event_dom;
}

//------------------------------------------------------------------------Scheduler_event::mail_dom

xml::Document_ptr Scheduler_event::mail_dom( const xml::Document_ptr& event_dom_ )
{
    xml::Document_ptr event_dom = event_dom_? event_dom_ : dom();


    // STYLESHEET AUSFÜHREN

    xml::Document_ptr    mail_dom;
    ptr<Xslt_stylesheet> stylesheet;

    if( _mail )
    {
        try
        {
            stylesheet = _mail->xslt_stylesheet();
            if( stylesheet )  mail_dom = stylesheet->apply( event_dom );
        }
        catch( exception& x )
        {
            _object->log()->warn( S() << "XSLT-Stylesheet: " << x );
        }    
    }

    if( !mail_dom )
    {
        try
        {
            stylesheet = _object->mail_xslt_stylesheet();
            if( stylesheet )  mail_dom = stylesheet->apply( event_dom );
        }
        catch( exception& x )
        {
            _object->log()->warn( S() << "XSLT-Stylesheet: " << x );
        }    
    }

    if( !mail_dom.has_node( mail_xpath ) )
    {
        if( xml::Element_ptr mail_element = event_dom.select_node( mail_xpath ) )
        {
            mail_dom.create();
            mail_dom.appendChild( mail_element.cloneNode(true) );
        }
    }

    return mail_dom;
}

//----------------------------------------------------------------------------Scheduler_event::mail

Com_mail* Scheduler_event::mail()
{
    if( !_mail )
    {
        ptr<Com_mail> mail = new Com_mail( _spooler );
        mail->init();

        set_mail( mail );
    }

    return _mail;
}

//------------------------------------------------------------------------Scheduler_event::set_mail

void Scheduler_event::set_mail( Com_mail* mail )
{
    switch( _object->scheduler_type_code() )
    {
        case Scheduler_object::type_job:
        {
            Job* job = static_cast<Job*>( +_object );
            mail->add_header_field( "X-SOS-Spooler-Job", job->name() );
            break;
        }

        case Scheduler_object::type_task:
        {
            Task* task = static_cast<Task*>( +_object );
            mail->add_header_field( "X-SOS-Spooler-Job", task->job()->name() );
            break;
        }

        default:                              
            mail->add_header_field( "X-SOS-Spooler", "" );
            break;
    }

    _mail = mail;
}

//-----------------------------------------------------------------------Scheduler_event::send_mail

int Scheduler_event::send_mail( const Mail_defaults& mail_defaults )
{
    try
    {
        //if( !_mail )
        {
            mail();
            //_mail->set_from_name( _spooler->name() );
            //_mail->add_header_field( "X-SOS-Spooler", "" );
        }

        xml::Document_ptr mail_dom;
        xml::Document_ptr event_dom;

        if( !mail_dom )
        {
            event_dom = this->dom();
            mail_dom = this->mail_dom( event_dom );
        }


        // MAIL SENDEN

        ptr<Com_mail> mail;

        if( xml::Element_ptr mail_element = mail_dom? mail_dom.select_node( mail_xpath ) : NULL )
        {
            if( !mail_element.bool_getAttribute( "suppress" ) )
            {
                mail = new Com_mail( _spooler );
                mail->init();
                if( _mail  &&  _mail->smtp() != "" )  mail->set_smtp( _mail->smtp() );
                mail->set_dom( mail_element );
            }
        }
        else
        {
            mail = _mail;
        }


        if( mail )
        {
            if( mail_dom  &&  mail_dom.documentElement()  &&  mail_dom.documentElement().bool_getAttribute( "attach_xml" ) )
            {
                if( event_dom )
                mail->add_attachment( event_dom.xml(true), "event.xml", "text/xml", "quoted-printable" );

                mail->add_attachment( mail_dom.xml(true), "mail.xml", "text/xml", "quoted-printable" );
            }


            return mail->send( mail_defaults );
        }
    }
    catch( exception& x )
    {
        Z_LOG( __FUNCTION__ << ": " << x.what() );
        _spooler->log()->warn( message_string( "SCHEDULER-302", x ) );
    }

    return 0; //?
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
