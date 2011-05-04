#ifndef TOOLSDEF_INCL

/* Node fuer doppelt verkettete Listen */

/*
Include: toolsex.h
Source:  tool.c
*/
struct node
{
  struct node *succ; /* Nachfolger */
  struct node *pred; /* Vorgaenger */
  TEXT *name; /* Name */
};


/* Kopf fuer doppelt verkettete Listen */

struct list
{
  struct node *head; /* 1. Node */
  struct node *tail; /* immer NULL */
  struct node *tailpred; /* letzter Node */
};


typedef struct tools_global
{
  ECBERROR ecb_errno;
  TEXT     ecb_errtxt[40];
} TOOLS_GLOBAL_S;


#ifdef WIN32
#ifdef TOOLS_MAIN
DWORD ToolsGlobTlsIndex;
#else
extern DWORD ToolsGlobTlsIndex;
#endif
#else
#ifdef TOOLS_MAIN
TOOLS_GLOBAL_S tools_g;
TOOLS_GLOBAL_S *GlobPoolTools = &tools_g;
#else
extern TOOLS_GLOBAL_S *GlobPoolTools;
#endif
#endif


/*
#ifdef WIN32
  TOOLS_GLOBAL_S *GlobPoolTools;
  GlobPoolTools=(TOOLS_GLOBAL_S *)TlsGetValue (ToolsGlobTlsIndex);
#endif
*/

#define TOOLSDEF_INCL

#endif /* TOOLSDEF_INCL */
