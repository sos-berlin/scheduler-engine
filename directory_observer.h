// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_DIRECTORY_OBSERVER_H
#define __SCHEDULER_DIRECTORY_OBSERVER_H

namespace sos {
namespace scheduler {
namespace directory_observer {

//-------------------------------------------------------------------------------------------------
    
struct Directory;

//-----------------------------------------------------------------------------------Directory_tree

struct Directory_tree : Scheduler_object,
                        Object
{
                                Directory_tree              ( Scheduler*, const file::File_path& directory_path );
                               ~Directory_tree              ();

    file::File_path             directory_path              () const                                { return _directory_path; }
  //bool                        read                        ();
    double                      last_change_at              () const                                { return _last_change_at; }
    double                      refresh_aged_entries_at     () const                                { return _refresh_aged_entries_at; }
    Directory*                  directory_or_null           ( const Absolute_path& );
    Directory*                  root_directory              () const                                { return _root_directory; }
  //void                        refresh_aged_entries        ();

    void                        set_last_change_at          ( double t )                            { _last_change_at = t; }
    void                        set_aging_until             ( double t )                            { if( _refresh_aged_entries_at > t )  _refresh_aged_entries_at = t; }
    void                        reset_aging                 ()                                      { _refresh_aged_entries_at = double_time_max; }

  private:
    Fill_zero                  _zero_;
    file::File_path            _directory_path;
    ptr<Directory>             _root_directory;
    double                     _last_change_at;
    double                     _refresh_aged_entries_at;
};

//----------------------------------------------------------------------------------Directory_entry

struct Directory_entry
{
                                Directory_entry             ();

    bool                        is_aging                    () const                                { return _is_aging_until > 0; }
    Directory_entry             clone                       ( Directory* new_parent ) const;


    Fill_zero                  _zero_;
    ptr<file::File_info>       _file_info;
    ptr<Directory>             _subdirectory;               // ( _subdirectory != NULL ) == _file_info.is_directory()
    double                     _is_aging_until;
    bool                       _is_removed;                 // _is_removed -> _is_aging_until > 0
};

//----------------------------------------------------------------------------------------Directory

struct Directory : Object
{
    enum Read_subdirectories
    {
        read_no_subdirectories,
        read_subdirectories
    };


                                Directory                   ( Directory_tree*, Directory* parent, const string& name );

    Absolute_path               path                        () const;
    File_path                   file_path                   () const;
    bool                        read                        ( Read_subdirectories );
  //bool                        refresh_aged_entries        ();
    const Directory_entry*      entry_or_null               ( const string& name ) const;
  //Directory*                  subdirectory                ( const string& name ) const;
    int                         version                     () const                                { return _version; }
    ptr<Directory>              clone                       () const                                { return clone2( NULL ); }
    ptr<Directory>              clone2                      ( Directory* parent ) const;
    void                        merge_new_entries           ( const Directory* );


  private:
    Fill_zero                  _zero_;
    Directory_tree* const      _directory_tree;
    Directory*                 _parent;
    string                     _name;
    int                        _version;

  public:
    typedef list<Directory_entry>  Entry_list;
    Entry_list                    _ordered_list;
};

//-------------------------------------------------------------------------------Directory_observer

//struct Directory_observer : Scheduler_object,
//                            Async_operation
//{
//                                Directory_observer          ( Scheduler*, const File_path& directory );
//                               ~Directory_observer          ();
//
//    void                        close                       ();
//
//
//
//    // Async_operation
//    bool                        async_finished_             () const                                { return false; }
//    string                      async_state_text_           () const;
//    bool                        async_continue_             ( Continue_flags );
//    bool                        async_signaled_             ()                                      { return is_signaled(); }
//    string                      obj_name                    () const                                { return Scheduler_object::obj_name(); }
//
//
//  //File_path                   directory_path              () const                                { return _directory_path; }
//    Directory_tree*             directory_tree              () const                                { return _directory_tree; }
//
//    bool                     is_signaled                    ()                                      { return _directory_event.signaled(); }
//    void                    set_signaled                    ( const string& text )                  { _directory_event.set_signaled( text ); }
//
//    void                        activate                    ();
//    bool                     is_activated                   () const                                { return _is_activated; }
//
//    bool                        check                       ( double minimum_age = 0 );
//
//
//  private:
//    Fill_zero                  _zero_;
//    Event                      _directory_event;
//    ptr<Directory_tree>        _directory_tree;
//    int                        _directory_watch_interval;
//    double                     _last_change_at;
//    double                     _read_again_at;
//    bool                       _is_activated;
//};

//-------------------------------------------------------------------------------------------------

} //namespace directory_observer

using namespace folder;

} //namespace scheduler
} //namespace sos

#endif
