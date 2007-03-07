// $Id$

#ifndef __SPOOLER_ORDER_FILE_H
#define __SPOOLER_ORDER_FILE_H


namespace sos {
namespace scheduler {

//-------------------------------------------------------------------------------------------------
    
extern const string             file_order_sink_job_name;

//-------------------------------------------------------------------------------------------------

void                            init_file_order_sink    ( Scheduler* );                             // Könnte ein Subsystem sein

//------------------------------------------------------------Directory_file_order_source_interface

struct Directory_file_order_source_interface : //idispatch_implementation< Directory_file_order_source, spooler_com::Idirectory_file_order_source >,
                                               Order_source
{
};

ptr<Directory_file_order_source_interface> new_directory_file_order_source( Scheduler* scheduler, const xml::Element_ptr& element );

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
