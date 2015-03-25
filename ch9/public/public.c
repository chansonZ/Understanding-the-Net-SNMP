

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

// 作为公共函数编译到snmpd中
#include "public.h"

/*******************************************************
初始化简单表
********************************************************/
T_TableSimple  *initlize_simpleTableRow(const T_SNMPMapTable *tachID,int index)
{
        T_TableSimple *tmpnode = NULL;
        if(NULL == tachID) 	return NULL;
		
        tmpnode = SNMP_MALLOC_TYPEDEF(T_TableSimple);
        
        if (tmpnode)
       {
            tmpnode->node.t_tacheID.ipcNo = tachID->ipcNo+index;                     
            tmpnode->node.t_tacheID.snmpmagic = tachID->snmpmagic;
            tmpnode->next =NULL;
       }

       return tmpnode;                                                                       
}


/*******************************************************
index1:行数
maxCol:列数
index2: 暂时没用，只为了和双整数索引统一
当一个模块中包含多个表格时，此类通用的函数可统一提取到
公共模块中去
********************************************************/
void   init_singleIndexTable(T_TableIndex1 **tablehead, 
							 const T_SNMPMapTable *tachID, int maxCol,
							 int index1,int index2)							 
{
	DEBUGMSG(("realtimedata","--init_singleIndexTable:maxCol = %d,index1= %d\n",maxCol,index1));
	T_TableIndex1 *pTable_temp1=NULL;
	T_TableIndex1 *pTable_temp2=NULL;
	T_TableSimple  *row_node=NULL;
	T_TableSimple  *tmpnode = NULL;
	int         col = 0;
	int         dex = 0;
	
	if( (index1<=0) )	  return;

	for(dex = 0;dex < index1; dex++) //行初始化
	{
		pTable_temp1 = SNMP_MALLOC_TYPEDEF(T_TableIndex1);
		if (!pTable_temp1)	   return;
        
		pTable_temp1->list_node = NULL;
		pTable_temp1->next    = NULL;
		pTable_temp1->index  = dex+1;
		
		/* 列初始化 */
		for(col = 0; col < maxCol; col++)
		{
			/*return NULL may be no effect*/
			tmpnode 
			  =  initlize_simpleTableRow(&tachID[col], dex*maxCol);
			if( tmpnode  == NULL)
				continue;
			
			if( pTable_temp1->list_node == NULL)
			{
				row_node =  pTable_temp1->list_node = tmpnode; 
			}
			else
			{
				row_node->next = tmpnode;
				row_node = tmpnode;  
			}
			
		}
		
		/* row list increasing */
		if( *tablehead == NULL)
		{
			pTable_temp2 =  *tablehead = pTable_temp1; 
		}
		else
		{
			pTable_temp2->next = pTable_temp1;
			pTable_temp2 = pTable_temp1;     
		}
	}
}

/*******************************************************
* 行中查找列节点
********************************************************/	
MIBIDSTRUCT  *findTableNode(T_TableSimple *theRowHead,
                          const unsigned int magic, int maxCol)
{
		
	int i;
    T_TableSimple  *pnd = theRowHead;
       
    DEBUGMSG(("realtimedata", "findTableNode: magic =%d\n ",magic));
       
	for(i = 0; pnd != NULL &&  i < maxCol; pnd = pnd->next, i++)
	{	   
		if( (pnd->node.t_tacheID.snmpmagic == magic) )
			return &( pnd->node);
	}
	return NULL;
}



