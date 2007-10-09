// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"

namespace sos {
namespace scheduler {

struct Scheduler_script_folder;
struct Scheduler_script_subsystem;

//--------------------------------------------------------------------------------------------const

const Absolute_path             default_scheduler_script_path ( root_path, "scheduler" );

//-----------------------------------------------------------------------Scheduler_script_subsystem

struct Scheduler_script_subsystem : Scheduler_script_subsystem_interface
{
                                Scheduler_script_subsystem  ( Scheduler* );
                               ~Scheduler_script_subsystem  ();


    // Subsystem:
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    string                      object_type_name            () const                                { return "Scheduler_script"; }
    string                      filename_extension          () const                                { assert(0), z::throw_xc( Z_FUNCTION ); }
    string                      xml_element_name            () const                                { return "scheduler_script"; }
    string                      xml_elements_name           () const                                { assert(0), z::throw_xc( Z_FUNCTION ); }
    void                        assert_xml_element_name     ( const xml::Element_ptr& ) const;
    string                      normalized_name             ( const string& name ) const            { return name; }
    ptr<Scheduler_script>       new_file_based              ()                                      { return Z_NEW( Scheduler_script( this ) ); }
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "scheduler_scripts" ); }
    ptr<Scheduler_script_folder> new_scheduler_script_folder( Folder* );


    // Scheduler_script_subsystem_interface:
    void                        set_dom                     ( const xml::Element_ptr& script_element );
    Scheduler_script*           default_scheduler_script    ();
    Scheduler_script*           default_scheduler_script_or_null();
    bool                        needs_java                  () const;

    vector<Scheduler_script*>   ordered_file_baseds         ();

  private:
    Fill_zero                  _zero_;
};

//-------------------------------------------------------------------new_scheduler_script_subsystem

ptr<Scheduler_script_subsystem_interface> new_scheduler_script_subsystem( Scheduler* scheduler )
{
    ptr<Scheduler_script_subsystem> scheduler_script_subsystem = Z_NEW( Scheduler_script_subsystem( scheduler ) );
    return +scheduler_script_subsystem;
}

//---------------------------------------------------------------Scheduler_script::Scheduler_script

Scheduler_script::Scheduler_script( Scheduler_script_subsystem* subsystem )
:
    My_file_based( subsystem, this, type_scheduler_script ),
    _zero_(this+1),
    _ordering(1)
{
    _module = Z_NEW( Module( subsystem->spooler(), subsystem->spooler()->include_path(), _log ) );
    _module->_dont_remote = true;
    _com_log = new Com_log( _log );
}

//------------------------------------------------------------------------Scheduler_script::set_dom
    
void Scheduler_script::set_dom( const xml::Element_ptr& element )
{
    if( element.nodeName_is( "script" ) )
    {
        _module->set_dom( element );
    }
    else
    {
        _ordering = element.int_getAttribute( "ordering", _ordering );

        DOM_FOR_EACH_ELEMENT( element, e )
        {
            if( e.nodeName_is( "script" ) )
            {
                _module->set_dom( e );
            }
        }
    }
}

//-------------------------------------------------------------------------Scheduler_script::module

Module* Scheduler_script::module() const
{
    if( !_module )  z::throw_xc( Z_FUNCTION );
    return _module;
}

//----------------------------------------------------------------Scheduler_script::module_instance

Module_instance* Scheduler_script::module_instance() const
{
    if( !_module_instance )  z::throw_xc( Z_FUNCTION );
    return _module_instance;
}

//--------------------------------------------------------------------------Scheduler_script::close

void Scheduler_script::close()
{
    try
    {
        if( _module_instance )  _module_instance->call_if_exists( spooler_exit_name );
    }
    catch( exception& x )  { _log->warn( message_string( "SCHEDULER-260", x.what() ) ); }  // "Scheduler-Skript spooler_exit(): $1"
    
    if( _com_log )  _com_log->set_log( NULL );
}

//------------------------------------------------------------------Scheduler_script::on_initialize

bool Scheduler_script::on_initialize()
{
    _module->init();

    return true;
}

//------------------------------------------------------------------------Scheduler_script::on_load

bool Scheduler_script::on_load()
{
    //Z_LOGI2( "scheduler", "Scheduler-Skript wird geladen\n" );

    _module_instance = _module->create_instance();
    _module_instance->init();

    _module_instance->add_obj( (IDispatch*)_spooler->_com_spooler, "spooler"     );
    _module_instance->add_obj( (IDispatch*)_com_log              , "spooler_log" );

    _module_instance->load();
    _module_instance->start();

    //Z_LOG2( "scheduler", "Scheduler-Skript ist geladen\n" );

    return true;
}

//--------------------------------------------------------------------Scheduler_script::on_activate

bool Scheduler_script::on_activate()
{
    bool ok = check_result( _module_instance->call_if_exists( spooler_init_name ) );
    if( !ok )  z::throw_xc( "SCHEDULER-183", obj_name() );

    return true;
}

//-------------------------------------------------------------Scheduler_script::can_be_removed_now

bool Scheduler_script::can_be_removed_now()
{ 
    return true; 
}

//--------------------------------------------------------------------Scheduler_script::dom_element

xml::Element_ptr Scheduler_script::dom_element( const xml::Document_ptr& dom_document, const Show_what& )
{
    return dom_document.createElement( "scheduler_script" );
}

