// profile.inl                                                  (c) SOS GmbH Berlin
//                                                                  Joacim Zschimmer

//-----------------------------------------------Profile::Section::Entry::Entry

inline Profile::Section::Entry::Entry( const string& entry_name, const string& default_value,
                                       Section* section_ptr )
:
    _section_ptr   ( section_ptr   ),
    _my_section    ( false         ),
    _entry_name    ( entry_name    ),
    _default_value ( default_value )
{
}

inline Profile::Section::Entry::Entry( const string& entry_name, const string& default_value,
                                       const Section& section )
:
    _section_ptr   ( new Section( section ) ),
    _my_section    ( true                   ),
    _entry_name    ( entry_name             ),
    _default_value ( default_value          )
{
}

inline const string& Profile::Section::filename() const
{
    return _filename;
}

inline const string& Profile::Section::section_name() const
{
    return _section_name;
}

