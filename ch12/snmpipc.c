/************************************************************
 * Copyright (C)	GPL
 * FileName:		snmpipc.c
 * Author:		张春强
 * Date:			2014-08
 * gcc -g -Wall -fPIC -shared snmpipc.c -o libsnmpipc.so
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stddef.h>
#include <stdlib.h>
#include "snmpipc.h"

int update_shm_data( int dType, 
T_ShmCellVal *pValue, int diretion );

// union semun {
// int val; /* value for SETVAL */
// struct semid_ds *buf; /* buffer for IPC_STAT, IPC_SET */
// unsigned short *array; /* array for GETALL, SETALL */
// struct seminfo *__buf; /* buffer for IPC_INFO */
// };

// 信号量ID
static int s_SemId;
static int s_ShmId;


// 这里手工维护,结构体的映射表
static T_MapTable s_tParaMapTable[] =
{
	{ .iNo = PARA_A, .iOffset = offsetof( T_ParaData, a ),					  .iLen = sizeof( int ) },
	{ .iNo = PARA_B, .iOffset = offsetof( T_ParaData, b ),					  .iLen = MAX_CHAR_LEN	},
	{ .iNo = PARA_C1, .iOffset = offsetof( T_ParaData, c ),					  .iLen = sizeof( int ) },
	{ .iNo = PARA_C2, .iOffset = offsetof( T_ParaData, c ) + sizeof( int ),	  .iLen = sizeof( int ) },
	{ .iNo = PARA_C3, .iOffset = offsetof( T_ParaData, c ) + 2 * sizeof( int ), .iLen = sizeof( int ) },
};
#define PARA_NUM ( sizeof( s_tParaMapTable ) / sizeof( T_MapTable ) )

static T_MapTable s_tRealDtMapTable[] =
{
	{ .iNo = XY0X, .iOffset = offsetof( T_RealData, xy[0].x ),	.iLen = sizeof( int ) },
	{ .iNo = XY0Y, .iOffset = offsetof( T_RealData, xy[0].y ),	 .iLen = sizeof( int ) },
	{ .iNo = XY1X, .iOffset = offsetof( T_RealData, xy[1].x ) , .iLen = sizeof( int ) },
	{ .iNo = XY1Y, .iOffset = offsetof( T_RealData, xy[1].y ) , .iLen = sizeof( int ) },
	{ .iNo = REALZ, .iOffset = offsetof( T_RealData, z ), .iLen = sizeof( int ) },
};
#define REALDATA_NUM ( sizeof( s_tRealDtMapTable ) / sizeof( T_MapTable ) )

static T_MapTable s_tAlarmDtMapTable[] =
{
	{ .iNo = ALARM1, .iOffset = offsetof( T_AlarmData, alarm1 ),	.iLen = sizeof( int ) },
	{ .iNo = ALARM2, .iOffset = offsetof( T_AlarmData, alarm2 ),	 .iLen =  MAX_CHAR_LEN },
	{ .iNo = ALARM_COUNTER, .iOffset = offsetof( T_AlarmData, alarmCounter ) , .iLen = sizeof( int ) },
};
#define ALARMDATA_NUM ( sizeof( s_tAlarmDtMapTable ) / sizeof( T_MapTable ) )


// 数据量大小定义
static int s_acMaxObjNum[SHM_TYPE_NUM] =
{
	PARA_NUM,
	REALDATA_NUM,
	ALARMDATA_NUM,
	0,
};

// 共享内存大小与地址
static T_ShareMem s_atShareMem[] =
{
	{ .iSize = PARA_NUM * sizeof( T_ShmCellVal ), .pShmAddr = NULL },
	{ .iSize = REALDATA_NUM * sizeof( int ),	  .pShmAddr = NULL },
	{ .iSize = ALARMDATA_NUM * sizeof( int ),	  .pShmAddr = NULL },
};

#define SHM_ARRAY_SIZE	sizeof( s_atShareMem ) / sizeof( T_ShareMem )
#define SHM_CELL_SIZE	sizeof( T_ShmCellVal )

