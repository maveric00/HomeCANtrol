#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "XMLConfig.h"

extern struct Node *Haus ;


struct Node *CreateNode (void)
{
  int i ;
  struct Node *This ;

  This = malloc (sizeof(struct Node)) ;
  if (!This) {
    fprintf (stderr, "Out of Memory \n") ;
    exit(-1) ;
  } ;
  This->Parent = NULL ;
  This->Next = NULL ;
  This->Prev = NULL ;
  This->Child = NULL ;

  This->Type = 0 ;
  This->Name[0] = 0 ;
  This->TypeDef[0] = 0 ;
  This->Value = 0 ;
  for (i=0;i<NAMELEN*5;i++) This->Data.PAD[i] = 0 ;
  return (This) ;
}

void FreeNode (struct Node *This) 
{
  
  // Aus der Parent-Liste loeschen

  if (This->Parent->Child == This) This->Parent->Child = This->Next; 

  // Nun rekursiv alle Kinder loeschen
  for (;This->Child!=NULL;) FreeNode(This->Child) ;

  // Nun aus der Peer-Kette entfernen
  if (This->Prev!=NULL) This->Prev->Next = This->Next ;
  if (This->Next!=NULL) This->Next->Prev = This->Prev ;

  // Nun noch den Speicherplatz freigeben

  free (This) ;
}

struct Node *NewChild (struct Node *This) 
{
  struct Node *There ;

  There = CreateNode () ;
  // Ende der Kinderliste suchen
  if (This->Child ==NULL) {
    This->Child = There ;
  } else {
    for (There->Prev=This->Child;There->Prev->Next!=NULL;There->Prev = There->Prev->Next) ;
    There->Prev->Next = There ;
  } ;
  There->Parent = This ;
  return (There) ; 
}

struct Node *FindNode (struct Node *Root,const char *Unit)
{
  int i ;
  struct Node *This ;
  
  for (i=0;(Unit[i]!='/')&&(Unit[i]!='\0');i++) ;

  // Knoten mit dem ersten Bezeichner finden
  for(This = Root ;This!=NULL;This=This->Next)
    if (strncmp(Unit,This->Name,i)==0) break ;

  if (This==NULL) return (NULL) ;


  if (Unit[i]=='/') { // Rekursiv die weiteren Teile finden
    return (FindNode(This->Child,&(Unit[i+1]))) ;
  } else {
    return (This) ;
  } ;
}

int GetNodeAdress (struct Node *Node, int *Linie, int *Knoten, int *Port)
{
  struct Node *This ;

  if (Node==NULL) return (1) ;
  
  for (This=Node->Child;This!=NULL;This=This->Next) if (This->Type==N_ADRESS) break ;

  if (This==NULL) return (1) ;

  *Linie = This->Data.Adresse.Linie ;
  *Knoten = This->Data.Adresse.Knoten ;
  *Port = This->Data.Adresse.Port ;
  return (0) ;
}

  
struct Node *FindNodeAdress (struct Node *Root,int Linie, int Knoten, int Port,struct Node *Except)
{
  struct Node *This,*That ;
  
  if (Root==NULL) return (NULL) ;

  // Knoten mit dem ersten Bezeichner finden
  for(This = Root ;This!=NULL;This=This->Next) {
    if ((This->Type==N_ADRESS)&&(This->Data.Adresse.Linie ==Linie)&&
	(This->Data.Adresse.Knoten==Knoten)&&(This->Data.Adresse.Port==Port)&&
	(This!=Except)) { 
      return (This->Parent) ;
    } ;
    That = FindNodeAdress(This->Child,Linie,Knoten,Port,Except) ;
    if (That!=NULL) return (That) ;
  } 
  return (NULL) ;
}

int CollectAdress (struct Node *Root, int Linie, int Knoten, struct Node *Result[], int *ResultNumber )
{
  int i ;
  struct Node *This ;
  
  if (Root==NULL) return (0) ;

  if ((Root->Type==N_ADRESS)&&(Root->Data.Adresse.Linie ==Linie)&&(Root->Data.Adresse.Knoten==Knoten)) { 
    // Passende Adresse gefunden, zu den Ergebnissen hinzufuegen
    if (*ResultNumber>=MAX_ADD_PER_NODE) return (1) ;
    Result[(*ResultNumber)] = Root->Parent ;
    (*ResultNumber) ++ ;
    return (0) ;
  }  ;

  // Kein Adressen-Knoten, also rekursiv weitersuchen

  i = 0;
  for (This = Root;This!=NULL;This=This->Next) i+=CollectAdress(This->Child,Linie,Knoten,Result,ResultNumber) ;
  return (i) ;
}

    
