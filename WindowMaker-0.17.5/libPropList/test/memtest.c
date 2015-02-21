#include <proplist.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX 4

void ArrayTest(BOOL describe)
{
  char *descr;
  int i, j;

  fprintf(stderr, "\nTesting Array\n");
  for(i=0; i<MAX; i++)
    {
      proplist_t arr, arr2;
      
      fprintf(stderr, "  Allocating (%d of %d)", i, MAX);
      
      if(i%2)
	{
	  proplist_t elem;
	  if((i/2)%2)
	    {
	      char e[100];
	      
	      fprintf(stderr, " with initial value (String)\n");
	      sprintf(e, "Elem%d", i);
	      elem = PLMakeString(e);
	    }
	  else
	    {
	      int n = 0;
	      
	      fprintf(stderr, " with initial value (Data)\n");
	      elem = PLMakeData((void *)&n, sizeof(int));
	    }
	  arr = PLMakeArrayFromElements(elem, NULL);
	  PLRelease(elem);
	}
      else
	{
	  fprintf(stderr, " empty\n");
	  arr = PLMakeArrayFromElements(NULL);
	}
      
      for(j=1; j<1000; j++)
	{
	  proplist_t elem;
	  
	  if((i/2)%2)
	    {
	      char e[100];
	      
	      sprintf(e, "Elem%d", j);
	      elem = PLMakeString(e);
	    }
	  else
	    elem = PLMakeData((void *)&j, sizeof(int));
	  
	  PLInsertArrayElement(arr, elem,
			       PLGetNumberOfElements(arr));
	  PLRelease(elem);
	}
      
      arr2 = PLDeepCopy(arr);
      
      if(describe)
	{
	  descr = PLGetDescription(arr2);
	  printf("**Array with %d entries\n",
		 PLGetNumberOfElements(arr2));	  
	  printf("**Array contains: %s\n", descr);
	  free(descr);
	}
      
      fprintf(stderr, "  Releasing.\n");
      PLRelease(arr);
      PLRelease(arr2);
    }
}


void DictTest(BOOL describe)
{
  char *descr;
  int i, j;
    
  fprintf(stderr, "\nTesting Dictionary.\n");
  for(i=0; i<MAX; i++)
    {
      proplist_t dict, dict2;
      int n = 0;
      
      fprintf(stderr, "  Allocating (%d of %d)", i, MAX);
      
      if(i%2)
	{
	  proplist_t key, value;
	  if((i/2)%2)
	    {
	      char k[100], v[100];
	      
	      fprintf(stderr, " with initial values (String)\n");
	      sprintf(k, "Key%d", i);
	      sprintf(v, "Value%d", i);
	      key = PLMakeString(k);
	      value = PLMakeString(v);
	    }
	  else
	    {
	      fprintf(stderr, " with initial values (Data)\n");
	      key = PLMakeData((void *)&n,sizeof(int));
	      value = PLMakeData((void *)&n, sizeof(int));
	    }
	  dict = PLMakeDictionaryFromEntries(key, value, NULL);
	  PLRelease(key); PLRelease(value);
	}
      else
	{
	  fprintf(stderr, " empty\n");
	  dict = PLMakeDictionaryFromEntries(NULL, NULL);
	}
      
      for(j=1; j<1000; j++)
	{
	  proplist_t key, value;
	  
	  if((i/2)%2)
	    {
	      char k[100], v[100];
	      
	      sprintf(k, "Key%d", j);
	      sprintf(v, "Value%d", j);
	      key = PLMakeString(k);
	      value = PLMakeString(v);
	    }
	  else
	    {	      
	      key = PLMakeData((void *)&j,sizeof(int));
	      value = PLMakeData((void *)&j, sizeof(int));
	    }
	  PLInsertDictionaryEntry(dict, key, value);
	  PLRelease(key); PLRelease(value);
	}
      
      dict2 = PLDeepCopy(dict);	  
      
      if(describe)
	{
	  descr = PLGetDescription(dict2);
	  printf("**Dictionary with %d entries\n",
		 PLGetNumberOfElements(dict2));	  
	  printf("**Dictionary contains: %s\n", descr);
	  free(descr);
	}
      
      fprintf(stderr, "  Releasing.\n");
      PLRelease(dict);
      PLRelease(dict2);
    }
}


void FileTest(BOOL describe)
{
  char *descr;
  int i;
  
  fprintf(stderr, "\nTesting File Handling\n");
  
  for(i=0; i<MAX; i++)
    {
      proplist_t pl;
      
      fprintf(stderr, "  Allocating (%d of %d)\n", i, MAX);
      
      pl = PLGetProplistWithPath("example.proplist");
      if(describe)
	{
	  descr = PLGetDescription(pl);
	  printf("**File contains: %s\n", descr);
	  free(descr);
	}
      
      fprintf(stderr, "  Releasing.\n");
      
      PLRelease(pl);
    }
}

int main(int argc, char **argv)
{
  int i;
  BOOL describe = YES;
  int num = 1;
  int turn = 0;
  BOOL forever = NO;
  BOOL testArray = YES;
  BOOL testDict = YES;
  BOOL testFile = YES;

  i=0;
  while(++i<argc)
    {
      if(!strcmp(argv[i], "--no-descr"))
	describe = NO;
      else if(!strcmp(argv[i], "-n"))
	num = atoi(argv[++i]);
      else if(!strcmp(argv[i], "--forever"))
	forever = YES;
      else if(!strcmp(argv[i], "-na"))
	testArray = NO;
      else if(!strcmp(argv[i], "-nd"))
	testDict = NO;
      else if(!strcmp(argv[i], "-nf"))
	testFile = NO;
      else
	{
	  fprintf(stderr, "Unknown argument %s.\n"
		          "Usage: %s [--no-descr] [-n num | --forever] [-na] [-nd] [-nf]\n"
		          "\t--no-descr:         Do not generate description output to stdout\n"
		          "\t-n num | --forever: Loop n times (default 1) or loop forever\n"
		          "\t-na:                Don't test array handling\n"
		          "\t-nd:                Don't test dictionary handling\n"
		          "\t-nf:                Don't test file handling\n",
		  argv[i], argv[0]);
	  exit(1);
	}
    }
  
  while(turn<num)
    {

      if(!forever && num!=1)
        fprintf(stderr, "\nTurn %d of %d\n", turn, num);

      if(testDict)
	DictTest(describe);

      if(testArray)
	ArrayTest(describe);

      if(testFile)
	FileTest(describe);
	
      
      if(!forever) turn++;
    }
}
  


