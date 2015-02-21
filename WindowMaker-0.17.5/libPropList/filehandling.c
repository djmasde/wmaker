/* filehandling.c: This is -*- c -*-

   This file contains the implementation of the file handling
   functions

   */

#include "proplistP.h"
#include "util.h"

#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define pl_scan_string(c) yy_scan_string(c)
#define plparse() yyparse()
#define pl_delete_buffer(b) yy_delete_buffer(b)

extern void *pl_scan_string(const char *c);
extern void *plparse();
extern void pl_delete_buffer(void *buf);

/* declared in proplist.l (lex.pl.c) */
extern int pl_line_count;

proplist_t parse_result;
char *pl_curr_file = NULL;

proplist_t PLGetProplistWithDescription(const char *description)
{
  void *bufstate;
  proplist_t obj;

  pl_line_count = 1;

  bufstate = (void *)pl_scan_string(description);
  plparse();
  obj = parse_result;
  pl_delete_buffer(bufstate);

  return obj;
}

proplist_t PLGetProplistWithPath(const char *filename)
{
  char *str;
  proplist_t pl, filename_pl;
  int fd;
  struct stat fstat_buf;
  char *actual_filename;
  struct flock flk;

  if((!filename)||(strlen(filename)==0)) /* Refers to GNUstep defaults file */
    actual_filename = MakeDefaultsFilename();
  else
    actual_filename = ManglePath(filename);

  if((fd = open(actual_filename, O_RDONLY))<0)
    {
      free(actual_filename);
      return NULL;
    }
#if 0
  if((flock(fd, LOCK_EX))<0)
#endif
  flk.l_type = F_RDLCK;
  flk.l_start = 0;
  flk.l_whence = SEEK_SET;
  flk.l_len = 0;
  if (fcntl(fd, F_SETLK, &flk)<0)
    {
      close(fd);
      free(actual_filename);
      return NULL;
    }
  if(fstat(fd, &fstat_buf)<0)
    {
      close(fd);
      free(actual_filename);
      return NULL;
    }
  str = (char *)MyMalloc(__FILE__, __LINE__, sizeof(char)*(fstat_buf.st_size+32));
  if(read(fd, str, fstat_buf.st_size) != fstat_buf.st_size)
    {
      close(fd);
      MyFree(__FILE__, __LINE__, str);
#if 0
      flock(fd, LOCK_UN);
#endif
      flk.l_type = F_UNLCK;
      fcntl(fd, F_SETLK, &flk);
      return NULL;
    }

  str[fstat_buf.st_size + 1] = '\0';
  flk.l_type = F_UNLCK;
#if 0
  if((flock(fd, LOCK_UN))<0)
#endif
  if (fcntl(fd, F_SETLK, &flk)<0)
    {
      close(fd);
      MyFree(__FILE__, __LINE__, str);
      fprintf(stderr, "PLGetPropListWithPath(): Couldn't unlock file!\n");
      return NULL;
    }

  close(fd);

  pl_curr_file = (char *)filename;
  
  pl = PLGetProplistWithDescription(str);
  MyFree(__FILE__, __LINE__, str);

  pl_curr_file = NULL;

  if(pl)
    {
      filename_pl = PLMakeString(actual_filename);
      PLSetFilename(pl, filename_pl);
      PLRelease(filename_pl);
      MyFree(__FILE__, __LINE__, actual_filename);
      return pl;
    }
  else
    {
      MyFree(__FILE__, __LINE__, actual_filename);
      return NULL;
    }
}

proplist_t PLSetUnchanged(proplist_t pl)
{
  plptr_t internal = (plptr_t)pl;
  int i;
  
  switch(internal->type)
    {
    case PLARRAY:
      for(i=0; i<internal->t.array.number; i++)
	PLSetUnchanged(internal->t.array.elements[i]);
      break;
    case PLDICTIONARY:
      for(i=0; i<internal->t.dict.number; i++)
	{
	  PLSetUnchanged(internal->t.dict.keys[i]);
	  PLSetUnchanged(internal->t.dict.values[i]);
	}
      break;
    }
  internal->changed = NO;
  return pl;
}

