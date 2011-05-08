// $Id$

#include "zschimmer.h"
#include "Memory_allocator.h"

using namespace std;

namespace zschimmer {

//-------------------------------------------------------------------------------------------static

Mutex                  Memory_allocator::_mutex;
Memory_allocator::Map  Memory_allocator::_map;
bool Memory_allocator::static_initialized = false;
bool Memory_allocator::static_debug_memory = false;
const char* environment_variable_name = "SOS_SCHEDULER_DEBUG_MEMORY";

/////////////////////////////////////////
// Stürzt am Ende ab, weil Memory_allocator::_mutex und ::_map zerstört sind, wenn noch ein z_free() aufgerufen wird.
// Also nur zum Debuggen verwenden.
/////////////////////////////////////////

//-----------------------------------------------------------------------------------------z_malloc

void* z_malloc(size_t size, const char* name, int lineno) 
{ 
    return Memory_allocator::debug_memory()? Memory_allocator::z_malloc(size, name, lineno) 
                                           : malloc(size);
}

//-------------------------------------------------------------------------------------------z_free

void z_free(void* p)
{ 
    assert(Memory_allocator::static_initialized);
    return Memory_allocator::debug_memory()? Memory_allocator::z_free(p) 
                                           : free(p);
}

//-------------------------------------------------------------------------------------------------

#if defined Z_WINDOWS  &&  defined _DEBUG

    inline void*                my_malloc               ( size_t size, const char* name, int lineno ) { return _malloc_dbg( size, _CLIENT_BLOCK, name, lineno ); }
    inline void                 my_free                 ( void* p )                                   { return _free_dbg( p, _CLIENT_BLOCK ); }

#else

    inline void*                my_malloc               ( size_t size, const char*, int )             { return malloc( size ); }
    inline void                 my_free                 ( void* p )                                   { return free( p ); }

#endif

//-----------------------------------------------------------Memory_allocator::debug_memory

bool Memory_allocator::debug_memory() {
    if (!static_initialized) {
        static_debug_memory = getenv(environment_variable_name) != NULL;
        static_initialized = true;
    }

    return static_debug_memory;
}

//-----------------------------------------------------------------------Memory_allocator::z_malloc
    
void* Memory_allocator::z_malloc( size_t size, const char* name, int lineno )
{
    void* result = NULL;

    Z_FAST_MUTEX(_mutex) {
        result = my_malloc(size, name, lineno);
        _map.insert(Map::value_type(result, Memory_type(name, lineno, size)));
    }

    return result;
}
 
//-------------------------------------------------------------------------Memory_allocator::z_free

void Memory_allocator::z_free( void* p )
{
    Z_FAST_MUTEX(_mutex) {
        _map.erase(p);
        my_free(p);
    }
}

//-------------------------------------------------------------Memory_allocator::memory_counter_map

Memory_allocator::Allocation_map Memory_allocator::allocation_map()
{
    Allocation_map result;

    Z_FAST_MUTEX(_mutex) {
        Z_FOR_EACH_CONST(Map, _map, it) {
            Allocation& e = result[it->second];
            e._count++;
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
