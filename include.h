// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SCHEDULER_INCLUDE_H
#define __SCHEDULER_INCLUDE_H

namespace sos {
namespace scheduler {
namespace include { 

//----------------------------------------------------------------------------------Include_command

struct Include_command
{
                                Include_command             ( const File_based&, const xml::Element_ptr& );

    bool                        denotes_configuration_file  () const                                { return !_path.empty(); }
    File_path                   file_path                   () const                                { return _file_path; }
    Absolute_path               path                        () const                                { return _path; }

  private:
    Fill_zero                  _zero_;
    Absolute_path              _path;                       // Mit Dateinamensendung
    File_path                  _file_path;
};

//-------------------------------------------------------------------------------------Has_includes

struct Has_includes       // <include live_file="..."> 
{
                                Has_includes                ();
    virtual                    ~Has_includes                ();

    virtual void                on_include_changed          ()                                      = 0;
    virtual Prefix_log*         log                         ()                                      = 0;
    virtual string              obj_name                    () const                                = 0;
    virtual Spooler*            spooler                     () const                                = 0;        

    void                    set_which_configuration_directory( Which_configuration_directory w )    { _which_configuration_directory = w; }
    void                        add_include                 ( const Absolute_path& );
    void                        remove_include              ( const Absolute_path& );
    void                        remove_includes             ();

    
  private:
    Fill_zero                  _zero_;
    Which_configuration_directory _which_configuration_directory;
    typedef stdext::hash_set< string >   Include_set;
    Include_set                _include_set;
};

//---------------------------------------------------------------------------------Include_register

struct Include_register : Object
{
                                Include_register            ();
                               ~Include_register            ();

    void                        add_include                 ( Has_includes*, const Absolute_path& );
    void                        remove_include              ( Has_includes*, const Absolute_path& );
    void                        check_files                 ( const directory_observer::Directory* );


  private:
    typedef stdext::hash_set< Has_includes* >   Has_includes_set;

    struct Entry
    {
        ptr<file::File_info>   _file_info;
        Has_includes_set       _has_includes_set;
    };

    typedef stdext::hash_map< string, Entry >   Include_map;


    Fill_zero                  _zero_;
    Include_map                _include_map;
};

//-------------------------------------------------------------------------------------------------

} //namespace include
} //namespace scheduler
} //namespace sos

#endif
