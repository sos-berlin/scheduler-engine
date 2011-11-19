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

}}} //namespace sos::scheduler::folder