static void check_file_exist( char *pName );
static int get_maxobj_num(int dType);
static void update_para_data(T_ParaData* pData, int direction);
static void update_realtime_data(T_RealData* pData, int direction);
/***********************************************************
* Description:    返回dType数据类型中对象的数量
***********************************************************/
static int get_maxobj_num(int dType)
{
	if( ( dType >= SHM_PARADATA) 
			&& (dType < SHM_TYPE_NUM ) 
	   )
		return s_acMaxObjNum[dType];
		
    return FAILURE;
}

/***********************************************************
* Description:    获取dType数据类型MAP内存的起始地址
***********************************************************/
T_MapTable* get_maptable(int dType)
{
	T_MapTable * pMapData =  NULL;
	if( dType == SHM_PARADATA )
	{
		pMapData = s_tParaMapTable;
		
	}else if( dType == SHM_REALDATA )
	{
		pMapData = s_tRealDtMapTable;
	}else if( dType == SHM_ALARM )
	{
		pMapData = s_tAlarmDtMapTable;
	}
	return pMapData;
}

/***********************************************************
* Description:    返回拷贝的长度
***********************************************************/
static int app_memcpy( void* pValInner, void* pShmVal, int iLen, int diretion )
{
	if( NULL == pValInner || NULL == pShmVal || iLen <= 0 )
		return 0;

	if( TO_SHM == diretion )            // 设置到共享内存
	{
		memcpy( pShmVal, pValInner, iLen );
	} else if( FROM_SHM == diretion )   // 从共享内存取
	{
		memcpy( pValInner, pShmVal, iLen );
	} else
	{
		return 0;
	}
	return iLen;
}

/***********************************************************
* Description:    检查文件是否存在，不存在则创建
***********************************************************/
static void check_file_exist( char *pName )
{
	FILE *pF;
	if( access( pName, F_OK ) < 0 )
	{
		pF = fopen( pName, "w" );
		if( pF )
		{
			///随意写个路径到该文件中
			fwrite( pName, strlen( pName ), 1, pF );
			fclose( pF );
		}else
		{
			printf( "check_file_exist: %s  failed!!", pName );
		}
	}
	return;
}

/***********************************************************
* Description:  
   bIsMaster为真创建共享内存，否则获取共享内存的ID；
   作为公共库，该函数只要主业务进程调用一次就OK了，
   传入参数bIsMaster为1，SNMP代理可不用调用该接口；   
***********************************************************/
static int init_share_memory( BOOLEAN bIsMaster ) 
{
	int		i;
	char	acFile[256] = { 0 };
	int		iShmSize	= 0;
	void	*pShmAddr	= NULL;
	int		iShmKey;//, iShmId;

	// 计算共享内存总大小
	for( i = 0; i < SHM_ARRAY_SIZE; i++ )
		iShmSize += iShmSize + s_atShareMem[i].iSize;

	if( iShmSize <= 0 )		return FAILURE;

	// 该文件用于获取共享内存的key
	sprintf( acFile, "%s%s", APP_DIR, SHM_CONF );
	check_file_exist( acFile );
	// 获得key值
	iShmKey = ftok( acFile, SHM_KEY_ID );
	if( iShmKey == (key_t)-1 )
	{
		printf( "get_share_memory:ftok() for shm failed!!\n" );
		return FAILURE;
	}
	printf( "ftok return key = 0x%x\n", iShmKey );

	if( bIsMaster )
	{
		// 创建所有用户可读写的共享内存
		s_ShmId = shmget( iShmKey, iShmSize, 0666 | IPC_CREAT );
		if( -1 == s_ShmId )
		{
			printf( "get_share_memory:shmget() failed!!\n" );
			return FAILURE;
		}
	}else
	{
		// 获取共享内存
		s_ShmId = shmget( iShmKey, iShmSize, 0666 );
		if( -1 == s_ShmId )
		{
			printf( "get_share_memory:shmget() failed!!\n" );
			return FAILURE;
		}
	}

	// 最后一个参数为0 表示可读写
	pShmAddr = shmat( s_ShmId, NULL, 0 );
	if( NULL == pShmAddr )
	{
		printf( "main:shmat() failed!!\n" );
		return FAILURE;
	}

	if( bIsMaster ) //共享内存显示初始化
	{
		memset( pShmAddr, 0x00, iShmSize );
	}

	// 为各类数据分配共享内存起始地址
	for( i = 0; i < SHM_ARRAY_SIZE; i++ )
	{
		s_atShareMem[i].pShmAddr = pShmAddr;
		printf( "shm adress:0x%x\n", (unsigned int )s_atShareMem[i].pShmAddr );
		pShmAddr = (void*)( (int)pShmAddr + s_atShareMem[i].iSize );
	}
	printf( "iShmId=%d \n", s_ShmId );
	return s_ShmId;
}

