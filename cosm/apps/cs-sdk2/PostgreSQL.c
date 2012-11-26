/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm CS-SDK2 - Client Server Software Development Kit

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  2007-2012 by Creator. All rights reserved. Further information about the
  Package can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include <libpq-fe.h>
#include "PostgreSQL.h"

s32 PostgreSQL_Connect( DATABASE * database )
{
  PGconn * connection;

  connection = PQconnectdb( "host='localhost' dbname='cssdk2' "
    "user='cssdk2_user' password='cssdk2_pass'" );

  if ( PQstatus( connection ) != CONNECTION_OK )
  {
    return COSM_FAIL;
  }

  database->state = POSTGRESQL_STATE_CONNECTED;
  database->connection = connection;
  database->result = NULL;
  return COSM_PASS;
}

s32 PostgreSQL_Exec( DATABASE * database, utf8 * query )
{
  ExecStatusType error;
  
  if ( database->state == POSTGRESQL_STATE_RESULT )
  {
    PQclear( database->result );
    database->state = POSTGRESQL_STATE_CONNECTED;
  }

  if ( database->state == POSTGRESQL_STATE_CONNECTED )
  {
    database->result = PQexec( database->connection, query );
    error = PQresultStatus( database->result );
    if ( ( error != PGRES_COMMAND_OK )
      && ( error != PGRES_TUPLES_OK ) )
    {
      return COSM_FAIL;
    }
    database->state = POSTGRESQL_STATE_RESULT;
    return COSM_PASS;
  }

  return COSM_FAIL;
}

s32 PostgreSQL_Columns( u32 * columns, DATABASE * database )
{
  if ( database->state == POSTGRESQL_STATE_RESULT )
  {
    *columns = PQnfields( database->result );
    return COSM_PASS;
  }

  return COSM_FAIL;
}

s32 PostgreSQL_ColumnName( utf8 ** name, DATABASE * database, u32 column )
{
  utf8 * tmp;

  if ( database->state == POSTGRESQL_STATE_RESULT )
  {
    tmp = PQfname( database->result, column );
    if ( tmp != NULL )
    {
      *name = tmp;
      return COSM_PASS;
    }
  }

  return COSM_FAIL;
}

s32 PostgreSQL_Rows( u32 * rows, DATABASE * database )
{
  if ( database->state == POSTGRESQL_STATE_RESULT )
  {
    *rows = PQntuples( database->result );
    return COSM_PASS;
  }

  return COSM_FAIL;
}

s32 PostgreSQL_GetValue( utf8 ** value, DATABASE * database,
  u32 row, u32 column )
{
  if ( database->state == POSTGRESQL_STATE_RESULT )
  {
    if ( PQgetisnull( database->result, row, column ) )
    {
      *value = NULL;
    }
    else
    {
      *value = PQgetvalue( database->result, row, column );
    }
    return COSM_PASS;
  }

  return COSM_FAIL;
}

s32 PostgreSQL_Close( DATABASE * database )
{
  if ( database->state == POSTGRESQL_STATE_RESULT )
  {
    PQclear( database->result );
    database->state = POSTGRESQL_STATE_CONNECTED;
  }

  if ( database->state == POSTGRESQL_STATE_CONNECTED )
  {
    PQfinish( database->connection );
  }

  database->state = POSTGRESQL_STATE_NONE;

  return COSM_PASS;
}
