#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//---------------------------------------------------------------------------File_based::File_based

File_based::File_based( File_based_subsystem* subsystem, IUnknown* iunknown, Type_code type_code )
: 
    Scheduler_object( subsystem->spooler(), iunknown, type_code ),
    _zero_(this+1),
    _visible(visible_yes),
    _file_based_subsystem(subsystem),
    _state(s_not_initialized)
{
}

//--------------------------------------------------------------------------File_based::~File_based
    
File_based::~File_based()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//--------------------------------------------------------------------------------File_based::close
    
void File_based::close()
{
    _state = s_closed;   //pure virtual function called: set_file_based_state( s_closed );
}

//---------------------------------------------------------------------------File_based::initialize

bool File_based::initialize()
{
    bool ok = _state >= s_initialized;

    if( _state == s_not_initialized  &&  subsystem()->subsystem_state() >= subsys_initialized )
    {
        ok = on_initialize();
        if( ok )  set_file_based_state( s_initialized );
    }

    return ok;
}

//---------------------------------------------------------------------------------File_based::load

bool File_based::load()
{
    if( !is_in_folder()  &&  this != spooler()->root_folder() )  assert(0), z::throw_xc( "NOT-IN-FOLDER", Z_FUNCTION, obj_name() );

    bool ok = _state >= s_loaded;
    if( !ok )
    {
        initialize();

        if( _state == s_initialized  &&  subsystem()->subsystem_state() >= subsys_loaded )
        {
            ok = on_load();

            if( ok )
            {
                set_file_based_state( s_loaded );
                subsystem()->dependencies()->announce_requisite_loaded( this ); 
            }
        }
    }

    return ok;
}

//-----------------------------------------------------------------------------File_based::activate

bool File_based::activate()
{
    bool ok = _state >= s_active;
    if( !ok ) {
        load();
        if( ( _state == s_loaded  ||  _state == s_incomplete )  &&   subsystem()->subsystem_state() >= subsys_active ) {
            ok = on_activate();
            if( ok ) {
                set_file_based_state( s_active );
                if (jobject sister = java_sister())  // Noch hat nicht jede Klasse eine Java-Schwester
                    report_event_code(fileBasedActivatedEvent, sister);
            }
        }
    }

    return ok;
}

//-----------------------------------------------------------------File_based::set_file_based_state

void File_based::set_file_based_state( State state )                         
{ 
    if( _state != state )
    {
        assert( _state == s_not_initialized && state == s_undefined  ||  
                _state == s_active          && state == s_incomplete ||
                _state < state );
        State previous_state = _state;
        _state = state; 

        if( _state == s_incomplete )
        {
            list<Requisite_path> missings = missing_requisites();
            S                    s;
            Z_FOR_EACH( list<Requisite_path>, missings, m )  s << ( s.empty()? "" :  ", " ) << m->obj_name();

            log()->log( subsystem()->subsystem_state() == subsys_active? log_warn 
                                                                       : log_debug, 
                        message_string( "SCHEDULER-893", subsystem()->object_type_name(), file_based_state_name(), "missing " + s ) );
        }
        else
        {
            log()->log( _state == s_active || previous_state == s_active? log_info 
                                                                        : log_debug9,
                        message_string( "SCHEDULER-893", subsystem()->object_type_name(), file_based_state_name() ) );
        }
    }
}

//------------------------------------------------------------------File_based::on_requisite_loaded

bool File_based::on_requisite_loaded( File_based* )
{
    bool result = false;

    if( is_in_folder() )  check_for_replacing_or_removing();    // F�r Standing_order: Wenn Jobkette gel�scht, Auftragsdatei ge�ndert und Jobkette wieder geladen wird, 2008-02-01
    if( is_in_folder() )  result = activate();                  // (Besser: nur wenn s_incomplete?)
    
    return result;
}

//-----------------------------------------------------------File_based::on_requisite_to_be_removed

bool File_based::on_requisite_to_be_removed( File_based* file_based )
{
    assert( file_based->is_in_folder() );

    return true;
}

//-----------------------------------------------------------------File_based::on_requisite_removed

void File_based::on_requisite_removed( File_based* )
{
    //assert( file_based->is_in_folder() );

    //check_for_replacing_or_removing();
    //Dependant::on_requisite_removed( file_based );
}

//---------------------------------------------------------------------File_based::assert_is_loaded

