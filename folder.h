// $Id: cluster.h 5126 2007-07-13 08:59:30Z jz $

#ifndef __SCHEDULER_FOLDER_H
#define __SCHEDULER_FOLDER_H

namespace sos {
namespace scheduler {
namespace folder {

//--------------------------------------------------------------------------------------------const

const extern char               folder_separator;

//-------------------------------------------------------------------------------------------------

struct Folder;
struct Folder_subsystem;
struct Typed_folder;
struct File_based;
struct File_based_subsystem;

//---------------------------------------------------------------------------------------------Path

struct Path : string
{
                                Path                        ()                                      {}
                                Path                        ( const string& path )                  { set_path( path ); }
                                Path                        ( const string& directory, const string& tail_path );

    Path&                       operator =                  ( const string& path )                  { set_path( path );  return *this; }

  //bool                        operator <                  ( const File_path& path ) const         { return compare( path ) <  0; }
  //bool                        operator <=                 ( const File_path& path ) const         { return compare( path ) <= 0; }
  //bool                        operator ==                 ( const File_path& path ) const         { return compare( path ) == 0; }
  //bool                        operator !=                 ( const File_path& path ) const         { return compare( path ) != 0; }
  //bool                        operator >=                 ( const File_path& path ) const         { return compare( path ) >= 0; }
  //bool                        operator >                  ( const File_path& path ) const         { return compare( path ) >  0; }
  //int                         compare                     ( const File_path& ) const;

    void                    set_name                        ( const string& );                                
    string                      name                        () const;
    void                    set_folder_path                 ( const string& );
    Path                        folder_path                 () const;
  //void                        prepend_folder_path         ( const string& );
    const string&               to_string                   ()  const                               { return *static_cast<const string*>( this ); }
    void                    set_path                        ( const string& path )                  { *static_cast<string*>( this ) = path; }
    bool                     is_absolute_path               () const;
    string                      absolute_path               () const;
};

//------------------------------------------------------------------------------------------Pendant

struct Pendant
{
                                Pendant                     ();
    virtual                    ~Pendant                     ();

    virtual string              obj_name                    () const                                = 0;
    void                        add_dependant               ( File_based_subsystem*, const Path& path );
    void                        remove_dependant            ( File_based_subsystem*, const Path& path );
    void                        remove_dependants           ();

    virtual bool                on_dependant_loaded         ( File_based* )                         = 0;
    virtual bool                on_dependant_to_be_removed  ( File_based* );
    virtual void                on_dependant_removed        ( File_based* );

  private:
    typedef stdext::hash_set< string >                                Dependant_set;
    typedef stdext::hash_map< File_based_subsystem*, Dependant_set >  Dependant_sets;

    Dependant_sets               _missing_sets;
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

//-------------------------------------------------------------------------------------------Folder
//
// Ein Ordner (Folder) enthält alle Objekte. 
// Bislang gibt es nur einen Ordner im Scheduler.

struct Folder : Scheduler_object, Object
{
    static int                  position_of_extension_point ( const string& filename );
    static string               object_name_of_filename     ( const string& filename );
    static string               extension_of_filename       ( const string& filename );


                                Folder                      ( Folder_subsystem*, Folder* parent_folder, const string& name );
                               ~Folder                      ();

    file::File_path             directory                   () const                                { return _directory; }
    Path                        path                        () const                                { return _path; }
    Path                        make_path                   ( const string& name );                 // Hängt den Ordernamen voran

    void                        adjust_with_directory       ();

    Process_class_folder*       process_class_folder        ()                                      { return _process_class_folder; }
    lock::Lock_folder*          lock_folder                 ()                                      { return _lock_folder; }
    Job_folder*                 job_folder                  ()                                      { return _job_folder; }
    Job_chain_folder_interface* job_chain_folder            ()                                      { return _job_chain_folder; }
    Standing_order_folder*      standing_order_folder       ()                                      { return _standing_order_folder; }

    string                      obj_name                    () const;

  private:
    void                        add_to_typed_folder_map     ( Typed_folder* );

    Fill_zero                  _zero_;
    string                     _path;
    string                     _name;
    file::File_path            _directory;

    typedef stdext::hash_map< string, Typed_folder* >  Typed_folder_map;
    Typed_folder_map           _typed_folder_map;
    
