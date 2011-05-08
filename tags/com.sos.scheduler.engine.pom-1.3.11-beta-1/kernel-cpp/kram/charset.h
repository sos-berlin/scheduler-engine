#ifndef __CHARSET_H
#define __CHARSET_H

struct Char_set_table_entry
{
    const char* name_ptr;
    const char* frame_name_ptr;
    const char* translate_string_ptr;
};

extern const Char_set_table_entry char_set_table [];

const int char_set_table_count();
#endif
