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

#include "cosm/cosm.h"

typedef enum database_states
{
  POSTGRESQL_STATE_NONE      = 0,
  POSTGRESQL_STATE_CONNECTED = 42,
  POSTGRESQL_STATE_RESULT    = 139
} database_states;

typedef struct DATABASE
{
  database_states state;
  void * connection;
  void * result;
} DATABASE;

s32 PostgreSQL_Connect( DATABASE * database );
s32 PostgreSQL_Exec( DATABASE * database, utf8 * query );
s32 PostgreSQL_Columns( u32 * columns, DATABASE * database );
s32 PostgreSQL_ColumnName( utf8 ** name, DATABASE * database, u32 column );
s32 PostgreSQL_Rows( u32 * rows, DATABASE * database );
s32 PostgreSQL_GetValue( utf8 ** value, DATABASE * database, u32 row, u32 column );
s32 PostgreSQL_Close( DATABASE * database );

// Common API
#define SQL_Connect PostgreSQL_Connect
#define SQL_Exec PostgreSQL_Exec
#define SQL_Columns PostgreSQL_Columns
#define SQL_ColumnName PostgreSQL_ColumnName
#define SQL_Rows PostgreSQL_Rows
#define SQL_GetValue PostgreSQL_GetValue
#define SQL_Close PostgreSQL_Close