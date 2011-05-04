#ifndef TOOLSEX_INCL

/* Ausgabe Systemfehlermeldung auf Bildschirm */
EXT DLL VOID syserr(TEXT *msg);

/* Abfrage auf Yes/No */
EXT DLL COUNT yes_no(COUNT Yes_no);

EXT DLL VOID hex_out (pTEXT buf, COUNT len); 


/* 
Funktionen fuer doppelt verkettete Listen.

Include: toolsdef.h
Source:  tool.c

Aufbau:
=======

Leere, initialisierte Liste:
----------------------------

  ----head<-------
  |-->tail=NULL  |
      tailpred----


Liste mit einem Node:
---------------------

  --------------------
  |                  |
  |  head---------->succ<--
  |    ^---------         |
  |->tail=NULL  |---pred  |
                          |
     tailpred--------------


Liste mit 2 Nodes:
------------------

  ----------------------------
  |                          |
  |  head---------->succ--->succ<--
  |    ^---------     ^----       |
  |->tail=NULL  |---pred  |-pred  |
                                  |
     tailpred----------------------


Durchscannen der Liste:
-----------------------
struct node *test;

for (test = list->head;test->succ;test = test->succ)


*/


/*
Neue Liste initialisieren.
Parameter: list = Liste
Rueckgabe: Keine
*/

EXT DLL VOID init_list (struct list *list);



/*
Hinzufuegen eines Nodes am Kopf der Liste.

Parameter: list = Liste
           node = Neuer Node
Rueckgabe: Keine
*/

EXT DLL VOID add_head (struct list *list,struct node *node);



/*
Hinzufuegen eines Nodes am Ende der Liste.

Parameter: list = Liste
           node = Neuer Node
Rueckgabe: Keine
*/

EXT DLL VOID add_tail (struct list *list,struct node *node);



/*
Entfernen eines Nodes am Kopf der Liste.

Parameter: list = Liste
Rueckgabe: Entfernter Node oder NULL bei leerer Liste
*/

EXT DLL struct node *rem_head (struct list *list);



/*
Entfernen eines Nodes am Ende der Liste.

Parameter: list = Liste
Rueckgabe: Entfernter Node oder NULL bei leerer Liste
*/

EXT DLL struct node *rem_tail (struct list *list);



/*
Einfuegen eines Nodes nach einem Node.

Parameter: list = Liste
           node = Einzufuegender Node
           pred = Vorgaenger von Node
Rueckgabe: Keine
*/

EXT DLL VOID insert_node (struct list *list,struct node *node,struct node *pred);



/*
Entfernen eines Nodes aus der Liste.

Parameter: list = Liste
           node = Zu entfernender Node
Rueckgabe: Keine
*/

EXT DLL VOID remove_node (struct list *list,struct node *node);



/*
Suchen eines Nodes anhand des Namens.
Zum Weitersuchen kann statt list auch ein Node uebergeben
werden.

Parameter: list   = Liste
           name   = Name des zu suchenden Nodes
           nocase:  0 = case sensitive, 1 = nicht case sensitive
Rueckgabe: Gesuchter Node oder NULL bei Fehlschlag
*/

EXT DLL struct node *find_name (struct list *list,TEXT *name,COUNT nocase);



/*
Zaehlen der Nodes in einer Liste.

Parameter: list = Liste
Rueckgabe: Anzahl Nodes in der Liste
*/

EXT DLL COUNT count_nodes (struct list *list);

/* Ende der Funktionen fuer doppelt verkettete Listen */





#define TOOLSEX_INCL

#endif /* TOOLSEX_INCL */