  //Folder*                    _parent;
  //Folder_set                 _child_folder_set;

    ptr<Process_class_folder>       _process_class_folder;
    ptr<lock::Lock_folder>          _lock_folder;
    ptr<Job_folder>                 _job_folder;
    ptr<Job_chain_folder_interface> _job_chain_folder;
    ptr<Standing_order_folder>      _standing_order_folder;
};

//---------------------------------------------------------------------------------Folder_subsystem

struct Folder_subsystem : Subsystem,
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


    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    bool                        async_signaled_             ()                                      { return is_signaled(); }


    void                    set_directory                   ( const file::File_path& );
    File_path                   directory                   () const                                { return _directory; }

  //Folder*                     folder                      ( const string& path );
    Folder*                     root_folder                 () const                                { return _root_folder; }

    bool                        is_signaled                 ()                                      { return _directory_event.signaled(); }

  private:
    Fill_zero                  _zero_;
    file::File_path            _directory;
    ptr<Folder>                _root_folder;
    Event                      _directory_event;
};


inline ptr<Folder_subsystem>    new_folder_subsystem        ( Scheduler* scheduler )                { return Z_NEW( Folder_subsystem( scheduler ) ); }

//-----------------------------------------------------------------------------------Base_file_info

struct Base_file_info
{
                                Base_file_info              ()                                      : _timestamp_utc(0) {}

                                Base_file_info              ( const string& filename, double timestamp_utc, const string& normalized_name ) 
                                                                                                    : _filename(filename), _timestamp_utc(timestamp_utc),
                                                                                                      _normalized_name(normalized_name) {}

    bool                        operator <                  ( const Base_file_info& f ) const       { return _normalized_name < f._normalized_name; }
    static bool                 less_dereferenced           ( const Base_file_info* a, const Base_file_info* b )  { return *a < *b; }

