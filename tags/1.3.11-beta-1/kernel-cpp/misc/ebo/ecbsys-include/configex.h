#ifndef CONFIGEX_INCL


EXT DLL VOID *read_config (TEXT *config);
EXT DLL VOID write_config (VOID *handle);
EXT DLL VOID close_config (VOID *handle);
EXT DLL VOID *find_section (VOID *handle,TEXT *partname,VOID *after);
EXT DLL COUNT find_key (VOID *handle,TEXT *section,TEXT *key,
                        TEXT *parameter);
EXT DLL VOID *find_parameter (VOID *handle,TEXT *section,TEXT *key,
                              TEXT *parameter,VOID *after);
EXT DLL COUNT write_key (VOID *handle,TEXT *section,TEXT *key,
                         TEXT *parameter);
EXT DLL COUNT write_comment (TEXT *config,TEXT *section,
                             TEXT *comment);


#define CONFIGEX_INCL

#endif /* CONFIGEX_INCL */
