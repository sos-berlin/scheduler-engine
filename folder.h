// $Id: cluster.h 5126 2007-07-13 08:59:30Z jz $

#ifndef __SCHEDULER_FOLDER_H
#define __SCHEDULER_FOLDER_H

namespace sos {
namespace scheduler {
namespace folder {

//--------------------------------------------------------------------------------------------const

//-------------------------------------------------------------------------------------------------

struct Folder;
struct Folder_subsystem;
struct Typed_folder;
struct File_based;
struct File_based_subsystem;

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
    string                      path                        () const                                { return _path; }
    string                      make_path                   ( const string& name );                 // Hängt den Ordernamen voran

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


    void                    set_directory                   ( const file::File_path& );
    File_path                   directory                   () const                                { return _directory; }

  //Folder*                     folder                      ( const string& path );
    Folder*                     root_folder                 () const                                { return _root_folder; }

  private:
    Fill_zero                  _zero_;
    file::File_path            _directory;
    ptr<Folder>                _root_folder;
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
    string                      path                        () const                                { return _name; }
    string                      normalized_name             () const;
    string                      normalized_path             () const;

    State                       file_based_state            () const                                { return _state; }
    string                      file_based_state_name       () const                                { return file_based_state_name( file_based_state() ); } 
    static string               file_based_state_name       ( State );

    void                    set_replacement                 ( File_based* replacement )             { _replacement = replacement;  _replacement_is_valid = false; }
    File_based*                 replacement                 () const                                { return _replacement; }
    bool                        replacement_is_valid        () const                                { return _replacement_is_valid; }

    void                    set_typed_folder                ( Typed_folder* tf )                    { _typed_folder = tf; }     // Nur für Typed_folder!
    bool                        check_for_replacing_or_removing();

    bool                        initialize                  ();
    bool                        load                        ();
    bool                        activate                    ();

    virtual void                set_dom                     ( const xml::Element_ptr& )             = 0;
    virtual bool                on_initialize               ()                                      = 0;
    virtual bool                on_load                     ()                                      = 0;
    virtual bool                on_activate                 ()                                      = 0;

    virtual bool                remove                      ();
  //virtual void                remove_now                  ()                                      = 0;
    virtual bool                prepare_to_remove           ()                                      { return can_be_removed_now(); }
    virtual bool                can_be_removed_now          ()                                      = 0;

    virtual bool                replace_with                ( File_based* );
    virtual bool                prepare_to_replace          ()                                      { _replacement_is_valid = true;  return true; }
    virtual bool                can_be_replaced_now         ()                                      { return _replacement_is_valid; }
    virtual File_based*         replace_now                 ()                                      = 0;

    virtual File_based*         on_base_file_changed        ( File_based* new_file_based )          = 0;
    virtual bool                on_base_file_removed        ()                                      { return remove(); }


    bool                        operator <                  ( const File_based& f ) const           { return _base_file_info < f._base_file_info; }
    static bool                 less_dereferenced           ( const File_based* a, const File_based* b )  { return *a < *b; }


  protected:
    void                    set_base_file_info              ( const Base_file_info& bfi )           { _base_file_info = bfi; }
    void                    set_file_based_state            ( State state )                         { _state = state; }

    Fill_zero                  _zero_;
    ptr<File_based>            _replacement;

  private:
    friend struct               Typed_folder;

    string                     _name;
    State                      _state;
    Base_file_info             _base_file_info;
    zschimmer::Xc              _base_file_xc;
    double                     _base_file_xc_time;
    bool                       _is_base_file_loaded;
    Typed_folder*              _typed_folder;
    File_based_subsystem*      _file_based_subsystem;
    bool                       _file_is_removed;
    bool                       _replacement_is_valid;
};

//-------------------------------------------------------------------------------------file_based<>

template< class FILE_BASED, class TYPED_FOLDER, class SUBSYSTEM >
struct file_based : File_based
{                                                           
                                file_based                  ( SUBSYSTEM* p, IUnknown* iu, Type_code t ): File_based( p, iu, t ) {}

    typedef SUBSYSTEM           My_subsystem;

    SUBSYSTEM*                  subsystem                   () const                                { return static_cast<SUBSYSTEM*>( File_based::subsystem() ); }

    File_based*                 on_base_file_changed        ( File_based* new_file_based )          { return on_base_file_changed( static_cast<FILE_BASED*>( new_file_based ) ); }
    virtual FILE_BASED*         on_base_file_changed        ( FILE_BASED* new_file_based )          { return new_file_based; }
  //void                    set_replacement                 ( FILE_BASED* file_based )              { File_based::set_replacement = file_based;  _replacement_is_valid = false; }
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

//-------------------------------------------------------------------------------Missings_requestor

struct Missings_requestor
{
                                Missings_requestor          ();
    virtual                    ~Missings_requestor          ();

    virtual void                close                       ();
    virtual string              obj_name                    () const                                = 0;
    void                        add_missing                 ( File_based_subsystem*, const string& path );
    void                        remove_missing              ( File_based_subsystem*, const string& path );

    virtual bool                on_missing_found            ( File_based* )                         = 0;

  private:
    typedef stdext::hash_set< string >                              Missing_set;
    typedef stdext::hash_map< File_based_subsystem*, Missing_set >  Missing_sets;

    Missing_sets               _missing_sets;
};

//-----------------------------------------------------------------------------------------Missings

struct Missings 
{
                                Missings                    ( File_based_subsystem* );
                               ~Missings                    ();

    void                        add_missing                 ( Missings_requestor*, const string& missing_path );
    void                        remove_missing              ( Missings_requestor*, const string& missing_path );
    void                        remove_requestor            ( Missings_requestor* );
    void                        announce_missing_is_found   ( File_based* found_missing );


  private:
    typedef stdext::hash_set< Missings_requestor* >    Requestor_set;
    typedef stdext::hash_map< string, Requestor_set >  Path_requestors_map;            // Vermisster Pfad -> { Requestor ... }

    Fill_zero                  _zero_;
    File_based_subsystem*      _subsystem;
    Path_requestors_map        _path_requestors_map;;
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
  //virtual ptr<Typed_folder>   new_typed_folder            ()                                      = 0;
    Missings*                   missings                    ()                                      { return &_missings; }

  protected:
    friend struct               Typed_folder;

    virtual ptr<File_based>     call_new_file_based         ()                                      = 0;
    virtual void                add_file_based              ( File_based* )                         = 0;
    virtual void                remove_file_based           ( File_based* )                         = 0;
    virtual void                replace_file_based          ( File_based*, File_based* )            = 0;

  private:
    Missings                   _missings;
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
    

    FILE_BASED* file_based_or_null( const string& path ) const
    {
        typename File_based_map::const_iterator it = _file_based_map.find( normalized_name( path ) );
        return it == _file_based_map.end()? NULL 
                                          : it->second;
    }

    
    FILE_BASED* file_based( const string& path ) const
    {
        FILE_BASED* result = file_based_or_null( path );
        if( !result )  z::throw_xc( "SCHEDULER-161", object_type_name(), path );
        return result;
    }


    FILE_BASED* active_file_based( const string& path ) const
    {
        FILE_BASED* result = file_based_or_null( path );
        if( !result )  z::throw_xc( "SCHEDULER-161", object_type_name(), path );
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
