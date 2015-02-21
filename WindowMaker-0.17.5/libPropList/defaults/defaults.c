/* defaults.c: This is -*- c -*-

   This file implements the libPropList "defaults" program, which
   allows the user to read, write, and delete user defaults from a
   UNIX shell.

   The defaults file is
   "~/$(GNUSTEP_USER_PATH)/$(GNUSTEP_DEFAULTS_FILE)".
   GNUSTEP_USER_PATH defaults to ~/GNUstep, and GNUSTEP_DEFAULTS_FILE
   defaults to .GNUstepDefaults, if the respective environment
   variables aren't set.

   */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "proplist.h"

proplist_t pl;
char *progname;

char *read_description(void)
{
  char *buf, *tmp;
  char line[256];
  int bytes_read;
  
  bytes_read = fread(line, sizeof(char), 255, stdin);
  buf = (char *)malloc(bytes_read+1);
  memcpy(buf, line, bytes_read);
  buf[bytes_read]='\0';

  while(!feof(stdin))
    {
      bytes_read = fread(line, sizeof(char), 255, stdin);
      tmp = (char *)malloc(strlen(buf)+bytes_read+1);
      memcpy(tmp, buf, strlen(buf));
      memcpy(&(tmp[strlen(buf)]), line, bytes_read);
      tmp[strlen(buf)+bytes_read] = '\0';
      free(buf);
      buf = tmp;
    }
  return buf;
}


int d_read(char *domain_name, char *key)
{
  proplist_t domain, entry;  
  proplist_t arr, dict;
  int i;

  if(!domain_name && !key)
    {
      arr = PLGetDomainNames();
      if(!arr)
	{
	  fprintf(stderr, "%s: No registered domains\n", progname);
	  return 1;
	}
      dict = PLMakeDictionaryFromEntries(NULL, NULL);
      for(i=0; i<PLGetNumberOfElements(arr); i++)
	PLInsertDictionaryEntry(dict, PLGetArrayElement(arr, i),
				PLGetDomain(PLGetArrayElement(arr, i)));
      printf("%s\n", PLGetDescriptionIndent(dict, 0));
      return 0;
    }
  else
    {
      domain = PLGetDomain(PLMakeString(domain_name));
      if(!domain)
	{
	  fprintf(stderr, "%s: No domain named %s\n",
		  progname, domain_name);
	  return 1;
	}
      
      if(!key)
	{
	  printf("%s\n", PLGetDescriptionIndent(domain, 0));
	  return 0;
	}

      if(!PLIsDictionary(domain))
	{
	  fprintf(stderr, "%s: Domain for key %s is not a dictionary\n",
		  progname, domain_name);
	  return 1;
	}
	  
      entry = PLGetDictionaryEntry(domain, PLMakeString(key));
      if(!entry)
	{
	  fprintf(stderr, "%s: Domain %s doesn't contain entry for %s\n",
		  progname, domain_name, key);
	  return 1;
	}

      printf("%s\n", PLGetDescriptionIndent(entry, 0));
    }
  return 0;
}

int d_write_rep(char *domain_name, char *domain_rep)
{
  proplist_t new;
  char *desc;
  
  if(!strcmp(domain_rep, "-"))
    { /* read from stdin */
      desc = read_description();
    }
  else
    desc = domain_rep;

      
  new = PLGetProplistWithDescription(desc);
  if(!new)
    {
      fprintf(stderr, "%s: Could not parse description '%s'\n", progname, desc);
      return 1;
    }

  if(!PLSetDomain(PLMakeString(domain_name), new, NO))
    return 1;
  else
    return 0;
}

int d_write_kv(char *domain_name, char *key, char *value_rep)
{
  proplist_t newv, newk, dict;
  char *desc;
  
  if(!strcmp(value_rep, "-"))
    desc = read_description();
  else
    desc = value_rep;
  
  newv = PLGetProplistWithDescription(desc);
  if(!newv)
    {
      fprintf(stderr, "%s: Could not parse description '%s'\n", progname, desc);
      return 1;
    }

  newk = PLGetProplistWithDescription(key);
  if(!newk)
    {
      fprintf(stderr, "%s: Could not parse description '%s'\n", progname, key);
      return 1;
    }

  dict = PLGetDomain(PLMakeString(domain_name));
  if(!dict)
    dict = PLMakeDictionaryFromEntries(newk, newv, NULL);

  PLInsertDictionaryEntry(dict, newk, newv);

  PLSetDomain(PLMakeString(domain_name), dict, NO);
  return 0;
}

