// $Id: spooler_command.h,v 1.7 2002/04/04 17:18:38 jz Exp $

#ifndef __SPOOLER_COMMAND_H
#define __SPOOLER_COMMAND_H

namespace sos {
namespace spooler {

void                            dom_append_text_element     ( const xml::Element_ptr& element, const char* element_name, const string& text );
void                            append_error_element        ( const xml::Element_ptr&, const Xc_copy& );
void                            dom_append_nl               ( const xml::Element_ptr& );

//--------------------------------------------------------------------------------Command_processor

struct Command_processor
{
                                Command_processor           ( Spooler* spooler )                    : _spooler(spooler),_host(NULL) {}

    string                      execute                     ( const string& xml_text );
    void                        execute_2                   ( const string& xml_text );
    xml::Element_ptr            execute_command             ( const xml::Element_ptr& );
    xml::Element_ptr            execute_config              ( const xml::Element_ptr& );
    xml::Element_ptr            execute_add_jobs            (  const xml::Element_ptr& );
    xml::Element_ptr            execute_show_state          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_history        ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_threads        ( bool show_all );
    xml::Element_ptr            execute_show_job            ( Job* );
    xml::Element_ptr            execute_modify_job          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_start_job           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_spooler      ( const xml::Element_ptr& );
    xml::Element_ptr            execute_signal_object       ( const xml::Element_ptr& );

    void                        set_host                    ( Host* host )                          { _host = host; }

    Spooler*                   _spooler;
    xml::Document_ptr          _answer;
    Xc_copy                    _error;
    Host*                      _host;
    Security::Level            _security_level;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