    string                     _filename;                   // Ohne Verzeichnis
    double                     _timestamp_utc;
    string                     _normalized_name;            // Ohne Dateinamenserweiterung
};

//---------------------------------------------------------------------------------------File_based

struct File_based : Scheduler_object,
                    Pendant,
                    zschimmer::Has_addref_release
{
    enum State
    {
        s_not_initialized,
        s_initialized,
        s_loaded,
        s_active,
        s_error,
        s_closed
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


    virtual xml::Element_ptr    dom_element                 ( const xml::Document_ptr&, const Show_what& );


    File_based_subsystem*       subsystem                   () const                                { return _file_based_subsystem; }
    bool                        is_in_folder                () const                                { return _typed_folder != NULL; }
    Typed_folder*               typed_folder                () const                                { return _typed_folder; }
    Folder*                     folder                      () const;
    bool                        has_base_file               () const                                { return _base_file_info._filename != ""; }
    const Base_file_info&       base_file_info              () const                                { return _base_file_info; }
    bool                        base_file_has_error         () const                                { return _base_file_xc_time != 0; }
    const zschimmer::Xc&        base_file_exception         () const                                { return _base_file_xc; }
    bool                        base_file_is_removed        () const                                { return _file_is_removed; }
    void                        assert_is_loaded            ();
    void                        assert_is_active            ();

    virtual void            set_name                        ( const string& name );
    string                      name                        () const                                { return _name; }
    Path                        path                        () const;
    string                      normalized_name             () const;
    string                      normalized_path             () const;

    State                       file_based_state            () const                                { return _state; }
    string                      file_based_state_name       () const                                { return file_based_state_name( file_based_state() ); } 
    static string               file_based_state_name       ( State );

    void                    set_replacement                 ( File_based* replacement )             { _replacement = replacement; }
    File_based*                 replacement                 () const                                { return _replacement; }

    void                    set_typed_folder                ( Typed_folder* tf )                    { _typed_folder = tf; }     // Nur für Typed_folder!
    bool                        check_for_replacing_or_removing();

    bool                        initialize                  ();
    bool                        load                        ();
    bool                        activate                    ();
    bool                        switch_file_based_state     ( State  );
    bool                        try_switch_wished_file_based_state();
    void                        remove_now                  ();
    File_based*                 replace_now                 ();
  //void                        set_file_based_incomplete   ( bool b )                              { _is_incomplete = true; }

    virtual void                set_dom                     ( const xml::Element_ptr& )             = 0;
    virtual bool                on_initialize               ()                                      = 0;
    virtual bool                on_load                     ()                                      = 0;
    virtual bool                on_activate                 ()                                      = 0;

    enum Remove_flags { rm_default, rm_base_file_too };
    bool                        remove                      ( Remove_flags = rm_default );
    void                        remove_base_file            ();

    virtual void                on_remove_now               ();
    virtual bool                prepare_to_remove           ();
    virtual bool                can_be_removed_now          ()                                      = 0;
    virtual zschimmer::Xc       remove_error                ();

    virtual bool                replace_with                ( File_based* );
    virtual void                prepare_to_replace          ();
    virtual bool                can_be_replaced_now         ();
    virtual File_based*         on_replace_now              ();

    virtual File_based*         on_base_file_changed        ( File_based* new_file_based )          = 0;
  //virtual bool                on_base_file_removed        ()                                      { return remove(); }


    bool                        operator <                  ( const File_based& f ) const           { return _base_file_info < f._base_file_info; }
    static bool                 less_dereferenced           ( const File_based* a, const File_based* b )  { return *a < *b; }


  protected:
    void                    set_base_file_info              ( const Base_file_info& bfi )           { _base_file_info = bfi; }
    void                    set_file_based_state            ( State );

    Fill_zero                  _zero_;
    ptr<File_based>            _replacement;

  private:
    friend struct               Typed_folder;

    bool                        initialize2                 ();
    bool                        load2                       ();
    bool                        activate2                   ();

    string                     _name;
    State                      _state;
    State                      _wished_state;
    Base_file_info             _base_file_info;
    zschimmer::Xc              _base_file_xc;
    double                     _base_file_xc_time;
    zschimmer::Xc              _remove_xc;
    bool                       _file_is_removed;
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

    File_based*                 on_base_file_changed        ( File_based* new_file_based )          { return on_base_file_changed( static_cast<FILE_BASED*>( new_file_based ) ); }
    virtual FILE_BASED*         on_base_file_changed        ( FILE_BASED* new_file_based )          { return new_file_based; }
    FILE_BASED*                 replacement                 () const                                { return static_cast<FILE_BASED*>( File_based::replacement() ); }
    TYPED_FOLDER*               typed_folder                () const                                { return static_cast<TYPED_FOLDER*>( File_based::typed_folder() ); }
};

//-------------------------------------------------------------------------------------Typed_folder

struct Typed_folder : Scheduler_object, 
                      Object
{
                                Typed_folder                ( Folder*, Type_code );

    Folder*                     folder                      () const                                { return _folder; }
    File_based_subsystem*       subsystem                   () const                                { return file_based_subsystem(); }
    virtual File_based_subsystem* file_based_subsystem      () const                                = 0;

    void                        adjust_with_directory       ( const list<Base_file_info>& );
    File_based*                 call_on_base_file_changed   ( File_based*, const Base_file_info* changed_base_file_info );
    File_based*                 file_based                  ( const string& name ) const;
    File_based*                 file_based_or_null          ( const string& name ) const;

    virtual bool                is_empty_name_allowed       () const                                { return false; }
    string                      obj_name                    () const;

    void                        add_file_based              ( File_based* );
    void                        remove_file_based           ( File_based* );
    File_based*                 replace_file_based          ( File_based* );

  private:
    Fill_zero                  _zero_;
    Folder*                    _folder;
    File_based_subsystem*      _file_based_subsystem;

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

//-----------------------------------------------------------------------------File_based_subsystem
// Für jeden dateibasierten Typ (File_based) gibt es genau ein File_based_subsystem

struct File_based_subsystem : Subsystem
{
                                File_based_subsystem        ( Spooler*, IUnknown*, Type_code );
    virtual                    ~File_based_subsystem        ()                                      {}

    virtual string              object_type_name            () const                                = 0;
    virtual string              filename_extension          () const                                = 0;
    virtual string              normalized_name             ( const string& name ) const            { return name; }
    virtual Path                normalized_path             ( const Path& path ) const;
  //virtual ptr<Typed_folder>   new_typed_folder            ()                                      = 0;
    Dependencies*               dependencies                ()                                      { return &_dependencies; }

  protected:
    friend struct               Typed_folder;

    virtual ptr<File_based>     call_new_file_based         ()                                      = 0;
    virtual void                add_file_based              ( File_based* )                         = 0;
    virtual void                remove_file_based           ( File_based* )                         = 0;
    virtual void                replace_file_based          ( File_based*, File_based* )            = 0;

  private:
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
        for( typename File_based_map::iterator it = _file_based_map.begin(); it != _file_based_map.end(); )
        {
            typename File_based_map::iterator next_it    = it;  next_it++;
            ptr<File_based>                   file_based = +it->second;
            
            try
            {
                file_based->remove();
            }
            catch( exception& x ) { file_based->log()->error( x.what() ); }

            it = next_it;
        }

        //Z_FOR_EACH( File_based_map, _file_based_map, it )       // _file_based_map sollte leer sein
        //{
        //    File_based* file_based = it->second;

        //    try
        //    {
        //        file_based->close();
        //    }
        //    catch( exception&  x ) { log()->error( message_string( "SCHEDULER-434", file_based->obj_name(), x ) ); }
        //    catch( _com_error& x ) { log()->error( message_string( "SCHEDULER-434", file_based->obj_name(), x.Description() ) ); }
        //}

        //typed_folder<> verweist noch auf File_based*    _file_based_map.clear();
    }
    

    bool subsystem_initialize()
    {
        Z_FOR_EACH( typename File_based_map, _file_based_map, it )
        {
            File_based* file_based = it->second;

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
        Z_FOR_EACH( typename File_based_map, _file_based_map, it )
        {
            File_based* file_based = it->second;

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
        Z_FOR_EACH( typename File_based_map, _file_based_map, it )
        {
            File_based* file_based = it->second;

            try
            {
                file_based->activate();
            }
            catch( exception& x ) { file_based->log()->error( x.what() ); }
        }

        return true;
    }
    

    FILE_BASED* file_based_or_null( const Path& path ) const
    {
        typename File_based_map::const_iterator it = _file_based_map.find( normalized_path( path ) );
        return it == _file_based_map.end()? NULL 
                                          : it->second;
    }

    
    FILE_BASED* file_based( const Path& path ) const
    {
        FILE_BASED* result = file_based_or_null( path );
        if( !result )  z::throw_xc( "SCHEDULER-161", object_type_name(), path.to_string() );
        return result;
    }


    FILE_BASED* active_file_based( const Path& path ) const
    {
        FILE_BASED* result = file_based_or_null( path );
        if( !result )  z::throw_xc( "SCHEDULER-161", object_type_name(), path.to_string() );
        result->assert_is_active();

        return result;
    }

  private:
    friend struct               Typed_folder;


    void add_file_based( File_based* file_based )                   // Nur für Typed_folder::add_file_based()
    {
        assert( !file_based_or_null( file_based->path() ) );

        FILE_BASED* casted_file_based = dynamic_cast<FILE_BASED*>( file_based );
        if( !casted_file_based )  z::throw_xc( __FUNCTION__ );

        _file_based_map[ file_based->normalized_path() ] = casted_file_based;
        _file_based_map_version++;
    }


    void remove_file_based( File_based* file_based )                // Nur für Typed_folder::remove_file_based()
    {
        assert( file_based_or_null( file_based->path() ) );

        FILE_BASED* casted_file_based = dynamic_cast<FILE_BASED*>( file_based );
        if( !casted_file_based )  z::throw_xc( __FUNCTION__ );

        _file_based_map.erase( casted_file_based->normalized_path() );
        _file_based_map_version++;
    }


    void replace_file_based( File_based* old_file_based, File_based* new_file_based )       // Nur für Typed_folder::replace_file_based()
    {
        assert( file_based_or_null( old_file_based->path() ) );

        FILE_BASED* casted_old_file_based = dynamic_cast<FILE_BASED*>( old_file_based );
        if( !casted_old_file_based )  z::throw_xc( __FUNCTION__ );

        FILE_BASED* casted_new_file_based = dynamic_cast<FILE_BASED*>( new_file_based );
        if( !casted_new_file_based )  z::throw_xc( __FUNCTION__ );

        _file_based_map[ casted_old_file_based->normalized_path() ] = casted_new_file_based;
        _file_based_map_version++;
    }


  public:
    Fill_zero                  _zero_;
    typedef stdext::hash_map< string, ptr< FILE_BASED > >  File_based_map;
    File_based_map             _file_based_map;

  protected:
    int                        _file_based_map_version;
};

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