void File_based::assert_is_initialized()
{
    if( _state < s_initialized  ||  _state > s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_initialized ), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_initialized ) );
    }
}

//---------------------------------------------------------------------File_based::assert_is_loaded

void File_based::assert_is_loaded()
{
    if( _state < s_loaded  ||  _state > s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_loaded ), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_loaded ) );
    }
}

//---------------------------------------------------------------------File_based::assert_is_active

void File_based::assert_is_active()
{
    if( _state != s_active )  
    {
        if( base_file_has_error() )  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_active ), _base_file_xc );
                               else  z::throw_xc( "SCHEDULER-153", obj_name(), file_based_state_name( s_active ) );
    }
}

//------------------------------------------------------------File_based::assert_is_not_initialized

void File_based::assert_is_not_initialized()
{
    if( _state >= s_initialized )
    {
        z::throw_xc( "SCHEDULER-148", obj_name() );
    }
}

//--------------------------------------------------------------------File_based::set_to_be_removed

void File_based::set_to_be_removed( bool b, Remove_flag remove_flag )
{ 
    _remove_flag      = remove_flag;
    _is_to_be_removed = b; 
    if( _is_to_be_removed )  set_replacement( NULL );
}

//----------------------------------------------------------------------File_based::set_replacement

void File_based::set_replacement( File_based* replacement )             
{ 
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );
    
    if( replacement  &&  
        _configuration_origin == confdir_cache &&  
        replacement->_configuration_origin != confdir_cache &&
        _base_file_info._path.exists() )    // Au�er, die zentrale Datei ist gel�scht
    {
        z::throw_xc( "SCHEDULER-460", subsystem()->object_type_name() );  // Original ist zentral konfiguriert, aber Ersatz nicht
    }

    _replacement = replacement; 
    if( _replacement )  set_to_be_removed( false ); 
}

//--------------------------------------------------------------------File_based::prepare_to_remove

void File_based::prepare_to_remove( Remove_flag remove_flag )
{ 
    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );

    set_to_be_removed( true, remove_flag );
    on_prepare_to_remove();
    subsystem()->dependencies()->announce_requisite_to_be_removed( this ); 
}

//-----------------------------------------------------------------File_based::on_prepare_to_remove

void File_based::on_prepare_to_remove()
{ 
}

//------------------------------------------------------------------------File_based::on_remove_now

void File_based::on_remove_now()
{
}

//---------------------------------------------------------------------------File_based::remove_now

void File_based::remove_now()
{
    ptr<File_based> me = this;

    on_remove_now();
    typed_folder()->remove_file_based( this );
    subsystem()->dependencies()->announce_requisite_removed( this ); 
    if (jobject sister = java_sister())   // Noch hat nicht jede Klasse eine Java-Schwester
        report_event_code(fileBasedRemovedEvent, sister);
}

//-------------------------------------------------------------------------------File_based::remove

bool File_based::remove( Remove_flag remove_flag )
{
    bool result = false;

    if( !is_in_folder() )  z::throw_xc( "SCHEDULER-433", obj_name() );



    if( remove_flag == rm_base_file_too  &&  has_base_file() )
    {
        remove_base_file();
    }


    prepare_to_remove( remove_flag );

    if( can_be_removed_now() )  
    {
        _remove_xc = zschimmer::Xc();

        remove_now();
        result = true;
    }
    else  
    {
        _remove_xc = remove_error();
        log()->info( _remove_xc.what() );   // Kein Fehler, L�schen ist nur verz�gert
    }


    return result;
}

//-------------------------------------------------------------------------File_based::remove_error

zschimmer::Xc File_based::remove_error()
{
    return zschimmer::Xc( "SCHEDULER-989", subsystem()->object_type_name() );
}

//----------------------------------------------------------------------File_base::remove_base_file

void File_based::remove_base_file()
{
    if( !has_base_file() )  assert(0), z::throw_xc( Z_FUNCTION );

    try
    {
#       ifdef Z_DEBUG
            _base_file_info._path.move_to( base_file_info()._path + "-REMOVED" );
#        else
            _base_file_info._path.unlink();
#       endif    

        handle_event( File_based::bfevt_removed );
    }
    catch( exception& )
    {
        if( _base_file_info._path.exists() )  throw;
    }

    _file_is_removed = true;
}

//-------------------------------------------------------------------------File_based::replace_with

