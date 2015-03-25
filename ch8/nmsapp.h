/************************************************************
 * Copyright (C)	GPL
 * FileName:		nmsapp.h
 * Author:		张春强
 * Date:			2014-07
 ***********************************************************/
#ifndef NMSAPP_H
#define NMSAPP_H

#ifdef __cplusplus
extern "C" {
#endif   /*__cplusplus */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include "list.h"

#define NMSAPP_LOCKFILE "/var/run/nmsapp.pid"
#define LOCK_MODE		( S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )
#define PEER_FILE		"/usr/local/etc/nmsapp/nmshosts.conf"
#define DEFAULT_OIDS_FILE		"/usr/local/etc/nmsapp/mibs/general/monitor.oids"

#define NMS_SCAN_FREQUENCY (1)  

#ifndef MAX_OID_LEN
//#define MAX_OID_LEN       (128) //最大子OID的数量
#endif

#define  SNMP_MSG_FF ( 0xFF )

#define USER			"user"
#define SECURITYLEVEL	"securityLevel"
#define AUTH_PASSPHRASE "auth_passphrase"
#define PRIV_PASSPHRASE "priv_passphrase"
#define FREQUENCY 		 "frequency"

//定义解析token的函数指针
//typedef int (*FP_DELIME_PARSER)( char *pIn, char *pLeft, char *pRight );


typedef struct counter
{	
	int counter;             // 当前计数值(秒)
	int frequence;         // 周期
} T_Counter;


typedef struct nmsoid
{
	oid oid[MAX_OID_LEN];   //MAX_OID_LEN = 128  // 1 3 6 1 2 1 1 1 0
	int oidLen;             // read_oids 后存储了实际的oid 长度
	int cmdType;            // 该OID 对应的命令类型
} T_NMSOids;

typedef struct snmpv3User
{
	char	user[32];       // 用户名：MD5_DES_User2
	int		secLevel;       // 安全级别
	char	authPass[64];
	char	privPass[64];
} T_SNMPv3User;

// 使用系统默认的UDP协议等
typedef struct
{
	char				hostName[32];   // ip
	T_SNMPv3User		snmpv3User;
	struct snmp_session *pSess;         // SNMP 会话信息:每个主机对应一个会话信息,
	T_List				*pList;         // 链表内容为 T_NMSOids
	// 则使用回调函数时，下一个OID节点时：ptCurrentOid = ptCurrentOid->next;
	T_ListNode *ptCurrentOid;           // 指向 T_NMSOids ，表示当前发送的OID
	T_Counter  tCounter;
} T_NMSHost;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /*  NMSAPP_H  */
