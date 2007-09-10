// $Id: spooler_job.cxx 4910 2007-03-23 23:15:25Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"


namespace sos {
namespace scheduler {
namespace order {

//--------------------------------------------------------------------------------------------const

const char                  job_chain_order_separator       = ',';                                  // Dateiname "jobchainname,orderid.order.xml"

//-----------------------------------------------Standing_order_subsystem::Standing_order_subsystem
    
Standing_order_subsystem::Standing_order_subsystem( Scheduler* scheduler )
:
    file_based_subsystem<Standing_order>( scheduler, this, type_standing_order_subsystem )
{
}

//------------------------------------------------------------------Standing_order_subsystem::close
    
void Standing_order_subsystem::close()
{
    set_subsystem_state( subsys_stopped );
    file_based_subsystem<Standing_order>::close();
}

//---------------------------------------------------Standing_order_subsystem::subsystem_initialize

bool Standing_order_subsystem::subsystem_initialize()
{
    file_based_subsystem<Standing_order>::subsystem_initialize();
    set_subsystem_state( subsys_initialized );
    return true;
}

//---------------------------------------------------------Standing_order_subsystem::subsystem_load

bool Standing_order_subsystem::subsystem_load()
{
    file_based_subsystem<Standing_order>::subsystem_load();
    set_subsystem_state( subsys_loaded );
    return true;
}

//-----------------------------------------------------Standing_order_subsystem::subsystem_activate

bool Standing_order_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    file_based_subsystem<Standing_order>::subsystem_activate();
    return true;
}

//---------------------------------------------------------Standing_order_subsystem::new_file_based

ptr<Standing_order> Standing_order_subsystem::new_file_based()
{
    return Z_NEW( Standing_order( this ) );
}

//------------------------------------------------------------Standing_order_subsystem::execute_xml

//xml::Element_ptr Standing_order_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
//{
//    xml::Element_ptr result;
//
//    if( element.nodeName_is( "standing_order" ) )  spooler()->root_folder()->standing_order_folder()->execute_xml_standing_order( element );
//    else
//    if( string_begins_with( element.nodeName(), "standing_order." ) ) 
//    {
//        standing_order( element.getAttribute( "standing_order" ) )->execute_xml( element, show_what );
//    }
//    else
//        z::throw_xc( "SCHEDULER-113", element.nodeName() );
//
//    return command_processor->_answer.createElement( "ok" );
//}

//-----------------------------------------------------Standing_order_folder::Standing_order_folder

Standing_order_folder::Standing_order_folder( Folder* folder )
:
    typed_folder<Standing_order>( folder->spooler()->standing_order_subsystem(), folder, type_standing_order_folder )
{
}

//----------------------------------------------------Standing_order_folder::~Standing_order_folder
    
Standing_order_folder::~Standing_order_folder()
{
}

//-----------------------------------------------------------------------------Standing_order_folder::set_dom

//void Standing_order_folder::set_dom( const xml::Element_ptr& standing_orders_element )
//{
//    assert( standing_orders_element.nodeName_is( "standing_orders" ) );
//
//    DOM_FOR_EACH_ELEMENT( standing_orders_element, standing_order_element )
//    {
//        execute_xml_standing_order( standing_order_element );
//    }
//}

//------------------------------------------------Standing_order_folder::execute_xml_standing_order

//void Standing_order_folder::execute_xml_standing_order( const xml::Element_ptr& standing_order_element )
//{
//    if( !standing_order_element.nodeName_is( "standing_order" ) )  z::throw_xc( "SCHEDULER-409", "standing_order", standing_order_element.nodeName() );
//
//    string standing_order_name = standing_order_element.getAttribute( "name" );
//    
//    ptr<Standing_order> standing_order = standing_order_or_null( standing_order_name );
//    if( !standing_order )  
//    {
//        standing_order = Z_NEW( Standing_order( subsystem(), standing_order_name ) );
//        standing_order->set_dom( standing_order_element );
//        add_standing_order( standing_order );
//        standing_order->activate();
//    }
//    else
//        standing_order->set_dom( standing_order_element );
//
//    standing_order->activate();
//}

//---------------------------------------------------------------Standing_order_folder::dom_element

//xml::Element_ptr Standing_order_folder::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
//{
//    xml::Element_ptr result = dom_document.createElement( "standing_orders" );
//
//    for( File_based_map::iterator it = _file_based_map.begin(); it != _file_based_map.end(); it++ )
//    {
//        Standing_order* standing_order = static_cast<Standing_order*>( +it->second );
//        result.appendChild( standing_order->dom_element( dom_document, show_what ) );
//    }
//
//    return result;
//}

//-------------------------------------------------------------------Standing_order::Standing_order

Standing_order::Standing_order( Standing_order_subsystem* standing_order_subsystem, const string& name )
:
    file_based<Standing_order,Standing_order_folder,Standing_order_subsystem>( standing_order_subsystem, this, type_standing_order ),
    _zero_(this+1)
{
    if( name != "" )  set_name( name );
}

//-------------------------------------------------------------------tanding_order::~Standing_order
    
Standing_order::~Standing_order()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", __FUNCTION__ << ": " << x.what() ); }
}

//----------------------------------------------------------------------------Standing_order::close

void Standing_order::close()
{
    if( _order )  
    {
        _order->close();
        _order = NULL;
    }
}

//--------------------------------------------------------------------Standing_order::on_initialize

bool Standing_order::on_initialize()
{
    return true;
}

//--------------------------------------------------------------------------Standing_order::on_load

bool Standing_order::on_load()
{
    return true;
}

//----------------------------------------------------------------------Standing_order::on_activate

bool Standing_order::on_activate()
{
    _order->place_or_replace_in_job_chain( folder()->job_chain_folder()->job_chain( job_chain_name() ) );
    return true;
}

//------------------------------------------------Standing_order::order_is_removable_or_replaceable

bool Standing_order::order_is_removable_or_replaceable()
{
    return !_order  ||
           _order->job_chain_path() == ""  ||  
           _order->end_state_reached();
}

//---------------------------------------------------------------Standing_order::can_be_removed_now

bool Standing_order::can_be_removed_now()
{
    return order_is_removable_or_replaceable();
}

//----------------------------------------------------------------Standing_order::prepare_to_remove

bool Standing_order::prepare_to_remove()
{
    bool result = false;

    if( _order )
    {
        if( order_is_removable_or_replaceable() )
        {
            _order->remove_from_job_chain();
            result = true;
        }
    }
    else
        result = true;

    return result;
}

//--------------------------------------------------------------Standing_order::can_be_replaced_now

bool Standing_order::can_be_replaced_now()
{
    return order_is_removable_or_replaceable();
}

//---------------------------------------------------------------Standing_order::prepare_to_replace

bool Standing_order::prepare_to_replace()
{
    bool result = false;

    if( _order )
    {
        if( order_is_removable_or_replaceable() )
        {
            if( Job_chain* job_chain = folder()->job_chain_folder()->job_chain_or_null( job_chain_name() ) )
            {
                result = true;
            }
        }
    }
    else
        result = true;

    return result;
}

//----------------------------------------------------------------------Standing_order::replace_now

Standing_order* Standing_order::replace_now()
{
    replacement()->_order->place_or_replace_in_job_chain( folder()->job_chain_folder()->job_chain( job_chain_name() ) );
    _order = replacement()->_order;
    set_replacement( NULL );

    return this;
}

//-------------------------------------------------------------------Standing_order::job_chain_name

string Standing_order::job_chain_name() const
{
    string filename = base_file_info()._filename;

    size_t separator = filename.find( job_chain_order_separator );
    if( separator == string::npos )  separator = 0;

    return filename.substr( 0, separator );
}

//-------------------------------------------------------------------------Standing_order::order_id

string Standing_order::order_id() const
{
    string object_name = Folder::object_name_of_filename( base_file_info()._filename );

    size_t separator = object_name.find( job_chain_order_separator );
    if( separator == string::npos )  separator = object_name.length() - 1;

    return object_name.substr( separator + 1);
}

//--------------------------------------------------------------------------Standing_order::set_dom

void Standing_order::set_dom( const xml::Element_ptr& order_element )
{
    assert( !_order );

    _order = new Order( spooler() );
    _order->set_id( order_id() );           int GEGEN_AENDERUNG_SICHERN;  int JOB_CHAIN_EBENSO;
    _order->set_dom( order_element );
    //_order->place_or_replace_in_job_chain( folder()->job_chain_folder()->job_chain( job_chain_name() ) );
}

//----------------------------------------------------------------------Standing_order::dom_element

xml::Element_ptr Standing_order::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result = _order? _order->dom_element( dom_document, show_what )
                                    : dom_document.createElement( "order" );

    if( has_base_file() )  result.appendChild_if( File_based::dom_element( dom_document, show_what ) );
    if( replacement()   )  result.append_new_element( "replacement" ).appendChild( replacement()->dom_element( dom_document, show_what ) );

    return result;
}

//-------------------------------------------------------------------------Standing_order::obj_name

string Standing_order::obj_name() const
{
    S result;

    result << "Standing_order";
    if( _order ) result << " " << _order->obj_name();

    return result;
}

//----------------------------------------------------------------------Standing_order::execute_xml

//void Standing_order::execute_xml( const xml::Element_ptr& element, const Show_what& )
//{
//    if( element.nodeName_is( "standing_order.remove" ) ) 
//    {
//        remove();
//    }
//    else
//        z::throw_xc( "SCHEDULER-409", "standing_order.remove", element.nodeName() );
//}

//-------------------------------------------------------------------------------------------------

} //namespace standing_order
} //namespace scheduler
} //namespace sos
