// recfile.h
// 14. 3.92                                             (c) Joacim Zschimmer

#ifndef __RECFILE_H
#define __RECFILE_H

#include "absfile.h"

namespace sos {

struct Record_file : public Abs_record_file
{
    void                        open               ( const char*, Open_mode, const File_spec& );
    void                        close              ( Close_mode = close_normal );

  protected:
    void                        rewind             ( Key::Number );
    void                        put_record         ( const Const_area& );
    void                        update             ( const Const_area& );
    void                        insert             ( const Const_area& );
    void                        get_record         ( Area& );

  private:
    void                        write              ( const Const_area& );

    fstream                     f;
    int                        _current_length;
    long                       _pos;
}; // struct Record_file

} //namespace sos


#endif

