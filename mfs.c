/*

    Name: Kunal Samant
    ID: 1001534662

*/

// The MIT License (MIT)
//  ;[l]
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

#define TRUE  1
#define FALSE 0         

int closed = TRUE;
FILE * in;

char BS_OEMName[8];
int16_t BPB_BytsPerSec; //////used
int8_t BPB_SecPerClus; //used
int16_t BPB_RsvdSecCnt; //used
int8_t BPB_NumFATs; //usef
int16_t BPB_RootEntCnt;
char BS_VolLab[11];
int32_t BPB_FATSz32; //used
int32_t BPB_RootClus;

int32_t RootDirSectors = 0;
int32_t FirstDataSectors = 0;
int32_t FirstSectorofCluster = 0;

struct __attribute__((__packed__)) DirectoryEntry {
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

int main()
{

  system("clear");
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while(!fgets (cmd_str, MAX_COMMAND_SIZE, stdin));

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality

    /*int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }*/


    if (strcmp(token[0], "open") == 0)
    {
      if (token[1] == NULL)
      {
        printf("<Open must have a second argument>\n");
        continue;
      }

      if (!closed)
      {
        printf("%s already opened!!\n", token[1]);
        continue;
      }
      

      in = fopen(token[1], "rb");
      closed = FALSE;
      //printf("open");
      if (in == NULL)
      {
        printf("Error: File system image not found!\n");
        continue;
      }
      continue;
    }

    if (strcmp(token[0], "close") == 0)
    {
      if (closed)
      {
        printf("Open a file first!!!\n");
      }
      if (closed == FALSE)
      {
        fclose(in);
        closed = TRUE;
        //printf("close");
      }
      continue;
    }

    if (strcmp(token[0], "info") == 0)
    {
      if (closed)
      {
        printf("Open a file first!!!\n");
        continue;
      }
      fseek(in, 11, SEEK_SET);
      fread(&BPB_BytsPerSec, 2, 1, in);
      printf("BPB_BytesPerSec: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_BytsPerSec, BPB_BytsPerSec);

      fseek(in, 13, SEEK_SET);
      fread(&BPB_SecPerClus, 2, 1, in);
      printf("BPB_SecPerClus: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_SecPerClus, BPB_SecPerClus);

      fseek(in, 14, SEEK_SET);
      fread(&BPB_RsvdSecCnt, 2, 1, in);
      printf("BPB_RsvdSecCnt: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);

      fseek(in, 16, SEEK_SET);
      fread(&BPB_NumFATs, 2, 1, in);
      printf("BPB_NumFATs: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_NumFATs, BPB_NumFATs);

      fseek(in, 36, SEEK_SET);
      fread(&BPB_FATSz32, 2, 1, in);
      printf("BPB_FATSz32: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_FATSz32, BPB_FATSz32);
      continue;
    }

    if (strcmp(token[0], "stat") == 0)
    {
      if (closed)
      {
        printf("Open a file first!!!\n");
        continue;
      }

      if (token[1] == NULL)
      {
        printf("<Open must have a second argument>\n");
        continue;
      }

      
    }

    free(working_root);
  }
  return 0;
}
