#!/usr/bin/perl -w

# Now SNMP tools are enterprisey. 
# They won't even give you the current uptime without a MIB. 
# So for "SHOW GLOBAL STATUS" we need a MIB file as well. 
# That is why I like perl so much. It gets things done. 
# In this case, it is making me a MIB file out of a MySQL connection.

##############
#  ./makemysqlmib.pl > mysqlmib
#[/mnt/hgfs/centosShare/perl/monitor_mysql]# smistrip mysqlmib
# MYSQL-MIB: 4133 lines.
# smilint -m /usr/local/share/snmp/mibs/NET-SNMP-MIB.txt ./MYSQL-MIB

# XX:echo "mibs +MYSQL-MIB" > snmp.conf
# export MIBS=ALL
# snmptranslate -On -Tp -IR mysqlMIB| less
##############
use strict;
use DBI;

# 配置链接数据库的信息
my $dsn  = "DBI:mysql:host=127.0.0.1;port=3306";
my $user = "user";
my $pass = "password";
# 声明数组变量
my @global_status_name = ();
my @mibName = ();

# 使用DBI链接数据库并执行“show global status”命令
my $dbh = DBI->connect($dsn, $user, $pass) or die("connect");#数据库句柄
my $cmd = "SHOW /*!50002 GLOBAL */ STATUS";# 执行的指令


my $result = $dbh->selectall_arrayref($cmd);
my $i = 1;

foreach my $row (@$result) {
			# $status{$row->[0]} = $row->[1];
			my $name = $row->[0] ;
			$global_status_name[$i]  = $name;
			$name =~ s/_(.)/\U$1\E/g;
			# $output{ "my$mib_name" } = $row->[1];
			$mibName[$i] = "my$name";
			$i++; 
}

# 断开链接
$dbh->disconnect();

# 生成MIB文件头部信息
print qq(MYSQL-MIB DEFINITIONS ::= BEGIN
IMPORTS
    MODULE-IDENTITY, OBJECT-TYPE
        FROM SNMPv2-SMI
    DisplayString
        FROM SNMPv2-TC
		
	MODULE-COMPLIANCE, OBJECT-GROUP
    FROM SNMPv2-CONF
	
    netSnmpPlaypen
        FROM NET-SNMP-MIB;
		

mysqlMIB MODULE-IDENTITY
    LAST-UPDATED "201411270000Z"
    ORGANIZATION "book example"
    CONTACT-INFO
            "xtdwxk\@gmail.com"
    DESCRIPTION
            "mysql status"
    REVISION      "201411270000Z"
    DESCRIPTION
            "mysql status MIB"
    ::= { netSnmpPlaypen 2 } 

mysqlStatus   OBJECT IDENTIFIER ::= { mysqlMIB 1 }
);

# 所有的信息都定义为只读、字符型类型
for (my $i=1; $i < @global_status_name; $i++) {
  printf qq(%s    OBJECT-TYPE
    SYNTAX    DisplayString
    MAX-ACCESS    read-only
    STATUS    current
    DESCRIPTION    "SHOW /*!50002 GLOBAL */ STATUS LIKE '%s'"
    ::= { mysqlStatus %d }

 ),
        $mibName[$i],
		$global_status_name[$i],
        $i;
}

# 加入一致性声明
print qq(
mySQLMIBConformance OBJECT IDENTIFIER ::= { mysqlMIB 2 }
mySQLMIBCompliances OBJECT IDENTIFIER ::= { mySQLMIBConformance 1 }
mySQLMIBGroups      OBJECT IDENTIFIER ::= { mySQLMIBConformance 2 }

mySQLMIBCompliance MODULE-COMPLIANCE
    STATUS  current
    DESCRIPTION
    "The compliance statement for SNMPv2 entities which implement MYSQL."
    MODULE  -- this module
    MANDATORY-GROUPS { mySQLGroup }
    ::= { mySQLMIBCompliances 1 }
);

# 加入组-part1
print qq(
mySQLGroup OBJECT-GROUP
    OBJECTS   { 
);

# 加入组-part2
my $j=1;
for ($j=1; $j < @mibName-1; $j++) {
  printf qq(%s,
  ),  $mibName[$j];
}
printf qq(%s),$mibName[$j];
		
# 加入组-part3
print qq(
}
    STATUS    current
    DESCRIPTION
     "The group of objects providing for management of MYSQL entities."
    ::= { mySQLMIBGroups 1 }
);

# 加入MIB 结束符END
print qq(
END
);
