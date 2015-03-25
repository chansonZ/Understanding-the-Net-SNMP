/************************************************************
 * Copyright (C)	GPL
 * FileName:		snmpipc.h
 * Author:		张春强
 * Date:			2014-08
 ***********************************************************/

#ifndef SNMPIPC_H
#define SNMPIPC_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */

// 保证该目录已经存在
#define APP_DIR		"/usr/local/etc/app/shm/"
#define SHM_CONF	"shm.conf"
#define SEM_CONF	"sem.conf"

#define SHM_KEY_ID	1
#define SEM_KEY_ID	1

#define SUCCESS 0
#define FAILURE -1
typedef int BOOLEAN;

// 数据类型定义
typedef enum
{
	// 从 0 开始，与数组，信号量匹配
	SHM_PARADATA = 0,                   // 参数数据类型
	SHM_REALDATA,                       // 实时数据类型
	SHM_ALARM,                          // 告警数据类型
	SHM_CTRL,                           // 控制数据类型
} SHM_TYPE;

typedef struct
{
	int iSize;                          //真实结构体大小或通用结构体大小
	void *pShmAddr;
} T_ShareMem;

#define SHM_TYPE_NUM ( SHM_CTRL + 1 )   // 需要保存在共享内存中的数据类型数量

// 信号量数量:每种数据种类一个信号量
#define SEM_NUM SHM_TYPE_NUM

#define TO_SHM		( 0 )
#define FROM_SHM	( 1 )

#define SLAVE	( 0 )
#define MASTER	( 1 )

#define MAX_CHAR_LEN (32)       // 系统中字符串参数最大长度，4的倍数
typedef   union
{
	int		iValue;                 // 统一的整形数据
	char	acValue[MAX_CHAR_LEN];  // 统一的字符串类型数据
} U_Value;


typedef struct
{
	int iNo;
	int iOffset;
	int iLen;
} T_MapTable;

/*参数数据: 模拟整形，字符型，简单表一多行单列 */
#define C_ROW_NUM (3)
typedef struct
{
	int		a;                  // 0
	char	b[MAX_CHAR_LEN];    // 1
	int		c[C_ROW_NUM];       // 2,3,4
	// ...
} T_ParaData;
typedef enum
{
	// 从 0 开始,连续
	PARA_A = 0,
	PARA_B ,
	PARA_C1 ,//2,c在结构体中的起始位置序号
	PARA_C2 ,
	PARA_C3,
}E_PARA;

#define  C_OFFSET (PARA_C1)

/* 实时数据,全部显示为整形，实际情况根据其精度进行处理。只读
   SNMP中NMS的显示由对象OID语法控制
   后面的注释数字表示该变量在该数据类型部分的共享内存中的位置——唯一。如实时数据共享位置的起始地址为
   0x1234，那么0x1234+0就表示变量a的地址
 */


/*实时数据:模拟整形，字符型，简单表多行多列 */
#define GROUP_NUM (2)// 示例某个组-表格列元素
struct _aGroup
{
	int x;
	int y;
	// ...
};

/*
序号No.	x y
第一行	0 1
第二行	2 3
z:		4
*/
typedef struct
{
	struct _aGroup xy[GROUP_NUM];
	int z;                          // 4
	// ...
} T_RealData;
typedef enum
{
	// 从 0 开始,连续
	XY0X = 0,
	XY0Y ,
	XY1X ,//2
	XY1Y ,
	REALZ,
}E_REALTIMEDATA;


typedef struct
{
	int alarm1;                       // 0
	//int alarm2;                       // 1
	char alarm2[MAX_CHAR_LEN];
	int  alarmCounter;                 // 2
	// ...
} T_AlarmData;
typedef enum
{
	// 从 0 开始,连续
	ALARM1 = 0,
	ALARM2,
	ALARM_COUNTER,//2
}E_ALARM;


// 为了节省内存，该结构体并没有应用到实时数据中
typedef struct
{
	// 这两个需要手动维护。当然我们可以将由程序的一些前端脚本自动生成
	int		iNo;    // 参数的编号值(此数据在参数表中的索引，即从0开始)
	int		iLen;   // 数据的字节长度，如对于整形为sizeof(int)。该变量有必要吗！！
	U_Value uValue; //参数的值
} T_ShmCellVal;

#ifndef  MIN
#define  MIN( a, b ) ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

// 测试用
int del_sem( void );
int del_shm( void );


int app_get_data(void* pStrVal, int dType);
int app_set_data(void* pStrVal, int dType);
int snmp_get_data( int dType, int no, int ll, void* pV );
int snmp_set_data( int dType, int no, int ll, void* pV );

void init_shm_sem(BOOLEAN isMaster);

	
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  SNMPIPC_H  */

