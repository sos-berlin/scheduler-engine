// $Id$

#ifndef __SPOOLER_ORDER_FILE_H
#define __SPOOLER_ORDER_FILE_H


namespace sos {
namespace scheduler {
namespace order {

//-------------------------------------------------------------------------------------------------
    
extern const string             file_order_sink_job_path;

//-------------------------------------------------------------------------------------------------

void                            init_file_order_sink    ( Scheduler* );                             // Könnte ein Subsystem sein

//------------------------------------------------------------Directory_file_order_source_interface

struct Directory_file_order_source_interface : //idispatch_implementation< Directory_file_order_source, spooler_com::Idirectory_file_order_source >,
                                               Order_source
{
                                Directory_file_order_source_interface( Job_chain* job_chain, Type_code t ) : Order_source( job_chain, t ) {}
};

ptr<Directory_file_order_source_interface> new_directory_file_order_source( Job_chain*, const xml::Element_ptr& );

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace scheduler
} //namespace sos

#endif
