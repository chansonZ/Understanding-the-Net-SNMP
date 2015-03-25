#!/usr/bin/perl -w

# mysql_agent.pl

use v5.10;
use DBI;
use warnings;
use NetSNMP::OID (':all');
use NetSNMP::agent(':all');
use NetSNMP::ASN(':all');
use NetSNMP::agent::default_store;
use NetSNMP::default_store qw(:all);
use SNMP;
use feature qw(say);
use Data::Dumper;


# 父节点的OID
# my $OID = '.1.3.6.1.4.1.8072.9999.9999.2.1';
my $regOID; # opt{oid} 对象（NetSNMP::OID）

my %global_status       = ();
my $global_last_refresh = 0; # 上次更新时间
my $running = 0;
my %oids    = ();   # 字符型和数值型OID(NetSNMP::OID)
my @ordKs;          # 排序的oids 
my %orign_new = (); # 原始字符型和mib名
my %oid_index = (); # 原始字符型和mib名

my %opt = (
    # daemon_pid => '/var/run/mysql-snmp.pid',
	# 父节点的OID
    oid        => '.1.3.6.1.4.1.8072.9999.9999.2.1',
    refresh    => 60, # 单位秒 #500
	dsn        => "DBI:mysql:host=127.0.0.1;port=3306",
	user       => "user",
    password   => "password",
	verbose    => 1
);

sub dolog {
    my ($level, $msg) = @_;
    # syslog($level, $msg);
    print STDERR $msg . "\n" if ($opt{verbose});
}

# 后台运行
sub daemonize {
}

# 取数据
sub get_mysql_show_status{
	my ($dsn, $dbuser, $dbpass) = @_;
	my $cmd = "SHOW /*!50002 GLOBAL */ STATUS";# 执行的指令

	# 使用DBI链接数据库并执行“show global status”命令
	my $dbh = DBI->connect($dsn, $dbuser, $dbpass) 
		or die("Unable to connect: $DBI::errstr\n");#数据库句柄
	my $result = $dbh->selectall_arrayref($cmd);
	# 断开链接
	$dbh->disconnect();

	return ($result); # 返回数组？正确吗
}

sub get_mysql_info{
 return get_mysql_show_status($opt{dsn}, $opt{user}, $opt{password});
}

# 对应关系-只执行一次
sub mk_oid_table{
	my $i = 1;
	my $result = get_mysql_info();
	foreach my $row (@$result) {
		my $mib_name = $row->[0] ;
		$mib_name =~ s/_(.)/\U$1\E/g;
		$orign_new{ $row->[0] } = "my$mib_name";
		$oid_index{ $row->[0] } = $i;
		$i++;
	}
 
}
# 把$orign_new对应到表
sub mk_oids{
	my $i=1;
	# 构建 oids 表
    foreach my $name ( keys %orign_new ) {
        $oids{$regOID . ".$oid_index{$name}.0"} = {
            'name' => $orign_new{$name},
            'oid' => new NetSNMP::OID($regOID . ".$oid_index{$name}.0")
        };
	    $i++;
    }
	# say Dumper(\%oids);
}


# 取数据：global_status{myXXX} = value
sub fetch_mysql_data {
	my $result = get_mysql_info();
	foreach my $row (@$result) {
		$global_status{ $orign_new{$row->[0]} } = $row->[1];
		#say $row->[0]; # ok
	}

}

# 周期调用，更新全局变量： global_status
sub refresh_status {
    my $now      = time();
	# 还没有到更新的时间点
    if (($now - $global_last_refresh) < $opt{refresh}) {
        return;
    }
	fetch_mysql_data();
    $global_last_refresh = $now;
    return;
}

