#include "spooler.h"

namespace sos {
namespace scheduler {
namespace folder {

//-------------------------------------------------------------------Base_file_info::Base_file_info

Base_file_info::Base_file_info( const directory_observer::Directory_entry& de ) 
: 
    _path           ( de._file_info->path()            ), 
    _filename       ( de._file_info->path().name()     ), 
    _last_write_time( de._file_info->last_write_time() ),
    _normalized_name( de._normalized_name              ) 
{
}

//------------------------------------------------------------------Base_file_info::directory_entry

directory_observer::Directory_entry Base_file_info::directory_entry(Configuration_origin origin) const 
{
    directory_observer::Directory_entry result;
    result._file_info = Z_NEW(file::File_info(_path));
    result._file_info->set_last_write_time(_last_write_time);
    result._configuration_origin = origin;
    result._normalized_name = _normalized_name;
    return result;
}

}}} //namespace sos::scheduler::folder
