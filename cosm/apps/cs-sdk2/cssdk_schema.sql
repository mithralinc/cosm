/*
-- select timestamptz '2000-01-01 00:00 GMT' + last_seen * interval '1 second'
--   AS last_seen FROM tracker;

CREATE ROLE cssdk_user LOGIN
  ENCRYPTED PASSWORD 'md501d22bc20de05534a7fcb2ea5dddc6fd'
  NOSUPERUSER INHERIT NOCREATEDB NOCREATEROLE NOREPLICATION;

CREATE DATABASE cssdk
  WITH OWNER = cssdk_user
       ENCODING = 'UTF8'
       TABLESPACE = pg_default
       LC_COLLATE = 'English, United States'
       LC_CTYPE = 'English, United States'
       CONNECTION LIMIT = -1;
*/


DROP TABLE IF EXISTS users CASCADE;
CREATE TABLE users
(
  PRIMARY KEY ( user_id ),
  user_id integer NOT NULL CHECK ( user_id > 0 ),
  
  UNIQUE ( username ),
  username varchar(64) NOT NULL,
  password varchar(32) NOT NULL,

  UNIQUE ( email ),
  email varchar(64) NOT NULL,
  joined timestamp NOT NULL DEFAULT NOW(), -- set on creation.
  
  php_cookie varchar(32)
); 
ALTER TABLE users OWNER TO cssdk_user;


DROP TABLE IF EXISTS banned_users;
CREATE TABLE banned_users (
  PRIMARY KEY ( username ),
  username varchar(64) NOT NULL
);
ALTER TABLE banned_users OWNER TO cssdk_user;


DROP INDEX IF EXISTS hosts_online_last_pong;
DROP TABLE IF EXISTS hosts;
CREATE TABLE hosts
(
  PRIMARY KEY ( machine_id ),
  machine_id integer NOT NULL CHECK ( machine_id < 0 ),
  user_id integer NOT NULL REFERENCES users( user_id ),

  os_type integer NULL,
  cpu_type integer NULL,
  memory integer NULL,
  addr_int integer NULL,
  addr_inet inet NULL,
  last_checkin integer NULL,
  last_pong integer NULL,
  online integer NOT NULL
) WITH ( OIDS=FALSE );
ALTER TABLE hosts OWNER TO cssdk_user;
CREATE INDEX hosts_online_last_pong ON hosts ( online, last_pong );


DROP TABLE IF EXISTS checkins;
CREATE TABLE checkins
(
  machine_id integer NOT NULL,
  checkin_time integer NOT NULL,
  checkin_type char(1) NOT NULL,

  param integer NOT NULL
  -- for 'R', since last shutdown (downtime).
  -- for 'C', since last ping heard.
  -- for 'S', uptime.
) WITH ( OIDS=FALSE );
ALTER TABLE checkins OWNER TO cssdk_user;


DROP TABLE IF EXISTS pongs;
CREATE TABLE pongs
(
  machine_id integer NOT NULL,
  addr_int integer NOT NULL,
  pong_time integer NOT NULL,
  latency_ms integer NOT NULL
) WITH ( OIDS=FALSE );
ALTER TABLE pongs OWNER TO cssdk_user;


CREATE OR REPLACE FUNCTION checkin( id integer, os integer, cpu integer,
  mem integer, ip_int integer, ip_inet inet, cosmtime integer, stat char(1),
  argv integer )
  RETURNS integer AS
$BODY$
DECLARE
  new_online int;
  addr integer;
BEGIN
  -- are we going online, or offline?
  IF stat = 'R' OR stat = 'C' THEN
    new_online := 1;
  ELSE
    new_online := 0;
  END IF;

  -- first see if host exists
  SELECT addr_int INTO addr FROM hosts WHERE machine_id = id;
  IF NOT FOUND THEN
    return 0;
  END IF;

  -- log the checkin
  INSERT INTO checkins( machine_id, checkin_time, checkin_type, param )
    VALUES ( id, cosmtime, stat, argv );

  -- now update that we have been seen
  UPDATE hosts SET os_type = os, cpu_type = cpu, memory = mem,
    addr_int = ip_int, addr_inet = ip_inet, last_checkin = cosmtime,
    online = new_online
    WHERE machine_id = id;
  IF NOT FOUND THEN
    RETURN 0;
  END IF;
  
  RETURN 1;
END;
$BODY$
LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION checkin( integer, integer, integer, integer, integer,
  inet, integer, char(1), integer ) OWNER TO cssdk_user;


CREATE OR REPLACE FUNCTION pong( id integer, ip integer,
  cosmtime integer, latency integer )
  RETURNS void AS
$BODY$
DECLARE
BEGIN
  INSERT INTO pongs ( machine_id, addr_int, pong_time, latency_ms )
    VALUES ( id, ip, cosmtime, latency );
  UPDATE hosts SET last_pong = cosmtime WHERE machine_id = id;
END;
$BODY$
LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION pong( integer, integer, integer, integer ) OWNER TO cssdk_user;

--        last_pong ->  |     0     |  < 360    |  < 295    | current |
-- ---------------------+-----------+-----------+-----------+---------+
-- last_checkin   < 30  |  no route |  offline  | ping time | online  |
-- last_checkin current |   newbie  | returning | ping time | online  |

CREATE OR REPLACE FUNCTION live_hosts( diem integer, OUT text, OUT text )
  RETURNS SETOF record AS
$BODY$
DECLARE
BEGIN
  -- trim the dead, make them checkin again to avoid resync!
  UPDATE hosts SET online = 0
    WHERE online = 1
      AND ( last_pong < ( diem - 360 ) OR last_pong IS NULL )
      AND last_checkin < ( diem - 360 );

  -- return the living
  RETURN QUERY SELECT to_hex( machine_id ), to_hex( addr_int )
    FROM hosts
    WHERE online = 1
      AND (  -- new and recent checkin, 30 sec
            ( last_pong IS NULL AND last_checkin > ( diem - 30 ) )
         OR -- rejoin system, > 6 min gone, 30 sec to try
            ( last_pong > 0  AND last_pong < ( diem - 360 )
              AND last_checkin > ( diem - 30 ) )
         OR -- NOT new, been > 5 min, < 6 min.
            ( last_pong > ( diem - 360 ) AND last_pong < ( diem - 295 ) )
          );
  RETURN;
END;
$BODY$
LANGUAGE plpgsql VOLATILE;
ALTER FUNCTION live_hosts( integer ) OWNER TO cssdk_user;

/*
-- Clear the hosts status
UPDATE hosts SET
  os_type = NULL,
  cpu_type = NULL,
  memory = NULL,
  addr_int = NULL,
  addr_inet = NULL,
  last_checkin = NULL,
  last_pong = NULL,
  online = -1;
*/