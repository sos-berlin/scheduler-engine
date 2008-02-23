// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_FOLDER_H
#define __SCHEDULER_FOLDER_H

#include "../zschimmer/z_md5.h"

namespace sos {
namespace scheduler {
namespace folder {

//-------------------------------------------------------------------------------------------------

struct Folder;
struct Folder_subsystem;
struct Typed_folder;
struct File_based;
struct File_based_subsystem;
struct Subfolder_folder;

//--------------------------------------------------------------------------------------------const

extern const char               folder_separator;

//------------------------------------------------------------------------------------------Pendant

struct Pendant                  // Abhängig von anderen File_based
{
                                Pendant                     ();
    virtual                    ~Pendant                     ();

    virtual string              obj_name                    () const                                = 0;
    void                        add_dependant               ( File_based_subsystem*, const Absolute_path& );
    void                        remove_dependant            ( File_based_subsystem*, const Absolute_path& );
    void                        remove_dependants           ();

    virtual bool                on_dependant_loaded         ( File_based* )                         = 0;
    virtual bool                on_dependant_to_be_removed  ( File_based* );
    virtual void                on_dependant_removed        ( File_based* );
    virtual Prefix_log*         log                         ()                                      = 0;

    
  private:
    typedef stdext::hash_set< string >                                Dependant_set;
    typedef stdext::hash_map< File_based_subsystem*, Dependant_set >  Dependant_sets;

    Dependant_sets             _dependants_sets;
};

//-------------------------------------------------------------------------------------Dependencies

struct Dependencies 
{
                                Dependencies                ( File_based_subsystem* );
                               ~Dependencies                ();

    void                        add_dependant               ( Pendant*, const string& missing_path );
    void                        remove_dependant            ( Pendant*, const string& missing_path );
    void                        announce_dependant_loaded   ( File_based* found_missing );
    bool                        announce_dependant_to_be_removed( File_based* to_be_removed );
  //void                        announce_dependant_removed  ( File_based* );


  private:
    typedef stdext::hash_set< Pendant* >               Requestor_set;
    typedef stdext::hash_map< string, Requestor_set >  Path_requestors_map;            // Vermisster Pfad -> { Requestor ... }

    Fill_zero                  _zero_;
    File_based_subsystem*      _subsystem;
    Path_requestors_map        _path_requestors_map;;
};

//-----------------------------------------------------------------------------------Base_file_info

struct Base_file_info
{
                                Base_file_info              ()                                      : _last_write_time(0) {}
                                Base_file_info              ( const directory_observer::Directory_entry& );
                                Base_file_info              ( const string& filename, time_t timestamp_utc, const string& normalized_name ) 
                                                                                                    : _filename(filename), _last_write_time(timestamp_utc),
                                                                                                      _normalized_name(normalized_name) {}

    bool                        operator <                  ( const Base_file_info& f ) const       { return _normalized_name < f._normalized_name; }
  //bool                        operator <                  ( const Base_file_info& f ) const       { return _filename < f._filename; }
    bool                        operator ==                 ( const Base_file_info& f ) const       { return _filename == f._filename  &&
                                                                                                             _last_write_time == f._last_write_time; }
    bool                        operator !=                 ( const Base_file_info& f ) const       { return !( *this == f ); }

    static bool                 less_dereferenced           ( const Base_file_info* a, const Base_file_info* b )  { return *a < *b; }