/***********************************************************
* Description:    获取共享内存地址
***********************************************************/
static void * get_shm_addr( int dtype )
{
	if( 0 <= dtype && dtype < SHM_TYPE_NUM )
	{
		return
		    s_atShareMem[dtype].pShmAddr;
	}

	return NULL;
}

/***********************************************************
* Description: 创建信号量   
bIsMaster 为真创建，否则获取
***********************************************************/
static int create_semaphore( BOOLEAN bIsMaster )
{
	int		i;
	int		iSemKey;
	char	acFile[256] = { 0 };

	// 该文件用于获取共享内存的key
	sprintf( acFile, "%s%s", APP_DIR, SEM_CONF );
	check_file_exist( acFile );
	iSemKey = ftok( acFile, SEM_KEY_ID );

	if( iSemKey == (key_t)-1 )
	{
		printf( "create_semaphore: ftok() for sem failed!!\n" );
		return FAILURE;
	}

	if( bIsMaster )
	{
		// 创建所有用户可读写的信号量
		s_SemId = semget( iSemKey, SEM_NUM, 0666 | IPC_CREAT );
		if( -1 == s_SemId )
		{
			printf( "get_share_memory:shmget() failed!!\n" );
			return FAILURE;
		}
		// 初始化信号量：
		//使用for循环和SETVAL单独设置每个信号量值为1——可获得
		for( i = 0; i < SEM_NUM; i++ )
		{
			if( semctl( s_SemId, i, SETVAL, 1 ) < 0 )
			{
				return FAILURE;
			}
		}
	}else 
	{
		s_SemId = semget( iSemKey, SEM_NUM, 0666 );
	}
	printf( "s_SemId=%d \n", s_SemId );
	return s_SemId;
}

