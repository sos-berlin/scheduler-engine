#include "spooler.h"

namespace sos {
namespace scheduler {

const string default_monitor_name = "scheduler";

//-------------------------------------------------------------------------Module_monitors::set_dom

void Module_monitors::set_dom( const xml::Element_ptr& element )
{
    if( !element.nodeName_is( "monitor" ) )  assert(0), z::throw_xc( "SCHEDULER-409", "monitor", element.nodeName() );
    
    string name = element.getAttribute( "name", default_monitor_name );

    ptr<Module_monitor> module_monitor = monitor_or_null( name );

    if( !module_monitor )
    {
        module_monitor = Z_NEW( Module_monitor() );
        module_monitor->_name   = name;
        module_monitor->_module = Z_NEW( Module( _main_module->_spooler, _main_module->_file_based, _main_module->_spooler->include_path(), &_main_module->_log ) );
        add_monitor( module_monitor );
    }
    module_monitor->set_dom(element);
}

//-----------------------------------------------------------------Module_monitors::monitor_or_null

Module_monitor* Module_monitors::monitor_or_null( const string& name )
{
    Module_monitor* result = NULL;

    Monitor_map::iterator m = _monitor_map.find( name );
    if( m != _monitor_map.end() )  result = m->second;

    return result;
}

//----------------------------------------------------------------------Module_monitors::initialize

void Module_monitors::initialize()
{
    vector<Module_monitor*> ordered_monitors = this->ordered_monitors();

    Z_FOR_EACH( vector<Module_monitor*>, ordered_monitors, m )
    {
        Module_monitor* monitor = *m;
        monitor->_module->init();
    }
}

//-------------------------------------------------------------Module_monitors::ordered_module_list

vector<Module_monitor*> Module_monitors::ordered_monitors()
{
    vector<Module_monitor*> result;

    result.reserve( _monitor_map.size() );
    Z_FOR_EACH( Monitor_map, _monitor_map, m )  result.push_back( m->second );
    sort( result.begin(), result.end(), Module_monitor::less_ordering );

    return result;
}

}} // namespaces
