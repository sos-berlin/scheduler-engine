// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_INCLUDE_H
#define __SCHEDULER_INCLUDE_H

namespace sos {
namespace scheduler {
namespace include { 

//----------------------------------------------------------------------------------Include_command

struct Include_command
{
                                Include_command             ( Scheduler*, const File_based*, const xml::Element_ptr& );

    bool                        denotes_configuration_file  () const                                { return !_path.empty(); }
    File_path                   file_path                   () const                                { return _file_path; }
    Absolute_path               path                        () const                                { return _path; }
    string                      add_include_and_read_content( File_based* source_file_based );
    string                      read_content                ()                                      { return add_include_and_read_content( NULL ); }
  //file::File_info*            file_info                   ();

  private:
    Fill_zero                  _zero_;
    Absolute_path              _path;                       // Mit Dateinamensendung
    File_path                  _file_path;
    ptr<file::File_info>       _file_info;
};

//-------------------------------------------------------------------------------------Has_includes

struct Has_includes       // <include live_file="..."> 
{
                                Has_includes                ();
    virtual                    ~Has_includes                ();

  //virtual void                on_include_changed          ()                                      = 0;
    virtual Prefix_log*         log                         ()                                      = 0;
    virtual string              obj_name                    () const                                = 0;
    virtual Which_configuration which_configuration         () const                                = 0;
    virtual Spooler*            spooler                     () const                                = 0;

    void                        add_include                 ( const Absolute_path&, file::File_info* );
    void                        remove_include              ( const Absolute_path& );
    void                        remove_includes             ();
    file::File_info*            changed_included_file_info  ();
    
  private:
    typedef stdext::hash_map< string, ptr<file::File_info> >   Include_map;

    Fill_zero                  _zero_;
    Configuration*             _configuration;              // live/ oder cache/
    Include_map                _include_map;                // Path -> File_info
};

//---------------------------------------------------------------------------------Include_register

//struct Include_register : Object
//{
//                                Include_register            ();
//                               ~Include_register            ();
//
//    void                        add_include                 ( Has_includes*, const Absolute_path& );
//    void                        remove_include              ( Has_includes*, const Absolute_path& );
//    void                        check_files                 ( const directory_observer::Directory* );
//    void                        assert_no_has_includes      ( const Has_includes* );
//
//
//  private:
//    typedef stdext::hash_set< Has_includes* >   Has_includes_set;
//
//    struct Entry
//    {
//        ptr<file::File_info>   _file_info;
//        Has_includes_set       _has_includes_set;
//    };
//
//    typedef stdext::hash_map< string, Entry >   Include_map;
//
//
//    Fill_zero                  _zero_;
//    Include_map                _include_map;
//};

//-------------------------------------------------------------------------------------------------

} //namespace include
} //namespace scheduler
} //namespace sos

#endif