/***********************************************************
* Description:    删除共享内存
***********************************************************/
int  del_shm( )
{
	int rc;
	rc = shmctl( s_ShmId, IPC_RMID, NULL );
	if( FAILURE == rc )
	{
		printf( "del_shm: shmctl() failed!!\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:   对单个信号量V操作，释放资源;
***********************************************************/
static int unlock_sem( int slNo )
{
	struct sembuf tSem;
	tSem.sem_num	= slNo;
	tSem.sem_op		= 1;
	tSem.sem_flg	= SEM_UNDO;
	return
	    semop( s_SemId, &tSem, 1 );
}

/***********************************************************
* Description:   
   对单个信号量P操作，取得资源
   使用SEM_UNDO选项，防止该进程异常退出时可能的死锁；
***********************************************************/
static int lock_sem( int slNo )
{
	struct sembuf tSem;
	tSem.sem_num	= slNo;
	tSem.sem_op		= -1;
	tSem.sem_flg	= SEM_UNDO;
	return
	    semop( s_SemId, &tSem, 1 );
}

/***********************************************************
* Description:   获取信号量的值
***********************************************************/
static int get_sem( int slNo )
{
	return
	    semctl( s_SemId, 0, GETVAL );
}

/***********************************************************
* Description:   删除信号量
***********************************************************/
int del_sem( void )
{
	int rc;
	rc = semctl( s_SemId, 0, IPC_RMID );
	if( rc == -1 )
	{
		//printf("del_sem:semctl() remove id failed!!\n");
		perror( "del_sem:semctl() remove id failed!!\n" );
		return FAILURE;
	}
	return SUCCESS;
}

/***********************************************************
* Description:  	
	SNMP代理使用的数据更新接口；
    更新数据，返回更新的字节数；
    实际上，这些初始化的工作都可以不用代码来实现，
    比如在真实的项目中，我们往往会定义所有需要的监控项。
    定义的内容无非就是各个监控量的属性，
    如数据类型，字节长度，读写权限，数组数量，缺省值等等。
    这份定义文件我们可以称之为系统监控对象的数据字典。
    我们只要解析该数据字典生成一份与代码中结构体一致的二进制的文件，
    每次初始化时只要直接读取该文件的内容到共享内存段，即可完成系统的初始化的工作。 
***********************************************************/
int  updata_cellvalue( int dType, int no, int ll, void* pV, int diretion )
{
	T_ShmCellVal	shmVal;
	int				ret = 0;
	if( no < 0
	    || ll < 0
	    || NULL == pV
	    || ( dType < SHM_PARADATA || dType >= SHM_TYPE_NUM )
	    )
	{
		printf( "init_shm_cellvalue failed!!\n" );
		return 0;
	}

	bzero( (void*)&shmVal, (size_t)sizeof( T_ShmCellVal ) );
	shmVal.iNo	= no;
	shmVal.iLen = ll;

	// 设置或获取
	// 设置时: pV -> shmVal.uValue ,获取时无需操作
	if( TO_SHM == diretion )
	{
		app_memcpy( pV, &shmVal.uValue, ll, diretion );
	}

	ret = update_shm_data( dType, &shmVal, diretion );

	// 获取时: shmVal.uValue -> pv,设置时无需操作
	if( (FROM_SHM == diretion) && (0 < ret) )
	{
		ret = app_memcpy( pV, &shmVal.uValue, ret, diretion );
	}

	return ret;
}


/***********************************************************
* Description:  
	SNMP代理使用的数据更新接口；
	pValue 传入的是T_ShmCellVal 指针,作为输入输出；
	返回拷贝的长度;
***********************************************************/
 int update_shm_data( int dType, T_ShmCellVal *pValue, int diretion )
{
	int			iLen		= 0;
	char		      *pShmAddr	= (char*)get_shm_addr( dType );
	T_MapTable   *pMapTable= get_maptable( dType );
	T_ShmCellVal * pV		= pValue;

	if( NULL == pShmAddr || NULL == pV || NULL == pMapTable)
		return 0;
	if( pV->iNo >= get_maxobj_num(dType) )
		return 0;

	// 取指定位置的单元地址
	pShmAddr += pMapTable[pV->iNo].iOffset;
	iLen	= MIN( pMapTable[pV->iNo].iLen, pV->iLen );
	
	lock_sem( dType );
		app_memcpy( &pV->uValue, pShmAddr, iLen, diretion );
	unlock_sem( dType );

	return iLen;
}


/*
   更新所有数据：
   下面硬编码了只有几个数据的参数结构，
   实际中，一个系统的参数远远不只这些，如果还使用这种方式，肯定不是一个好的解决方案。
   一般情况下，对于多个数据的处理方式最简洁的编码形式是使用循环。
   而使用循环必然要求对所有的数据类型的处理方式都一样。
   依照这种思路，常规的编码方式是定义通用的结构体和增加额外的可供循环统一处理的信息，如唯一标识的方法。
   对于数据的处理较为常规的编码技巧是将对象分为：数值、属性、标识；
   采用这种分层的思想，所有的数据都抽象为统一的处理方法了！
   这里就不详述了，“废话”似乎够多了！
 */

/***********************************************************
* Description:   更新通用结构体与共享内存的数据
***********************************************************/
static void _update_data(void* pData, int dType,int direction)
{
	char *pAddr = NULL;
	int 	 i = 0;
	T_MapTable *pMap = get_maptable( dType );
	if( NULL == pMap || NULL == pData ) return ;
	
	for(i = 0; i < s_acMaxObjNum[dType]; i++ )
	{
	    pAddr = (char *)pData;
		updata_cellvalue( dType,i,pMap[i].iLen, 
			                 pAddr+pMap[i].iOffset, direction);
	}

	return;
}

/***********************************************************
* Description:   参数结构体变量与共享内存数据更新
***********************************************************/
static void update_para_data(T_ParaData* pData, int direction)
{
	return 
		_update_data(pData,SHM_PARADATA,direction);

}

/***********************************************************
* Description:   实时数据结构体变量与共享内存数据更新
***********************************************************/
static void update_realtime_data(T_RealData* pData, int direction)
{
	
	void * pShmVal = get_shm_addr( SHM_REALDATA );
	if( NULL == pShmVal || NULL == pData ) return ;

	lock_sem( SHM_REALDATA );
		app_memcpy( pData, pShmVal, sizeof(T_RealData),direction);
	unlock_sem( SHM_REALDATA );
	
	return ;
}

/***********************************************************
* Description:  告警数据结构体变量与共享内存数据更新
***********************************************************/
static void update_alarm_data(T_AlarmData* pData, int direction)
{
	return 
		_update_data(pData,SHM_ALARM,direction);

}


/***********************************************************
* Description:   业务进程使用: 结构体变量与共享内存数据更新；
pStrVal,为业务进程中结构体变量；
dir,取值为FROM_SHM和TO_SHM,分别表示读写共享内存；
***********************************************************/
int update_data(void* pStrVal, int dType, int dir)
{
	if( NULL == pStrVal ) return FAILURE;
	
	if( SHM_PARADATA == dType)
		update_para_data( pStrVal, dir );
	else if( SHM_REALDATA == dType )
		update_realtime_data(pStrVal, dir);
	else if( SHM_ALARM== dType )
		update_alarm_data(pStrVal, dir);
	else
		return FAILURE;

	return SUCCESS;
}

/***********************************************************
* Description:   业务进程使用: 
从共享内存中读数据更新到结构体
pStrVal,为业务进程中结构体变量指针;
dType:数据类型
***********************************************************/
int app_get_data(void* pStrVal, int dType)
{	
	return 
		update_data(pStrVal, dType,FROM_SHM);

}

/***********************************************************
* Description:   业务进程使用: 
从结构体写到共享内存中
pStrVal,为业务进程中结构体变量指针;
dType:数据类型
***********************************************************/
int app_set_data(void* pStrVal, int dType)
{	
	return 
		update_data(pStrVal, dType,TO_SHM);

}


/***********************************************************
* Description:   snmp进程使用: 
从共享内存中读数据到pV
dType:数据类型
no:序号
ll:字节长度
pV:变量地址
***********************************************************/
int snmp_get_data( int dType, int no, int ll, void* pV )
{
	return
		updata_cellvalue(dType,no,ll,pV,FROM_SHM);

}

/***********************************************************
* Description:   snmp进程使用: 
将pV内容写入到共享内存中
dType:数据类型
no:序号
ll:字节长度
pV:变量地址
***********************************************************/
int snmp_set_data( int dType, int no, int ll, void* pV )
{
	return
		updata_cellvalue(dType,no,ll,pV,TO_SHM);

}


/***********************************************************
* Description:共享内存和信号量初始化
isMaster指示是否为主业务进程，该进程负责创建和初始化
***********************************************************/
void init_shm_sem(BOOLEAN isMaster)
{
	if( FAILURE == init_share_memory( isMaster ) )
	{
		exit (-1);
	}
	if( FAILURE == create_semaphore( isMaster ) )
	{
		exit (-1);
	}
}


