// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_COMMAND_H
#define __SPOOLER_COMMAND_H

namespace sos {
namespace scheduler {

struct Http_file_directory;

//-------------------------------------------------------------------------------------------------

const int                       recommended_response_block_size = Z_NDEBUG_DEBUG( 100000, 1000 );

//-------------------------------------------------------------------------------------------------

xml::Element_ptr                create_error_element        ( const xml::Document_ptr&, const zschimmer::Xc&, time_t = 0 );
xml::Element_ptr                create_error_element        ( const xml::Document_ptr&, const Xc_copy&, time_t gmt = 0 );

//-----------------------------------------------------------------------------------Show_what_enum

enum Show_what_enum
{
    show_standard           = 0,
    
    show_task_queue         = 0x01,     // Task Queue zeigen
    show_job_orders         = 0x02,     // Jede Order in der Order_queue zeigen, nur unter <job>
    show_job_chain_orders   = 0x04,     // Jede Order in der Order_queue zeigen, nur unter <job_chain>
    show_orders             = 0x08,     // Jede Order in der Order_queue zeigen
    show_description        = 0x10,
    show_log                = 0x20,
    show_task_history       = 0x40,
    show_no_subfolders_flag = 0x80,             // internees Flag für das weitere Auflössen von Unterordnern 
    show_remote_schedulers  = 0x100,
    show_schedule           = 0x200,
    show_job_chains         = 0x400,
    show_job_chain_jobs     = 0x800,
    show_jobs               = 0x1000,   // <jobs>
    show_tasks              = 0x2000,   // <tasks>
    show_payload            = 0x4000,
    show_statistics         = 0x10000,
    show_job_commands       = 0x20000,
    show_blacklist          = 0x40000,
    show_order_source_files = 0x80000,
    show_job_params         = 0x100000,
    show_cluster            = 0x200000,
    show_operations         = 0x400000,
    show_folders            = 0x800000,
    show_no_subfolders      = 0x1000000,
    show_schedules          = 0x2000000,
    show_source             = 0x4000000,

  //show_web_services       = 0x20000,
  //show_web_service_operations = 0x40000,

    show_all_               = 0x40000000,
    show_all                = 0x7FFFFFFF,   // Alle Flags und show_all_ (Bei <show_state> ist z.B. show_orders nicht in show_all enthalten)
    show_for_database_only  = 0x80000000,   // XML-Element nur für Datenbank
};

inline Show_what_enum operator | ( Show_what_enum a, Show_what_enum b )  { return (Show_what_enum)( (int)a | (int)b ); } 
inline Show_what_enum operator & ( Show_what_enum a, Show_what_enum b )  { return (Show_what_enum)( (int)a & (int)b ); } 
inline Show_what_enum operator ~ ( Show_what_enum a )                    { return (Show_what_enum) ~(int)a; } 

//----------------------------------------------------------------------------------------Show_what

struct Show_what
{
                                Show_what                   ( Show_what_enum = show_standard );

    Show_what                   operator |                  ( Show_what_enum w ) const              { Show_what ww = *this; ww |= w; return ww;; }
    void                        operator |=                 ( Show_what_enum w )                    { _what = _what | w; }
    void                        operator &=                 ( Show_what_enum w )                    { _what = _what & w; }
    bool                        is_set                      ( Show_what_enum w ) const              { return ( _what & w ) != 0; }


    Fill_zero                  _zero_;
    Show_what_enum             _what;
    int                        _max_orders;
    int                        _max_order_history;
    int                        _max_task_history;
    string                     _job_name;
    int                        _task_id;
    Absolute_path              _folder_path;
};

//----------------------------------------------------------------------------Show_calendar_options

struct Show_calendar_options
{
                                Show_calendar_options       ()                                      : _zero_(this+1) {}

    Fill_zero                  _zero_;
    Time                       _from;
    Time                       _before;
    int                        _limit;
    int                        _count;
};

//-------------------------------------------------------------------------------------------------

void                            append_error_element        ( const xml::Element_ptr&, const Xc_copy& );
void                            append_error_element        ( const xml::Element_ptr&, const zschimmer::Xc& );
Xc_copy                         xc_from_dom_error           ( const xml::Element_ptr& );
void                            dom_append_nl               ( const xml::Element_ptr& );
string                          xml_as_string               ( const xml::Document_ptr&, const string& indent_string = "" );

//---------------------------------------------------------------------------------Command_response

struct Command_response : Xml_response
{
                              //Command_response            ();

