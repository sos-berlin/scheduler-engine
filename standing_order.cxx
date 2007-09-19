// $Id: spooler_job.cxx 4910 2007-03-23 23:15:25Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"


namespace sos {
namespace scheduler {
namespace order {

//--------------------------------------------------------------------------------------------const

const char                  job_chain_order_separator       = ',';                                  // Dateiname "jobchainname,orderid.order.xml"

//------------------------------------------------------------------------split_standing_order_name

static void split_standing_order_name( const string& name, string* job_chain_name, string* order_id )
{
    size_t pos = name.find( job_chain_order_separator );
    if( pos == string::npos )  pos = 0;

    *job_chain_name = name.substr( 0, pos );

    if( pos < name.length() )  pos++;
    *order_id = name.substr( pos );
}

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

//--------------------------------------------------------Standing_order_subsystem::normalized_name

string Standing_order_subsystem::normalized_name( const string& name ) const
{
    string job_chain_name;
    string order_id;

    split_standing_order_name( name, &job_chain_name, &order_id );

    return spooler()->order_subsystem()->normalized_name( job_chain_name ) + 
           job_chain_order_separator +
           order_id;
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

Standing_order::Standing_order( Standing_order_subsystem* standing_order_subsystem )
:
    file_based<Standing_order,Standing_order_folder,Standing_order_subsystem>( standing_order_subsystem, this, type_standing_order ),
    _zero_(this+1)
{
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
    if( _order )  set_order( NULL );

    if( _job_chain_path != "" )  remove_dependant( order_subsystem(), _job_chain_path );
    _job_chain_path = "";
}

//-------------------------------------------------------------------------Standing_order::set_name

void Standing_order::set_name( const string& name )
{
    file_based< Standing_order, Standing_order_folder, Standing_order_subsystem >::set_name( name );

    split_standing_order_name( name, &_job_chain_name, &_order_id );

    if( _job_chain_name == ""  ||  _order_id == "" )  z::throw_xc( "SCHEDULER-435", base_file_info()._filename );
}

//------------------------------------------------------------------------Standing_order::set_order

void Standing_order::set_order( Order* order )
{
    if( _order )  
    {
        _order->connect_with_standing_order( NULL );
        _order = NULL;
    }

    if( order )
    {
        _order = order;
        _order->connect_with_standing_order( this );
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
    _job_chain_path = folder()->make_path( _job_chain_name );
    add_dependant( order_subsystem(), _job_chain_path );

    return true;
}

//----------------------------------------------------------------------Standing_order::on_activate

bool Standing_order::on_activate()
{
    bool result = false;

    if( Job_chain* job_chain = folder()->job_chain_folder()->job_chain_or_null( job_chain_name() ) )
    {
        if( job_chain->is_distributed() )  z::throw_xc( "SCHEDULER-384", job_chain->obj_name(), base_file_info()._filename );

        _order->place_or_replace_in_job_chain( job_chain );
        _order->connect_with_standing_order( this );

        result = true;
    }

    return result;
}

//-----------------------------------------------------------------Standing_order::on_dependant_loaded

//bool Standing_order::on_dependant_loaded( File_based* file_based )
//{
//    Order_subsystem_interface* order_subsystem = spooler()->order_subsystem();
//
//    assert( file_based->subsystem() == order_subsystem );
//    assert( file_based->normalized_path() == order_subsystem->normalized_name( _missing_job_chain_path ) );
//
//    Job_chain* job_chain = dynamic_cast<Job_chain*>( file_based );
//    assert( job_chain );
//
//    bool ok = on_activate();
//    assert( ok );
//
//    return ok;
//}

//------------------------------------------------Standing_order::order_is_removable_or_replaceable

bool Standing_order::order_is_removable_or_replaceable()
{
    return !_order  ||
           _order->job_chain_path() == ""  ||  
           _order->is_virgin()  || 
           _order->end_state_reached();
}

//---------------------------------------------------------------Standing_order::can_be_removed_now

bool Standing_order::can_be_removed_now()
{
    //return true;
    return order_is_removable_or_replaceable();
}

//----------------------------------------------------------------Standing_order::prepare_to_remove

bool Standing_order::prepare_to_remove()
{
    //if( order_is_removable_or_replaceable() )
    //{
    //    if( _order )  
    //    {
    //        _its_me_removing = true;
    //        _order->remove_from_job_chain();
    //        _its_me_removing = false;
    //    }
    //}

    return My_file_based::prepare_to_remove();
}

//---------------------------------------------------------------------------Standing_order::remove

void Standing_order::on_remove_now()
{
    if( _order )  
    {
        _its_me_removing = true;
        _order->remove_from_job_chain();
        _its_me_removing = false;
    }
}

//--------------------------------------------------------------Standing_order::can_be_replaced_now

bool Standing_order::can_be_replaced_now()
{
    //return replacement()  &&  
    //       ( !_order  ||
    //         _order->job_chain_path() == ""  ||  
    //         _order->end_state_reached()        );

    return replacement()  &&  order_is_removable_or_replaceable();
}

//---------------------------------------------------------------Standing_order::prepare_to_replace

void Standing_order::prepare_to_replace()
{
    //if( _order )
    //{
    //    if( order_is_removable_or_replaceable() )
    //    {
    //        if( Job_chain* job_chain = folder()->job_chain_folder()->job_chain_or_null( job_chain_name() ) )    // Jobkette bekannt?
    //        {
    //            result = true;
    //        }
    //    }
    //}
}

//-------------------------------------------------------------------Standing_order::on_replace_now

Standing_order* Standing_order::on_replace_now()
{
    ptr<Order> order = replacement()->_order;

    replacement()->_order = NULL;
    set_replacement( NULL );

    set_order( order );
    set_file_based_state( s_loaded );

    return this;
}

//-------------------------------------------------------------Standing_order::on_order_carried_out

void Standing_order::on_order_carried_out()
{
    // Wird auch von on_order_removed() gerufen

    check_for_replacing_or_removing();
}

//-----------------------------------------------------------------Standing_order::on_order_removed

void Standing_order::on_order_removed()
{
    if( !_its_me_removing )
    {
        on_order_carried_out();
    }
}

//--------------------------------------------------------------------------Standing_order::set_dom

void Standing_order::set_dom( const xml::Element_ptr& order_element )
{
    assert( !_order );

    ptr<Order> order = new Order( spooler() );
    order->set_id( order_id() ); 
    order->set_dom( order_element );

    if( order->string_id() != order_id() )  z::throw_xc( "SCHEDULER-436" );
    if( order->job_chain_path() != "" )  z::throw_xc( "SCHEDULER-437" );

    set_order( order );
}

//----------------------------------------------------------------------Standing_order::dom_element

xml::Element_ptr Standing_order::dom_element( const xml::Document_ptr& dom_document, const Show_what& show_what )
{
    xml::Element_ptr result;
    
    if( _order )
    {
        result = _order->dom_element( dom_document, show_what );
    }
    else
    {
        result = dom_document.createElement( "order" );
        fill_file_based_dom_element( result, show_what );
        //if( has_base_file() )  result.appendChild_if( File_based::dom_element( dom_document, show_what ) );
        //if( replacement()   )  result.append_new_element( "replacement" ).appendChild( replacement()->dom_element( dom_document, show_what ) );
    }

    return result;
}

//-------------------------------------------------------------------------Standing_order::obj_name

string Standing_order::obj_name() const
{
    if( _order )
    {
        S result;

        result << "Standing_order";
        if( _order ) result << " " << _order->obj_name();

        return result;
    }
    else
        return file_based< Standing_order, Standing_order_folder, Standing_order_subsystem >::obj_name();
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
