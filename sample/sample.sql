create table card(
			 name varchar(50) primary key,
			 cost int not null,
			 type varchar(32) not null,
			 text varchar(1024) not null,
			 flavortext varchar(1024) null,
			 points int null,
			 runner int not null
);
create table keyword(
			 card varchar(50) references card(name) on delete cascade,
			 keyword varchar(32)
);
create unique index keyword_pk on keyword(card, keyword);
create table illustration (
			 card varchar(50) references card(name) on delete cascade,
			 version varchar(32),
			 data blob
);
create unique index illustration_pk on illustration (card, version);