    string                     _filename;                   // == _path.name()
    File_path                  _path;
    time_t                     _last_write_time;            // Derselbe Typ wie zschimmer::file::File_info::last_write_time()
    string                     _normalized_name;            // Ohne Dateinamenserweiterung
};

//---------------------------------------------------------------------------------------File_based

struct File_based : Scheduler_object,
                    Pendant,
                    Has_includes,
                    zschimmer::Has_addref_release
{
    enum State
    {
        s_undefined,            // Fehler in XML-Definition
        s_not_initialized,      // on_initialized() gscheitert
        s_initialized,          // on_initiailzed() ok, Objekt sieht gut aus
        s_loaded,               // Mit Daten gefüllt: bei Jobs die Task-Warteschlange, bei Jobketten die Auftragswarteschlangen
        s_active,
        s_closed
    };


    enum Base_file_event
    {
        bfevt_added,
        bfevt_modified,
        bfevt_removed
    };



                                File_based                  ( File_based_subsystem*, IUnknown*, Type_code );
    virtual                    ~File_based                  ();


    // Scheduler_object
    void                        close                       ();
    string                      obj_name                    () const;


    // Pendant
    bool                        on_dependant_loaded         ( File_based* );
    bool                        on_dependant_to_be_removed  ( File_based* );
    void                        on_dependant_removed        ( File_based* );
    Prefix_log*                 log                         ()                                      { return Scheduler_object::log(); }


    // Has_includes
    Spooler*                    spooler                     () const                                { return Scheduler_object::spooler(); }
    Which_configuration         which_configuration         () const                                { return _which_configuration; }


    void                        fill_file_based_dom_element ( const xml::Element_ptr& element, const Show_what& );
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& );
    virtual bool                is_visible_in_xml_folder    ( const Show_what& ) const              { return true; }


    File_based_subsystem*       subsystem                   () const                                { return _file_based_subsystem; }
    bool                        is_in_folder                () const                                { return _typed_folder != NULL; }
    Typed_folder*               typed_folder                () const                                { return _typed_folder; }
    Folder*                     folder                      () const;
    bool                        has_base_file               () const                                { return _base_file_info._filename != ""; }
    const Base_file_info&       base_file_info              () const                                { return _base_file_info; }
    bool                        base_file_has_error         () const                                { return _base_file_xc_time != 0; }
    const zschimmer::Xc&        base_file_exception         () const                                { return _base_file_xc; }
    bool                        base_file_is_removed        () const                                { return _file_is_removed; }
    void                        assert_is_initialized       ();
    void                        assert_is_loaded            ();
    void                        assert_is_active            ();
    void                        assert_is_not_initialized   ();

    virtual void            set_name                        ( const string& name );
    string                      name                        () const                                { return _name; }
    void                        fix_name                    ()                                      { _name_is_fixed = true; }
    Absolute_path               path                        () const;
    string                      normalized_name             () const;
    string                      normalized_path             () const;
    File_path                   configuration_root_directory() const;

    State                       file_based_state            () const                                { return _state; }
    string                      file_based_state_name       () const                                { return file_based_state_name( file_based_state() ); } 
    static string               file_based_state_name       ( State );
  //void                    set_defined                     ();                                     // Für Objekte, die kein XML brauchen

    void                    set_to_be_removed               ( bool );
    bool                     is_to_be_removed               () const                                { return _is_to_be_removed; }

    void                    set_replacement                 ( File_based* );
    File_based*                 replacement                 () const                                { return _replacement; }

    void                    set_typed_folder                ( Typed_folder* );                      // Nur für Typed_folder!
    void                    set_folder_path                 ( const Absolute_path& );
    Absolute_path               folder_path                 () const;

    enum When_to_act { act_later, act_now };
    void                        check_for_replacing_or_removing( When_to_act = act_later );

    bool                        initialize                  ();
    bool                        load                        ();
    bool                        activate                    ();
    bool                        switch_file_based_state     ( State  );
  //bool                        try_switch_wished_file_based_state();
    enum Remove_flags { rm_default, rm_base_file_too };
    bool                        remove                      ( Remove_flags = rm_default );
    void                        remove_base_file            ();

    void                        remove_now                  ();
    File_based*                 replace_now                 ();

    void                        handle_event                ( Base_file_event );

    virtual void                set_dom                     ( const xml::Element_ptr& )             = 0;
    virtual bool                on_initialize               ()                                      = 0;
    virtual bool                on_load                     ()                                      = 0;
    virtual bool                on_activate                 ()                                      = 0;

    virtual void                on_remove_now               ();
    virtual void                prepare_to_remove           ();
    virtual bool                can_be_removed_now          ()                                      = 0;
    virtual zschimmer::Xc       remove_error                ();

