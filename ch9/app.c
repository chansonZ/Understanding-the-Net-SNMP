/************************************************************
 * Copyright (C)	GPL
 * FileName:		app.c
 * Author:		张春强
 * Date:			2014-08
 ***********************************************************/

//主业务进程
// gcc -g -Wall app.c snmpipc.c -o app
// gcc -g -Wall app.c -lsnmpipc -o app
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include "snmpipc.h"

static T_RealData	s_tRealData;
static T_ParaData	s_tParaData = {
	.a	= 99,
	.c	= { 88,	77,	 66 },
	.b	= "192.168.43.132",
};
static T_AlarmData	s_tAlarmData ={
	.alarm1 = 1,
	.alarm2 = "This is An alarm",
	.alarmCounter = 0,
};


int	REAL_DATA_SIZE = sizeof( T_RealData ) / sizeof( int );
int ALARM_DATA_SIZE = sizeof( T_AlarmData ) / sizeof( int );


/***********************************************************
* Description:    打印实时数据
***********************************************************/
void print_int_data(int *pIntV,int nn)

{
	int i;
	int *pInt = (int*)pIntV;
	if( NULL == pInt || nn <= 0) return ;

	printf( "--int data: " );
	for( i = 0; i < nn; i++ )
	{
		printf( " %d, ", *pInt );
		pInt++;
	}
	printf( "\n\n" );
}

/***********************************************************
* Description:    更新实时数据
***********************************************************/
void set_int_data(int *pIntV,int nn )
{
	int i;
	int *pInt = (int*)pIntV;
	if( NULL == pInt || nn <= 0) return ;

	for( i = 0; i < nn; i++ )
	{
        if( nn == ALARM_DATA_SIZE)// only example
		( *pInt ) += i; // 模拟告警数据
		else
		( *pInt ) += 2*i;// 模拟实时数据
		
		pInt++;
	}
	printf( "----------------------\n\n" );
}


/***********************************************************
* Description:    打印数据
***********************************************************/
void print_para_data( void )
{
	printf( "--para data is :\n" );
	printf( "%d,", s_tParaData.a );
	printf( "%d,%d,%d,", s_tParaData.c[0], s_tParaData.c[1], s_tParaData.c[2] );
	printf( "%s\n\n", s_tParaData.b );
	//printf( "%d,\n", s_tParaData.g );
}

/***********************************************************
* Description:    打印数据
***********************************************************/
void print_alarm_data( void )
{
	printf( "--alarm data is :\n" );
	printf( "%d,", s_tAlarmData.alarm1);
	printf( "%s,", s_tAlarmData.alarm2);
	printf( "%d\n\n", s_tAlarmData.alarmCounter);
	//printf( "%d,\n", s_tParaData.g );
}

/***********************************************************
* Description:    
 主业务进程
 app sleep loop
 ./app 10 5
***********************************************************/
int main( int argc, char **argv )
{

	int	nn = 0;
	int sec =10; // 周期秒
	int loop = 5;

	// 初始化共享内存
	//init_shm_sem( MASTER );
	init_shm_sem_master();

	if( argc >= 2 )
	{
		sec = atoi( argv[1] );
	}
	if( argc >= 3 )
	{
		loop = atoi( argv[2] );
	}


	//-----模拟实时数据部分------------------
	while( nn < loop )
	{
		/* ------- 模拟实时数据更新 ------- */
		if( nn%3 == 0)
		{
			printf( "----- realtime data -----\n" );
			app_set_data(&s_tRealData,SHM_REALDATA);
			print_int_data( (int*)&s_tRealData,REAL_DATA_SIZE);
			sleep( sec );                 // 给SNMP获取数据留时间
		}

		// 更新数据到共享内存
		if( nn%3 == 0 )
		{
			printf( "----- realtime data -----\n" );
			set_int_data((int*)&s_tRealData,REAL_DATA_SIZE);
			print_int_data( (int*)&s_tRealData,REAL_DATA_SIZE);
			app_set_data( &s_tRealData ,SHM_REALDATA );
			sleep( sec );                 // 给SNMP获取数据留时间
		}


		/* ---------模拟参数部分------------------ */
		if( nn )
		{
			printf( "----- parameter data -----\n" );
			printf( "before set parameter:\n" );
			//app_get_data(&s_tParaData,SHM_PARADATA);
			print_para_data( );
			
			app_set_data( &s_tParaData ,SHM_PARADATA );
			printf( "after set parameter:\n" );
			print_para_data( );
			sleep( 2*sec ); //等待SNMP代理获取数据
		}

		if( nn )
		{
			printf( "before set parameter:\n" );
			app_get_data(&s_tParaData,SHM_PARADATA);
			print_para_data( );
			
			s_tParaData.c[0] = 12345;
			strcpy( s_tParaData.b, "1.1.1.1" );
			app_set_data(&s_tParaData,SHM_PARADATA );
			printf( "after set parameter:\n" );
			print_para_data( );
			sleep( 2*sec );
		}

		if( nn )
		{
			printf( "get parameter:\n" );
			app_get_data(&s_tParaData,SHM_PARAMETER);
			print_para_data( );
		}

		/* ----- 告警部分 ------ */
	    if( nn  )
		{
			printf( "----- alarm data -----\n" );
			//print_int_data( (int*)&s_tAlarmData,ALARM_DATA_SIZE);
			s_tAlarmData.alarmCounter += 1;
			app_set_data( &s_tAlarmData ,SHM_ALARM );
			print_alarm_data();
			sleep( sec );                 // 给SNMP获取数据留时间
			app_get_data( &s_tAlarmData ,SHM_ALARM );
			print_alarm_data();
		}
		nn++;
	}
	
	// 告警部分
	//init_alarm_shm();

	
	sleep( sec ); // 等待代理结束
	del_shm( );
	del_sem( );


	return 0;
}


