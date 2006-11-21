// $Id$

/*
    VERBESSERUNG:

    request_order() durch ein Abonnement ersetzen: 
    Job oder Task kann Aufträge abonnieren und das Abonnement auch wieder abbestellen.
    Dann wird das Verzeichnis außerhalb der <run_time/> nicht überwacht.

    Wir brauchen ein Verzeichnis der Abonnementen (struct Job/Task : Order_source_abonnent)
*/


#include "spooler.h"

using stdext::hash_set;
using stdext::hash_map;

namespace sos {
namespace spooler {

//--------------------------------------------------------------------------------------------const

const string    scheduler_file_path_variable_name           = "scheduler_file_path";
const string    file_order_sink_job_name                    = "scheduler_file_order_sink";
const int       delay_after_error_default                   = INT_MAX;
const int       file_order_sink_job_idle_timeout_default    = 60;
const int       directory_file_order_source_max_default     = 100;      // Nicht zuviele Aufträge, sonst wird der Scheduler langsam (in remove_order?)
const int       max_tries                                 = 2;        // Nach Fehler machen wie sofort einen zweiten Versuch

#ifdef Z_WINDOWS
    const int   directory_file_order_source_repeat_default  = 60;
#else
    const int   directory_file_order_source_repeat_default  = 10;
#endif

//-------------------------------------------------------------------------------------------static

//Directory_file_order_source::Class_descriptor    Directory_file_order_source::class_descriptor ( &typelib, "Spooler.Directory_file_order_source", Directory_file_order_source::_methods );

//------------------------------------------------------------------File_order_sink_module_instance

struct File_order_sink_module_instance : Internal_module_instance
{
    File_order_sink_module_instance( Module* m ) 
    : 
        Internal_module_instance( m ) 
    {
    }



    bool spooler_process()
    {
        bool   result = false;
        Order* order  = _task->order();

        if( !order )  return false;         // Fehler

        File_path path = string_from_variant( order->param( scheduler_file_path_variable_name ) );
        if( path == "" )
        {
            _log->warn( message_string( "SCHEDULER-343", order->obj_name() ) );
            result = false;
        }
        else
        {
            Job_chain_node* job_chain_node = order->job_chain_node();
            if( !job_chain_node )  z::throw_xc( __FUNCTION__ );

            if( !file_exists( path ) )
            {
                _log->warn( message_string( "SCHEDULER-339", path ) );
                result = false;
            }
            else
            {
                try
                {
                    if( job_chain_node->_file_order_sink_move_to != "" ) 
                    {
                        order->log()->info( message_string( "SCHEDULER-980", path, job_chain_node->_file_order_sink_move_to ) );

                        path.move_to( job_chain_node->_file_order_sink_move_to );

                        result = true;
                    }
                    else
                    if( job_chain_node->_file_order_sink_remove )                
                    {
                        order->log()->info( message_string( "SCHEDULER-979", path ) );

                        path.unlink();

                        result = true;
                    }
                    else
                        z::throw_xc( __FUNCTION__ );
                }
                catch( exception& x )
                {
                    order->log()->warn( x.what() );     // Nicht error(), damit der Job nicht stoppt
                }

                if( result == false )  order->add_to_blacklist();
            }

            //if( result == true  &&  job_chain_node->_next_state == job_chain_node->_state )  
            order->set_finished();  //remove_from_job_chain();
        }

        return result;
    }
};

//---------------------------------------------------------------------------File_order_sink_module

struct File_order_sink_module : Internal_module
{
    File_order_sink_module( Spooler* spooler )
    :
        Internal_module( spooler )
    {
    }


