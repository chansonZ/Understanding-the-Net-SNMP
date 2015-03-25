#!/usr/bin/python
# -*- coding: utf-8 -*-
import netsnmp
import sys,os


class HostResult():
    """This creates a host record"""
    def __init__(self,
                        hostname = None,
                        result = None):

        self.hostname = hostname
        self.result = result

class PysnmpClass(object):#Version=3,host='localhost',
    '''A Class For python　SNMP'''
    def __init__(self,host='localhost',*oids,**para):

        #oid可以传入原始的OID
        #也可以传入已经格式化绑定的OID
        self.oids = oids
        self.Version = 3
        self.DestHost = host
        self.SecLevel=para['securityLevel']
        self.SecName=para['user']
        self.PrivPass=para['priv_passphrase']
        self.AuthPass=para['auth_passphrase']
        self.hostrecd=HostResult(self.DestHost)

    def query(self):
        '''Creates SNMP query session'''
        result = None
        try:
            sess = netsnmp.Session(Version = self.Version,
                               DestHost=self.DestHost,
							   SecLevel=self.SecLevel,
                               SecName=self.SecName,
                               PrivPass=self.PrivPass,
                               AuthPass=self.AuthPass,  
							   )
							
            vars = netsnmp.VarList( *self.oids )
            result = sess.get(vars)
        except Exception, err:
            print 'except:',err
            #返回元组(type, value, traceback)的异常信息
            #print sys.exc_info()
            result = None
        finally:
            self.hostrecd.result = result
            return self.hostrecd # 返回一个HostResult类。直接打印类即可显示结果
