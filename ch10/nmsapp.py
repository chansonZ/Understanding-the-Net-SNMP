#!/usr/bin/python
# -*- coding: utf-8 -*-
# http://www.cnblogs.com/sislcb/archive/2008/11/25/1340587.html
# list --> set http://www.peterbe.com/plog/uniqifiers-benchmark/
# 系统列表：[ ip1,ip2,... ]
# ip1列表：[snmpv3user,oids]
# snmpv3user为固定的元组或字典：(v1,v2...){'user':v1,...}
# oids为列表：[1.2.3, 1.2.4...]

__ver__ = "V0.1 201411"
__doc__ = """
    作者:张春强
    功能：监控SNMP主机
    """

import netsnmp
import ConfigParser
import os
import sys
from query import PysnmpClass

V3OPTIONS=('user','securityLevel','auth_passphrase','priv_passphrase')
PEER_FILE  = "/usr/local/etc/nmsapp/nmshosts.conf"
GENERAL_OIDS_FILE  = "/usr/local/etc/nmsapp/mibs/general/monitor.oids"

def checkipv4addr(addr):
    try:
        if 'localhost' == str(addr):
            return True
        ss = addr.split('.')
        if len(ss) != 4:
            return False
        for x in ss:
            if not (0 <= int( x ) <= 255):
                return False
    except ValueError:
        return False
    return True

def get_ip( alist=None ):
    """
    获取不重复的ip地址。
    如果存在端口标记号':'则取该字符前部分内容作为ip
    """
    ips=[]
    for ip in alist:
        ips.append(ip.split(':')[0])

        if len( ips ) != len(set( ips )):
            print 'duplicated ip address!! exit(1)'
            sys.exit()
    return ips

def mk_oidpath(aset):
    """
    生成各主机oids路径文件名。
    """
    pth=[]
    for ip in aset:
        pth.append('/usr/local/etc/nmsapp/mibs/%s/monitor.oids'%(ip))
    return pth

def get_oidpath(host_list):
    """
    生成各主机oids路径文件名。
    """
    return mk_oidpath( get_ip(host_list) )

def get_oids(f):
    """
    读取f文件中的oid:每行作为一个oid，忽略空行和#起始注释行
    为了便于(其他程序)处理，统一使用数值型的OID.
    当然netsnmp包也支持sysContact.0等形式的OID
    """
    import re
    regex = re.compile(r'^(\.[1-9][1-9]*)+\.0$')
    oids=[]
    try:
        with open(f,'r') as pf:
            for ll in pf:
                if ll.split() and ll.split()[0] != '#':
                    str = ll.split()[0]
                    if regex.match(str):
                        oids.append(str.split('\n')[0])
                    else:
                        print 'oids: wrong format!!'
    except IOError as messg:
        print('open file failed!!\n'+str(messg))
    return oids

def get_conf_option(cfp,sec):
    """
    读取监控配置文件（nmshosts.conf）中某部分的选项
    """
    adict = {}
    #print(sec)
    #print('---------------')
    global V3OPTIONS
    for o in V3OPTIONS:
        try:
            adict[o] = cfp.get(sec,o)
        except:
            print("%s:exception!!"%o)
            adict[o]= None
    return adict

def get_all_sec_opt(cfp,host_list):
    """
    读取监控配置文件（nmshosts.conf）中的选项
    """
    list_dict={}
    for h in host_list:
        list_dict[h]=get_conf_option(cfp,h)
    return list_dict

def read_nmsapp_conf():
    """
    读取监控配置文件（nmshosts.conf）
    """
    cfp = ConfigParser.ConfigParser()
    global PEER_FILE
    cfp.read(PEER_FILE)

    ss = cfp.sections()
    host_list = []

    for s in ss:#去除空格，取:前的字段
        if checkipv4addr( s.strip().split(':')[0] ):
            host_list.append( s.strip() )
    #print host_list
    return (host_list, get_all_sec_opt(cfp,host_list) )

def read_oids(host_list):
    """
    读取所有主机待监控OID
    """
    oid =[]
    oids_dict={}
    i = 0
    oid_paths = get_oidpath(host_list)
    #读取公共oid
    general_oids = get_oids(GENERAL_OIDS_FILE)
    for p in oid_paths:
        oid = get_oids(p)
        oids_dict[host_list[i]] = oid + general_oids
        i += 1
    return oids_dict
    #pass

def get_oid_list_size():
    pass

def get_main_list_size():
    pass

def print_nmsapp_confs(host_list,host_oids,monitor_hosts_dict):
    print 'host_list:',host_list
    print '---------------------'
    print 'host_oids:',host_oids
    print '---------------------'
    print 'monitor_hosts_dict:',monitor_hosts_dict

def print_dict_para(**kw):
    for k, v in kw.iteritems():
        print k,'=',v

def print_list_para(*ll):
    for l in ll:
        print l

def main():
    host_list=[]
    host_oids = {}
    monitor_hosts_dict={}

    (host_list, monitor_hosts_dict) = read_nmsapp_conf()
    #模拟 get_main_list_size() get_oid_list_size
    if (not host_list) or (not monitor_hosts_dict):
        sys.exit(0)
    host_oids = read_oids( host_list )
    if not host_oids:
        sys.exit(0)
    #print_nmsapp_confs(host_list,host_oids,monitor_hosts_dict)

    qurerys = []
    for h in host_list:
        #print_list_para( *host_oids[h] )
        #print '-------------------------------'
        #print_dict_para(**monitor_hosts_dict[h])
        qurerys.append( PysnmpClass(h,*host_oids[h], **monitor_hosts_dict[h]) )

    for qq in qurerys:
        tt = qq.query()
        print 'hostname',tt.hostname
        print 'result:',tt.result


if __name__=='__main__':
    main()