int d_delete(char *domain_name, char *key)
{
  proplist_t domain, entry, allkeys;
  char buf[255];
  
  if(!domain_name && !key)
    {
      fprintf(stderr, "This will delete all user defaults! Are you sure? (y/n) ");
      fflush(stdout);
      gets(buf);
      if((buf[0]!='y') && (buf[0]!='Y'))
	{
	  printf("Nothing changed.\n");
	  return 1;
	}
      allkeys = PLGetDomainNames();

      while((PLGetNumberOfElements(allkeys)))
	{
	  PLDeleteDomain(PLGetArrayElement(allkeys, 0), NO);
	  PLRemoveArrayElement(allkeys, 0);
	}
      
      return 0;
    }
  else
    {
      
      domain = PLGetDomain(PLMakeString(domain_name));
      if(!domain)
	{
	  fprintf(stderr, "%s: No domain named %s\n",
		  progname, domain_name);
	  return 1;
	}
      
      if(!key)
	{
	  PLDeleteDomain(PLMakeString(domain_name), NO);

	  return 0;
	}

      if(!PLIsDictionary(domain))
	{
	  fprintf(stderr, "%s: Domain for key %s is not a dictionary\n",
		  progname, domain_name);
	  return 1;
	}
	  
      entry = PLGetDictionaryEntry(domain, PLMakeString(key));
      if(!entry)
	{
	  fprintf(stderr, "%s: Domain %s doesn't contain entry for %s\n",
		  progname, domain_name, key);
	  return 1;
	}

      PLRemoveDictionaryEntry(domain, PLMakeString(key));
      PLSetDomain(PLMakeString(domain_name), domain, NO);
    }
  return 0;
}

int d_domains(void)
{
  proplist_t arr;
  int i;

  arr = PLGetDomainNames();

  if(!arr)
    {
      fprintf(stderr, "%s: Error in User Defaults file\n", progname);
      return 1;
    }

  for(i=0; i<PLGetNumberOfElements(arr); i++)
    printf("%s\n", PLGetDescription(PLGetArrayElement(arr, i)));
  
  return 0;
}

proplist_t d_rec_find(char *word, proplist_t list)
{
  int i;
  proplist_t arr = NULL;
  proplist_t dict = NULL;
  proplist_t tmp;
  
  if(PLIsString(list))
    if(strstr(PLGetString(list), word))
      return list;
    else
      return NULL;
  if(PLIsData(list))
    if(strstr(PLGetDataDescription(list), word))
      return list;
    else
      return NULL;
  if(PLIsArray(list))
    {
      for(i=0; i<PLGetNumberOfElements(list); i++)
	{
	  tmp = d_rec_find(word, PLGetArrayElement(list, i));
	  if(tmp)
	    {
	      if(!arr) arr = PLMakeArrayFromElements(NULL);
	      PLAppendArrayElement(arr, list);
	    }
	}
      return arr;
    }
  if(PLIsDictionary(list))
    {
      arr = PLGetAllDictionaryKeys(list);
      for(i=0; i<PLGetNumberOfElements(arr); i++)
	{
	  tmp = d_rec_find(word, PLGetArrayElement(arr, i));
	  if(tmp)
	    {
	      if(!dict) dict = PLMakeDictionaryFromEntries(NULL, NULL);
	      PLInsertDictionaryEntry(dict, PLGetArrayElement(arr, i),
				      PLGetDictionaryEntry(list,
							   PLGetArrayElement(arr, i)));
	    }
	  tmp = d_rec_find(word, PLGetDictionaryEntry(list, PLGetArrayElement(arr, i)));
	  if(tmp)
	    {
	      if(!dict) dict = PLMakeDictionaryFromEntries(NULL, NULL);
	      PLInsertDictionaryEntry(dict, PLGetArrayElement(arr, i), tmp);
	    }
	}
      return dict;
    }
  return NULL; /* neither Str nor Data nor Dict nor Array */
}
	  

