// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_FILE_LOGGER_H
#define __SCHEDULER_FILE_LOGGER_H

#include "../zschimmer/mutex.h"

namespace sos {
namespace scheduler {

//------------------------------------------------------------------------------------Stdout_reader

struct File_logger : Async_operation
{
    struct File_line_reader : Object
    {
                                File_line_reader            ( const File_path&, const string& prefix );

        void                    close                       ();
        string                  read_lines                  ();
        string                  read_remainder              ();

        Fill_zero              _zero_;
        File_path              _path;
        //File                   _file;
        size_t                 _read_length;
        string                 _prefix;
    };


    struct File_logger_thread : Thread
    {
                                File_logger_thread          ( File_logger* );
                               ~File_logger_thread          ();

        int                     thread_main                 ();
        void                    terminate                   ();

      private:
        Fill_zero              _zero_;

        File_logger*           _file_logger;
        Async_manager          _async_manager;
        zschimmer::Event       _terminate_event;
    };

                                File_logger                 ( Has_log* );
                               ~File_logger                 ();

    void                        close                       ();
    void                        set_object_name             ( const string& text )                  { _for_object = text; }
    bool                        has_files                   () const                                { return !_file_line_reader_list.empty(); }
  //void                        set_remove_files            ( bool b )                              { _remove_files = b; }
    void                        add_file                    ( const File_path&, const string& prefix );
    void                        start                       ();
    void                        start_thread                ();
    void                        finish                      ();
    bool                        flush                       ();
    bool                        flush_lines                 ();

  protected:
    // Async_operation:
    string                      async_state_text_           () const;
    bool                        async_continue_             ( Continue_flags );
    bool                        async_finished_             () const                                { return false; }


  private:
    bool                        log_lines                   ( const string& );


    typedef list< ptr<File_line_reader> >  File_line_reader_list;

    Fill_zero                  _zero_;
    Mutex                      _mutex;
    Has_log*                   _log;
    File_line_reader_list      _file_line_reader_list;
    ptr<File_logger_thread>    _thread;
  //bool                       _remove_files;
    string                     _for_object;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
