/************************************************************
 * Copyright (C)	GPL
 * FileName:		tools.h
 * Author:		’≈¥∫«ø
 * Date:			2014-07
 ***********************************************************/
#ifndef TOOLS_H
#define TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */


#include <sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/stat.h>   //fedora ÷–
#include <ctype.h>      //isspace

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

#define  COLON					":"
#define  DOT					"."
#define  LEFT_SQUARE_BRACKET	"["
#define  RIGHT_SQUARE_BRACKET	"]"
#define  EQUAL_SIGN				"="

int lock_file( int iLkFd );


int unlock_file( int iLkFd );


int parser_delim( const char *pIn, char *pDelim, char *pLeft, char *pRight );


char* get_token_ip( const char* pval, char *ip );


char *trim_ends_space( char *pSrc );


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  TOOLS_H  */
