// $Id: Memory_allocator.h 14145 2010-12-02 12:11:47Z jz $

#ifndef __ZSCHIMMER_MEMORY_ALLOCATOR_H
#define __ZSCHIMMER_MEMORY_ALLOCATOR_H

#include "mutex.h"



namespace zschimmer {


struct Memory_allocator
{
    static bool                 debug_memory            ();
    static void*                z_malloc                ( size_t size, const char* name, int lineno );
    static void                 z_free                  ( void* p );

    struct Memory_type {
        Memory_type(const char* name, int line_no, size_t size) : _name(name),_line_no(line_no), _size(size) {}
        string to_string() const { return S() << _name << ":" << _line_no; }
        const char* _name;
        int _line_no;
        size_t _size;
    };

    struct Allocation {
        Allocation() : _count(0) {}
        int _count;
    };

    typedef std::map<Memory_type,Allocation> Allocation_map;
    static Allocation_map allocation_map();
    static bool static_debug_memory;
    static bool static_initialized;

private:
    static Mutex _mutex;

    typedef stdext::hash_map<void*,Memory_type>  Map;
    static Map _map;
};


inline size_t hash_value(const Memory_allocator::Memory_type& o) { 
    return (size_t)o._name ^ o._line_no ^ o._size; 
}

inline bool operator<(const Memory_allocator::Memory_type& a, const Memory_allocator::Memory_type& b) { 
    int c = strcmp(a._name, b._name); 
    return c < 0 || (c == 0 && a._size < b._size); 
} 

}

#endif
