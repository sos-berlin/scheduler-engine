// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SCHEDULER_STANDING_ORDER_H
#define __SCHEDULER_STANDING_ORDER_H

namespace sos {
namespace scheduler {
namespace order {

//---------------------------------------------------------------------------------------------Standing_order

struct Standing_order : file_based< Standing_order, Standing_order_folder, Standing_order_subsystem >,
                        Object
{
                                Standing_order              ( Standing_order_subsystem* );
                               ~Standing_order              ();


    // Scheduler_object

    void                        close                       ();
    string                      obj_name                    () const;


    // file_based<>

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Object::Release(); }

    void                        set_name                    ( const string& );
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

    bool                        can_be_removed_now          ();
    void                        on_remove_now               ();


    //

    Standing_order_folder*      standing_order_folder       () const                                { return typed_folder(); }
    string                      job_chain_name              () const                                { return _job_chain_name; }
    string                      order_id                    () const                                { return _order_id; }

    
  private:
    bool                        order_is_removable_or_replaceable();
  //void                        set_order                   ( Order* );


    Fill_zero                  _zero_;
    string                     _job_chain_name;
    Absolute_path              _job_chain_path;
    string                     _order_id;
    ptr<Order>                 _order;
    bool                       _its_me_removing;
};

//----------------------------------------------------------------------------Standing_order_folder

struct Standing_order_folder : typed_folder< Standing_order >
{
                                Standing_order_folder       ( Folder* );
                               ~Standing_order_folder       ();


  //void                        set_dom                     ( const xml::Element_ptr& );
  //void                        execute_xml_standing_order  ( const xml::Element_ptr& );
  //xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& )  { return doc.createElement( "standing_orders" ); }

    void                        add_standing_order          ( Standing_order* standing_order )      { add_file_based( standing_order ); }
    void                        remove_standing_order       ( Standing_order* standing_order )      { remove_file_based( standing_order ); }
    Standing_order*             standing_order              ( const string& name )                  { return file_based( name ); }
    Standing_order*             standing_order_or_null      ( const string& name )                  { return file_based_or_null( name ); }
};

//-------------------------------------------------------------------------Standing_order_subsystem

struct Standing_order_subsystem : file_based_subsystem< Standing_order >,
                                  Object
{
                                Standing_order_subsystem    ( Scheduler* );

    // Subsystem

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();



    // File_based_subsystem

    string                      object_type_name            () const                                { return "Standing_order"; }
    string                      filename_extension          () const                                { return ".order.xml"; }
    void                        assert_xml_element_name     ( const xml::Element_ptr& ) const;
    string                      xml_element_name            () const                                { return "order"; }
    string                      xml_elements_name           () const                                { assert(0), z::throw_xc( Z_FUNCTION ); }
    string                      normalized_name             ( const string& ) const;
    ptr<Standing_order>         new_file_based              ();
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "standing_orders" ); }


    ptr<Standing_order_folder>  new_standing_order_folder   ( Folder* folder )                      { return Z_NEW( Standing_order_folder( folder ) ); }


    Standing_order*             standing_order              ( const Absolute_path& path ) const     { return file_based( path ); }
    Standing_order*             standing_order_or_null      ( const Absolute_path& path ) const     { return file_based_or_null( path ); }

  //xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );
};

inline ptr<Standing_order_subsystem> new_standing_order_subsystem( Scheduler* scheduler )           { return Z_NEW( Standing_order_subsystem( scheduler ) ); }

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace scheduler
} //namespace sos

#endif
