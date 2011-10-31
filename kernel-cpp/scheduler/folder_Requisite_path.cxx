#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//-------------------------------------------------------------------------Requisite_path::obj_name

string Requisite_path::obj_name() const
{
    S result;
    result << _subsystem->object_type_name() << " " << _path;
    return result;
}

}}} //namespace sos::scheduler::folder