    virtual bool                replace_with                ( File_based* );
    virtual void                prepare_to_replace          ();
    virtual bool                can_be_replaced_now         ();
    virtual File_based*         on_replace_now              ();

    bool                        operator <                  ( const File_based& f ) const           { return _base_file_info < f._base_file_info; }
    static bool                 less_dereferenced           ( const File_based* a, const File_based* b )  { return *a < *b; }


  protected:
    void                    set_base_file_info              ( const Base_file_info& bfi )           { _base_file_info = bfi; }
    void                    set_file_based_state            ( State );

    Fill_zero                  _zero_;

  private:
    friend struct               Typed_folder;
    friend struct               Subfolder_folder;

    bool                        initialize2                 ();
    bool                        load2                       ();
    bool                        activate2                   ();

    string                     _name;
    State                      _state;
    Base_file_info             _base_file_info;
    bool                       _name_is_fixed;
  //Md5                        _md5;
    zschimmer::Xc              _base_file_xc;
    double                     _base_file_xc_time;
    zschimmer::Xc              _remove_xc;
    int                        _duplicate_version;
    bool                       _file_is_removed;
    bool                       _is_to_be_removed;
    Which_configuration        _which_configuration;        // Aus live/ oder aus cache/ ?
    ptr<File_based>            _replacement;
    Absolute_path              _folder_path;                // assert( !is_in_folder()  ||  _folder_path == folder()->path() )
    Typed_folder*              _typed_folder;
    File_based_subsystem*      _file_based_subsystem;
};

//-------------------------------------------------------------------------------------file_based<>

template< class FILE_BASED, class TYPED_FOLDER, class SUBSYSTEM >
struct file_based : File_based
{                                                           
                                file_based                  ( SUBSYSTEM* p, IUnknown* iu, Type_code t ): File_based( p, iu, t ) {}

    typedef SUBSYSTEM           My_subsystem;
    typedef file_based          My_file_based;

    SUBSYSTEM*                  subsystem                   () const                                { return static_cast<SUBSYSTEM*>( File_based::subsystem() ); }

    FILE_BASED*                 replacement                 () const                                { return static_cast<FILE_BASED*>( File_based::replacement() ); }
    TYPED_FOLDER*               typed_folder                () const                                { return static_cast<TYPED_FOLDER*>( File_based::typed_folder() ); }
};

//-------------------------------------------------------------------------------------Typed_folder
// Order mit Objekten nur eines Typs

struct Typed_folder : Scheduler_object, 
                      Object
{
                                Typed_folder                ( Folder*, Type_code );

    Folder*                     folder                      () const                                { return _folder; }
    File_based_subsystem*       subsystem                   () const                                { return file_based_subsystem(); }

    bool                        adjust_with_directory       ( const list< const directory_observer::Directory_entry* >& );
    File_based*                 file_based                  ( const string& name ) const;
    File_based*                 file_based_or_null          ( const string& name ) const;
  //void                        remove_all_file_baseds      ();

    string                      obj_name                    () const;

    void                        add_file_based              ( File_based* );
    void                        remove_file_based           ( File_based* );
    File_based*                 replace_file_based          ( File_based* );
    bool                        is_empty                    () const                                { return _file_based_map.empty(); }

    ptr<File_based>             new_initialized_file_based_xml( const xml::Element_ptr&, const string& default_name = "" );
    void                        add_file_based_xml            ( const xml::Element_ptr&, const string& default_name = "" );
    void                        add_or_replace_file_based_xml ( const xml::Element_ptr&, const string& default_name = "" );
    void                        add_to_replace_or_remove_candidates( const File_based& file_based );
    void                        handle_replace_or_remove_candidates();
    void                        ignore_duplicate_configuration_file( File_based*, File_based*, const directory_observer::Directory_entry& );
    void                        remove_duplicates_from_list ( list< const directory_observer::Directory_entry* >* );

    virtual File_based_subsystem* file_based_subsystem      () const                                = 0;
    virtual bool                is_empty_name_allowed       () const                                { return false; }
    virtual bool                on_base_file_changed        ( File_based*, const directory_observer::Directory_entry* changed_base_file_info );
    virtual void                set_dom                     ( const xml::Element_ptr& );
    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& );
    virtual xml::Element_ptr    new_dom_element             ( const xml::Document_ptr&, const Show_what& ) = 0;

