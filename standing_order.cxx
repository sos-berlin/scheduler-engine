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

//------------------------------------------------Standing_order_subsystem::assert_xml_element_name

void Standing_order_subsystem::assert_xml_element_name( const xml::Element_ptr& e ) const
{ 
    if( !e.nodeName_is( "add_order" ) )  File_based_subsystem::assert_xml_element_name( e );
}

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
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << ": " << x.what() ); }
}

//----------------------------------------------------------------------------Standing_order::close

void Standing_order::close()
{
    if( _order )  
    {
        _order->connect_with_standing_order( NULL );
        _order = NULL;
    }

    remove_dependant( order_subsystem(), _job_chain_path );
}

//-------------------------------------------------------------------------Standing_order::set_name

void Standing_order::set_name( const string& name )
{
    file_based< Standing_order, Standing_order_folder, Standing_order_subsystem >::set_name( name );

    split_standing_order_name( name, &_job_chain_name, &_order_id );

    if( _job_chain_name == ""  ||  _order_id == "" )  z::throw_xc( "SCHEDULER-435", base_file_info()._filename );

    _job_chain_path = Absolute_path( folder_path(), _job_chain_name );
}

//--------------------------------------------------------------------Standing_order::on_initialize

bool Standing_order::on_initialize()
{
    bool result = true;

    add_dependant( order_subsystem(), _job_chain_path );

    if( Job_chain* job_chain = folder()->job_chain_folder()->job_chain_or_null( job_chain_name() ) )
    {
        result = job_chain->file_based_state() >= File_based::s_loaded;
    }
    else
        result = false;

    return result;
}

//--------------------------------------------------------------------------Standing_order::on_load

bool Standing_order::on_load()
{
    return true;
}

//----------------------------------------------------------------------Standing_order::on_activate

bool Standing_order::on_activate()
{
    if( !_order )  assert(0), z::throw_xc( Z_FUNCTION, "no order" );

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
    return order_is_removable_or_replaceable();
}

//---------------------------------------------------------------------------Standing_order::remove

void Standing_order::on_remove_now()
{
    if( _order )
    {
        ptr<Order> order = _order;
        
        _order->connect_with_standing_order( NULL );    // Verbindung zwischen Order und Standing_order lösen,
        _order = NULL;                                  // dass remove_job_chain() nicht check_for_replacing_or_removing() rufe!
                                            
        order->remove_from_job_chain();
    }
}

//--------------------------------------------------------------------------Standing_order::set_dom

void Standing_order::set_dom( const xml::Element_ptr& element )
{
    assert( !_order );

    subsystem()->assert_xml_element_name( element );

    ptr<Order> order = new Order( spooler() );
    order->set_id( order_id() ); 
    order->set_dom( element );

    if( order->string_id() != order_id() )  z::throw_xc( "SCHEDULER-436" );     // Bei <add_order> vom Schema erlaubt
    
    if( !order->job_chain_path().empty() )  z::throw_xc( "SCHEDULER-437", order->job_chain_path(), _job_chain_path );     // Order->set_dom() liest Attribut job_chain nicht!

    Absolute_path job_chain_path ( folder_path(), element.getAttribute( "job_chain" ) );  // Bei <add_order> vom Schema erlaubt
    if( !job_chain_path.empty()  &&  subsystem()->normalized_path( job_chain_path ) != subsystem()->normalized_path( _job_chain_path ) )  z::throw_xc( "SCHEDULER-437", order->job_chain_path(), _job_chain_path );

    _order = order;
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

//-------------------------------------------------------------------------------------------------

} //namespace standing_order
} //namespace scheduler
} //namespace sos