bool File_based::replace_with( File_based* file_based_replacement )
{
    bool result = false;

    set_replacement( file_based_replacement );
    
    prepare_to_replace();

    if( can_be_replaced_now() )
    {
        replace_now();
        result = true;
    }
    else  
        log()->info( message_string( "SCHEDULER-888", subsystem()->object_type_name() ) );

    return result;
}

//------------------------------------------------------File_based::check_for_replacing_or_removing

void File_based::check_for_replacing_or_removing( When_to_act when_to_act )
{
    try
    {
        if( is_in_folder() )
        {
            if( replacement() )
            {
                bool ok = replacement()->file_based_state() == File_based::s_initialized;

                if( !ok )
                {
                    ok = replacement()->initialize();
                    if( ok )  prepare_to_replace();
                }
                
                if( ok  &&  can_be_replaced_now() )
                {
                    if( when_to_act == act_now )  replace_now();
                                            else  typed_folder()->add_to_replace_or_remove_candidates( *this );
                }
            }
            else
            if( is_to_be_removed()  &&  can_be_removed_now() )
            {
                if( when_to_act == act_now )  remove_now();
                                        else  typed_folder()->add_to_replace_or_remove_candidates( *this );
            }
        }
    }
    catch( exception& x ) { log()->debug( message_string( "SCHEDULER-897", x ) ); }
}

//-------------------------------------------------------------------File_based::prepare_to_replace

void File_based::prepare_to_replace()
{
    assert( _replacement );
    ptr<File_based> replacement = _replacement;     // on_prepare_to_remove() entfernt _replacement

    prepare_to_remove( rm_temporary );        

    _replacement = replacement;
}

//------------------------------------------------------------------File_based::can_be_replaced_now

bool File_based::can_be_replaced_now()
{
    return replacement()  &&  
           replacement()->file_based_state() == File_based::s_initialized  &&
           can_be_removed_now();
}

//-----------------------------------------------------------------------File_based::on_replace_now

File_based* File_based::on_replace_now()
{
    Typed_folder*   typed_folder = this->typed_folder();
    ptr<File_based> replacement  = this->replacement();

    assert( can_be_replaced_now() );

    typed_folder->remove_file_based( this );
    // this ist ung�ltig

    typed_folder->add_file_based( replacement );

    return replacement;
}

//--------------------------------------------------------------------------File_based::replace_now

File_based* File_based::replace_now()
{
    ptr<File_based> replacement = this->replacement();
    assert( replacement );

    Base_file_info file_info = replacement->base_file_info();

    File_based* new_file_based = on_replace_now();

    if( new_file_based == this )              // Process_class und Lock werden nicht ersetzt. Stattdessen werden die Werte �bernommen
    {                                       
        set_base_file_info( file_info );        // Alte Werte ge�nderten Objekts �berschreiben
        _source_xml        = replacement->_source_xml;
        _base_file_xc      = zschimmer::Xc();
        _base_file_xc_time = 0;
        if( file_based_state() == s_undefined )  set_file_based_state( File_based::s_not_initialized );     // Wenn altes fehlerhaft war
    }
    else
    {
        // this ist ung�ltig
    }

    //SS: replacement->report_event_replace ... (weil "this" ung�ltig)
    new_file_based->activate();
    return new_file_based;
}

//------------------------------------------------------------------File_based::replace_with_source

//void File_based::replace_with_source() {
//    set_force_file_reread();
//    directory_observer::Directory_entry e = _base_file_info.directory_entry(_configuration_origin);
//    _typed_folder->on_base_file_changed(this, &e);
//}

//----------------------------------------------------------------File_based::file_based_state_name

string File_based::file_based_state_name( State state )
{
    switch( state )
    {
        case s_undefined:       return "undefined";
        case s_not_initialized: return "not_initialized";
        case s_initialized:     return "initialized";
        case s_loaded:          return "loaded";
        case s_incomplete:      return "incomplete";
        case s_active:          return "active";
        case s_closed:          return "closed";
        default:                return S() << "File_based_state-" << state;
    }
}

//----------------------------------------------------------------------File_based::set_folder_path

void File_based::set_folder_path( const Absolute_path& folder_path )
{
    assert( !folder_path.empty() );
    assert( !_typed_folder );
    assert( _folder_path.empty()  ||  spooler()->folder_subsystem()->normalized_path( _folder_path ) == spooler()->folder_subsystem()->normalized_path( folder_path ) );

    _folder_path = folder_path;
}