proplist_t PLSynchronize2(proplist_t pl1, proplist_t pl2)
/* pl1 is the proplist changed in the program, pl2 the one that comes
   from the outside */
{
  plptr_t int1, int2, tmp_v1, tmp_v2, tmp_k, tmp;
  proplist_t arr1, arr2;
  int i;
  int num1, num2, num;
  int tmpnum;
  
  int1 = (plptr_t)pl1; int2 = (plptr_t)pl2;

  if (int1->type != int2->type) 
    { /* FIXME */
      puts("ERROR: DIFFERENT TYPE OBJECTS BEING SYNC'ED");
      return pl1;
    }
    
  switch(int1->type)
    {
    case PLSTRING:
      if(int1->changed)
	{
	  MyFree(__FILE__, __LINE__, int2->t.str.string);
	  int2->t.str.string =
	    (char *)MyMalloc(__FILE__, __LINE__, strlen(int1->t.str.string));
	  strcpy(int2->t.str.string, int1->t.str.string);
	}
      else if(!PLIsEqual(int1, int2))
	{
	  MyFree(__FILE__, __LINE__, int1->t.str.string);
	  int1->t.str.string =
	    (char *)MyMalloc(__FILE__, __LINE__, strlen(int2->t.str.string));
	  strcpy(int1->t.str.string, int2->t.str.string);
	}
      PLSetUnchanged(pl1);
      break;
    case PLDATA:
      if(int1->changed)
	{
	  MyFree(__FILE__, __LINE__, int2->t.data.data);
	  int2->t.data.data =
	    (unsigned char *)MyMalloc(__FILE__, __LINE__, int1->t.data.length);
	  memcpy(int2->t.data.data, int1->t.data.data,
		 int1->t.data.length);
	}
      else if(!PLIsEqual(int1, int2))
	{
	  MyFree(__FILE__, __LINE__, int1->t.data.data);
	  int1->t.data.data =
	    (unsigned char *)MyMalloc(__FILE__, __LINE__, int2->t.data.length);
	  memcpy(int1->t.data.data, int2->t.data.data,
		 int2->t.data.length);
	}
      PLSetUnchanged(pl1);
      break;
    case PLARRAY:
      /* if the list from the file has more elements than the local
	 array, append all to the local one. if it has less, check
	 which ones are changed by us and append those to the remote
	 one, remove the others from the local one. After that, the
	 numbers of elements are equal; check for changed ones and
	 synchronize them */
      num1 = PLGetNumberOfElements(pl1);
      num2 = PLGetNumberOfElements(pl2);
      if(num1<num2)
	for(i=num1; i<num2; i++)
	  {
	    if(int1->changed)
	      PLRemoveArrayElement(pl2, i);
	    else
	      {
		PLAppendArrayElement(pl1, PLGetArrayElement(pl2, i));
		PLSetUnchanged(PLGetArrayElement(pl1, i));
	      }
	  }
      else if(num1>num2)
	for(i=num2; i<num1; i++)
	  {
	    tmp = PLGetArrayElement(pl1, i);
	    if(tmp->changed)
	      {
		PLAppendArrayElement(pl2, PLGetArrayElement(pl1, i));
		tmpnum = PLGetNumberOfElements(pl2)-1;
		PLSetUnchanged(PLGetArrayElement(pl2, tmpnum));
	      }
	    else
	      PLRemoveArrayElement(pl1, i);
	  }
      num = PLGetNumberOfElements(pl1);
      for(i=0; i<num; i++)
	PLSynchronize2(PLGetArrayElement(pl1, i),
		       PLGetArrayElement(pl2, i));

      break;
    case PLDICTIONARY:
      arr1 = PLGetAllDictionaryKeys(pl1);
      arr2 = PLGetAllDictionaryKeys(pl2);
      num1 = PLGetNumberOfElements(arr1);
      num2 = PLGetNumberOfElements(arr2);
      if(num1<num2)
	{
	  for(i=0; i<num2; i++)
	    {
	      tmp_k = PLGetArrayElement(arr2, i);
	      if(!(tmp_v1 = PLGetDictionaryEntry(pl1, tmp_k)))
		{
		  /* have we changed anything in the current
		     container? If yes, remove this entry. If no,
		     leave it where it is */
		  if(int1->changed)
		    PLRemoveDictionaryEntry(pl2, tmp_k);
		  else
		    PLInsertDictionaryEntry(pl1, tmp_k, PLGetDictionaryEntry(pl2, tmp_k));
		}
	      else
		{
		  PLSynchronize2(tmp_v1, PLGetDictionaryEntry(pl2, tmp_k));
		}
	    }
	}
      /* Now, check for entries that aren't in pl2 */
      for(i=0; i<num1; i++)
	{
	  tmp_k = PLGetArrayElement(arr1, i);
	  tmp_v1 = PLGetDictionaryEntry(pl1, tmp_k);
	  if(!(tmp_v2 = PLGetDictionaryEntry(pl2, tmp_k)))
	    {
	      if(tmp_v1->changed)
		PLInsertDictionaryEntry(pl2, tmp_k, tmp_v1);
	      else
		PLRemoveDictionaryEntry(pl1, tmp_k);
	    }
	  else
	    {
	      PLSynchronize2(tmp_v1,tmp_v2);
	    }
	}
      break;
    }

  PLSetUnchanged(pl1);
  PLSetUnchanged(pl2);
  return pl1;
}
	      
      
BOOL PLSynchronize(proplist_t pl)
{
  char lockfilename[255];
  proplist_t fromFile;
  plptr_t internal, fF_internal;

  if(!PLGetFilename(pl))
    return NO;

  sprintf(lockfilename, "%s.lock", PLGetString(PLGetFilename(pl)));

  if((mkdir(lockfilename, 0755))<0)
    return NO;
  if(!(fromFile = PLGetProplistWithPath(PLGetString(PLGetFilename(pl)))))
    {
      rmdir(lockfilename);
      return NO;
    }

  internal = (plptr_t)pl;
  fF_internal = (plptr_t)fromFile;

  internal = PLSynchronize2(internal, fF_internal);

  if(!PLSave(fF_internal, YES))
    {
      PLRelease(fF_internal);
      return NO;
    }

  rmdir(lockfilename);
  return YES;
}