    virtual string              complete_text               ()                                      { z::throw_xc( "SCHEDULER-353" ); }  // Nur für Synchronous_command_response
    void                        begin_standard_response     ();
    void                        end_standard_response       ();
};

//---------------------------------------------------------------------Synchronous_command_response

struct Synchronous_command_response : Command_response
{
                                Synchronous_command_response( const string& text ) : _response_text(text) {}

    // Async_operation
    virtual bool                async_continue_             ( Continue_flags )                      { z::throw_xc( Z_FUNCTION ); }
    virtual bool                async_finished_             () const                                { return true; }
    virtual string              async_state_text_           () const                                { return "Synchronous_command_response"; }

    // Xml_response
    string                      get_part                    ()                                      { string result = _response_text;  _response_text = "";  return result; }
    void                        flush                       ()                                      {}

    // Command_response
    void                        write                       ( const io::Char_sequence& seq )        { _response_text.append( seq.ptr(), seq.length() ); }
    string                      complete_text               ()                                      { return _response_text; }

  private:
    string                     _response_text;
};

//-------------------------------------------------------------------File_buffered_command_response
//
//struct Buffered_command_response : Command_response
//{
//    enum State
//    {
//        s_ready, 
//        s_congested,
//        s_finished
//    };
//
//                                Buffered_command_response   ();
//
//    // Async_operation
//    virtual bool                async_continue_             ( Continue_flags );
//    virtual bool                async_finished_             () const                                { return _state == s_finished; }
//    virtual string              async_state_text_           () const                                { return "Buffered_xml_response"; }
//
//    // Xml_response
//    string                      get_part                    ();
//    void                        append_text                 ( const string& );
//
//    void                        close                       ();
//
//  private:   
//    Fill_zero                  _zero_;
//    State                      _state;
//    int                        _buffer_size;
//    string                     _buffer;
//    z::File                    _congestion_file;
//    bool                       _last_seek_for_read;
//    int64                      _congestion_file_write_position;
//    int64                      _congestion_file_read_position;
//    bool                       _close;
//};
//
//-------------------------------------------------------------------File_buffered_command_response

struct File_buffered_command_response : Command_response
{
    enum State
    {
        s_ready, 
        s_congested,
        s_finished
    };

                                File_buffered_command_response();
                               ~File_buffered_command_response();

    // Async_operation
    virtual bool                async_continue_             ( Continue_flags );
    virtual bool                async_finished_             () const                                { return _state == s_finished; }
    virtual string              async_state_text_           () const                                { return "File_buffered_xml_response"; }

    // Xml_response
    string                      get_part                    ();
    void                        write                       ( const io::Char_sequence& );

    // Writer
    void                        close                       ();
    void                        flush                       ();

  protected:
    Fill_zero                  _zero_;

  private:   
    State                      _state;
    int                        _buffer_size;
    string                     _buffer;
    zschimmer::file::File      _congestion_file;
    bool                       _last_seek_for_read;
    int64                      _congestion_file_write_position;
    int64                      _congestion_file_read_position;
    bool                       _close;
};

//----------------------------------------------------------------------Get_events_command_response

struct Get_events_command_response : File_buffered_command_response
{
                                Get_events_command_response ( Scheduler_event_manager* m )          : _zero_(this+1), _scheduler_event_manager(m) {}
                               ~Get_events_command_response ();


    void                        close                       ();
    void                    set_append_0_byte               ( bool b )                              { _append_0_byte = b; }

    bool                        is_event_selected           ( const Scheduler_event& )              { return true; }
    void                        write_event                 ( const Scheduler_event& );

  private:
    Fill_zero                  _zero_;
    Scheduler_event_manager*   _scheduler_event_manager;
    bool                       _append_0_byte;
    bool                       _closed;
};

//--------------------------------------------------------------------------------Command_processor

struct Command_processor
{
                                Command_processor           ( Spooler*, Security::Level, Communication::Operation* cp = NULL );
                               ~Command_processor           ();