//--------------------------------------------------------------------------File_based::folder_path

Absolute_path File_based::folder_path() const
{
    assert( !is_in_folder()  ||  spooler()->folder_subsystem()->normalized_path( _folder_path ) == folder()->normalized_path() );
    return _folder_path;
}

//---------------------------------------------------------------------File_based::set_typed_folder

void File_based::set_typed_folder( Typed_folder* typed_folder )
{ 
    if( typed_folder )
    {
        assert( _folder_path.empty()  ||  spooler()->folder_subsystem()->normalized_path( _folder_path ) == typed_folder->folder()->normalized_path() );
        _typed_folder = typed_folder; 
        _folder_path = _typed_folder->folder()->path();
    }
    else
        _typed_folder = NULL;
}

//----------------------------------------------------------File_based::fill_file_based_dom_element

void File_based::fill_file_based_dom_element( const xml::Element_ptr& result, const Show_what& show_what )
{
    result.setAttribute         ( "path", path().with_slash() );
    result.setAttribute_optional( "name", name() );

    xml::Node_ptr original_first_node = result.firstChild();
    result.insertBefore( File_based::dom_element( result.ownerDocument(), show_what ), original_first_node );

    if( replacement() ) 
    {
        xml::Element_ptr replacement_element = result.ownerDocument().createElement( "replacement" );
        replacement_element.appendChild( replacement()->dom_element( result.ownerDocument(), show_what ) );
        result.insertBefore( replacement_element, original_first_node );
    }

    if( show_what.is_set( show_source )  &&  _source_xml != "" )
    {
        xml::Element_ptr source_element = result.ownerDocument().createElement( "source" );
        result.insertBefore( source_element, original_first_node );

        try
        {
            xml::Document_ptr source_dom ( _source_xml );
            source_element.appendChild( source_element.ownerDocument().clone( source_dom.documentElement() ) );      // Ein "prune()" w�re effizienter als clone()
        }
        catch( exception& x )
        {
            source_element.appendChild( result.ownerDocument().createTextNode( _source_xml ) );
            append_error_element( source_element, x );
        }
    }
}

//--------------------------------------------------------File_based::set_identification_attributes

void File_based::set_identification_attributes( const xml::Element_ptr& result )
{
    result.setAttribute( subsystem()->xml_element_name(), path() );    // Z.B. job="/xx"
}

//--------------------------------------------------------------------------File_based::execute_xml

xml::Element_ptr File_based::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& )
{
    if( element.nodeName_is( subsystem()->xml_element_name() + ".remove" ) ) 
    {
        remove( File_based::rm_base_file_too );
    }
    else
        z::throw_xc( "SCHEDULER-409", subsystem()->xml_element_name() + ".remove", element.nodeName() );

    return command_processor->_answer.createElement( "ok" );
}

//--------------------------------------------------------------------------File_based::dom_element

xml::Element_ptr File_based::dom_element( const xml::Document_ptr& document, const Show_what& show_what )
{
    xml::Element_ptr result = document.createElement( "file_based" );

    result.setAttribute( "state", file_based_state_name() );

    if( has_base_file() )
    {
        result.setAttribute_optional( "file", _base_file_info._path );
        if( _file_is_removed )  result.setAttribute( "removed", "yes" );

        Time t;
        t.set_utc( _base_file_info._last_write_time );
        result.setAttribute( "last_write_time", t.xml_value() );

        if( base_file_has_error() )  result.appendChild( create_error_element( document, _base_file_xc, (time_t)_base_file_xc_time ) );
    }

    if( has_includes() )  result.appendChild( Has_includes::dom_element( document, show_what ) );

    xml::Element_ptr requisites_element = document.createElement( "requisites" );
    append_requisite_dom_elements( requisites_element );
    result.appendChild( requisites_element );

    if( !_remove_xc.is_empty() )  result.append_new_element( "removed" ).appendChild( create_error_element( document, _remove_xc ) );

    return result;
}

//---------------------------------------------------------------------------------File_based::path

Absolute_path File_based::path() const
{ 
    return _typed_folder? _typed_folder->folder()->make_path( _name ) : 
           _name == ""  ? root_path 
                        : Absolute_path( _folder_path.empty()? Absolute_path( "/(not in a folder)" ) 
                                                             : _folder_path                        , _name ); 
}

