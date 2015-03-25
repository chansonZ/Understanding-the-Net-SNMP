

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

// 作为公共函数编译到snmpd中
#include "public.h"

/*******************************************************
   初始化简单表
 ********************************************************/
T_TableSimple  *initlize_simpleTableRow(const MIBIDSTRUCT *ptachID,int index)
{
	T_TableSimple *tmpnode = NULL;
	if(NULL == ptachID) return NULL;

	tmpnode = SNMP_MALLOC_TYPEDEF(T_TableSimple);

	if (tmpnode )
	{
		tmpnode->node.t_tacheID.ipcNo = ptachID->t_tacheID.ipcNo+index;
		tmpnode->node.t_tacheID.snmpmagic = ptachID->t_tacheID.snmpmagic;
		tmpnode->node.asnType = ptachID->asnType;
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
                             const MIBIDSTRUCT *tachID, int maxCol,
                             int index1,int index2)
{
	DEBUGMSG(("realtimedata","--init_singleIndexTable:maxCol = %d,index1= %d\n",maxCol,index1));
	T_TableIndex1 *pTable_temp1=NULL;
	T_TableIndex1 *pTable_temp2=NULL;
	T_TableSimple  *row_node=NULL;
	T_TableSimple  *tmpnode = NULL;
	int col = 0;
	int dex = 0;

	if( (index1<=0) ) return;

	for(dex = 0; dex < index1; dex++) //行初始化
	{
		pTable_temp1 = SNMP_MALLOC_TYPEDEF(T_TableIndex1);
		if (!pTable_temp1) return;

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


/*******************************************************
   返回magic对应的MIBIDSTRUCT对象
 ********************************************************/
MIBIDSTRUCT  *get_scalar_object(MIBIDSTRUCT scalarBuf[],
                                int scalarNum,int magic)
{
	int i;
	DEBUGMSG(("public","--get_scalar_object:magic:%d\n ",magic));

	for(i = 0; i < scalarNum; i++)
	{
		if((scalarBuf[i].t_tacheID.snmpmagic == magic))
		{
			return &scalarBuf[i];
		}
	}
	DEBUGMSG(("public","--get_scalar_object return NULL!!\n "));
	return NULL;
}

/*******************************************************
   返回请求节点对应的的MIBIDSTRUCT
   注意这里使用了结构体变量variable4!
 ********************************************************/
MIBIDSTRUCT  *get_scalar_node( const oid *name,
                               int namelen, int fatherNodeLen,
                               struct variable4 *pVariables,
                               MIBIDSTRUCT scalarBuf[],int scalarNum)
{
	int magic = 0;

	magic = find_magic(name,namelen,fatherNodeLen,pVariables);
	if (magic == 0)
	{
		DEBUGMSG(("public","--get_scalar_node:find_magic return magic 0 !!\n "));
		return NULL;
	}

	return get_scalar_object( scalarBuf, scalarNum, magic);
}

/*************************************************
   找到请求对象的magic和SNMP数据类型.适合变量和表格对象的查找
   注意这里使用了结构体变量variable4!
 **************************************************/
int find_magic(const oid *name,
               int namelen, int fatherNodeLen,
               struct variable4 *pVariables)
{
	int i = 0, j=0;
	int compLen = namelen - fatherNodeLen - 1; //-1 for scalar .0 or  for table .row      !!!!
	int verLen =0;

	oid compName[8];

	for(j = 0; j < compLen; j++)
		compName[j] = name[fatherNodeLen+j];

	DEBUGMSG(("parameter"," --in find_magic:%d\n",j));

	while( pVariables[i].magic != 0) //0 结尾
	{
		verLen =pVariables[i].namelen;
		if(compLen == verLen)
		{
			if( 0 ==
			    snmp_oid_compare(pVariables[i].name,verLen,compName,compLen)
			    )
			{
				//*magic = pVariables[i].magic;
				//*type  = pVariables[i].type;
				DEBUGMSG(("parameter","find_magic: magic = %d\n",
				          pVariables[i].magic));
				return pVariables[i].magic;

				break;
			}

		}

		i++;
	}

}


/*******************************************************
   写标量和表格对象
 ********************************************************/
int snmp_write_action(int action, u_char snmpType,
                      int var_val_len, u_char * var_val,
                      const MIBIDSTRUCT  *writeNode,int dType)
{
	switch (action) {
	case RESERVE1:
		if(writeNode !=NULL && writeNode->t_tacheID.ipcNo <  0) // 有必要?
			return SNMP_ERR_RESOURCEUNAVAILABLE;

		return
		        check_type_lenth(snmpType, writeNode->asnType,var_val_len,MAX_CHAR_LEN);

	case RESERVE2:
		break;

	case FREE:
		/*
		 * Release any resources that have been allocated
		 */
		break;

	case ACTION:
		/*
		 * The variable has been stored in 'value' for you to use,
		 * and you have just been asked to do something with it.
		 * Note that anything done here must be reversable in the UNDO case
		 */
		//if( writeNode !=NULL )
		//	return SNMP_ERR_RESOURCEUNAVAILABLE;
		DEBUGMSG(("parameter"," --snmp_write_action:%d\n",var_val_len));
		if( set_data(writeNode,dType,var_val_len,var_val) <= 0)
		{
			DEBUGMSG(("parameter"," --snmp_write_action:return SNMP_ERR_WRONGVALUE !!\n"));
			return SNMP_ERR_WRONGVALUE;
		}

		break;

	case UNDO:
		/*
		 * Back out any changes made in the ACTION case
		 */
		break;

	case COMMIT:
		//判断特殊参数
		break;
	}

	return SNMP_ERR_NOERROR;

}

/*******************************************************
   检查设置数据类型和值的长度是否匹配正确
 ********************************************************/
int check_type_lenth(int snmpType, int defindType,
                     int var_val_len, int stringLen)
{
	if (snmpType != defindType)      //判断是否是该对象的类型
	{
		fprintf(stderr, "write to parameter scalar type error\n");
		return SNMP_ERR_WRONGTYPE;
	}
	if(snmpType != ASN_INTEGER && snmpType != ASN_IPADDRESS)
	{
		if (var_val_len > (stringLen)) //业务要求不超过32个字符
		{
			fprintf(stderr, "write to parameter: bad length\n");
			return SNMP_ERR_WRONGLENGTH;
		}
	}
	//......
	return SNMP_ERR_NOERROR;
}


/*******************************************************
   nonindex 表示无需通过index去查找数据
 ********************************************************/
void init_nonindex_table( T_TableSimple **tablehead,  const MIBIDSTRUCT tachID[],
                          int maxCol,  int rownums)
{
	T_TableSimple  *table_node=NULL;
	T_TableSimple  *tmpnode = NULL;
	int s, col;

	if(rownums <=0)
	{
		DEBUGMSG(("public", "--initParaTable: rownums <=0 ;return\n"));
		return;
	}

	for(s = 0; s < rownums; s++)
	{
		for(col = 0; col < maxCol; col++)
		{
			tmpnode =  initlize_simpleTableRow(&tachID[col],s*maxCol);
			if( tmpnode  == NULL)
				continue;

			if(*tablehead == NULL)
			{
				table_node = *tablehead = tmpnode;
			}
			else
			{
				table_node->next = tmpnode;
				table_node = tmpnode;
			}
		}
	}
}

/*******************************************************
   获取节点的MIBIDSTRUCT data
 ********************************************************/
MIBIDSTRUCT  *get_table_node(T_TableSimple *ptr,int realColNum,
                             int rowIndex, const int magic)
{
	/*get the wanted row*/
	T_TableSimple *pptr = find_table_row( ptr,  rowIndex,  realColNum);

	return findTableNode( pptr,  magic,  realColNum);
}

/*******************************************************
   获取表ptr中的第rowNum行
 ********************************************************/
T_TableSimple  *find_table_row(T_TableSimple *ptr, int rowNum, int maxCol)
{
	int i = 0,j =0;

	DEBUGMSG(("parameter", "find_table_row: rowNum = %d ,maxCol= %d\n",
	          rowNum, maxCol));
	for (i = 1; ptr != NULL && i < rowNum; ptr = ptr->next, i++)
	{

		for (j = 1; ptr != NULL && j <  maxCol; ptr = ptr->next, j++)
		{; }

		DEBUGMSG(("parameter", "find_table_row -- %d != %d\n",
		          i, rowNum));

	}

	if (ptr == NULL) {
		DEBUGMSG(("parameter", "find_table_row -- no match: %lu\n",
		          i));
		return NULL;
	}
	DEBUGMSG(("parameter", "find_table_row -- match: %lu\n", i));
	return ptr;

}

/*******************************************************
   获取有效数据字节长
 ********************************************************/
int get_data_length(MIBIDSTRUCT *pN,void*pV)
{
	if( NULL != pN && NULL != pV)
	{
		if( ASN_INTEGER == pN->asnType )
			return sizeof(long);
		else if( ASN_OCTET_STR ==  pN->asnType )
			return strlen( (char*)pV );
		// else if ......
	}
	return 0;
}


/*******************************************************
   格式化数据-设置时
 ********************************************************/
static int format_value(const MIBIDSTRUCT *pN,void* pV,int ll,void* poutPut)
{

	if( NULL != pN && NULL != pV && NULL != poutPut)
	{
		if( ASN_INTEGER == pN->asnType )
		{
			memcpy( poutPut, pV, ll );
			return ll;
		}
		else if( ASN_OCTET_STR ==  pN->asnType )
		{
			strncpy((u_char*)poutPut,pV,ll);
			((u_char*)poutPut)[ ll ]='\0';
			return (ll +1) ;//(strlen(poutPut)+1); // 验证是否要加1
		}
		// else if ......
	}
	return 0;
}


int set_data(const MIBIDSTRUCT  *writeNode,int dType,int ll,void* pV)
{
	u_char VAR[MAX_CHAR_LEN]={0};
	int len =  format_value(writeNode,pV,ll,VAR);
	if(0 < len )
		return
		        snmp_set_data(dType,writeNode->t_tacheID.ipcNo,len,VAR);
	return FAILURE;
}
