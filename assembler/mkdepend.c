/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "stringlists.h"

#define ObjExtension ".o"

static char *getobj(const char *pSrc)
{
  static char buffer[255];
  int l = strlen(pSrc), bm5 = sizeof(buffer) - 5;
  char *pSearch;
  
  if (l < bm5)
    l = bm5;
  memcpy(buffer, pSrc, l); buffer[l] = '\0';

  pSearch = strrchr(buffer, '.');
  if (pSearch)
    strcpy(pSearch, ObjExtension);

  return buffer;
}

int MayRecurse(const char *pFileName)
{
  int l = strlen(pFileName);

  /* .rsc files are autogenerated and do not contain any
     include statements - no need to scan them */

  if ((l > 4) && (!strcmp(pFileName + l - 4, ".rsc")))
    return 0;

  return 1;
}

static void ParseFile(const char *pFileName, const char *pParentFileName, StringRecPtr *pFileList)
{
  FILE *pFile;
  int l;
  char line[512], *pCmd, *pName;
  String Str;

  pFile = fopen(pFileName, "r");
  if (!pFile)
  {
    if (pParentFileName)
      fprintf(stderr, "%s: ", pParentFileName);
    perror(pFileName);
    return;
  }

  while (!feof(pFile))
  {
    if (!fgets(line, sizeof(line), pFile))
      break;
    l = strlen(line);
    if ((l > 0) && (line[l - 1] == '\n'))
      line[l - 1] = '\0';

    if (*line != '#')
      continue;
    pCmd = strtok(line + 1, " \t");
    if (strcmp(pCmd, "include"))
      continue;

    pName = strtok(NULL, " \t");
    if (!pName)
      continue;
    l = strlen(pName);
    if ((*pName != '"') || (pName[l - 1] != '"'))
      continue;

    if (l - 1 < (int)sizeof(Str))
    {
      memcpy(Str, pName + 1, l -2);
      Str[l - 2] = '\0';
      if (!StringListPresent(*pFileList, Str))
      {
        AddStringListLast(pFileList, Str);
        if (MayRecurse(Str))
          ParseFile(Str, pFileName, pFileList);
      }
    }
  }
  fclose(pFile);
}

int main(int argc, char **argv)
{
  int z;
  FILE *pDestFile;
  char *pDestFileName = NULL, *pIncFileName;
  char used[1024];
  StringRecPtr FileList;

  if (argc < 2)
  {
    fprintf(stderr, "usage: %s [args] <file(s)>\n", *argv);
    exit(1);
  }

  memset(used, 0, sizeof(used));

  for (z = 1; z < argc; z++)
    if ((!used[z]) && (*argv[z] == '-'))
    {
      used[z] = 1;
      if (!strcmp(argv[z] + 1, "o"))
      {
        if (z >= argc - 1)
          pDestFileName = NULL;
        else
        {
          pDestFileName = argv[z + 1];
          used[z + 1] = 1;
        }
      }
    }

  if (pDestFileName)
  {
    pDestFile = fopen(pDestFileName, "w");
    if (!pDestFile)
    {
      perror(pDestFileName);
      exit(errno);
    }
  }
  else
    pDestFile = stdout;

  fprintf(pDestFile, "# auto-generated by %s - do not edit\n\n", *argv);

  InitStringList(&FileList);
  for (z = 1; z < argc; z++)
  {
    if (used[z])
      continue;

    ClearStringList(&FileList);
    ParseFile(argv[z], NULL, &FileList);

    if (!StringListEmpty(FileList))
    {
      fprintf(pDestFile, "%s:", getobj(argv[z]));
      while (True)
      {
        pIncFileName = GetAndCutStringList(&FileList);
        if (*pIncFileName)
          fprintf(pDestFile, " %s", pIncFileName);
        else
          break;
      }
      fprintf(pDestFile, "\n\n");
    }
  }

  if (pDestFileName)
  {
    fprintf(pDestFile, "%s:", pDestFileName);
    for (z = 1; z < argc; z++)
      if (!used[z])
        fprintf(pDestFile, " %s", argv[z]);
    fprintf(pDestFile, "\n");
    fclose(pDestFile);
  }

  return 0;
}
