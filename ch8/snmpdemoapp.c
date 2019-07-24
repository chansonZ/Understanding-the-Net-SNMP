
/*
从最简单的同步应用学Net-SNMP通讯流程：
在这里我们的通信主机仅仅一台，获取的对象只有两个
make: gcc snmpdemoapp.c `net-snmp-config --libs` -o snmpdemoapp 
gcc snmpdemoapp.c -L/usr/local/lib -lnetsnmp -lrt -lm -o snmpdemoapp 

*/

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <string.h>

int main (int argc, char **argv)
{
	/* 
	结构体 netsnmp_session 中记录了SNMP会话信息；
	第一个变量需要填充准备会话的信息，
	第二个为一指针用于记录库返回的会话信息；
	*/
    netsnmp_session session, *ss;
	
	// 该结构体中记录了远程主机所有的信息
    netsnmp_pdu *pdu;
	
	// 该结构体中记录了远程主机返回的PDU信息	
    netsnmp_pdu *response;
	
	// 记录OID节点位置信息
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
	
	// 变量绑定列表（为list数据结构），也就是需要操作的数据 
    netsnmp_variable_list *vars;
    int status;
    int count=1;	
	
	/* 
	初始SNMP库：
	初始化互斥量、MIB 解析、传输层、
	调试信息的初始化、解析配置文件的初始化（netsnmp_ds_register_config）、各句柄的初始化
	定时器的初始化、读取配置文件
	*/
    init_snmp("snmpdemoapp");	
	
    /* 
	session 的初始化：包括初始化目的地，SNMP协议版本、认证机制、 	
	初始化会话结构体(为默认值)，不涉及任何的MIB文件处理
	*/
    snmp_sess_init( &session ); 
	// 设置会话结构体：目标地址；可以为其他有效的网络地址	
	session.peername = strdup("localhost"); 
	
	// 使用SNMPv1版本
    session.version = SNMP_VERSION_1;

	// 设置共同体
    session.community = "public";
    session.community_len = strlen(session.community);	
	
	/* 
	开启和绑定底层的传输层（TCP/UDP），建立会话，返回会话句柄.
	每个回话都对应一个socket
	*/
    ss = snmp_open(&session); 

    if (!ss) {
	  snmp_sess_perror("snmpdemoapp", &session); // 记录错误信息到日志中，
      exit(1);
    }               
	/* 回话创建完成后，接下来是创建指定类型（命令类型）的PDU，
	 作为本次回话的实例执行指定操作的准备，
	 这就包括绑定准备通信的OID。
	*/
    pdu = snmp_pdu_create(SNMP_MSG_GET);
    anOID_len = MAX_OID_LEN; // 该宏值为128，正如RFC建议所述“节点下子OID数量不超过128个”。
	
	/* 
	此处 read_objid / snmp_parse_oid的作用是检查OID的正确性，
	并赋予anOID正确的值,也就是最终请求的OID
	*/
	//read_objid("system.sysDescr.0", anOID, &anOID_len);	

    //if (!snmp_parse_oid(".1.3.6.1.2.1.1.1.0", anOID, &anOID_len)) {
	if (!snmp_parse_oid("system.sysDescr.0", anOID, &anOID_len)) {
      snmp_perror(".1.3.6.1.2.1.1.1.0");
      exit(1);
    }
	
	/* 
	按照协议PDU格式的要求，我们将该OID加入到PDU报文中，
	同时赋予一个NULL值作为变量的绑定;
	当然对于SET命令的PDU来说就是赋予待设置的值。
	*/
    snmp_add_null_var(pdu, anOID, anOID_len);

	//同步发送报文
    status = snmp_synch_response(ss, pdu, &response);
	
	/*
	下面的代码是处理返回的PDU报文，我们需要根据返回的信息作出合理的后续处理：
	报文的返回和执行状态都正确，读取返回的值
	*/
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR) {
      //将读取到的返回值打印到标准输出(stdout)出来
      for(vars = response->variables; vars; vars = vars->next_variable)
        print_variable(vars->name, vars->name_length, vars);

	  //处理（打印）接收到的信息： 
       for(vars = response->variables; vars; vars = vars->next_variable) {
        if (vars->type == ASN_OCTET_STR) {
		  char *sp = (char *)malloc(1 + vars->val_len);
		  memcpy(sp, vars->val.string, vars->val_len);
		  sp[vars->val_len] = '\0';
			  printf("value #%d is a string: %s\n", count++, sp);
		  free(sp);
		}
        else
          printf("value #%d is NOT a string! Ack!\n", count++);
     }
    } else {

	  // 如果返回值有错误：打印其错误信息
      if (status == STAT_SUCCESS)
        fprintf(stderr, "Error in packet\nReason: %s\n",
                snmp_errstring(response->errstat)); 
      else if (status == STAT_TIMEOUT)
        fprintf(stderr, "Timeout: No response from %s.\n",
                session.peername);
      else
        snmp_sess_perror("snmpdemoapp", ss); //记录错误日志

    }
	
	//释放分配的空间；关闭会话，关闭socket，释放资源
    if (response)
      snmp_free_pdu(response);
    snmp_close(ss);
    return (0);		
}
