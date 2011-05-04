// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_DIRECTORY_OBSERVER_H
#define __SCHEDULER_DIRECTORY_OBSERVER_H

#include "../zschimmer/directory_lister.h"

namespace sos {
namespace scheduler {
namespace directory_observer {

//-------------------------------------------------------------------------------------------------
    
struct Directory;

//---------------------------------------------------------------------------------Directory_lister

struct Folder_directory_lister : file::Directory_lister
{
                                Folder_directory_lister     ( Prefix_log* log )                     : _zero_(this+1), _log(log) {}

    bool                        open                        ( const File_path& root, const Absolute_path& folder_path );
    ptr<file::File_info>        get                         ();
    bool                        is_removed                  () const                                { return _is_removed; }

  private:
    Fill_zero                  _zero_;
    ptr<Prefix_log> const      _log;
    bool                       _is_removed;
};

//-----------------------------------------------------------------------------------Directory_tree

struct Directory_tree : Scheduler_object,
                        Object
{
                                Directory_tree              ( Scheduler*, const file::File_path& directory_path, Configuration_origin );
                               ~Directory_tree              ();

    file::File_path             directory_path              () const                                { return _directory_path; }
  //bool                        read                        ();
    double                      last_change_at              () const                                { return _last_change_at; }
    double                      refresh_aged_entries_at     () const                                { return _refresh_aged_entries_at; }
    Directory*                  directory_or_null           ( const string& name );
    Directory*                  root_directory              () const                                { return _root_directory; }
  //void                        refresh_aged_entries        ();

    void                        set_last_change_at          ( double t )                            { _last_change_at = t; }
    void                        set_aging_until             ( double t )                            { if( _refresh_aged_entries_at > t )  _refresh_aged_entries_at = t; }
    void                        reset_aging                 ()                                      { _refresh_aged_entries_at = double_time_max; }
    void                        withdraw_aging              ();
    Configuration_origin        configuration_origin        () const                                { return _configuration_origin; }
    void                    set_is_watched                  ()                                      { _is_watched = true; }
    bool                        is_watched                  () const                                { return _is_watched; }

  private:
    Fill_zero                  _zero_;
    file::File_path            _directory_path;
    ptr<Directory>             _root_directory;
    double                     _last_change_at;
    double                     _refresh_aged_entries_at;
    Configuration_origin       _configuration_origin;       // cache/ oder live/ ?
    bool                       _is_watched;
};

//----------------------------------------------------------------------------------Directory_entry

struct Directory_entry
{
    static bool                 normalized_less_dereferenced( const Directory_entry*, const Directory_entry* );


                                Directory_entry             ();

    bool                        is_aging                    () const                                { return _is_aging_until > 0; }
    Directory_entry             clone                       ( Directory* new_parent ) const;


    Fill_zero                  _zero_;
    ptr<file::File_info>       _file_info;
    ptr<Directory>             _subdirectory;               // ( _subdirectory != NULL ) == _file_info.is_directory()
    double                     _is_aging_until;
    bool                       _is_removed;                 // _is_removed -> _is_aging_until > 0
    Configuration_origin       _configuration_origin;
    int                        _version;
    int                        _duplicate_version;          // Für merged_new_entries() und Meldung SCHEDULER-703
    string                     _normalized_name;
};

//----------------------------------------------------------------------------------------Directory

struct Directory : Object
{
    enum Read_flags
    {
        read_no_subdirectories,
        read_subdirectories                 = 0x01,
        read_suppress_aging                 = 0x02,
        read_subdirectories_suppress_aging  = 0x03
    };


                                Directory                   ( Directory_tree*, Directory* parent, const string& name );

    string                      name                        () const                                { return _name; }
    Absolute_path               path                        () const;
    File_path                   file_path                   () const;
    bool                        read                        ( Read_flags, double minimum_age = 0.0 );
    bool                        read_deep                   ( double minimum_age )                  { return read( read_subdirectories, minimum_age ); }
    bool                        read_without_subdirectories ()                                      { return read( read_no_subdirectories ); }
    const Directory_entry*      entry_or_null               ( const string& name ) const;
    const Directory_entry*      entry_of_path_or_null       ( const File_path& ) const;
  //Directory*                  subdirectory                ( const string& name ) const;
    int                         version                     () const                                { return _version; }
    ptr<Directory>              clone                       () const                                { return clone2( NULL ); }
    ptr<Directory>              clone2                      ( Directory* parent ) const;
    void                        merge_new_entries           ( const Directory* );
    void                        withdraw_aging_deep         ();
    void                        assert_ordered_list         ();


  private:
    void                        set_aging_until             ( Directory_entry*, double until );


    Fill_zero                  _zero_;
    string                     _name;
    int                        _version;
    double                     _last_read_at;
    Directory*                 _parent;
    Directory_tree* const      _directory_tree;
  //bool                       _repeated_read;

  public:
    typedef list<Directory_entry>  Entry_list;
    Entry_list                    _ordered_list;
};

//-------------------------------------------------------------------------------Directory_observer

struct Directory_observer : Scheduler_object,
                            Event_operation
{
    struct Directory_handler
    {
        virtual bool            on_handle_directory         ( Directory_observer* )                 = 0;
    };


                                Directory_observer          ( Scheduler*, const File_path& directory, Configuration_origin w );
                               ~Directory_observer          ();

    // Async_operation
    bool                        async_finished_             () const                                { return false; }
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    Socket_event*               async_event                 ()                                      { return &_directory_event; }
    string                      obj_name                    () const;

    void                        close                       ();
    bool                        activate                    ();
    bool                        run_handler                 ();
    void                        set_alarm                   ();
    void                        set_signaled                ( const string& text );
    void                        register_directory_handler  ( Directory_handler* );
    bool                        is_active                   () const                                { return _directory_tree && _directory_tree->is_watched(); }

    Directory_tree*             directory_tree              () const                                { return _directory_tree; }
    File_path                   directory_path              () const { return _directory_path; }


  private:
    Fill_zero                  _zero_;
    Event                      _directory_event;
    ptr<Directory_tree>        _directory_tree;
  //int                        _directory_watch_interval;
    double                     _next_check_at;
    Directory_handler*         _directory_handler;
    File_path                  _directory_path;

};

//-------------------------------------------------------------------------------------------------

} //namespace directory_observer

using namespace folder;

} //namespace scheduler
} //namespace sos

#endif
