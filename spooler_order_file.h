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
    Order*                      request_order           ( const string& cause );

  private:
    void                        send_mail               ( Scheduler_event::Event_code, const exception* );
    void                        start_or_continue_notification( bool was_notified );
    void                        close_notification      ();
    Order*                      read_directory          ( bool was_notified, const string& cause );
    void                        read_new_files_and_handle_deleted_files( const string& cause );
    int                         delay_after_error       ();

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
    Time                       _new_files_time;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
