// $Id: scheduler_script.h 13198 2007-12-06 14:13:38Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_SCRIPT_H
#define __SCHEDULER_SCRIPT_H

namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------
    
struct Scheduler_script_folder;
struct Scheduler_script_subsystem;
struct Scheduler_script_subsystem_interface;

extern const Absolute_path      default_scheduler_script_path;

//---------------------------------------------------------------------------------Scheduler_script

struct Scheduler_script : file_based< Scheduler_script, Scheduler_script_folder, Scheduler_script_subsystem_interface >,
                          Object
{
    static bool                 ordering_is_less            ( const Scheduler_script* a, const Scheduler_script* b )  { return a->ordering() < b->ordering(); }
    

                                Scheduler_script            ( Scheduler_script_subsystem* );

    // file_based<>

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Object::Release(); }
    string                      obj_name                    () const                                { return My_file_based::obj_name(); }

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        close                       ();
    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

    bool                        can_be_removed_now          ();


    Module*                     module                      () const;
    Module_instance*            module_instance             () const;
    int                         ordering                    () const                                { return _ordering; }


  private:
    Fill_zero                  _zero_;
    ptr<Module>                _module;                     // <script>
    ptr<Module_instance>       _module_instance;
    ptr<Com_log>               _com_log;                    // COM-Objekt spooler.log
    int                        _ordering;
};

//--------------------------------------------------------------------------Scheduler_script_folder

struct Scheduler_script_folder : typed_folder< Scheduler_script >
{
                                Scheduler_script_folder     ( Folder* );
                               ~Scheduler_script_folder     ();


    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& )  { return doc.createElement( "scheduler_script" ); }

    void                        add_scheduler_script        ( Scheduler_script* scheduler_script )  { add_file_based( scheduler_script ); }
    void                        remove_scheduler_script     ( Scheduler_script* scheduler_script )  { remove_file_based( scheduler_script ); }
    Scheduler_script*           scheduler_script            ( const string& name )                  { return file_based( name ); }
    Scheduler_script*           scheduler_scriptr_or_null   ( const string& name )                  { return file_based_or_null( name ); }
};

//-------------------------------------------------------------Scheduler_script_subsystem_interface

struct Scheduler_script_subsystem_interface : file_based_subsystem<Scheduler_script>,
                                              Object
{
    Scheduler_script*           default_scheduler_script    ()                                      { return scheduler_script( default_scheduler_script_path ); }
    Scheduler_script*           default_scheduler_script_or_null()                                  { return scheduler_script_or_null( default_scheduler_script_path ); }
    Scheduler_script*           scheduler_script            ( const Absolute_path& path )           { return file_based( path ); }
    Scheduler_script*           scheduler_script_or_null    ( const Absolute_path& path )           { return file_based_or_null( path ); }

    virtual ptr<Scheduler_script_folder> new_scheduler_script_folder( Folder* )                     = 0;
    virtual void                set_dom                     ( const xml::Element_ptr& script_element ) = 0;
    virtual bool                needs_java                  () const                                = 0;

  protected:                  
                                Scheduler_script_subsystem_interface( Scheduler*, Type_code );
};

//-------------------------------------------------------------------------------------------------

ptr<Scheduler_script_subsystem_interface> new_scheduler_script_subsystem( Scheduler* );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
