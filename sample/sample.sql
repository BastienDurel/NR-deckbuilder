create table card(name varchar(50) primary key, cost int);
create table keyword(card varchar(50) references card(name) on delete cascade, keyword varchar(32));
create unique index keyword_pk on keyword(card, keyword);
create table illustration (card varchar(50) references card(name) on delete cascade, version varchar(32), data blob);
create unique index illustration_pk on illustration (card, version);

-- data
insert into card values ('Datacomb', 4);
insert into card values ('The Personal Touch', 4);
insert into keyword values ('The Personal Touch', 'Prep');
insert into keyword values ('Datacomb', 'Ice');
insert into keyword values ('Datacomb', 'Wall');