    ptr<Module_instance> create_instance_impl()
    { 
        ptr<File_order_sink_module_instance> result = Z_NEW( File_order_sink_module_instance( this ) );  
        return +result;
    }
};

//------------------------------------------------------------------------------File_order_sink_job

struct File_order_sink_job : Internal_job
{
    File_order_sink_job( Spooler* spooler )
    :
        Internal_job( file_order_sink_job_name, +Z_NEW( File_order_sink_module( spooler ) ) )
    {
    }
};

//--------------------------------------------------------------------Spooler::init_file_order_sink

void Spooler::init_file_order_sink()
{
    ptr<File_order_sink_job> file_order_sink_job = Z_NEW( File_order_sink_job( _spooler ) );
    file_order_sink_job->set_visible( false );
    file_order_sink_job->set_order_controlled();
    file_order_sink_job->set_idle_timeout( file_order_sink_job_idle_timeout_default );
    add_job( +file_order_sink_job, true );

    // Der Scheduler führt Tasks des Jobs scheduler_file_order_sink in jedem Scheduler-Schritt aus,
    // damit sich die Aufträge nicht stauen (Der interne Job läuft nicht in einem eigenen Prozess)
    // Siehe Spooler_thread::step().
}

//-----------------------------------------Directory_file_order_source::Directory_file_order_source

Directory_file_order_source::Directory_file_order_source( Job_chain* job_chain, const xml::Element_ptr& element )
:
    //Idispatch_implementation( &class_descriptor ),
    Order_source( job_chain, type_directory_file_order_source ),
    _zero_(this+1),
    _delay_after_error(delay_after_error_default),
    _repeat(directory_file_order_source_repeat_default),
    _max_orders(directory_file_order_source_max_default)
{
    _path = element.getAttribute( "directory" );

    _regex_string = element.getAttribute( "regex" );
    if( _regex_string != "" )
    {
        _regex.compile( _regex_string );
    }

    _delay_after_error = element.int_getAttribute( "delay_after_error", _delay_after_error );

    if( element.getAttribute( "repeat" ) == "no" )  _repeat = INT_MAX;
                                              else  _repeat = element.int_getAttribute( "repeat", _repeat );

    _max_orders        = element.int_getAttribute( "max", _max_orders );
    _next_state        = normalized_state( element.getAttribute( "next_state", _next_state.as_string() ) );
}

//----------------------------------------Directory_file_order_source::~Directory_file_order_source
    
Directory_file_order_source::~Directory_file_order_source()
{
    //if( _spooler->_connection_manager )  _spooler->_connection_manager->remove_operation( this );

    close();
}

//--------------------------------------------------------------Directory_file_order_source::finish

void Directory_file_order_source::close()
{
    close_notification();

    if( _next_job ) 
    {
        _next_job->unregister_order_source( this );
        _next_job = NULL;
    }

    _job_chain = NULL;   // close() wird von ~Job_chain gerufen, also kann Job_chain ungültig sein
}

//-------------------------------------------------------------xml::Element_ptr Job_chain_node::xml

xml::Element_ptr Directory_file_order_source::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr element = document.createElement( "file_order_source" );

                                             element.setAttribute         ( "directory"  , _path );
                                             element.setAttribute_optional( "regex", _regex_string );
        if( _notification_event._signaled )  element.setAttribute         ( "signaled", "yes" );
        if( _max_orders < INT_MAX )          element.setAttribute         ( "max", _max_orders );
        if( !_next_state.is_missing() )      element.setAttribute         ( "next_state", debug_string_from_variant( _next_state ) );

        if( delay_after_error() < INT_MAX )  element.setAttribute( "delay_after_error", delay_after_error() );
        if( _repeat             < INT_MAX )  element.setAttribute( "repeat"           , _repeat);
        
        if( _directory_error )  append_error_element( element, _directory_error );

        if( _new_files_index < _new_files.size() )
        {
            xml::Element_ptr files_element = document.createElement( "files" );
            files_element.setAttribute( "snapshot_time", _new_files_time.as_string() );
            files_element.setAttribute( "count"        , (int)_new_files.size() );

            if( show & show_order_source_files )
            {
                for( int i = _new_files_index, j = show._max_orders; i < _new_files.size() && j > 0; i++, j-- )
                {
                    z::File_info* f = _new_files[ i ];

                    xml::Element_ptr file_element = document.createElement( "file" );
                    file_element.setAttribute( "last_write_time", Time( f->last_write_time() ).xml_value( Time::without_ms ) + "Z" );
                    file_element.setAttribute( "path"           , f->path() );

                    files_element.appendChild( file_element );
                }
            }

            element.appendChild( files_element );
        }