//--------------------------------------------------cheduler_script_folder::Scheduler_script_folder

Scheduler_script_folder::Scheduler_script_folder( Folder* folder )
:
    typed_folder<Scheduler_script>( folder->spooler()->scheduler_script_subsystem(), folder, type_scheduler_script_folder )
{
}

//------------------------------------------------Scheduler_script_folder::~Scheduler_script_folder

Scheduler_script_folder::~Scheduler_script_folder()
{
    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//-----------------------Scheduler_script_subsystem_interface::Scheduler_script_subsystem_interface
    
Scheduler_script_subsystem_interface::Scheduler_script_subsystem_interface( Scheduler* scheduler, Type_code type_code )  
: 
    file_based_subsystem< Scheduler_script >( scheduler, this, type_code ) 
{
}

//--------------------------------------------cheduler_script_subsystem::Scheduler_script_subsystem

Scheduler_script_subsystem::Scheduler_script_subsystem( Scheduler* scheduler )
: 
    Scheduler_script_subsystem_interface( scheduler, type_scheduler_script_subsystem ),
    _zero_(this+1)
{
}

//------------------------------------------Scheduler_script_subsystem::~Scheduler_script_subsystem
    
Scheduler_script_subsystem::~Scheduler_script_subsystem()
{
    try
    {
        close();
    }
    catch( exception& x )  { Z_LOG( Z_FUNCTION << "  ERROR  " << x.what() << "\n" ); }
}

//----------------------------------------------------------------Scheduler_script_subsystem::close

void Scheduler_script_subsystem::close()
{
    _subsystem_state = subsys_stopped;
    //Z_LOG2( "scheduler", "Scheduler-Skripte werden beendet ...\n" );

    //vector<Scheduler_script*> scripts = ordered_file_baseds();

    //Z_FOR_EACH_REVERSE( vector<Scheduler_script*>, scripts, s )
    //{
    //    Scheduler_script* script = *s;

    //    try
    //    {
    //        script->close();
    //    }
    //    catch( exception& x )  { script->log()->warn( message_string( "SCHEDULER-260", x.what() ) ); }  // "Scheduler-Skript spooler_exit(): $1"
    //}

    //Z_FOR_EACH_REVERSE( vector<Scheduler_script*>, scripts, s )
    //{
    //    Scheduler_script* script = *s;
    //    script->remove();
    //}
 
    //Z_LOG2( "scheduler", "Scheduler-Skripte sind beendet.\n" );

    file_based_subsystem<Scheduler_script>::close();
}

//------------------------------------------Scheduler_script_subsystem::new_scheduler_script_folder

ptr<Scheduler_script_folder> Scheduler_script_subsystem::new_scheduler_script_folder( Folder* folder )
{ 
    return Z_NEW( Scheduler_script_folder( folder ) ); 
}

//----------------------------------------------Scheduler_script_subsystem::assert_xml_element_name

void Scheduler_script_subsystem::assert_xml_element_name( const xml::Element_ptr& element ) const
{
    if( !element.nodeName_is( "script" ) )  
    {
        File_based_subsystem::assert_xml_element_name( element );
    }
}

//--------------------------------------------------Scheduler_script_subsystem::ordered_file_baseds

vector<Scheduler_script*> Scheduler_script_subsystem::ordered_file_baseds()
{
    vector<Scheduler_script*> result;
    
    result.reserve( _file_based_map.size() );
    Z_FOR_EACH( File_based_map, _file_based_map, fb )  result.push_back( fb->second );

    sort( result.begin(), result.end(), Scheduler_script::ordering_is_less );

    return result;
}

//--------------------------------------------------------------Scheduler_script_subsystem::set_dom

void Scheduler_script_subsystem::set_dom( const xml::Element_ptr& element )
{
    assert_subsystem_state( subsys_not_initialized, Z_FUNCTION );

    spooler()->root_folder()->scheduler_script_folder()->add_or_replace_file_based_xml( element, default_scheduler_script_path.name() );
}

//-------------------------------------------------Scheduler_script_subsystem::subsystem_initialize

bool Scheduler_script_subsystem::subsystem_initialize()
{
    _subsystem_state = subsys_initialized;
    file_based_subsystem<Scheduler_script>::subsystem_initialize();
    return true;
}

//-------------------------------------------------------Scheduler_script_subsystem::subsystem_load

bool Scheduler_script_subsystem::subsystem_load()
{
    _subsystem_state = subsys_loaded;
    file_based_subsystem<Scheduler_script>::subsystem_load();
    return true;
}

//---------------------------------------------------Scheduler_script_subsystem::subsystem_activate

bool Scheduler_script_subsystem::subsystem_activate()
{
    _subsystem_state = subsys_active;  // Jetzt schon aktiv für die auszuführenden Skript-Funktionen
    file_based_subsystem<Scheduler_script>::subsystem_activate();
    return true;
}

//-----------------------------------------------------------Scheduler_script_subsystem::needs_java

bool Scheduler_script_subsystem::needs_java() const
{
    bool result = false;

    Z_FOR_EACH_CONST( File_based_map, _file_based_map, fb )
    {
        Scheduler_script* script = fb->second;
        result = script->module()->kind() == Module::kind_java;
        if( result )  break;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
