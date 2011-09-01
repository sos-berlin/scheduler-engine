// $Id: embedded_files.h 14010 2010-09-13 08:05:23Z jz $

#ifndef __ZSCHIMMER__INLINE_FILES_H
#define __ZSCHIMMER__INLINE_FILES_H

/**
* \change 2.0.224 - jira-XXX: Dynamisch eingebundes XSD verwenden
* \detail
*/
#include <vector>

namespace zschimmer {

    //------------------------------------------------------------------------------------Embedded_file

    struct Embedded_file
    { 
        const char*                _filename;
        const char*                _content;
        int                        _length; 
        time_t                     _last_modified_time;
    };

    //-----------------------------------------------------------------------------------Embedded_files

    struct Embedded_files
    { 
        const Embedded_file*        get_embedded_file           ( const string& filename ) const;
        const Embedded_file*        get_embedded_file_or_null   ( const string& filename ) const;
        string                      string_from_embedded_file   ( const string& filename ) const;

        const Embedded_file*       _embedded_files;
    };

    /**
    * \change 2.0.224 - jira-XXX: Dynamisch eingebundenes XSD verwenden
    * \detail
    */
    struct Dynamic_file
    { 
        string                     _filename;
        string*                    _content;
        int                        _length; 
        time_t                     _last_modified_time;
    };

    /**
    * \change 2.0.224 - jira-XXX: Dynamisch eingebundes XSD verwenden
    * \detail
    */
    struct Embedded_and_dynamic_files // : public Embedded_files
    {
        string                          string_from_embedded_file   ( const string& filename );
        void                            set_dynamic_file            ( string& filename, const string key );
        const Embedded_files           _embedded_files;
        std::vector<Dynamic_file*>*    _dynamic_files;
    };

    //-------------------------------------------------------------------------------------------------


    //-------------------------------------------------------------------------------------------------

} //namespace zschimmer

#endif
