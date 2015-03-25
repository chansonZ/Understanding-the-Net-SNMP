/************************************************************
 * Copyright (C)	GPL
 * FileName:		tools.c
 * Author:		张春强
 * Date:			2014-07
 ***********************************************************/
// gcc -Wall tools.c -o tools

#include "tools.h"
#include "defines.h"


/***********************************************************
* Description:    锁文件
***********************************************************/
int lock_file( int iLkFd )
{
	if( iLkFd < 0 )
	{
		return FAILURE;
	}

	//设置文件描述符close-on-exec标志
	fcntl( iLkFd, F_SETFD, FD_CLOEXEC );

	// 加锁：LOCK_EX
	if( 0 > flock( iLkFd, LOCK_EX | LOCK_NB ) )
	{
		printf( "lock file failed !\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:    解锁文件
***********************************************************/
int unlock_file( int iLkFd )
{
	if( iLkFd < 0 )
	{
		return FAILURE;
	}

	// 解锁：LOCK_UN
	if( 0 > flock( iLkFd, LOCK_UN | LOCK_NB ) )
	{
		printf( "unlock file failed !\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:   字符串 删除两端的空格
***********************************************************/
char *trim_ends_space( char *pSrc )
{
	char	*pStr		= pSrc;
	char	*pStartPos	= NULL;                         // 保存非空格的起始位置
	int		len = 0;

	if( NULL != pSrc )
	{
		while( *pStr != 0 && isspace( (unsigned char)*pStr ) ) 
		{
			pStr++;                                     // 跳过前导空格
		}
		pStartPos	= pStr;                             // 保存非空格的起始位置
		len = strlen( pSrc ) - 1;
		pStr		= pSrc + len;    	// pStr 指向串结束符前的一个字符
		while( isspace( *pStr ) && (len-- > 0 ))
		{
			pStr--;                                     // 跳过后缀空格
		}
		*( pStr + 1 ) = '\0';                           // 在空格位填充结束符
	}
	return pStartPos;
}


 /***********************************************************
* Description:   解析分隔符左右两边的字符串:
* 左边字符串由return 返回;
* 右边字符串由saveptr传出
***********************************************************/
static char * strtok_r_Ex( char *str, const char *delim, char **saveptr )
{
	char	*pLeft	= NULL;
	char	*pRight = NULL;
	char	*ptemp	= NULL;

	if( NULL == str
	    || NULL == delim
	    )
	{
		return NULL;
	}

	// 处理分隔符在第一个字符的特殊情况
	ptemp = trim_ends_space( str );
	if( 1 == strspn( ptemp, delim ) ) // delim == ptemp[0]
	{
		*saveptr = trim_ends_space( ptemp + 1 );
		printf( "delim in the first positon:%s\n\n", *saveptr );
		return NULL;
	}

	pLeft = strtok_r( str, delim, &pRight );
	if( NULL != pLeft )
	{
		pLeft = trim_ends_space( pLeft );
	}
	if( NULL != pRight )
	{
		pRight		= trim_ends_space( pRight );
		*saveptr	= pRight;
	}

	return pLeft;
}


 /***********************************************************
* Description:   delim:为分隔的字符串的符号
* 输出：pLeft为分隔符左边的字符串，
* pRight为右边的字符串，这些字符串都已经去除收尾空格
***********************************************************/
int parser_delim( const char *pIn, char *pDelim, char *pLeft, char *pRight )
{
	char	*pStr = NULL;
	char	*pToken		= NULL;
	char	*pSubToken	= NULL;
	int		ii			= 0;

	if( NULL == pIn
	    || NULL == pDelim
	    || NULL == pLeft
	    || NULL == pRight
	    )
	{
		return FAILURE;
	}

	ii = strlen( pIn ) + 1;

	// 复制待处理的字符串
	if( NULL == ( pStr = (char*)malloc( ii ) ) )
	{
		return FAILURE;
	}

	bzero( pStr, ii );
	memcpy( pStr, pIn, ii );

	// 存在pDelim 字符时才进行分隔
	if( NULL != strstr( pStr, pDelim ) )
	{
		pToken = strtok_r_Ex( pStr, pDelim, &pSubToken );
		if( NULL != pToken )
		{
			strcpy( pLeft, pToken );
		}

		//printf("pSubToken:%s,",pSubToken);
		if( NULL != pSubToken )
		{
			strcpy( pRight, pSubToken );
		}
		//printf("pRight:%s\n\n",pRight);
	}else
	{
		printf( "parser_delim: delimiter '%s' not found !!\n", pDelim );
		free( pStr );
		return FAILURE;
	}

	free( pStr );
	return SUCCESS;
}

/***********************************************************
* Description:    返回字符串中的IP，
* 如192.168.43.146:161 中的192.168.43.146
***********************************************************/
char*  get_token_ip( const char* pval, char *ip ) // 
{
	char pRight[128] = { 0 };

	if( NULL != pval && NULL != ip )
	{
		if( NULL != strstr( pval, COLON ) ) //存在
		{
			parser_delim( pval, COLON, ip, pRight );
		} else
		{
			strcpy( ip, pval );
		}
	}
	return ip;
}