  private:
    Fill_zero                  _zero_;
    Folder*                    _folder;
    String_set                 _replace_or_remove_candidates_set;

  protected:

  public:
    typedef stdext::hash_map< string, File_based* >  File_based_map;     // File_based::normalized_name() --> File_based
    File_based_map             _file_based_map;    
};

//-----------------------------------------------------------------------------------typed_folder<>

template< class FILE_BASED >
struct typed_folder : Typed_folder
{
    typedef typename FILE_BASED::My_subsystem My_subsystem;


                                typed_folder                ( My_subsystem* subsystem, Folder* f, Type_code tc )
                                                                                                    : Typed_folder( f, tc ), _subsystem(subsystem) {}

    File_based_subsystem*       file_based_subsystem        () const                                { return _subsystem; }
    My_subsystem*               subsystem                   () const                                { return _subsystem; }

    FILE_BASED*                 file_based                  ( const string& name ) const            { return static_cast<FILE_BASED*>( Typed_folder::file_based( name ) ); }
    FILE_BASED*                 file_based_or_null          ( const string& name ) const            { return static_cast<FILE_BASED*>( Typed_folder::file_based_or_null( name ) ); }

    FILE_BASED*                 replace_file_based          ( FILE_BASED* file_based )              { return static_cast<FILE_BASED*>( Typed_folder::replace_file_based( file_based ) ); }

  private:
    My_subsystem*              _subsystem;
};

//-------------------------------------------------------------------------------------------Folder
//
// Ein Ordner (Folder) enthält alle Objekte. 
// Bislang gibt es nur einen Ordner im Scheduler.

struct Folder : file_based< Folder, Subfolder_folder, Folder_subsystem >, 
                Object
{
    static int                  position_of_extension_point ( const string& filename );
    static string               object_name_of_filename     ( const string& filename );
    static string               extension_of_filename       ( const string& filename );


                                Folder                      ( Folder_subsystem*, Folder* parent );
                               ~Folder                      ();


    // file_based<>

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Object::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Object::Release(); }

    void                        close                       ();
    bool                        on_initialize               ();
    bool                        on_load                     ();
    bool                        on_activate                 ();
    void                        set_name                    ( const string& );
    void                        set_dom                     ( const xml::Element_ptr& )             { zschimmer::throw_xc( Z_FUNCTION ); }
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                        prepare_to_remove           ();
    bool                        can_be_removed_now          ();


  //void                        remove_all_file_baseds      ();
    bool                        is_root                     () const;
    file::File_path             directory                   () const;
    Absolute_path               make_path                   ( const string& name );                 // Hängt den Ordernamen voran
    bool                        is_valid_extension          ( const string& extension );

    bool                        adjust_with_directory       ( directory_observer::Directory* );

    Process_class_folder*       process_class_folder        ()                                      { return _process_class_folder; }
    lock::Lock_folder*          lock_folder                 ()                                      { return _lock_folder; }
    Job_folder*                 job_folder                  ()                                      { return _job_folder; }
    Job_chain_folder_interface* job_chain_folder            ()                                      { return _job_chain_folder; }
    Standing_order_folder*      standing_order_folder       ()                                      { return _standing_order_folder; }
    Scheduler_script_folder*    scheduler_script_folder     ();

    string                      obj_name                    () const;

  private:
    void                        add_to_typed_folder_map     ( Typed_folder* );

    Fill_zero                  _zero_;

    typedef stdext::hash_map< string, Typed_folder* >  Typed_folder_map;
    typedef stdext::hash_set< ptr<Folder> >            Folder_set;

    Typed_folder_map           _typed_folder_map;
    Folder*                    _parent;
    Folder_set                 _child_folder_set;