BOOL PLSave(proplist_t pl, BOOL atomically)
{
  const char *theFileName;
  const char *theRealFileName = NULL;
  char tmp_fileName[255];
  char tmp_realFileName[255];
  char dirname[255];
  char *tmp_dirname, *tmp2_dirname;
  char *basename, *tmp_basename;
  FILE *theFile;
  int c;
  char *desc = NULL;
  
  theRealFileName = PLGetString(PLGetFilename(pl));
  if(!theRealFileName) return NO;
  
  if (atomically)
    {
      theFileName = tmpnam(NULL);
      strcpy(tmp_fileName, theFileName);

      if((tmp_basename=strtok(tmp_fileName, "/")))
	do
	  basename=tmp_basename;
	while((tmp_basename=strtok(NULL, "/")));
      else
	basename=(char *)theFileName;
	
      strcpy(tmp_realFileName, theRealFileName);
      dirname[0]='\0';

      if((tmp_dirname=strtok(tmp_realFileName, "/")))
 	{
	  if(theRealFileName[0]=='/')
	    strcat(dirname, "/");
	  tmp2_dirname = strtok(NULL, "/");
	  while((tmp2_dirname)) 
	    { 
	      strcat(dirname, tmp_dirname); 
	      strcat(dirname, "/"); 
	      tmp_dirname = tmp2_dirname; 
	      tmp2_dirname = strtok(NULL, "/"); 
	    } 
	} 
      
      theFileName = strcat(dirname, basename);
    } 
  else
    { 
      theFileName = theRealFileName;
    } 

  /* Open the file (whether temp or real) for writing. */
  theFile = fopen(theFileName, "w");

  if (theFile == NULL)          /* Something went wrong; we weren't
                                 * even able to open the file. */
    goto failure;

  /* Pretty-print the description. I assume this is what we want? */
  desc = PLGetDescriptionIndent(pl, 0);
  /* FIXME: Do we need the `sizeof(char)' here? Is there any system
   * where sizeof(char) isn't just 1?  Or is it guaranteed to be 8
   * bits? */
  c = fwrite(desc, sizeof(char), strlen(desc), theFile);

  if (c < strlen(desc))        /* We failed to write everything for
				* some reason. */
    goto failure;

  /* We're done, so close everything up. */
  c = fclose(theFile);

  if (c != 0)                   /* I can't imagine what went wrong
                                 * closing the file, but we got here,
                                 * so we need to deal with it. */
    goto failure;

  /* If we used a temporary file, we still need to rename() it be the
   * real file.  Am I forgetting anything here? */
  if (atomically)
    {
      c = rename(theFileName, theRealFileName);

      if (c != 0)               /* Many things could go wrong, I
                                 * guess. */
	goto failure;
    }

  /* success: */
  MyFree(__FILE__, __LINE__, desc);
  return YES;

  /* Just in case the failure action needs to be changed. */
 failure:
  if(desc)
    MyFree(__FILE__, __LINE__, desc);
  return NO;

}      

proplist_t PLSetFilename(proplist_t pl, proplist_t filename)
{
  int i;
  plptr_t current;

  ((plptr_t)pl)->filename = filename;
  PLRetain(filename);
  
  switch(((plptr_t)pl)->type)
    {
    case PLARRAY:
      for(i=0;i<PLGetNumberOfElements(pl);i++)
	{
	  current = (plptr_t)PLGetArrayElement(pl, i);
	  PLSetFilename(current, filename);
	}
      break;
    case PLDICTIONARY:
      for(i=0;i<PLGetNumberOfElements(pl); i++)
	{
	  current = (plptr_t)(((plptr_t)pl)->t.dict.keys[i]);
	  PLSetFilename(current, filename);
	  current = (plptr_t)(((plptr_t)pl)->t.dict.values[i]);
	  PLSetFilename(current, filename);
	}
      break;
    }
  return pl;
}

proplist_t PLGetFilename(proplist_t pl)
{
  plptr_t internal = (plptr_t)pl;

  if(internal->filename)
    return internal->filename;
  else
    return NULL;
}



