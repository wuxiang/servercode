############
#2012.11.02#
############

#mysql -u root;

create database if not exists pricealarm default character set utf8;
use pricealarm;

create table if not exists user_basic_info_v0200
(
	MapID int unsigned NOT NULL AUTO_INCREMENT,
	PushServId SMALLINT unsigned NOT NULL,
	PlatformCode char(1) NOT NULL,	#android ios wp7
	DBSynTime TIMESTAMP NOT NULL default 0,
	MaxUserSpace int unsigned NOT NULL,
	OccupyNum int unsigned NOT NULL,
	HisSpace int unsigned NOT NULL,
	EarlyWarningSpace int unsigned NOT NULL,
	UseEarlyWarning int unsigned NOT NULL,
	EarlyWarningPerUser int unsigned NOT NULL,
	LastestActiveTime TIMESTAMP default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	
	PRIMARY KEY(MapID, PushServId, PlatformCode),
	KEY indexUpdDate (LastestActiveTime)
	
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

create table if not exists user_v0200
(
	MapID INT UNSIGNED NOT NULL AUTO_INCREMENT,
	UserID varchar(32) NOT NULL,
	UserName varchar(50) DEFAULT NULL,
	UserPwd varchar(20) DEFAULT NULL,
	LocalVersion SMALLINT unsigned NOT NULL,
	
	PlatformCode char(1) NOT NULL,	
	TelNum varchar(16) DEFAULT NULL,
	PushServId SMALLINT unsigned NOT NULL,	
	UserProperty int unsigned NOT NULL,
	RegDate TIMESTAMP NOT NULL default 0,	
	LastestActiveTime TIMESTAMP default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
	
	RecordSetMarkID INT UNSIGNED NOT NULL,
	PushWarningTotal INT UNSIGNED NOT NULL,
	PushWarningNum INT UNSIGNED NOT NULL,
	PushWarningDBNum INT UNSIGNED NOT NULL,
	RebackWarningNum INT UNSIGNED,
	
	InfoMineTotal INT UNSIGNED,
	InfoMineNum INT UNSIGNED,
	InfoMineDBNum INT UNSIGNED,
	
	PublicNoticeTotal INT UNSIGNED,
	PublicNoticeNum INT UNSIGNED,
	PublicNoticeDBNum INT UNSIGNED,
	
	PushToken varchar(128),

	primary key(UserID, PlatformCode),
	KEY indexPushtoken(PushToken),
	KEY indexMapid (MapID),
	KEY indexPlatform(PlatformCode),
	KEY indexRegdate (RegDate),
	KEY indexUpdDate (LastestActiveTime)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

create table if not exists ewarning_v0200
(
	MapID int unsigned NOT NULL AUTO_INCREMENT,
	UserID varchar(32) NOT NULL,
	PlatformCode char(1) NOT NULL,
	UniqueID int unsigned NOT NULL,
	RecordProperty char(1) NOT NULL,
	StkIndex smallint unsigned,
	StockCode varchar(10) NOT NULL,
	PriceGreaterTrigger int unsigned,
	PriceGreaterProperty smallint unsigned,
	PriceSmallerTrigger int unsigned,
	PriceSmallerProperty smallint unsigned,
	IncreaseGreaterTrigger int unsigned,
	IncreaseGreaterProperty smallint unsigned,
	DecreaseGreaterTrigger int unsigned,
	DecreaseGreaterProperty smallint unsigned,
	ExchangeGreaterTrigger int unsigned,
	ExchangeGreaterProperty smallint unsigned,
	LatestInfoMineCrc int unsigned  NOT NULL default 0,
	TriggerTime TIMESTAMP NOT NULL default 0,
	LastestActiveTime TIMESTAMP default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

	primary key(UserID, PlatformCode, UniqueID),
	KEY indexMapid (MapID),
	KEY indexUpdDate (LastestActiveTime)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE if not exists pushmsghis_v0200
(
	MapID int unsigned NOT NULL AUTO_INCREMENT,
	UserID varchar(32) NOT NULL,
	PlatformCode char(1) NOT NULL,
	MsgID int NOT NULL,
	MsgType char(1) DEFAULT NULL,
	MsgSubType char(1) DEFAULT NULL,
	TriggerDateTime TIMESTAMP NOT NULL default 0,
	LatestValue int unsigned DEFAULT NULL,
	SettingRecordID int unsigned DEFAULT NULL,
	StockCode varchar(10) DEFAULT NULL,
	LastestActiveTime TIMESTAMP default CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

	PRIMARY KEY (UserID, PlatformCode, MsgType, MsgID),
	KEY indexMapID(MapID),
	KEY indexDatetime (TriggerDateTime),
	KEY indexUpdDate (LastestActiveTime)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

#user_v0200添加自选股字段
alter table user_v0200 add SelfSelectStk varchar(300) NOT NULL default '';

#扩充单字符字段
alter table user_basic_info_v0200 modify PlatformCode SMALLINT unsigned NOT NULL;
alter table user_v0200 modify PlatformCode SMALLINT unsigned NOT NULL;
alter table ewarning_v0200 modify PlatformCode SMALLINT unsigned NOT NULL;
alter table ewarning_v0200 modify RecordProperty SMALLINT unsigned NOT NULL;
alter table pushmsghis_v0200 modify PlatformCode SMALLINT unsigned NOT NULL;
alter table pushmsghis_v0200 modify MsgType SMALLINT unsigned NOT NULL;
alter table pushmsghis_v0200 modify MsgSubType SMALLINT unsigned NOT NULL;

#扩充PushToken
alter table user_v0200 modify PushToken varchar(256) NOT NULL default '';

