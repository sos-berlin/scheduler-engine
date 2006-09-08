// $Id$

#ifndef __SPOOLER_ORDER_FILE_H
#define __SPOOLER_ORDER_FILE_H


namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------
    
extern const string             file_order_sink_job_name;

//----------------------------------------------------------------------Directory_file_order_source

struct Directory_file_order_source : //idispatch_implementation< Directory_file_order_source, spooler_com::Idirectory_file_order_source >,
                                     Order_source
{
    enum State
    {
        s_none,
        s_order_requested
    };


    explicit                    Directory_file_order_source( Job_chain*, const xml::Element_ptr& );
                               ~Directory_file_order_source();

    // Scheduler_object:
    virtual Prefix_log*         log                     ();

    // Async_operation:
    virtual Socket_event*       async_event             ()                                          { return &_notification_event; }
    virtual bool                async_continue_         ( Continue_flags );
    virtual bool                async_finished_         () const                                    { return false; }
    virtual string              async_state_text_       () const;

    void                        start                   ();
    Order*                      request_order           ( const string& cause );
    Order*                      read_directory          ( const string& cause );

  private:
    void                        send_mail               ( Scheduler_event::Event_code, const exception* );
    void                        start_or_continue_notification();
    void                        close_notification      ();

    Fill_zero                  _zero_;
    File_path                  _path;
    string                     _regex_string;
    Regex                      _regex;
    Job_chain*                 _job_chain;
    int                        _delay_after_error;
    int                        _repeat;
    bool                       _directory_error;
  //bool                       _first;
    Event                      _notification_event;             // Nur Windows
    bool                       _wait_for_notification_event;    // Nur Windows. Verzeichnis erst lesen, wenn _notification_event.signaled()
    int                        _max_orders;
    vector< ptr<z::File_info> > _new_files;
    int                         _new_files_index;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
