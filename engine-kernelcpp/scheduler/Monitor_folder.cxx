#include "spooler.h"

namespace sos {
namespace scheduler {
    
Monitor_folder::Monitor_folder(Folder* folder) :
    typed_folder<Monitor>(folder->spooler()->monitor_subsystem(), folder, Scheduler_object::type_monitor_folder)
{}


}}
