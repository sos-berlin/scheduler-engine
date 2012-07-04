// $Id: Module_monitor_instances.cxx 13753 2009-03-05 12:40:59Z jz $


#include "spooler.h"
#include "../file/anyfile.h"
#include "../kram/sos_java.h"

using namespace std;

namespace sos {
namespace scheduler {

const string spooler_task_before_name    = "spooler_task_before()Z";       
const string spooler_task_after_name     = "spooler_task_after()V";
const string spooler_process_before_name = "spooler_process_before()Z";    
const string spooler_process_after_name  = "spooler_process_after(Z)Z";    

//-----------------------------------------------Module_monitor_instances::Module_monitor_instances
    
Module_monitor_instances::Module_monitor_instances( Has_log* log, Module_monitors* m )
:
    _zero_(this+1),
    _log(log),
    _monitors(m)
{
}

//------------------------------------------------------------------Module_monitor_instances::close
    
void Module_monitor_instances::close_instances()
{
    Z_FOR_EACH_REVERSE( Instance_list, _instance_list, m )  
    {
        Module_monitor_instance* monitor_instance = *m;

        try
        {
            monitor_instance->_module_instance->call_if_exists( spooler_task_after_name );
        }
        catch( exception& x )
        {
            _log.error( monitor_instance->obj_name() + " " + spooler_task_after_name + ": " + x.what() );
        }

        try
        {
            monitor_instance->_module_instance->close();
        }
        catch( exception& x )
        {
            _log.error( monitor_instance->obj_name() + " " + x.what() );
        }
    }
}

//--------------------------------------------------------Module_monitor_instances::clear_instances

void Module_monitor_instances::clear_instances()
{
    Z_FOR_EACH_REVERSE( Instance_list, _instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->clear();
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }

    _instance_list.clear();
}

//-------------------------------------------------------Module_monitor_instances::create_instances
    
void Module_monitor_instances::create_instances()
{
    vector<Module_monitor*> ordered_monitors = _monitors->ordered_monitors();

    Z_FOR_EACH( vector<Module_monitor*>, ordered_monitors, m )
    {
        Module_monitor* monitor = *m;

        _instance_list.push_back( Z_NEW( Module_monitor_instance( monitor, monitor->_module->create_instance() ) ) );
    }
}

//-------------------------------------------------------------------Module_monitor_instances::init

void Module_monitor_instances::init()
{
    Z_FOR_EACH( Instance_list, _instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->init();
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//-----------------------------------------------------------Module_monitor_instances::set_job_name

void Module_monitor_instances::set_job_name( const string& job_name )
{
    Z_FOR_EACH( Instance_list, _instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->set_job_name( job_name );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//------------------------------------------------------------Module_monitor_instances::set_task_id

void Module_monitor_instances::set_task_id( int id )
{
    Z_FOR_EACH( Instance_list, _instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->set_task_id( id );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//----------------------------------------------------------------Module_monitor_instances::set_log

void Module_monitor_instances::set_log( Has_log* log )
{
    _log = log;

    Z_FOR_EACH( Instance_list, _instance_list, m )
    {
        try
        {
            (*m)->_module_instance->set_log( log );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//------------------------------------------------------------Module_monitor_instances::attach_task

void Module_monitor_instances::attach_task( Task* task, Prefix_log* log )
{
    _log = log;

    Z_FOR_EACH( Instance_list, _instance_list, m )
    {
        try
        {
            (*m)->_module_instance->attach_task( task, log );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//------------------------------------------------------------Module_monitor_instances::detach_task

void Module_monitor_instances::detach_task()
{
    Z_FOR_EACH_REVERSE( Instance_list, _instance_list, m )
    {
        try
        {
            (*m)->_module_instance->detach_task();
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//----------------------------------------------------------------Module_monitor_instances::add_obj

void Module_monitor_instances::add_obj( IDispatch* object, const string& name )
{
    Z_FOR_EACH( Instance_list, _instance_list, m )  
    {
        try
        {
            (*m)->_module_instance->add_obj( object, name );
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }
}

//-------------------------------------------------------------------Module_monitor_instances::load

bool Module_monitor_instances::load()
{
    bool ok = true;

    Z_FOR_EACH( Instance_list, _instance_list, m )  
    {
        try
        {
            Module_monitor_instance* monitor_instance = *m;

            if( !monitor_instance->_module_instance->_load_called )  
            {
                ok = monitor_instance->_module_instance->implicit_load_and_start();
                if( !ok )  return false;

                Variant result = monitor_instance->_module_instance->call_if_exists( spooler_task_before_name );
                if( !result.is_missing() )  ok = check_result( result );

                if( !ok )  break;
            }
        }
        catch( exception& x ) { z::throw_xc( "SCHEDULER-447", (*m)->obj_name(), x ); }
    }

    return ok;
}

//-------------------------------------------------Module_monitor_instances::spooler_process_before

Variant Module_monitor_instances::spooler_process_before()
{
    Variant result;

    Z_FOR_EACH( Instance_list, _instance_list, m )  
    {
        Module_monitor_instance* monitor_instance = *m;

        result = monitor_instance->_module_instance->call_if_exists( spooler_process_before_name );
        if( !check_result( result ) )  break;
    }

    return result;
}

//--------------------------------------------------Module_monitor_instances::spooler_process_after

Variant Module_monitor_instances::spooler_process_after( const Variant& result )
{
    Variant new_result = result;
    Z_FOR_EACH_REVERSE( Instance_list, _instance_list, m )  
    {
        Module_monitor_instance* monitor_instance = *m;

        Variant call_result = monitor_instance->_module_instance->call_if_exists( spooler_process_after_name, check_result( new_result ) );
        if( call_result.vt != VT_ERROR && V_ERROR( &call_result ) != DISP_E_UNKNOWNNAME )
           new_result = call_result;
    }

    return new_result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