    ptr<Scheduler_script_folder>    _scheduler_script_folder;
    ptr<Process_class_folder>       _process_class_folder;
    ptr<lock::Lock_folder>          _lock_folder;
    ptr<Job_folder>                 _job_folder;
    ptr<Job_chain_folder_interface> _job_chain_folder;
    ptr<Standing_order_folder>      _standing_order_folder;
    ptr<Subfolder_folder>           _subfolder_folder;         // Unterordner

  //stdext::hash_set<string>        _known_duplicate_filenames;       // Doppelte Dateien mit unterschiedlicher Großschreibung (vor der Normalisierung)
};

//---------------------------------------------------------------------------------Subfolder_folder

struct Subfolder_folder : typed_folder< Folder >
{
                                Subfolder_folder            ( Folder* );
                               ~Subfolder_folder            ();


    // Typed_folder
    bool                        on_base_file_changed        ( File_based*, const directory_observer::Directory_entry* );


    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& )  { return doc.createElement( "folders" ); }
};

//-----------------------------------------------------------------------------File_based_subsystem
// Für jeden dateibasierten Typ (File_based) gibt es genau ein File_based_subsystem

struct File_based_subsystem : Subsystem
{
                                File_based_subsystem        ( Spooler*, IUnknown*, Type_code );
    virtual                    ~File_based_subsystem        ()                                      {}

    Dependencies*               dependencies                ()                                      { return &_dependencies; }

    virtual void                check_file_based_element    ( const xml::Element_ptr& );
    virtual string              object_type_name            () const                                = 0;
    virtual string              filename_extension          () const                                = 0;
    virtual void                assert_xml_element_name     ( const xml::Element_ptr& ) const;
    virtual void                assert_xml_elements_name    ( const xml::Element_ptr& ) const;
    virtual string              xml_element_name            () const                                = 0;
    virtual string              xml_elements_name           () const                                = 0;
    virtual string              normalized_name             ( const string& name ) const            { return name; }
    virtual string              normalized_path             ( const Path& path ) const;
    virtual xml::Element_ptr    file_baseds_dom_element     ( const xml::Document_ptr&, const Show_what& ) = 0;
    virtual xml::Element_ptr    new_file_baseds_dom_element ( const xml::Document_ptr&, const Show_what& ) = 0;

  protected:
    friend struct               Typed_folder;

    virtual ptr<File_based>     call_new_file_based         ()                                      = 0;
    virtual void                add_file_based              ( File_based* )                         = 0;
    virtual void                remove_file_based           ( File_based* )                         = 0;
    virtual void                replace_file_based          ( File_based*, File_based* )            = 0;

  private:
    void                        normalized_name             ( const Path& ) const;                  // Nicht implementiert! normalized_path() sollte aufgerufen werden?

    Dependencies               _dependencies;
};

//---------------------------------------------------------------------------file_based_subsystem<>

template< class FILE_BASED >
struct file_based_subsystem : File_based_subsystem
{
                                file_based_subsystem        ( Spooler* spooler, IUnknown* u, Type_code t ) : File_based_subsystem( spooler, u, t ), _zero_(this+1) {}

    bool                        is_empty                    () const                                { return _file_based_map.empty(); }
    int                         file_based_map_version      () const                                { return _file_based_map_version; }
    ptr<File_based>             call_new_file_based         ()                                      { return +new_file_based(); }
    virtual ptr<FILE_BASED>     new_file_based              ()                                      = 0;


    void close()
    {
        vector<FILE_BASED*> ordered_file_baseds = this->ordered_file_baseds();

        Z_FOR_EACH_REVERSE( typename vector<FILE_BASED*>, ordered_file_baseds, it )
        {
            ptr<File_based> file_based = *it;
            
            try
            {
                file_based->remove();
            }
            catch( exception& x ) { file_based->log()->error( x.what() ); }
        }
    }
    

    bool subsystem_initialize()
    {
        vector<FILE_BASED*> ordered_file_baseds = this->ordered_file_baseds();

        Z_FOR_EACH( typename vector<FILE_BASED*>, ordered_file_baseds, it )
        {
            File_based* file_based = +*it;

            try
            {
                file_based->initialize();
            }
            catch( exception& x ) { file_based->log()->error( x.what() ); }
        }

        return true;
    }
    

