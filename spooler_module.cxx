// $Id: spooler_module.cxx,v 1.23 2003/06/11 14:38:14 jz Exp $
/*
    Hier sind implementiert

    Module
    Com_module_instance
*/


#include "spooler.h"
#include "../file/anyfile.h"
#include "../kram/sos_java.h"

using namespace std;

namespace sos {
namespace spooler {


//----------------------------------------------------------------xml::Element_ptr Source_part::dom

xml::Element_ptr Source_part::dom( const xml::Document_ptr& doc ) const
{
    xml::Element_ptr part_element = doc.createElement( "part_element" );

    part_element.setAttribute( "linenr", as_string( _linenr ) );
    part_element.setAttribute( "modtime", _modification_time.as_string() );
    part_element.appendChild( doc.createTextNode( _text ) );

    return part_element;
}

//---------------------------------------------------------xml::Document_ptr Source_with_parts::dom

xml::Element_ptr Source_with_parts::dom( const xml::Document_ptr& doc ) const
{
    xml::Element_ptr source_element = doc.createElement( "source" ); 

    Z_FOR_EACH_CONST( Parts, _parts, part )
    {
        xml::Element_ptr part_element = part->dom(doc);
        source_element.appendChild( part_element );
    }

    return source_element;
}

//---------------------------------------------------------xml::Document_ptr Source_with_parts::dom

xml::Document_ptr Source_with_parts::dom_doc() const
{
    xml::Document_ptr doc;

    doc.create();
    doc.appendChild( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"iso-8859-1\"" ) );
    doc.appendChild( dom(doc) );

    return doc;
}

//--------------------------------------------------------------------Source_with_parts::assign_dom
// Für spooler_module_remote_server.cxx

void Source_with_parts::assign_dom( const xml::Element_ptr& source_element )
{
    clear();

    if( !source_element.nodeName_is( "source" ) )  throw_xc( "Source_with_parts", "dom" );

    DOM_FOR_EACH_ELEMENT( source_element, part )
    {
        Sos_optional_date_time dt = part.getAttribute( "modtime" );
        add( part.int_getAttribute( "linenr", 1 ), part.getTextContent(), dt.time_as_double() );
    }
}

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

    _language         = element.getAttribute     ( "language"  );
    _com_class_name   = element.getAttribute     ( "com_class" );
    _filename         = element.getAttribute     ( "filename"  );
    _java_class_name  = element.getAttribute     ( "java_class" );
    _recompile        = element.bool_getAttribute( "recompile" );

    bool separate_process_default = false;
#   ifndef Z_WINDOWS
        // Bei Perl automatisch separate_process="yes", weil Perl sonst beim zweiten Aufruf abstürzt
        separate_process_default = string_begins_with( lcase(_language), "perl" ) );   
#   endif

    _separate_process = element.bool_getAttribute( "separate_process", separate_process_default );
                                                                                                                           

    string use_engine = element.getAttribute     ( "use_engine" );
    
    if( use_engine == ""
     || use_engine == "task" )  _reuse = reuse_task;
    else
    if( use_engine == "job"  )  _reuse = reuse_job;

    init();
}

//----------------------------------------------------------------------Module::set_dom_source_only

void Module::set_dom_source_only( const xml::Element_ptr& element, const Time& xml_mod_time, const string& include_path )
{
    set_source_only( text_from_xml_with_include( element, xml_mod_time, include_path ) );
}

//--------------------------------------------------------------------------Module::set_source_only

void Module::set_source_only( const Source_with_parts& source )
{
    _source = source;

    switch( _kind )
    {
        case kind_remote:
            break;

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
            throw_xc( "Module::set_source_only" );
    }

    clear_java();

    _set = true;
}

//-------------------------------------------------------------------------------------Module::init

void Module::init()
{
    if( _separate_process )
    {
        _kind = kind_remote;
    }
    else
    {
#     ifdef Z_WINDOWS
        if( _com_class_name != "" )
        {
            _kind = kind_com;
        
            if( _language        != "" )  throw_xc( "SPOOLER-145" );
            if( _java_class_name != "" )  throw_xc( "SPOOLER-168" );
        }
        else
#     endif

        if( _java_class_name != ""  ||  lcase(_language) == "java" )
        {
            _kind = kind_java;
     
            if( _language == "" )  _language = "Java";

            if( lcase(_language) != "java" )  throw_xc( "SPOOLER-166" );
            if( _com_class_name  != ""     )  throw_xc( "SPOOLER-168" );

            if( _spooler )  _spooler->_has_java = true;
        }
        else
        {
            _kind = kind_scripting_engine;
             if( _language == "" )  _language = SPOOLER_DEFAULT_LANGUAGE;
        }
    }
}

//--------------------------------------------------------------------------Module::create_instance

ptr<Module_instance> Module::create_instance()
{
    switch( _kind )
    {
        case kind_java:              
        {
            //if( !_spooler->_java_vm->running() )  throw_xc( "SPOOLER-177" );

            _java_vm = get_java_vm();
            ptr<Java_module_instance> p = Z_NEW( Java_module_instance( _java_vm, this ) );
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

        case kind_remote:
        {
            ptr<Remote_module_instance_proxy> p = Z_NEW( Remote_module_instance_proxy( this ) );
            return +p;
        }

        default:                     
            throw_xc( "SPOOLER-173" );
    }
}

//----------------------------------------------------------------------------Module_instance::init

void Module_instance::init()
{
    if( !_module->set() )  throw_xc( "SPOOLER-146" );

    _spooler_exit_called = false;
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
    if( !_spooler_exit_called  &&  callable() )     // _spooler_exit_called wird auch von Remote_module_instance_server gesetzt.
    {
        try
        {
            _spooler_exit_called = true;
            call_if_exists( "spooler_exit()V" );
        }
        catch( const exception& x ) 
        { 
            _log.error( x.what() ); 
        }
    }

    if( _com_context )  _com_context->close(), _com_context = NULL;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos
