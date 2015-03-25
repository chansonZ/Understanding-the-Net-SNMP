/************************************************************
 * Copyright (C)	GPL
 * FileName:		 list.h:提供创建链表、末尾添加和链表循环处理的3个接口
 * Author:		张春强
 * Date:			2014-07
 ***********************************************************/
#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif                          /*__cplusplus */

typedef struct  _list_node
{
	void* data;                
	struct _list_node *next;
} T_ListNode;

struct  _list
{
	T_ListNode	*head;        // 指向链表第一个节点
	T_ListNode	*tail;          // 指向链表最后一个节点
	int			size;          // 链表中的节点数	
};
typedef struct _list T_List;

//list_foreach  中最后一个参数传入到context; data为list中的data
typedef int (*LIST_FUN_CUSTOM)( void* data, void* context );

#define PRINT_STRING	( 0 )
#define PRINT_INT		( 1 )
#define list_size( pList ) ( ( pList )->size )

T_List * create_list( void );


void free_list( T_List * pList );


int list_append( T_List *pList, void* data, LIST_FUN_CUSTOM fun_custom );


int list_foreach( T_List *pList, LIST_FUN_CUSTOM fun, void* ctx );


// which = PRINT_INT,PRINT_STRING
void list_printf( T_List *pList, int which );


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /*  LIST_H  */