int d_find(char *word)
{
  proplist_t found;
  proplist_t dict, arr;
  int i;

  arr = PLGetDomainNames();
  if(!arr)
    {
      return 1;
    }
  dict = PLMakeDictionaryFromEntries(NULL, NULL);
  for(i=0; i<PLGetNumberOfElements(arr); i++)
    PLInsertDictionaryEntry(dict, PLGetArrayElement(arr, i),
			    PLGetDomain(PLGetArrayElement(arr, i)));

  found = d_rec_find(word, dict);

  if(found)
    {
      printf("%s\n", PLGetDescriptionIndent(found, 0));
      return 0;
    }
  
  return 1;
}

int d_help(void)
{
  fprintf(stderr, "Usage:\n"
	          "\t%s COMMAND [ ARGS ]\n"
	          "\t%s { -h | --help }\n"
	          "Recognized commands:\n"
	          "\tread [ DOMAIN-NAME [ KEY ] ]\n"
	          "\twrite { DOMAIN-NAME KEY { 'VALUE-REP'  | - } |\n"
	          "\t        DOMAIN-NAME     { 'DOMAIN-REP' | - } } \n"
	          "\tdelete [ DOMAIN-NAME [ KEY ] ]\n"
	          "\tdomains\n"
	          "\tfind WORD\n"
	          "\thelp\n"
	          "Refer to the manual page (Section 1) for more information.\n",
	  progname, progname);
  return 0;
}

int main(int argc, char **argv)
{
  char *arg1, *arg2, *arg3;
  int cmd_argno;
  
  progname = argv[0];
  
  cmd_argno = 1;

  if(argc==1)
    {
      fprintf(stderr, "%s: Too few arguments\n", argv[0]);
      d_help();
      return 1;
    }

  if(argv[1][0] == '-') /* process options */
    {
      {
	fprintf(stderr, "%s: Unrecognized option %s\n",
		argv[0], argv[cmd_argno]);
	d_help();
	return 1;
      }
    }

  if(!strcmp(argv[cmd_argno], "help"))
    {
      if(argc>cmd_argno+1)
	{
	  fprintf(stderr, "%s: Too many arguments to \"help\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      return d_help();
    }
  
  if(!strcmp(argv[cmd_argno], "read"))
    {
      if(argc>cmd_argno+3)
	{
	  fprintf(stderr, "%s: Too many arguments to \"read\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      if(argc==cmd_argno+3)
	{
	  arg1 = argv[cmd_argno+1];
	  arg2 = argv[cmd_argno+2];
	}
      else if(argc==cmd_argno+2) {arg1 = argv[cmd_argno+1]; arg2 = NULL;}
      else {arg1 = arg2 = NULL;}
      return d_read(arg1, arg2);
    }
  if(!strcmp(argv[cmd_argno], "write"))
    {
      if(argc>cmd_argno+4)
	{
	  fprintf(stderr, "%s: Too many arguments to \"write\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      if(argc<cmd_argno+3)
	{
	  fprintf(stderr, "%s: Too few arguments to \"write\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      arg1 = argv[cmd_argno+1]; arg2 = argv[cmd_argno+2];
      if(argc==cmd_argno+4)
	{
	  arg3 = argv[cmd_argno+3];
	  return d_write_kv(arg1, arg2, arg3);
	}
      return d_write_rep(arg1, arg2);
    }
  if(!strcmp(argv[cmd_argno], "delete"))
    {
      if(argc>cmd_argno+3)
	{
	  fprintf(stderr, "%s: Too many arguments to \"delete\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      if(argc==cmd_argno+3)
	{
	  arg1 = argv[cmd_argno+1];
	  arg2 = argv[cmd_argno+2];
	}
      else if(argc==cmd_argno+2) {arg1 = argv[cmd_argno+1]; arg2 = NULL;}
      else arg1 = arg2 = NULL;
      return d_delete(arg1, arg2);
    }
  if(!strcmp(argv[cmd_argno], "domains"))
    {
      if(argc!=cmd_argno+1)
	{
	  fprintf(stderr, "%s: Too many arguments to \"domains\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      return d_domains();
    }
  if(!strcmp(argv[cmd_argno], "find"))
    {
      if(argc<cmd_argno+2)
	{
	  fprintf(stderr, "%s: Missing argument to \"find\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      else if(argc>cmd_argno+2)
	{
	  fprintf(stderr, "%s: Too many arguments to \"find\" command\n",
		  argv[0]);
	  d_help();
	  return 1;
	}
      arg1 = argv[cmd_argno+1];
      return d_find(arg1);
    }

  fprintf(stderr, "%s: Unrecognized command %s\n", argv[0], argv[cmd_argno]);
  d_help();
  return 1;
}