    Security::Level             security_level              () const                                { return _security_level; }
    Communication::Operation*   communication_operation     () const                                { return _communication_operation; }

    void                        execute_config_file         ( const string& xml_filename );
    void                        execute_http                ( http::Operation*, Http_file_directory* );
    ptr<Command_response>       response_execute            ( const string& xml_text, const string& indent_string = "" );
    string                      execute                     ( const string& xml_text, const string& indent_string = "" );
    xml::Document_ptr           execute                     ( const xml::Document_ptr& );
    void                        execute_2                   ( const string& xml_text );
    void                        execute_2                   ( const xml::Document_ptr& );
    void                        execute_2                   ( const xml::Element_ptr& );
    xml::Document_ptr           dom_from_xml                ( const string& xml_text );
    void                        execute_commands            ( const xml::Element_ptr& );
    xml::Element_ptr            execute_command             ( const xml::Element_ptr& );
    void                        begin_answer                ();
    void                        append_error_to_answer      ( const exception& );
    void                        append_error_to_answer      ( const Xc& );
    xml::Element_ptr            execute_config              ( const xml::Element_ptr& );
    void                        set_log                     ( Has_log* log )                        { _log = log; }     // Optional

    xml::Element_ptr            execute_scheduler_log       ( const xml::Element_ptr&, const Show_what& );
    void                        execute_scheduler_log__append( const xml::Element_ptr&, const string&, const xml::Element_ptr& );
    xml::Element_ptr            execute_licence             ( const xml::Element_ptr& element, const Show_what& );
    xml::Element_ptr            execute_subsystem           ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_state          ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_calendar       ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_history        ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_jobs           ( const Show_what& );
    xml::Element_ptr            execute_show_process_classes( const Show_what& );
    xml::Element_ptr            execute_add_jobs            ( const xml::Element_ptr& );
    xml::Element_ptr            execute_job                 ( const xml::Element_ptr& );
    xml::Element_ptr            execute_job_chain           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_job            ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_modify_job          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_start_job           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_remote_scheduler_start_remote_task( const xml::Element_ptr& );
    xml::Element_ptr            execute_remote_scheduler_remote_task_close( const xml::Element_ptr& ); 
    xml::Element_ptr            execute_show_cluster        ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_task           ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_check_folders       ( const xml::Element_ptr& );
    xml::Element_ptr            execute_kill_task           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_hot_folder   ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_spooler      ( const xml::Element_ptr& );
    xml::Element_ptr            execute_terminate           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_signal_object       ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_job_chains     ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_job_chain      ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_order          ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_add_order           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_order        ( const xml::Element_ptr& );
    xml::Element_ptr            execute_remove_order        ( const xml::Element_ptr& );
    xml::Element_ptr            execute_remove_job_chain    ( const xml::Element_ptr& );
    xml::Element_ptr            execute_register_remote_scheduler( const xml::Element_ptr& );
    xml::Element_ptr            execute_service_request     ( const xml::Element_ptr& );
    xml::Element_ptr            execute_get_events          ( const xml::Element_ptr& );

    void                        get_id_and_next             ( const xml::Element_ptr& element, int* id, int* next );

  //void                        set_communication_operation ( Communication::Operation* p )         { _communication_operation = p; }
    void                        set_validate                ( bool b )                              { _validate = b; }
    void                        set_variable_set            ( const string& name, Com_variable_set* v ) { _variable_set_map[ name ] = v; }
  //void                        set_subst                   ( Com_variable_set* env )               { set_variable_set( variable_set_name_for_substitution, env ); }

  //void                        abort_immediately           ( int exit_code = 1 );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Communication::Operation*  _communication_operation;
    bool                       _load_config_immediately;
    bool                       _dont_log_command;
    xml::Document_ptr          _answer;
    Xc_copy                    _error;
    bool                       _validate;
    Security::Level            _security_level;
    string                     _source_filename;            // Das Verzeichnis wird für <base file=".."> verwendet
    Has_log*                   _log;
    Variable_set_map           _variable_set_map;
    ptr<Command_response>      _response;
};

//-------------------------------------------------------------------------------------------------


} //namespace scheduler
} //namespace sos

#endif