        if( _bad_map.size() > 0 )
        {
            xml::Element_ptr bad_files_element = document.createElement( "bad_files" );
            bad_files_element.setAttribute( "count", (int)_bad_map.size() );

            if( show & show_order_source_files )
            {
                Z_FOR_EACH( Bad_map, _bad_map, it )
                {
                    Bad_entry* bad_entry = it->second;

                    xml::Element_ptr file_element = document.createElement( "file" );
                    file_element.setAttribute( "path", bad_entry->_file_path );
                    append_error_element( file_element, bad_entry->_error );

                    bad_files_element.appendChild( file_element );
                }
            }

            element.appendChild( bad_files_element );
        }

    return element;
}

//---------------------------------------------------Directory_file_order_source::async_state_text_

string Directory_file_order_source::async_state_text_() const
{ 
    S result;
    
    result << "Directory_file_order_source(\"" << _path << "\"";
    if( _regex_string != "" )  result << ",\"" << _regex_string << "\"";
    if( _notification_event.signaled_flag() )  result << ",signaled!";
    result << ")";

    return result;
}

//--------------------------------------Directory_file_order_source::start_or_continue_notification
#ifdef Z_WINDOWS

void Directory_file_order_source::start_or_continue_notification( bool was_notified )
{
    // Windows XP:
    // Ein überwachtes lokales Verzeichnis kann entfernt (rd), aber nicht angelegt (mkdir) werden. Der Name ist gesperrt.
    // Aber ein überwachtes Verzeichnis im Netzwerk kann entfernt und wieder angelegt werden, 
    // ohne dass die Überwachung das mitbekommt. Sie signaliert keine Veränderung im neuen Verzeichnis, ist also nutzlos.
    // Deshalb erneuern wir die Verzeichnisüberwachung, wenn seit _repeat Sekunde kein Signal gekommen ist.

    Time now = Time::now();
    
    if( !_notification_event.handle()  ||  Time::now() >= _notification_event_time + _repeat )
    {
        Z_LOG2( "scheduler.file_order", "FindFirstChangeNotification( \"" << _path.path() << "\", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
        HANDLE h = FindFirstChangeNotification( _path.path().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );

        if( h == INVALID_HANDLE_VALUE )  z::throw_mswin( "FindFirstChangeNotification", _path.path() );

        if( _notification_event.handle() )      // Signal retten. Eigentlich überflüssig, weil wir hiernach sowieso das Verzeichnis lesen
        {
            _notification_event.wait( 0 );
            if( _notification_event.signaled() )      
            {
                _notification_event.set_signaled();     
                Z_LOG2( "scheduler.file_order", __FUNCTION__ << " Signal der alten Überwachung auf die neue übertragen.\n" );
            }

            close_notification();
        }

        _notification_event.set_handle( h );
        _notification_event.set_name( "FindFirstChangeNotification " + _path );
        
        add_to_event_manager( _spooler->_connection_manager );

        _notification_event_time = now;
    }
    else
    if( was_notified )
    {
        Z_LOG2( "scheduler.file_order", "FindNextChangeNotification(\"" << _path << "\")\n" );
        BOOL ok = FindNextChangeNotification( _notification_event.handle() );
        if( !ok )  throw_mswin_error( "FindNextChangeNotification" );

        _notification_event_time = now;
    }

    /*
    if( !_notification_event.handle() )
    {
        Z_LOG2( "scheduler.file_order", "FindFirstChangeNotification( \"" << _path.path() << "\", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
        HANDLE h = FindFirstChangeNotification( _path.path().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );

        if( h == INVALID_HANDLE_VALUE )  z::throw_mswin( "FindFirstChangeNotification", _path.path() );

        _notification_event.set_handle( h );
        _notification_event.set_name( "FindFirstChangeNotification " + _path );

        add_to_event_manager( _spooler->_connection_manager );
    }
    else
    {
        if( was_notified )
        {
            Z_LOG2( "scheduler.file_order", "FindNextChangeNotification(\"" << _path << "\")\n" );
            BOOL ok = FindNextChangeNotification( _notification_event.handle() );
            if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
        }
    }
    */
}

#endif
//--------------------------------------------------Directory_file_order_source::close_notification

void Directory_file_order_source::close_notification()
{
#   ifdef Z_WINDOWS
        if( _notification_event.handle() )
        {
            remove_from_event_manager();
            set_async_manager( _spooler->_connection_manager );   // remove_from_event_manager() für set_async_next_gmtime() rückgängig machen

            Z_LOG2( "scheduler.file_order", "FindCloseChangeNotification()\n" );
            FindCloseChangeNotification( _notification_event.handle() );
            _notification_event._handle = NULL;   // set_handle() ruft CloseHandle(), das wäre nicht gut
        }
#   endif
}

//--------------------------------------------------------------Directory_file_order_source::finish

void Directory_file_order_source::finish()
{
    Order_source::finish();     // Setzt _next_job
    assert( _next_job );

    _next_job->register_order_source( this );
}

//---------------------------------------------------------------Directory_file_order_source::start

void Directory_file_order_source::start()
{
    if( _next_job->name() == file_order_sink_job_name )  z::throw_xc( "SCHEDULER-342", _job_chain->obj_name() );

    _expecting_request_order = true;            // Auf request_order() warten
    set_async_next_gmtime( double_time_max );
    //set_async_next_gmtime( (time_t)0 );     // Am Start das Verzeichnis auslesen

    set_async_manager( _spooler->_connection_manager );
}

//-------------------------------------------------------Directory_file_order_source::request_order

Order* Directory_file_order_source::request_order( const string& cause )
{
    Order* result = NULL;

    if( _expecting_request_order 
     || async_next_gmtime_reached() )       // Das, weil die Jobs bei jeder Gelegenheit do_something() durchlaufen, auch wenn nichts anliegt (z.B. bei TCP-Verkehr)
    {
        Z_LOG2( "scheduler.file_order", __FUNCTION__ << " cause=" << cause << "\n" );

        result = read_directory( false, cause );
        if( result )  assert( result->is_immediately_processable() );
    }
    else
    {
        //Z_LOG2( "scheduler.file_order", __FUNCTION__ << " cause=" << cause << ", !async_next_gmtime_reached()\n" );
    }

    return result;
}

//------------------------------------------------------Directory_file_order_source::read_directory

Order* Directory_file_order_source::read_directory( bool was_notified, const string& cause )
{
    Order*          result              = NULL;
    Order_queue*    first_order_queue   = _next_job->order_queue();


    for( int try_index = 1;; try_index++ )           // Nach einem Fehler machen wir einen zweiten Versuch, bevor wir eine eMail schicken
    {
        try
        {
#           ifdef Z_WINDOWS
                // Verzeichnisüberwachung starten oder fortsetzen,
                // bevor die Dateinamen gelesen werden, damit Änderungen während oder kurz nach dem Lesen bemerkt werden.
                // Das kann ein Ereignis zu viel geben. Aber besser als eins zu wenig.

                start_or_continue_notification( was_notified );
#           endif


            if( first_order_queue->has_order( Time(0) ) )      // Auftragswarteschlange ist nicht leer?
            {
                // Erst die vorhandenen Aufträge abarbeiten lassen und nächsten Aufruf von request_order() abwarten
            }
            else
            {
                if( _new_files_index < _new_files.size() )     // Noch Dateien im Puffer
                {
                    log()->info( message_string( "SCHEDULER-986", _new_files_time.as_string( Time::without_ms ) ) );
                }
                else
                {
                    read_new_files_and_handle_deleted_files( cause );
                }

                int n = min( _new_files_index + _max_orders, (int)_new_files.size() );
                for(; _new_files_index < n; _new_files_index++ )
                {
                    z::File_info* new_file = _new_files[ _new_files_index ];
                    File_path     path     = new_file->path();

                    try
                    {
                        ptr<Order> order = new Order( _spooler );

                        order->set_file_path( path );
                        order->set_state( _next_state );

                        string date = Time( new_file->last_write_time() ).as_string( Time::without_ms ) + " GMT";   // localtime_from_gmtime() rechnet alte Sommerzeit-Daten in Winterzeit um
                        log()->info( message_string( "SCHEDULER-983", order->obj_name(), "written at " + date ) );

                        order->add_to_job_chain( _job_chain );

                        if( !result )  result = order;      // Der erste, sofort ausführbare Auftrag

                        _new_files[ _new_files_index ] = NULL;

                        if( _bad_map.find( path ) != _bad_map.end() )   // Zurzeit nicht denkbar, weil es nur zu lange Pfade betrifft
                        {
                            _bad_map.erase( path );
                            log()->info( message_string( "SCHEDULER-347", path ) );
                        }
                    }
                    catch( exception& x )   // Möglich bei für Order.id zu langem Pfad
                    {
                        if( _bad_map.find( path ) == _bad_map.end() )
                        {
                            log()->error( x.what() );
                            z::Xc xx ( "SCHEDULER-346", path );
                            log()->warn( xx.what() );
                            _bad_map[ path ] = Z_NEW( Bad_entry( path, x ) );

                            xx.append_text( x.what() );
                            send_mail( evt_file_order_error, &xx );
                        }
                    }
                }

                if( n < _new_files.size() )  log()->info( message_string( "SCHEDULER-985", _new_files.size() - n ) );
            }

            if( _directory_error )
            {
                log()->info( message_string( "SCHEDULER-984", _path ) );

                if( _send_recovered_mail )
                {
                    _send_recovered_mail = false;
                    send_mail( evt_file_order_source_recovered, NULL );
                }

                _directory_error = NULL;
            }
        }
        catch( exception& x )
        {
            _new_files.clear();
            _new_files_index = 0;

            //if( try_index < max_tries )
            //{
            //    log()->warn( x.what() ); 
            //    close_notification();
            //    continue;
            //}

            if( _directory_error )
            {
                log()->debug( x.what() );      // Nur beim ersten Mal eine Warnung
            }
            else
            {
                log()->warn( x.what() ); 

                if( _spooler->_mail_on_error )
                {
                    _send_recovered_mail = true; 
                    send_mail( evt_file_order_source_error, &x );
                }
            }

            _directory_error = x;

            close_notification();  // Schließen, sonst kann ein entferntes Verzeichnis nicht wieder angelegt werden (Windows blockiert den Namen)
        }

        break;
    }

    _expecting_request_order = result != NULL;  // Nächstes request_order() abwarten

    time_t delay = _directory_error        ? delay_after_error() :
                   _expecting_request_order? INT_MAX                // Nächstes request_order() abwarten
                                           : _repeat;               // Unter Unix funktioniert's _nur_ durch wiederkehrendes Nachsehen

    if( delay < 1 )  delay = 1;     // Falls ein Spaßvogel es geschafft hat, repeat="0" anzugeben

    set_async_delay( delay );
    //Z_LOG2( "scheduler.file_order", __FUNCTION__  << " set_async_delay(" << delay << ")  _expecting_request_order=" << _expecting_request_order << 
    //          "   async_next_gmtime" << Time( async_next_gmtime() ).as_string() << "GMT \n" );

    return result;
}

//-----------------------------Directory_file_order_source::read_new_files_and_handle_deleted_files

void Directory_file_order_source::read_new_files_and_handle_deleted_files( const string& cause )
{
    hash_set<string>            removed_blacklist_files;
    hash_set<string>            virgin_known_files;


    Z_LOG2( "scheduler.file_order", __FUNCTION__ << "  " << _path << " wird gelesen wegen \"" << cause << "\" ...\n" );


    _new_files.clear();
    _new_files.reserve( 1000 );
    _new_files_index = 0;
    _new_files_time = Time::now();

    Z_FOR_EACH( Job_chain::Blacklist_map, _job_chain->_blacklist_map, it )  removed_blacklist_files.insert( it->first );


    for( Directory_watcher::Directory_reader dir ( _path, _regex_string == ""? NULL : &_regex );; )
    {
        ptr<z::File_info> file_info = dir.get();
        if( !file_info )  break;

        string path = file_info->path();

        if( Order* order = _job_chain->order_or_null( path ) ) 
        {
            removed_blacklist_files.erase( path );
            if( order->is_virgin() )  virgin_known_files.insert( path );
        }
        else
        {
            file_info->last_write_time();     // last_write_time füllen für sort, quick_last_write_less()
            _new_files.push_back( file_info );
        }
    }

    Z_LOG2( "scheduler.file_order", __FUNCTION__ << "  " << _path << "  " << _new_files.size() << " Dateinamen gelesen\n" );
    //log()->info( "******* WATCHING " + _path + " ******* " + cause );   // TEST


    sort( _new_files.begin(), _new_files.end(), zschimmer::File_info::quick_last_write_less );


    // removed_blacklist_files: 
    // Aufträge in der Blacklist, deren Dateien nicht mehr da sind, entfernen

    Z_FOR_EACH( hash_set<string>, removed_blacklist_files, it ) 
    {
        Job_chain::Blacklist_map::iterator it2 = _job_chain->_blacklist_map.find( *it );
        if( it2 != _job_chain->_blacklist_map.end() )
        {
            Order* removed_file_order = it2->second;
            removed_file_order->log()->info( message_string( "SCHEDULER-981" ) );   // "File has been removed"
            removed_file_order->remove_from_job_chain();   
        }
    }


    // virgin_known_files:
    // Jungfräuliche Aufträge, denen die Datei abhanden gekommen sind, entfernen

    Order_queue::Queue* queue = &_next_job->order_queue()->_queue;    // Zugriff mit Ausnahmegenehmigung. In _setback_queue verzögerte werden nicht beachtet
    for( Order_queue::Queue::iterator it = queue->begin(); it != queue->end(); )
    {
        Order* order = *it++;  // Hier schon weiterschalten, bevor it durch remove_from_job_chain ungültig wird

        if( order->is_virgin()  &&  virgin_known_files.find( order->string_id() ) == virgin_known_files.end()  &&  order->is_file_order() )
        {
            order->log()->warn( message_string( "SCHEDULER-982" ) );
            order->remove_from_job_chain();
        }
    }
}

//-----------------------------------------------------------Directory_file_order_source::send_mail

void Directory_file_order_source::send_mail( Scheduler_event_type event_code, const exception* x )
{
    try
    {                                   
        switch( event_code )
        {
            case evt_file_order_source_error:
            {
                assert( x );

                Scheduler_event scheduler_event ( event_code, log_error, this );

                scheduler_event.set_message( x->what() );
                scheduler_event.set_error( *x );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( string("ERROR ") + x->what() );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n";
                body << "\n";
                body << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_hostname << "\n";
                body << "\n";
                body << "<file_order_source directory=\"" << _path << "\"/> doesn't work because of following error:\n";
                body << x->what() << "\n";
                body << "\n";

                if( delay_after_error() < INT_MAX )
                {
                    body << "Retrying every " << delay_after_error() << " seconds.\n";
                    body << "You will be notified when the directory is accessible again\n";
                }

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            case evt_file_order_source_recovered:
            {
                string msg = message_string( "SCHEDULER-984", _path );
                Scheduler_event scheduler_event ( event_code, log_info, this );

                scheduler_event.set_message( msg );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( msg );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n\n" << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_hostname << "\n\n";
                body << msg << "\n";

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            case evt_file_order_error:
            {
                string msg = x->what();
                Scheduler_event scheduler_event ( event_code, log_info, this );

                scheduler_event.set_message( msg );
                scheduler_event.set_error( *x );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( msg );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n\n" << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_hostname << "\n\n";
                body << msg << "\n";

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            default:
                z::throw_xc( __FUNCTION__ );
        }
    }
    catch( const exception& x )  { log()->warn( x.what() ); }
}

//-----------------------------------------------------Directory_file_order_source::async_continue_

bool Directory_file_order_source::async_continue_( Async_operation::Continue_flags flags )
{
    bool    was_notified = _notification_event.signaled_flag();

    string  cause = was_notified                    ? "Notification" :
                    flags & cont_next_gmtime_reached? "Wartezeit abgelaufen"   // Das Flag ist doch immer gesetzt, oder?
                                                    : __FUNCTION__;

    _notification_event.reset();

    read_directory( was_notified, cause );

    return true;
}

//---------------------------------------------------Directory_file_order_source::delay_after_error

int Directory_file_order_source::delay_after_error()
{
    return _delay_after_error < INT_MAX? _delay_after_error 
                              : _repeat;
}

//-------------------------------------------------------------------------------------------------

} //namespace spoooler
} //namespace sos
