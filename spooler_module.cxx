// $Id: spooler_module.cxx,v 1.14 2002/12/02 17:19:32 jz Exp $
/*
    Hier sind implementiert

    Module
    Com_module_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"

using namespace std;

namespace sos {
namespace spooler {

//---------------------------------------------------------------------------Source_with_parts::add

void Source_with_parts::add( int linenr, const string& text, const Time& mod_time )
{ 
    _parts.push_back( Source_part( linenr, text, mod_time ) );

    if( mod_time  &&  _max_modification_time < mod_time )   _max_modification_time = mod_time;
}

//----------------------------------------------------------------------------------Module::set_dom

void Module::set_dom_without_source( const xml::Element_ptr& element )
{
    _source.clear();  //clear();

    _language       = element.getAttribute( "language"  );
    _com_class_name = element.getAttribute( "com_class" );
    _filename       = element.getAttribute( "filename"  );

    _java_class_name = element.getAttribute( "java_class" );
    _recompile       = element.bool_getAttribute( "recompile" );

#ifdef Z_WINDOWS
    if( _com_class_name != "" )
    {
        _kind = kind_com;
        
        if( _language        != "" )  throw_xc( "SPOOLER-145" );
        if( _java_class_name != "" )  throw_xc( "SPOOLER-168" );
    }
    else
#endif

    if( _java_class_name != ""  ||  lcase(_language) == "java" )
    {
        _kind = kind_java;
     
        if( _language == "" )  _language = "Java";

        if( lcase(_language) != "java" )  throw_xc( "SPOOLER-166" );
        if( _com_class_name  != ""     )  throw_xc( "SPOOLER-168" );

        _spooler->_has_java = true;
    }
    else
    {
        _kind = kind_scripting_engine;
         if( _language == "" )  _language = SPOOLER_DEFAULT_LANGUAGE;
    }

    string use_engine = element.getAttribute( "use_engine" );
    
    if( use_engine == ""
     || use_engine == "task" )  _reuse = reuse_task;
    else
    if( use_engine == "job"  )  _reuse = reuse_job;
}

//----------------------------------------------------------------------------------Module::set_dom

void Module::set_dom_source_only( const xml::Element_ptr& element, const Time& xml_mod_time, const string& include_path )
{
    _source = text_from_xml_with_include( element, xml_mod_time, include_path );

    switch( _kind )
    {
        case kind_java:
            //if( !_source.empty() )  throw_xc( "SPOOLER-167" );
            break;

        case kind_scripting_engine:
            if( _source.empty() )  throw_xc( "SPOOLER-173" );
            break;

#     ifdef Z_WINDOWS
        case kind_com:
            if( !_source.empty() )  throw_xc( "SPOOLER-167" );
            break;
#     endif

        default: 
            throw_xc( "Module::set_dom_source_only" );
    }

    clear_java();

    _set = true;
}

//--------------------------------------------------------------------------Module::create_instance

ptr<Module_instance> Module::create_instance()
{
    switch( _kind )
    {
        case kind_java:              
        {
            ptr<Java_module_instance> p = Z_NEW( Java_module_instance( this ) );
            return +p;
        }

        case kind_scripting_engine:  
        {
            ptr<Scripting_engine_module_instance> p = Z_NEW( Scripting_engine_module_instance( this ) );
            return +p;
        }

#     ifdef Z_WINDOWS
        case kind_com:               
            return Z_NEW( Com_module_instance( this ) );
#     endif

        default:                     
            throw_xc( "SPOOLER-173" );
    }
}

//----------------------------------------------------------------------------Module_instance::init

void Module_instance::init()
{
    if( !_module->set() )  throw_xc( "SPOOLER-146" );

    _com_context = new Com_context;
}

//-------------------------------------------------------------------------Module_instance::add_obj

void Module_instance::add_obj( const ptr<IDispatch>& object, const string& name )
{
    if( name == "spooler_log"    )  _com_context->_log     = (qi_ptr<spooler_com::Ilog>)    object;
    else
    if( name == "spooler"        )  _com_context->_spooler = (qi_ptr<spooler_com::Ispooler>)object;
    else
    if( name == "spooler_thread" )  _com_context->_thread  = (qi_ptr<spooler_com::Ithread>) object;
    else
    if( name == "spooler_job"    )  _com_context->_job     = (qi_ptr<spooler_com::Ijob>)    object;
    else
    if( name == "spooler_task"   )  _com_context->_task    = (qi_ptr<spooler_com::Itask>)   object;
    else
        throw_xc( "Module_instance::add_obj", name.c_str() );
}

//------------------------------------------------------------------Module_instance::call_if_exists

Variant Module_instance::call_if_exists( const string& name )
{
    if( name_exists(name) )  return call( name );
                       else  return Variant();
}

//---------------------------------------------------------------------------Module_instance::close

void Module_instance::close()
{
    if( callable() )
    {
        try
        {
            call_if_exists( "spooler_exit()V" );
        }
        catch( const Xc& x ) 
        { 
            _log->error( x.what() ); 
        }
    }

    if( _com_context )  _com_context->close(), _com_context = NULL;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
