/************************************************************
 * Copyright (C)	GPL
 * FileName:		defines.h
 * Author:		张春强
 * Date:			2014-07
 ***********************************************************/
#ifndef DEFINES_H
#define DEFINES_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */


/* 分配使用了typedef声明的结构体内存空间,
   清零，返回结构体指针*/
#define MALLOC_TYPEDEF_STRUCT( td ) (td*)calloc( 1, sizeof( td ) )


/* 分配未使用了typedef声明的结构体内存空间,
   清零，返回结构体指针*/
#define MALLOC_STRUCT( s ) (struct s *)calloc( 1, sizeof( struct s ) )

// 释放非空的指针的内存空间，同时置NULL
#define NMSAPP_FREE( s )	do { if( s ){ free( (void*)s ); s = NULL; } } while( 0 )
#define MIN( a, b )			( ( a ) > ( b ) ? ( b ) : ( a ) )

#define TRUE	1
#define FALSE	0
#define SUCCESS 0
#define FAILURE -1

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  DEFINES_H  */
