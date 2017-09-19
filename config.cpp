//****************************************************************************
//  Copyright (c) 2009  Daniel D Miller
//  config.cpp - manage configuration data file
//
//  Written by:  Dan Miller
//****************************************************************************
//  Filename will be same as executable, but will have .ini extensions.
//  Config file will be stored in same location as executable file.
//  Comments will begin with '#'
//****************************************************************************
#include <windows.h>
#include <stdio.h>   //  fopen, etc
#include <stdlib.h>  //  atoi()
#include <limits.h>  //  PATH_MAX

#include "common.h"
#include "serial_iface.h"

//****************************************************************************
static char ini_name[PATH_MAX+1] = "" ;

//****************************************************************************
static void strip_comments(char *bfr)
{
   char *hd = strchr(bfr, '#') ;
   if (hd != 0)
      *hd = 0 ;
}

//****************************************************************************
static LRESULT save_default_ini_file(void)
{
   FILE *fd = fopen(ini_name, "wt") ;
   if (fd == 0) {
      LRESULT result = (LRESULT) GetLastError() ;
      syslog("%s open: %s\n", ini_name, get_system_message(result)) ;
      return result;
   }
   //  save any global vars
   fprintf(fd, "commport=%u\n", uart_comm_num) ;
   fprintf(fd, "baudrate=%u\n", uart_baud_rate) ;
   fprintf(fd, "debug=%u\n", (debug_level) ? 1U : 0U) ;
   fclose(fd) ;
   return ERROR_SUCCESS;
}

//****************************************************************************
LRESULT save_cfg_file(void)
{
   return save_default_ini_file() ;
}

//****************************************************************************
//  - derive ini filename from exe filename
//  - attempt to open file.
//  - if file does not exist, create it, with device_count=0
//    no other data.
//  - if file *does* exist, open/read it, create initial configuration
//****************************************************************************
LRESULT read_config_file(void)
{
   char inpstr[128] ;
   LRESULT result = derive_filename_from_exec(ini_name, (char *) ".ini") ; //lint !e1773
   if (result != 0)
      return result;

   if (debug_level) {
      syslog("ini file: %s\n", ini_name);
   }
   FILE *fd = fopen(ini_name, "rt") ;
   if (fd == 0) {
      return save_default_ini_file() ;
   }

   while (fgets(inpstr, sizeof(inpstr), fd) != 0) {
      strip_comments(inpstr) ;
      strip_newlines(inpstr) ;
      if (strlen(inpstr) == 0)
         continue;

      if (strncmp(inpstr, "commport=", 9) == 0) {
         // syslog("enabling factory mode\n") ;
         uart_comm_num = (uint) strtoul(&inpstr[9], 0, 0) ;
      } else
      if (strncmp(inpstr, "baudrate=", 9) == 0) {
         // syslog("enabling factory mode\n") ;
         uart_baud_rate = (uint) strtoul(&inpstr[9], 0, 0) ;
      } else
      if (strncmp(inpstr, "debug=", 6) == 0) {
         // syslog("enabling factory mode\n") ;
         debug_level = (uint) strtoul(&inpstr[6], 0, 0) ;
      } else
      {
         //  don't print errors for unknown keys.
         //  This aids compatibility between applications
         // syslog("unknown: [%s]\n", inpstr) ;
      }
   }
   return 0;
}