    bool subsystem_load()
    {
        vector<FILE_BASED*> ordered_file_baseds = this->ordered_file_baseds();

        Z_FOR_EACH( typename vector<FILE_BASED*>, ordered_file_baseds, it )
        {
            File_based* file_based = +*it;

            try
            {
                file_based->load();
            }
            catch( exception& x ) { file_based->log()->error( x.what() ); }
        }

        return true;
    }
    

    bool subsystem_activate()
    {
        vector<FILE_BASED*> ordered_file_baseds = this->ordered_file_baseds();

        Z_FOR_EACH( typename vector<FILE_BASED*>, ordered_file_baseds, it )
        {
            File_based* file_based = +*it;

            try
            {
                file_based->activate();
            }
            catch( exception& x ) { file_based->log()->error( x.what() ); }
        }

        return true;
    }
    

    virtual vector<FILE_BASED*> ordered_file_baseds()
    {
        vector<FILE_BASED*> result;
        
        result.reserve( _file_based_map.size() );
        Z_FOR_EACH( typename File_based_map, _file_based_map, fb )  result.push_back( fb->second );

        //Wir müssen nicht ordnen und ordering_is_less() betrachtet nicht den Pfad: sort( result, result.begin(), result.end(), File_based::ordering_is_less );

        return result;
    }


    FILE_BASED* file_based_or_null( const Absolute_path& path ) const
    {
        Absolute_path p ( normalized_path( path ) );

        typename File_based_map::const_iterator it = _file_based_map.find( p );
        return it == _file_based_map.end()? NULL 
                                          : it->second;
    }

    
    FILE_BASED* file_based( const Absolute_path& path ) const
    {
        FILE_BASED* result = file_based_or_null( path );
        if( !result )  z::throw_xc( "SCHEDULER-161", object_type_name(), path );
        return result;
    }


    FILE_BASED* active_file_based( const Absolute_path& path ) const
    {
        FILE_BASED* result = file_based( path );
        result->assert_is_active();

        return result;
    }


    xml::Element_ptr file_baseds_dom_element( const xml::Document_ptr& document, const Show_what& show_what )
    {
        xml::Element_ptr result = new_file_baseds_dom_element( document, show_what );

        Z_FOR_EACH( typename File_based_map, _file_based_map, it )
        {
            FILE_BASED* file_based = it->second;
            
            if( file_based->is_visible_in_xml_folder( show_what ) )
            {
                result.appendChild( file_based->dom_element( document, show_what ) );
            }
        }

        return result;
    }


  private:
    friend struct               Typed_folder;
    friend struct               Folder_subsystem;


    void add_file_based( File_based* file_based )                   // Nur für Typed_folder::add_file_based()
    {
        assert( !file_based_or_null( file_based->path() ) );

        FILE_BASED* casted_file_based = dynamic_cast<FILE_BASED*>( file_based );
        if( !casted_file_based )  assert(0), z::throw_xc( Z_FUNCTION );

        _file_based_map[ file_based->normalized_path() ] = casted_file_based;
        _file_based_map_version++;
    }


    void remove_file_based( File_based* file_based )                // Nur für Typed_folder::remove_file_based()
    {
        assert( file_based_or_null( file_based->path() ) );

        FILE_BASED* casted_file_based = dynamic_cast<FILE_BASED*>( file_based );
        if( !casted_file_based )  assert(0), z::throw_xc( Z_FUNCTION );

        //casted_file_based->log( subsystem_state() < subsys_stopped? log_info : log_debug9, message_string( "SCHEDULER-861", object_type_name() ) );

        _file_based_map.erase( casted_file_based->normalized_path() );
        _file_based_map_version++;
    }


    void replace_file_based( File_based* old_file_based, File_based* new_file_based )       // Nur für Typed_folder::replace_file_based()
    {
        assert( file_based_or_null( old_file_based->path() ) );

        FILE_BASED* casted_old_file_based = dynamic_cast<FILE_BASED*>( old_file_based );
        if( !casted_old_file_based )  assert(0), z::throw_xc( Z_FUNCTION );

        FILE_BASED* casted_new_file_based = dynamic_cast<FILE_BASED*>( new_file_based );
        if( !casted_new_file_based )  assert(0), z::throw_xc( Z_FUNCTION );

        _file_based_map[ casted_old_file_based->normalized_path() ] = casted_new_file_based;
        _file_based_map_version++;
    }


