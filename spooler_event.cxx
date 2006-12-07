// $Id$


#include "spooler.h"


namespace sos {
namespace spooler {

const string mail_xpath = "( /mail | /scheduler_event/mail )";

//-------------------------------------------------------------------------------------------------

string Scheduler_event::name_of_event_code( Scheduler_event_type event_code )
{
    switch( event_code )
    {
        case evt_none:                          return "none";
        case evt_unknown:                       return "unknown";
        case evt_scheduler_state_changed:       return "scheduler_state_changed";
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
        case evt_file_order_source_error:       return "file_order_source_error";
        case evt_file_order_source_recovered:   return "file_order_source_recovered";
        case evt_file_order_error:              return "file_order_error";
        case evt_order_state_changed:           return "order_state_changed";
        default:                                return S() << "Scheduler_event_type(" << event_code << ")";
    }
}

//-----------------------------------------------------------------Scheduler_event::Scheduler_event

Scheduler_event::Scheduler_event( Scheduler_event_type event_code, Log_level severity, Scheduler_object* object )
:
    _zero_(this+1),
    _spooler(object->_spooler),
    _event_code(event_code),
    _severity(severity),
    _object(object),
    _object_iunknown( object->_my_iunknown )
{
    assert( _object_iunknown );
}

//-----------------------------------------------------------------------Scheduler_event::timestamp
    
Time Scheduler_event::timestamp()
{
    if( !_timestamp )  _timestamp = Time::now();
    return _timestamp;
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

    scheduler_event_element.setAttribute( "time"          , timestamp().xml_value() );
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
            _object->log()->warn( x.what() );
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
            _object->log()->warn( x.what() );
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
        mail();

        xml::Document_ptr mail_dom;
        xml::Document_ptr event_dom;

        if( !mail_dom )
        {
            try
            {
                event_dom = this->dom();
                mail_dom = this->mail_dom( event_dom );
            }
            catch( exception& x )
            {
                _object->log()->warn( message_string( "SCHEDULER-334", x.what() ) );
            }
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
        Z_LOG2( "scheduler", __FUNCTION__ << ":\n" );
        _spooler->log()->warn( message_string( "SCHEDULER-302", x ) );
    }

    return 0; //?
}

//-------------------------------------------------Scheduler_event_manager::Scheduler_event_manager

Scheduler_event_manager::Scheduler_event_manager( Spooler* spooler ) 
: 
    Scheduler_object( spooler, this, Scheduler_object::type_scheduler_event_manager ),
    _zero_(this+1)
{
    _log = Z_NEW( Prefix_log( this, "Scheduler_event_manager" ) );
}

//------------------------------------------cheduler_event_manager::add_get_events_command_response

void Scheduler_event_manager::add_get_events_command_response( Get_events_command_response* response )
{
    _get_events_command_response_list.push_back( response );
}

//---------------------------------------cheduler_event_manager::remove_get_events_command_response

void Scheduler_event_manager::remove_get_events_command_response( Get_events_command_response* response )
{
    Z_FOR_EACH( Get_events_command_response_list, _get_events_command_response_list, it )
    {
        Get_events_command_response* r = *it;
        if( r == response )  
        {
            _get_events_command_response_list.erase( it );
            return;
        }
    }
}

//-----------------------------------------------------------------------------Scheduler_event::xml

void Scheduler_event::print_xml( ostream* s )
{
    *s << "<event";
    *s << " time=\"" << timestamp().xml_value() << '"';
    *s << " event_type=\"" << name_of_event_code( _event_code ) << "\"";

    //if( _object )
    //{
    //    _object->print_xml_attributes_for_event( s, this );
    //}

    *s << '>';
    int length_without_children = s->tellp(); 

    if( _object )
    {
        _object->print_xml_child_elements_for_event( s, this );
    }


    if( length_without_children == s->tellp() )
    {
        s->seekp( length_without_children - 1 );
        *s << "/>";
    }
    else
    {
        *s << "</event>";
    }
}

//------------------------------------------------------------------------Scheduler_event::make_xml

void Scheduler_event::make_xml()
{
    S s;
    print_xml( &s );
    _xml = s;

#   ifdef Z_DEBUG
        try { xml::Document_ptr doc = _xml; }
        catch( exception& x )  { _spooler->log()->error( S() << __FUNCTION__ << " " << x.what() << "\n" << _xml );  assert(("Scheduler_event::make_xml()",false)); }
#   endif
}

//--------------------------------------------------------------------Scheduler_event_manager::open

void Scheduler_event_manager::open()
{
    string filename;
/*
    if( _spooler->_directory != "*stderr" )
    {
        Sos_optional_date_time time = Time::now().as_time_t();
        string filename = _spooler->_directory;

        filename += "/scheduler-";
        filename += time.formatted( "yyyy-mm-dd-HHMMSS" );
        if( !_spooler->id().empty() )  filename += "." + _spooler->id();
        filename += ".events.xml";

        _event_file.open( filename, O_CREAT | O_TRUNC | O_WRONLY | O_NOINHERIT, 0666 );
    }
*/
}

//---------------------------------------------------------Scheduler_event_manager::close_responses

void Scheduler_event_manager::close_responses()
{
    Z_FOR_EACH( Get_events_command_response_list, _get_events_command_response_list, it )
    {
        Get_events_command_response* get_events_command_response = *it;

        get_events_command_response->close();
    }
}

//-------------------------------------------------------------------------------------------------

void Scheduler_event_manager::report_event( Scheduler_event* event )
{
    if( !_get_events_command_response_list.empty() )
    {
        event->make_xml();

        //event->_file_position = _event_file.tell();
        //_event_file_positions.set( event->id(), event->_file_position );


        if( _event_file.opened() )
        {
            //TODO Was tun wir bei einen Fehler, z.B. wenn die Platte erschöpft ist? *********************************************************
            _event_file.print( event->xml() );
            _event_file.print( "\n" );        // Platz für '\0' schaffen
        }

        Z_FOR_EACH( Get_events_command_response_list, _get_events_command_response_list, it )
        {
            Get_events_command_response* get_events_command_response = *it;

            if( get_events_command_response->is_event_selected( *event ) )
            {
                get_events_command_response->write_event( *event );
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------
/*
struct File_based_array_imlementation
{
                                    File_based_array_imlementation   ();
    virtual                        ~File_based_array_imlementation   ();

    void                        set_page_size               ( int );
  //void                        set_first_index             ( int );
    void                            page_out                ();
    void*                           page_in                 ( int64 index );
    void                        set_page_modified           ()                                      { _page_modified = true; }

  private:
    int                            _page_size;
    int64                          _first_index;
    int                            _items_per_page;
    int                            _item_size;
    int64                          _file_size;
    int64                          _item_count;
    int64                          _page_index;
    File                           _file;
    Byte*                          _page;
    bool                           _page_modified;
};

//-----------------------------------File_based_array_imlementation::File_based_array_imlementation

File_based_array_imlementation::File_based_array_imlementation( int item_size )
:
    _zero_(this+1),
    _item_size( item_size )
{
    //set_page_size( 4096 );
    set_page_size( 16 );
}

//----------------------------------File_based_array_imlementation::~File_based_array_imlementation
    
File_based_array_imlementation::~File_based_array_imlementation()
{
    delete [] _page;
}

//----------------------------------------------------File_based_array_imlementation::set_page_size
    
void File_based_array_imlementation::set_page_size( int page_size )
{
    if( _page )  z::throw_xc( __FUNCTION__ );
    if( page_size <= 0 )  z::throw_xc( __FUNCTION__, page_size );

    assert( _item_size > 0 );
    _item_per_page = ( page_size + _item_size - 1 ) / _item_size;
    _page_size     = _items_per_page * _item_size;
    _page = new Byte[ _page_size ];
}

//---------------------------------------------------------File_based_array_imlementation::page_out

void File_based_array_imlementation::page_out()
{
    if( _page_modified )
    {
        if( !_file.opened() )  _file.open_temporary();

        int byte_position = _page_index * _item_size;
        _file.seek( byte_position );
        _file.write( _page, _page_size );
        if( _file_size < byte_position )  _file_size = byte_position + _page_size;

        _page_modified = false;
    }
}

//----------------------------------------------------------File_based_array_imlementation::page_in

void* File_based_array_imlementation::page_in( int64 index )
{
    if( index < _page_index  ||  index >= _page_index + _items_per_page )
    {
        if( index < _first_index )  z::throw_xc( __FUNCTION__, index, _first_index );
        page_out();

        int new_page_index = index / _items_per_page * _items_per_page;

        if( index > _file_size * 8 )
        {
            memset( _page, 0, sizeof _page );
        }
        else
        {
            _file.seek( new_page_index );
            int length = _file.read( _page, _page_size );
            if( length != _page_size )  z::throw_xc( __FUNCTION__, "file " + _file.path() + " is truncated" );

            _page_index = new_page_index;
        }
    }

    return _page + ( index - _page_index ) * _item_size;
}

//-------------------------------------------------------------------------------------------------

template< typename T >
struct File_based_array : File_based_array_imlementation
{
                                    File_based_array        ()                                      {}
                                   ~File_based_array        ();

    const T&                        operator[]              ( int index )                           { return _base.get( index ); }
  //T&                              operator[]              ( int index )                           { return *get_ptr( index ); }

    const T&                        get                     ( int index )                           { return (const T*)_base.page_in( index ); }
    T*                              temporary_ptr           ( int index )                           { T* result = (T*)_base.page_in( index ); _base.set_page_modified(); return result; }
    void                            set                     ( int index, const T& o )               { *temporary_ptr( index ) = o; }

  private:
    File_based_array_imlementation          _base;
};

//-----------------------------------------------------------File_based_array<T>::~File_based_array

template< typename T>
File_based_array<T>::~File_based_array()
{
    delete _page;
}

//---------------------------------------------------------------------------File_based_array<bool>

struct File_based_array<bool>
{
    const bool                      operator[]              ( int64 index ) const                     { return get( index ); }

    bool                            get                     ( int64 index );
    void                            set                     ( int64 index, bool );

    File_based_array<Byte>         _byte_array;
};

//----------------------------------------------------------------------File_based_array<bool>::get

const bool File_based_array<bool>::get( int64 index ) const
{ 
    Byte byte = _byte_array.get( index >> 3 );
    Byte mask = (byte)1 << ( index & 3 ) );

    return ( byte & mask ) != 0;
}

//---------------------------------------------------------------------------File_based_bitmap::set

void File_based_bitmap::set( int64 index, bool value )
{
    Byte* byte = &_byte_array->temporary_ptr( index >> 3 );
    Byte  mask = (Byte)1 << ( index & 7 ):

    if( value )  *byte |= mask;
           else  *byte &= ~mask;
}
*/
//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
