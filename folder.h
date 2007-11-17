// $Id: cluster.h 5126 2007-07-13 08:59:30Z jz $

#ifndef __SCHEDULER_FOLDER_H
#define __SCHEDULER_FOLDER_H

#include "../zschimmer/z_md5.h"

namespace sos {
namespace scheduler {
namespace folder {

//-------------------------------------------------------------------------------------------------

struct Absolute_path;
struct Folder;
struct Folder_subsystem;
struct Typed_folder;
struct File_based;
struct File_based_subsystem;
struct Subfolder_folder;

//--------------------------------------------------------------------------------------------const

const extern char               folder_separator;
extern const Absolute_path      root_path;

//---------------------------------------------------------------------------------------------Path

struct Path : string
{
                                Path                        ()                                      {}
                                Path                        ( const string& path )                  { set_path( path ); }
                                Path                        ( const char* path )                    { set_path( path ); }
                                Path                        ( const string& directory, const string& tail_path );

    Path&                       operator =                  ( const string& path )                  { set_path( path );  return *this; }
    Path&                       operator =                  ( const char* path )                    { set_path( path );  return *this; }


    void                    set_name                        ( const string& );                                
    string                      name                        () const;
    void                    set_folder_path                 ( const string& );
    Path                        folder_path                 () const;
  //void                    set_absolute_if_relative        ( const Absolute_path& );
    void                    set_absolute                    ( const Absolute_path& absolute_base, const Path& relative );
  //void                        prepend_folder_path         ( const string& );
    const string&               to_string                   () const                                { return *static_cast<const string*>( this ); }
    void                    set_path                        ( const string& path )                  { *static_cast<string*>( this ) = path; }
    bool                     is_absolute_path               () const;
    string                      absolute_path               () const;
    bool                     is_root                        () const;
    string                      to_filename                 () const;

  private:
    bool                        operator <                  ( const File_path& path ) const         { return compare( path ) <  0; }
    bool                        operator <=                 ( const File_path& path ) const         { return compare( path ) <= 0; }
    bool                        operator ==                 ( const File_path& path ) const         { return compare( path ) == 0; }
    bool                        operator !=                 ( const File_path& path ) const         { return compare( path ) != 0; }
    bool                        operator >=                 ( const File_path& path ) const         { return compare( path ) >= 0; }
    bool                        operator >                  ( const File_path& path ) const         { return compare( path ) >  0; }
    int                         compare                     ( const File_path& ) const;             // Nicht implementiert, weil Großkleinschreibung manchmal beachtet werden muss
};


inline void insert_into_message( Message_string* m, int index, const Path& path ) throw()           { return m->insert( index, path.to_string() ); }

//-------------------------------------------------------------------------------------------------

struct Absolute_path : Path
{
                                Absolute_path               ()                                      {}
                              //Absolute_path               ( const string& path )                  { set_path( path ); }
                              //Absolute_path               ( const char* path )                    { set_path( path ); }
                                Absolute_path               ( const Absolute_path& absolute_directory, const string& path )  { set_absolute( absolute_directory, path ); }
                                Absolute_path               ( const Absolute_path& absolute_directory, const Bstr&   path )  { set_absolute( absolute_directory, string_from_bstr( path ) ); }
                                Absolute_path               ( const Absolute_path& absolute_directory, const char*   path )  { set_absolute( absolute_directory, path ); }
    explicit                    Absolute_path               ( const Path& );

    void                    set_path                        ( const string& path );

    string                      with_slash                  () const;
    string                      without_slash               () const;

  private: 
    Absolute_path&              operator =                  ( const string& path );                 // Nicht implementiert
  //Absolute_path&              operator =                  ( const char* path );
};

//------------------------------------------------------------------------------------------Pendant

struct Pendant
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
                                Base_file_info              ()                                      : _timestamp_utc(0) {}

                                Base_file_info              ( const string& filename, double timestamp_utc, const string& normalized_name, double clock ) 
                                                                                                    : _filename(filename), _timestamp_utc(timestamp_utc),
                                                                                                      _normalized_name(normalized_name), 
                                                                                                      _info_timestamp(clock) {}

    bool                        operator <                  ( const Base_file_info& f ) const       { return _normalized_name < f._normalized_name; }
    static bool                 less_dereferenced           ( const Base_file_info* a, const Base_file_info* b )  { return *a < *b; }

