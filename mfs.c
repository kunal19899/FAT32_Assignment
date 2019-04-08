/*
    Name: Kunal Samant & Shivangi Vyas 
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
FILE * stat_file;
FILE * ofp;
FILE *putFile;
int i;
char buffer[512];

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

int LBAToOffset(int32_t sector)
{
    return ((sector - 2 ) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) +
          (BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec);

}

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

int16_t NextLB ( uint32_t sector )
{
    uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt ) + ( sector * 4 );
    int16_t val;
    fseek( in, FATAddress, SEEK_SET);
    fread( &val, 2, 1, in);
    return val;
}

int CheckFile(char *token[5])
{
	int flag=1;
	if (token[1] == NULL)
  {
    printf("Must have a second argument>\n");
    flag=1;
  }
        
  char input[12];
  strcpy(input, token[1]);

  char expanded_name[12];
  memset(expanded_name, ' ', 12);

  char *tok = strtok(input, ".");

  strncpy(expanded_name, tok, strlen(tok));
        
  tok = strtok( NULL, "." );

  if (tok)
  {
    strncpy((char*)(expanded_name + 8), tok, strlen(tok));
  }

  expanded_name[11] = '\0';

  for (i = 0; i < 11; i++)
  {
    expanded_name[i] = toupper(expanded_name[i]);
  }
    
  int file;
  for (i = 0; i < 16; i++)
  {
    if (strncmp(expanded_name, dir[i].DIR_Name, 11) == 0)
    {
      if ((dir[i].DIR_Attr != 53) && ((dir[i].DIR_Attr == 32) || (dir[i].DIR_Attr == 16) || (dir[i].DIR_Attr == 1)))
      {
        flag = 0;
        file = i;
        break;
      }	
    }
    file = i;   
  }
  
  if(flag)
	{
    printf("Error: Sorry! File not found\n");
	}
  
  return file;
}

int compare(int k, char *filename)
{
  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok(filename, "." );
  
  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );
  
  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }
  
  expanded_name[11] = '\0';

  int j;
  for( j = 0; j < 11; j++ )
  {
    expanded_name[j] = toupper( expanded_name[j] );
  }

  if( strncmp( expanded_name, dir[k].DIR_Name, 11 ) == 0 )
  {
    return 1;
  }
  else{
    return 0;
  }  
}

int compare_put(char *filename, char *buffer)
{
  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok(filename, "." );
  
  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );
  
  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }
  
  expanded_name[11] = '\0';

  int j;
  for( j = 0; j < 11; j++ )
  {
    expanded_name[j] = toupper( expanded_name[j] );
  }

  char buff[12];
  strcpy(buff, buffer);
  buff[11] = '\0';

  if( strncmp( expanded_name, buffer, 11 ) != 0 )
  {
    return 1;
  }
  else{
    return 0;
  } 
}

void get(char *filename)
{
  int found = 0;
  for (i = 0; i < 16; i++)
  {
    if(compare(i, filename))
    {
      found = i+1;
      break;
    }
  }

  if (found == 0)
  {
    printf("Error: File not found.\n");
    return;
  }

  int cluster = dir[i].DIR_FirstClusterLow;
  int size = dir[i].DIR_FileSize;
  int offset = LBAToOffset(cluster);

  fseek(in, offset, SEEK_SET);
  ofp = fopen(filename, "w");
  char buffer[512];
  fread(&buffer[0], 512, 1, in);
  fwrite(&buffer[0], 512, 1, ofp);

  size = size - 512;

  while (size > 0)
  {
    cluster = NextLB(cluster);
    int addr = LBAToOffset(cluster);
    fseek(in, addr, SEEK_SET);
    fread(&buffer[0], size, 1, in);
    fwrite(&buffer[0], size, 1, ofp);
  }

  fclose(ofp);
}

void put(char *filename)
{
  int found = 0;
  for (i = 0; i < 16; i++)
  {
    if(compare_put(dir[i].DIR_Name, buffer))
    {
      found = i+1;
      break;
    }
    
  }
  if (found == 0)
  {
    printf("Error: File not found.\n");
    return;
  }
  int cluster = dir[i].DIR_FirstClusterLow;
  int size = dir[i].DIR_FileSize;
  int offset = LBAToOffset(cluster);
  fseek(ofp, offset, SEEK_SET);
  in = fopen(filename, "w");
  //char buffer[512];
  fread(&buffer[0], 512, 1, in);
  fwrite(&buffer[0], 512, 1, ofp);
  size = size - 512;
  while (size > 0)
  {
    cluster = NextLB(cluster);
    int addr = LBAToOffset(cluster);
    fseek(ofp, addr, SEEK_SET);
    fread(&buffer[0], size, 1, in);
    fwrite(&buffer[0], size, 1, ofp);
  }

  fclose(ofp);
}

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

    if (strcmp(cmd_str, "\n") == 0) {
      continue;
    }

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

    if(closed)
    {
      if (strcmp(token[0], "open") == 0)
      {
        if (token[1] == NULL)
        {
          printf("<Open must have a second argument>\n");
          continue;
        }
        
        in = fopen(token[1], "rb");
        closed = FALSE;

        fseek(in, 11, SEEK_SET);
        fread(&BPB_BytsPerSec, 2, 1, in);
    
        fseek(in, 13, SEEK_SET);
        fread(&BPB_SecPerClus, 2, 1, in);
    
        fseek(in, 14, SEEK_SET);
        fread(&BPB_RsvdSecCnt, 2, 1, in);
    
        fseek(in, 16, SEEK_SET);
        fread(&BPB_NumFATs, 2, 1, in);
    
        fseek(in, 36, SEEK_SET);
        fread(&BPB_FATSz32, 2, 1, in);

        fseek(in, ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +(BPB_RsvdSecCnt * BPB_BytsPerSec)), SEEK_SET);
        for (i = 0; i < 16; i++)
        {
          fread(&dir[i], 32, 1, in);
        }
        
        if (in == NULL)
        {
          printf("Error: File system image not found!\n");
          continue;
        }
        continue;
      }
      else{
        printf("OPEN A FILE FIRST!!\n");
        continue;
      }
    }

    if(!closed)
    {
      if (strcmp(token[0], "open") == 0)
      {
        printf("File already opened!!\n");
        continue;
      }

      if (strcmp(token[0], "close") == 0)
      {
        fclose(in);
        closed = TRUE;
        //printf("close");
      }

      if (strcmp(token[0], "info") == 0)
      {
        printf("BPB_BytesPerSec: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_BytsPerSec, BPB_BytsPerSec);
        printf("BPB_SecPerClus: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_SecPerClus, BPB_SecPerClus);
        printf("BPB_RsvdSecCnt: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);
        printf("BPB_NumFATs: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_NumFATs, BPB_NumFATs);
        printf("BPB_FATSz32: \n\tDecimal: %d\tHexadecimal: 0x%04x\n", BPB_FATSz32, BPB_FATSz32);
      }

      if (strcmp(token[0], "stat") == 0)
      {
        if (token[1] == NULL)
        {
          printf("<stat must have a second argument>\n");
          continue;
        }

        int positive = CheckFile(token);
        if(positive < 15)
		    {
			    printf("Attribute: 0x%02x\t\tStarting Cluster Number: %d\n", dir[i].DIR_Attr, dir[i].DIR_FirstClusterLow);
		    }
	    }

      if (strcmp(token[0], "ls") == 0)
      {
        //printf("%d", ((BPB_NumFATs * BPB_FATSz32 * BPB_BytsPerSec) +(BPB_RsvdSecCnt * BPB_BytsPerSec)));
        for (i = 0; i < 16; i++)
        {
          char string[12];
          strcpy(string, dir[i].DIR_Name);
          string[11] = '\0';
          if ((dir[i].DIR_Attr != 53) && ((dir[i].DIR_Attr == 32) || (dir[i].DIR_Attr == 16) || (dir[i].DIR_Attr == 1)))
          {
            printf("%s\n", string);
          }
        }
      }

      if (strcmp(token[0], "cd") == 0)
      {
        if (token[1] == NULL)
        {
          printf("<""cd"" must have a second argument>\n");
          continue;
        }

        char *direc = strtok(token[1], "/");

        while(direc != NULL)
        {
          //printf("%s\n", direc);
          chdir(direc);
          direc = strtok(NULL, "/");
        }
        
      }

      if (strcmp(token[0], "get") == 0)
      {
        get(token[1]);
      }

      if (strcmp(token[0], "put") == 0)
      {
        put(token[1]);
      }

      
      /*if (strcmp(token[0], "read") == 0)
      {
        int index = 0 ;
        char name[12];
        memset( name, 0, 12);
        strncpy( name, "           ", 11);
        char* tok = strtok(token[1], ".");
        int a = strlen(tok);
        strncpy(name, token[1], a);
        tok = strtok(NULL, ".");
        strncpy(&name[11-a], tok, 3);
        
        int i =0;
        for (i = 0 ; i <= 11 ; i++)
        {
          name[i] = toupper(name[i]);
        }
        //printf("name :%s", name );
  
        for (i = 0 ; i < 11 ; i++)
        { 
          if (strncmp(name, dir[i].DIR_Name, 11) == 0 )
          {
            index = i;
            //printf("index in: %d\n", i);
          }
          
        }
        
        int user_offset = atoi(token[2]);
        //printf("token%d\n", user_offset);
        int block = dir[index].DIR_FirstClusterLow;
        //printf("block %d\n", block);
        
        while (user_offset > BPB_BytsPerSec)
        {
          block = NextLB( block );
          user_offset -= BPB_BytsPerSec;
        }
        // printf("block %d\n", block);
        // printf("user_offset %d\n", user_offset);
        
        //block has the data
        int file_offset = LBAToOFFset(block);
        fseek( in, file_offset + user_offset, SEEK_SET);
        
        uint8_t value;
        int user_count = atoi(token[3]);
      
        for (i= 1; i <= user_count; i++ )
        {
          fread( &value, 1, 1, in);
          printf("Read value : %d \n", value );
        }
        printf("\n");
      }*/
    }

    free(working_root);
  }
  return 0;
}
