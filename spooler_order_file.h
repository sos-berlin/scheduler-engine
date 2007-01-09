// $Id$

#ifndef __SPOOLER_ORDER_FILE_H
#define __SPOOLER_ORDER_FILE_H


namespace sos {
namespace scheduler {

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

    // Async_operation:
    virtual Socket_event*       async_event             ()                                          { return &_notification_event; }
    virtual bool                async_continue_         ( Continue_flags );
    virtual bool                async_finished_         () const                                    { return false; }
    virtual string              async_state_text_       () const;

    // Order_source:
    void                        close                   ();
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& );
    void                        finish                  ();
    void                        start                   ();
    bool                        request_order           ( const string& cause );
    Order*                      fetch_and_occupy_order  ( const Time& now, const string& cause, Task* occupying_task );
    void                        withdraw_order_request  ();
    string                      obj_name                () const;


  private:
    void                        send_mail               ( Scheduler_event_type, const exception* );
    void                        start_or_continue_notification( bool was_notified );
    void                        close_notification      ();
    Order*                      read_directory          ( bool was_notified, const string& cause );
    void                        read_new_files_and_handle_deleted_files( const string& cause );
    bool                        read_new_files          ();
    bool                        clean_up_blacklisted_files();
    bool                        clean_up_virgin_orders  ();
    Order*                      fetch_and_occupy_order_from_new_files( const Time& now, const string& cause, Task* occupying_task );
    int                         delay_after_error       ();
    void                        clear_new_files         ();

    Fill_zero                  _zero_;
    File_path                  _path;
    string                     _regex_string;
    Regex                      _regex;
    int                        _delay_after_error;
    int                        _repeat;
    bool                       _expecting_request_order;
    Xc_copy                    _directory_error;
    bool                       _send_recovered_mail;
    Event                      _notification_event;             // Nur Windows
    Time                       _notification_event_time;        // Wann wir zuletzt die Benachrichtigung bestellt haben
    int                        _max_orders;

    vector< ptr<z::File_info> > _new_files;
    int                        _new_files_index;
    int                        _new_files_count;        // _new_files.size() ohne NULL-Einträge
    Time                       _new_files_time;


    struct Bad_entry : Object
    {
                                Bad_entry               ( const File_path& p, const exception& x )  : _file_path(p), _error(x) {}

        File_path              _file_path;
        Xc_copy                _error;
    };

    typedef stdext::hash_map< string, ptr<Bad_entry> >  Bad_map;
    Bad_map                    _bad_map;

    bool                       _are_blacklisted_orders_cleaned_up;
    bool                       _are_virgin_orders_cleaned_up;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