    string                     _filename;                   // Ohne Verzeichnis
    double                     _timestamp_utc;
    string                     _normalized_name;            // Ohne Dateinamenserweiterung
    double                     _info_timestamp;             // Wann dieses Base_file_info erstellt worden ist
};

//---------------------------------------------------------------------------------------File_based

struct File_based : Scheduler_object,
                    Pendant,
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
    //State                      _wished_state;
    Base_file_info             _base_file_info;
    bool                       _name_is_fixed;
    bool                       _read_again;                 // Wegen ungenauer Zeitstempel und langsam schreibender Editoren jede Datei zweimal Lesen
    bool                       _error_ignored;
    Md5                        _md5;
    zschimmer::Xc              _base_file_xc;
    double                     _base_file_xc_time;
    zschimmer::Xc              _remove_xc;
    double                     _base_file_check_removed_again_at;
    bool                       _file_is_removed;
    bool                       _is_to_be_removed;
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

    bool                        adjust_with_directory       ( const list<Base_file_info>&, double now );
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
    void                        add_to_replace_or_remove_candidates( const string& name );
    void                        handle_replace_or_remove_candidates();

    virtual File_based_subsystem* file_based_subsystem      () const                                = 0;
    virtual bool                is_empty_name_allowed       () const                                { return false; }
    virtual bool                on_base_file_changed        ( File_based*, const Base_file_info* changed_base_file_info, double now );
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
    file::File_path             directory                   () const                                { return _directory; }
    Absolute_path               make_path                   ( const string& name );                 // Hängt den Ordernamen voran

    bool                        adjust_with_directory       ( double now );

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
    file::File_path            _directory;

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
};

//---------------------------------------------------------------------------------Subfolder_folder

struct Subfolder_folder : typed_folder< Folder >
{
                                Subfolder_folder            ( Folder* );
                               ~Subfolder_folder            ();


    // Typed_folder
    bool                        on_base_file_changed        ( File_based*, const Base_file_info* changed_base_file_info, double now );


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

//---------------------------------------------------------------------------------Folder_subsystem

struct Folder_subsystem : file_based_subsystem<Folder>,
                          Async_operation
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

    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    bool                        async_signaled_             ()                                      { return is_signaled(); }


    void                    set_directory                   ( const file::File_path& );
    File_path                   directory                   () const                                { return _directory; }

    Folder*                     folder                      ( const Absolute_path& path )           { return file_based( path ); }
    Folder*                     folder_or_null              ( const Absolute_path& path )           { return file_based_or_null( path ); }
    Folder*                     root_folder                 () const                                { return _root_folder; }
    ptr<Subfolder_folder>       new_subfolder_folder        ( Folder* folder )                      { return Z_NEW( Subfolder_folder( folder ) ); }

    bool                     is_signaled                    ()                                      { return _directory_event.signaled(); }
    void                    set_signaled                    ( const string& text )                  { _directory_event.set_signaled( text ); }

    void                    set_read_again_at_or_later      ( double at )                           { if( _read_again_at < at )  _read_again_at = at; }

    bool                        handle_folders              ( double minimum_age = 0 );


  private:
    Fill_zero                  _zero_;
    file::File_path            _directory;
    ptr<Folder>                _root_folder;
    Event                      _directory_event;
    int                        _directory_watch_interval;
    double                     _last_change_at;
    double                     _read_again_at;
};


inline ptr<Folder_subsystem>    new_folder_subsystem        ( Scheduler* scheduler )                { return Z_NEW( Folder_subsystem( scheduler ) ); }

//------------------------------------------------------------------------------FOR_EACH_FILE_BASED

#define FOR_EACH_FILE_BASED( FILE_BASED_CLASS, OBJECT ) \
    Z_FOR_EACH( FILE_BASED_CLASS::My_subsystem::File_based_map, spooler()->subsystem( (FILE_BASED_CLASS*)NULL )->_file_based_map, __file_based_iterator__ )  \
        if( FILE_BASED_CLASS* OBJECT = __file_based_iterator__->second )

//-------------------------------------------------------------------------------------------------

} //namespace folder

using namespace folder;

} //namespace scheduler
} //namespace sos

#endif
