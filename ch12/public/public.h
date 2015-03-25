#ifndef PUBLIC_H
#define PUBLIC_H

#include "snmpipc.h" //将该文件放入到标准目录中

typedef  struct{
	int ipcNo;   // ipc 中的序号
	int snmpmagic; // snmp magic 
}T_SNMPMapTable;


typedef struct{
		T_SNMPMapTable t_tacheID; 
		int asnType;
		// ....
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
*initlize_simpleTableRow(const MIBIDSTRUCT *ptachID,int index);

void   
init_singleIndexTable(T_TableIndex1 **tablehead, 
						const MIBIDSTRUCT *ptachID, int maxCol,
						int index1,int index2);

MIBIDSTRUCT *
findTableNode(T_TableSimple *theRowHead,
                    const unsigned int magic, int maxCol);


MIBIDSTRUCT  *
get_scalar_object(MIBIDSTRUCT scalarBuf[],
                            int  scalarNum,int  magic);

 MIBIDSTRUCT  *
get_scalar_node( const oid *name,
                 int namelen, int fatherNodeLen,
                 struct variable4 *pVariables, 
                 MIBIDSTRUCT scalarBuf[],int scalarNum);

int 
find_magic(const oid *name,
                     int namelen, int fatherNodeLen,
                     struct variable4 *pVariables);

int 
snmp_write_action(int action, u_char snmpType,
                      int var_val_len, u_char * var_val,
                      const MIBIDSTRUCT  *writeNode,int dType);
int 
check_type_lenth(int inType, int defindType,
					 int var_val_len, int stringLen);

 MIBIDSTRUCT  *
 get_table_node(T_TableSimple *ptr,int realColNum,
                           int  rowIndex, const int magic);

  T_TableSimple  *
  find_table_row(T_TableSimple *ptr , int  rowNum, int maxCol);

  void 
  init_nonindex_table( T_TableSimple **tablehead,  const MIBIDSTRUCT tachID[],
                          int maxCol,  int rownums);

  int get_data_length(MIBIDSTRUCT *pN,void*pV);

  int set_data(const MIBIDSTRUCT  *writeNode,int dType,int ll,void* pV);

#endif
