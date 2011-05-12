#!/usr/bin/perl -w
# -*- cperl -*-

use DBI;
use DBD::SQLite;
use DBI qw(:sql_types);
use File::Slurp;
use Getopt::Long;
use strict;

my $sql = "insert into illustration (card, version, data) values (?,?,?)";

my $version = "Netrunner Limited";
my $card = "Aujourd'Oui";
my $img = '/tmp/netrunner-aujourd-oui.jpg';
my $dbname = 'nr-full.db';
my $delete = 0;

my $result = GetOptions ("card=s" => \$card,
												 "img|file=s" => \$img,
												 "dbname=s" => \$dbname,
												 "version=s" => \$version,
												 "delete" => \$delete);


my $dbargs = { AutoCommit => 0,
							 PrintError => 1};

my $dbh = DBI->connect("dbi:SQLite:dbname=$dbname", "", "", $dbargs) 
		or die "DB";
my $img_sth = $dbh->prepare($sql) or die $dbh->error;

my $img_blob = read_file( $img, binmode => ':raw' ) or die "slurp $img: $!";

if ($delete) {
	my $del_sth = $dbh->prepare("delete from illustration where ".
															"card = ? and version = ?") or die $dbh->error;
	$del_sth->bind_param(1, $card, SQL_VARCHAR) or die $dbh->error;
	$del_sth->bind_param(2, $version, SQL_VARCHAR) or die $dbh->error;
	$del_sth->execute() or die $dbh->error;
}

$img_sth->bind_param(1, $card, SQL_VARCHAR) or die $dbh->error;
$img_sth->bind_param(2, $version, SQL_VARCHAR) or die $dbh->error;
$img_sth->bind_param(3, $img_blob, SQL_BLOB) or die $dbh->error;
$img_sth->execute() or die $dbh->error;

$dbh->commit();
$dbh->disconnect();