# 主函数
sub run
{
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_NO_ROOT_ACCESS, 1);
    my $agent = new NetSNMP::agent('Name' => 'mysql', 'AgentX' => 1);
	# if (!$agent) {
		# dolog(LOG_INFO, "agent register feature");
	# }
	# say $?;
	
	mk_oid_table();
	# 后台运行
	#daemonize() if !$opt{'no-daemon'};
	
	# 注册OID
	$regOID = new NetSNMP::OID($opt{oid});
	if ($regOID && %orign_new) {
		mk_oids();
	}
	
	$agent->register("mysql", $regOID, \&mysql_handler);
	# say Dumper(keys%oids);# 如：mysqlStatus.291.0
	# say Dumper(\%oids);
	say "###########################";
	say $oids{"netSnmpPlaypen.2.1.58.0"}->{'name'}; # myHandlerCommit
	say $oids{"netSnmpPlaypen.2.1.58.0"}->{'oid'};  # myUptimeSinceFlushStatus.0
	

	# 转换为有序的数组形式存储，便于使用
	# this contains a lexicographycally sorted oids array
	# 按 MIB 中的OID顺序排列
    @ordKs = sort {$a <=> $b} map {$_ = new NetSNMP::OID($_)} keys %oids;
    $lowestOid  = $ordKs[0];
    $highestOid = $ordKs[$#ordKs];
	
	say "lowestOid:",$lowestOid;
	say "highestOid:",$highestOid;
	
	$running = 1;
	$SIG{'TERM'} = sub {$running = 0;};
	$SIG{'INT'} = sub {$running = 0;};
	while ($running) {
       refresh_status($opt{dsn});
       $agent->agent_check_and_process(1);   # 1 = block 使用阻塞
	   # test
	   # sleep(1);
	   # $running++;
    }
	
    $agent->shutdown();
    dolog(LOG_INFO, "agent shutdown");
}	

sub set_value {
    my ($request, $oid, $request_info) = @_;
	# 取key
    my $oidname = $oids{$oid}->{'name'};
    if (!defined $oidname) {
       if ($oid != $regOID) {
           dolog(LOG_INFO, "OID $oid is not available") if ($opt{verbose});
           $request->setError($request_info, SNMP_ERR_NOSUCHNAME);
       }
       return 0;
    }

	# 取值
    my $value = $global_status{$oidname};
    if (defined $value) {
        dolog(LOG_DEBUG, "$oid($oidname) ->  $value") if ($opt{verbose});;
        $request->setOID($oid);
        $request->setValue(ASN_OCTET_STR, "$value");
    }
    else {
       dolog(LOG_DEBUG, "OID $oid has no value") if ($opt{verbose});
       return 0;
    }
    return 1;
}


sub mysql_handler {
    my ($handler, $registration_info, $request_info, $requests) = @_;
    my ($request);

	#say  $registration_info->getRootOID();#netSnmpPlaypen.2.1
	
    for ($request = $requests; $request; $request = $request->next()) {
        my $oid  = $request->getOID();
        my $mode = $request_info->getMode();

		# say "--".$request->getValue();
		# use Socket;
		# say "Source: ", inet_ntoa($request->getSourceIp()), "\n";
		# say "Destination: ", inet_ntoa($request->getDestIp()), "\n";
		
        dolog(LOG_DEBUG, "asking for oid $oid (mode $mode)") if ($opt{verbose});

        if ($mode == MODE_GET) {
            set_value($request, $oid, $request_info);
        }
		elsif ($mode == MODE_GETNEXT) {
            if (NetSNMP::OID::compare($oid, $lowestOid) < 0) {
                set_value($request, $lowestOid, $request_info);
            }
            elsif (NetSNMP::OID::compare($oid, $highestOid) <= 0) #查找下一个OID
            {
                my $i = 0;
                my $oidToUse = undef;

                # 定位到下一个OID
                do {
                    $oidToUse = $ordKs[$i];
                    $i++;
                } while (NetSNMP::OID::compare($oid, $oidToUse) > -1 and $i < scalar @ordKs);

                #获取“下一个”有效值
                if (defined $oidToUse) {
				    # 对右边界OID请求GETNEXT，直接返回
                    if (NetSNMP::OID::compare($oid, $oidToUse) == 0) {
                        dolog(LOG_DEBUG, "GETNEXT $oid == $oidToUse, no next, returning nothing") if ($opt{verbose});
                        next;
                    }
                    dolog(LOG_DEBUG, "Next oid to $oid is $oidToUse") if ($opt{verbose});
                    while (!set_value($request, $oidToUse, $request_info)) {
                        # 如果请求OID子典序的下一个OID没有值，则继续循环到一个范围内有值的OID
                        $oidToUse = $ordKs[$i];
                        $i++;
                        last if $i > scalar @ordKs; # break
                    }
                }
            }
        }
    }
    dolog(LOG_DEBUG, "finished processing") if ($opt{verbose});
}

# 运行
run() unless caller;