  public:
    Fill_zero                  _zero_;
    typedef stdext::hash_map< string, ptr< FILE_BASED > >  File_based_map;
    File_based_map             _file_based_map;

  private:
    int                        _file_based_map_version;
};

//------------------------------------------------------------------------------------Configuration

struct Configuration
{
    ptr<directory_observer::Directory_observer>  _directory_observer;                         // Konfigurationsverzeichnis
  //ptr<Include_register>                        _include_register;
};

//---------------------------------------------------------------------------------Folder_subsystem

struct Folder_subsystem : Object,
                          file_based_subsystem<Folder>,
                          directory_observer::Directory_observer::Directory_handler

{
                                Folder_subsystem            ( Scheduler* );
                               ~Folder_subsystem            ();

    // Subsystem
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();
                     Subsystem::obj_name;


    // file_based_subsystem
    string                      object_type_name            () const                                { return "Folder"; }
    string                      filename_extension          () const                                { return "/"; }             // Wird nicht verwendet
    string                      xml_element_name            () const                                { assert(0), z::throw_xc( Z_FUNCTION ); }
    string                      xml_elements_name           () const                                { assert(0), z::throw_xc( Z_FUNCTION ); }
  //string                      normalized_name             ( const string& name ) const            { return name; }
    ptr<Folder>                 new_file_based              ();
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "folders" ); }


    // directory_observer::Directory_observer::Directory_handler
    bool                        on_handle_directory         ( directory_observer::Directory_observer* );


    void                        initialize_cache_directory  ();

    Folder*                     folder                      ( const Absolute_path& path )           { return file_based( path ); }
    Folder*                     folder_or_null              ( const Absolute_path& path )           { return file_based_or_null( path ); }
    Folder*                     root_folder                 () const                                { return _root_folder; }
    ptr<Subfolder_folder>       new_subfolder_folder        ( Folder* folder )                      { return Z_NEW( Subfolder_folder( folder ) ); }
    bool                        is_valid_extension          ( const string& );
    Configuration*              configuration               ( Which_configuration );

    void                    set_signaled                    ( const string& text );

  //void                    set_read_again_at_or_later      ( double at )                           { if( _read_again_at < at )  _read_again_at = at; }
    ptr<directory_observer::Directory> merged_cache_and_local_directories();

    bool                        handle_folders              ( double minimum_age = 0 );
    xml::Element_ptr            execute_xml                 ( const xml::Element_ptr& );


  private:
    Fill_zero                  _zero_;
    ptr<Folder>                _root_folder;
    double                     _last_change_at;

    vector<Configuration>      _configurations;
    //ptr<directory_observer::Directory_observer>  _local_directory_observer;                         // Konfigurationsverzeichnis
    //ptr<Includes_register>                       _local_includes;

    //ptr<directory_observer::Directory_observer>  _cache_directory_observer;                         // Cache mit Konfiguration vom Supervisor, s. Supervisor_client
    //ptr<Includes_register>                       _cache_includes;
};


inline ptr<Folder_subsystem>    new_folder_subsystem        ( Scheduler* scheduler )                { return Z_NEW( Folder_subsystem( scheduler ) ); }

//------------------------------------------------------------------------------FOR_EACH_FILE_BASED

#define FOR_EACH_FILE_BASED( FILE_BASED_CLASS, OBJECT ) \
    Z_FOR_EACH( FILE_BASED_CLASS::My_subsystem::File_based_map, spooler()->subsystem( (FILE_BASED_CLASS*)NULL )->_file_based_map, __file_based_iterator__ )  \
        if( FILE_BASED_CLASS* OBJECT = __file_based_iterator__->second )

//-------------------------------------------------------------------------------------------------

} //namespace folder
} //namespace scheduler
} //namespace sos

#endif
