/*
  Copyright 1995-2019 Mithral Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
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
s32 PostgreSQL_GetValue( utf8 ** value, DATABASE * database,
  u32 row, u32 column );
s32 PostgreSQL_Close( DATABASE * database );

/* Common API */
#define SQL_Connect PostgreSQL_Connect
#define SQL_Exec PostgreSQL_Exec
#define SQL_Columns PostgreSQL_Columns
#define SQL_ColumnName PostgreSQL_ColumnName
#define SQL_Rows PostgreSQL_Rows
#define SQL_GetValue PostgreSQL_GetValue
#define SQL_Close PostgreSQL_Close
