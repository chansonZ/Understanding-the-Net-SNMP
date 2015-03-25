#ifndef PUBLIC_H
#define PUBLIC_H

#include "snmpipc.h" //将该文件放入到标准目录中

typedef  struct{
	int ipcNo;   // ipc 中的序号
	int snmpmagic; // snmp magic 
}T_SNMPMapTable;


typedef struct{
      u_char 	    utype;    		
	T_SNMPMapTable t_tacheID; 
	
}MIBIDSTRUCT;

typedef struct  tableRows
{
	MIBIDSTRUCT      node;
	struct tableRows *next;
	
}T_TableSimple;

// 通用表-单索引表
typedef struct tableIndex1
{
	int 		        index;	      
    T_TableSimple       *list_node;
	
    struct tableIndex1	*next;
	
}T_TableIndex1;


T_TableSimple  
*initlize_simpleTableRow(const T_SNMPMapTable *tachID,int index);

void   
init_singleIndexTable(T_TableIndex1 **tablehead, 
						const T_SNMPMapTable *tachID, int maxCol,
						int index1,int index2);

MIBIDSTRUCT  
*findTableNode(T_TableSimple *theRowHead,
                    const unsigned int magic, int maxCol);


#endif