//------------------------------------------------------------------------File_based::path_or_empty

Absolute_path File_based::path_or_empty() const
{ 
    Absolute_path result = path();
    if( result.is_root() )  result.clear();     //2008-07-09 path() liefert "/", wenn !_typed_folder und _name = "". Warum?
    return result;
}

//----------------------------------------------------------------------File_based::normalized_name

string File_based::normalized_name() const
{ 
    return _file_based_subsystem->normalized_name( _name ); 
}

//----------------------------------------------------------------------File_based::normalized_path

string File_based::normalized_path() const
{ 
    return _file_based_subsystem->normalized_path( path() ); 
}

//---------------------------------------------------------File_based::configuration_root_directory

File_path File_based::configuration_root_directory() const
{
    assert( has_base_file() );

    string result = _configuration_origin == confdir_cache? _spooler->_configuration_directories[ confdir_cache ] 
                                                          : _spooler->_configuration_directories[ confdir_local ];

    assert( string_begins_with( _base_file_info._path, result ) );

    return result;
}

//-----------------------------------------------------------------------------File_based::obj_name

string File_based::obj_name() const
{ 
    S result;
    
    result << _file_based_subsystem->object_type_name();
    result << " ";
    result << path().without_slash(); 

    return result;
}

//-----------------------------------------------------------------------------File_based::set_name
    
void File_based::set_name( const string& name )
{
    _spooler->check_name( name );

    string normalized_name = this->normalized_name();

    if( normalized_name != _file_based_subsystem->normalized_name( name ) )
    {
        if( _name_is_fixed )  z::throw_xc( "SCHEDULER-429", obj_name(), name );       // Name darf nicht ge�ndert werden, au�er Gro�schreibung
        _name = name;
        log()->set_prefix(obj_name() + "*");    // Provisorisch, noch ohne Pfad

        if( !has_base_file() )
        {
            assert( _base_file_info._normalized_name == ""  ||  _base_file_info._normalized_name == normalized_name );
            _base_file_info._normalized_name = subsystem()->normalized_name( name );
        }
    }
}

//-------------------------------------------------------------------------------File_based::folder
    
Folder* File_based::folder() const
{ 
    return _typed_folder? _typed_folder->folder()
                        : spooler()->folder_subsystem()->folder( folder_path() );   // _state < s_initialized, noch nicht im Typed_folder eingeh�ngt, replacement()
}

//-------------------------------------------------------------------------File_based::handle_event

void File_based::handle_event( Base_file_event base_file_event )
{
    if( spooler()->folder_subsystem()->subsystem_state() == subsys_active )
    {
        Absolute_path job_path;
        
        switch( base_file_event )
        {
            case bfevt_added:       job_path = spooler()->_configuration_start_job_after_added;     break;
            case bfevt_modified:    job_path = spooler()->_configuration_start_job_after_modified;  break;
            case bfevt_removed :    job_path = spooler()->_configuration_start_job_after_deleted;   break;
            default:                assert(0), z::throw_xc( Z_FUNCTION );
        }

        if( !job_path.empty() )
        {
            try
            {
                ptr<Com_variable_set> parameters  = new Com_variable_set;
                ptr<Com_variable_set> environment = new Com_variable_set;

                environment->set_var( "SCHEDULER_LIVE_FILEBASE", _base_file_info._filename );
                environment->set_var( "SCHEDULER_LIVE_FILEPATH", _base_file_info._path );
                environment->set_var( "SCHEDULER_LIVE_EVENT"   , base_file_event == bfevt_added   ? "add"      :
                                                                 base_file_event == bfevt_modified? "modified" :
                                                                 base_file_event == bfevt_removed ? "deleted"  : "" );

                Z_FOR_EACH( Com_variable_set::Map, environment->_map, v )  parameters->set_var( lcase( v->second->name() ), v->second->string_value() );
                
                Job*  job  = spooler()->job_subsystem()->job( job_path );
                ptr<Task> task = job->create_task( +parameters , "", 0 );
                task->merge_environment( environment );
                job->enqueue_task( task );
            }
            catch( exception& x )
            {
                spooler()->folder_subsystem()->log()->error( x.what() );
            }
        }
    }
}

}}} //namespace sos::scheduler::folder
