/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Utility Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2012 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#include "cosm/hashtable.h"
#include "cosm/os_mem.h"

static const u64 prime_table[] =
{
  53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
  196613, 393241, 786433, 1572869, 3145739, 6291469, 12582917, 25165843,
  50331653, 100663319, 201326611, 402653189, 805306457, 1610612741,
  3221225473LL
  /* !!! need more entries, up to 2^59 */
};
#define PRIME_TABLE_LENGTH ( sizeof( prime_table ) / sizeof( u64 ) )
#define TARGET_TABLE_LOAD ( 3.0f / 4.0f )

s32 CosmHashTableInit( cosm_HASH_TABLE * hashtable, u64 minimum_size,
  u64 (*hash)( void * ), s32 (*equal)( void *, void * ),
  void * (*dup_key)( void * ), void * (*dup_value)( void * ) )
{
  u32 index;
  u64 memory, size;

  if ( ( NULL == hashtable ) || ( NULL == hash ) || ( NULL == equal )
    || ( COSM_HASH_TABLE_STATE_NONE != hashtable->state ) )
  {
    return COSM_FAIL;
  }

  /* do a quick test to see if we can have a hash table that big in RAM */
  if ( ( COSM_PASS == CosmMemSystem( &memory ) )
    && ( minimum_size < ( memory << 5 ) ) )
  {
    return COSM_FAIL;
  }

  /* find the prime just over our target factoring in load */
  size = 0;
  for ( index = 0 ; index < PRIME_TABLE_LENGTH ; index++ )
  {
    if ( prime_table[index] > ( minimum_size / TARGET_TABLE_LOAD ) )
    {
      size = prime_table[index];
      break;
    }
  }
  if ( 0 == size )
  {
    return COSM_FAIL;
  }

  if ( NULL == ( hashtable->table =
    CosmMemAlloc( size * sizeof( void * ) ) ) )
  {
    return COSM_FAIL;
  }

  hashtable->state = COSM_HASH_TABLE_STATE_INIT;
  hashtable->table_index = index;
  hashtable->count = 0;
  hashtable->hash = hash;
  hashtable->equal = equal;
  hashtable->dup_key = dup_key;
  hashtable->dup_value = dup_value;
  hashtable->table_length = size;
  hashtable->iterator_row = 0;
  hashtable->iterator_next = NULL;

  return COSM_PASS;
}

s32 CosmHashTableCount( u64 * count, cosm_HASH_TABLE * hashtable )
{
  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state ) )
  {
    return COSM_FAIL;
  }

  *count = hashtable->count;

  return COSM_PASS;
}

s32 CosmHashTableAdd( cosm_HASH_TABLE * hashtable, void * key, void * value )
{
  cosm_HASH_TABLE_ENTRY * entry, * new_entry;
  void * new_key, * new_value;
  u64 hash, row;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state )
    || ( NULL == key ) || ( NULL == value ) )
  {
    return COSM_FAIL;
  }

  /* check for expansion before calculating row */
  if ( hashtable->count > ( TARGET_TABLE_LOAD * hashtable->table_length ) )
  {
    Cosm_HashTableGrow( hashtable );
  }

  /* where does the entry go? */
  hash = (*hashtable->hash)( key );
  row = hash % hashtable->table_length;

  /* check for duplicate keys, and do NOT allow */
  entry = hashtable->table[row];
  while ( NULL != entry )
  {
    if ( ( entry->key_hashed == hash )
      && ( (*hashtable->equal)( key, entry->key ) ) )
    {
      return COSM_FAIL;
    }
    entry = entry->next;
  }

  /* prepare the entry, duplicating if needed, do first in case of errors */
  if ( NULL ==
    ( new_entry = CosmMemAlloc( sizeof( cosm_HASH_TABLE_ENTRY ) ) ) )
  {
    return COSM_FAIL;
  }

  if ( hashtable->dup_key )
  {
    if ( NULL == ( new_key = (*hashtable->dup_key)( key ) ) )
    {
      CosmMemFree( new_entry );
      return COSM_FAIL;
    }
  }
  else
  {
    new_key = key;
  }

  if ( hashtable->dup_value )
  {
    if ( NULL == ( new_value = (*hashtable->dup_value)( value ) ) )
    {
      if ( hashtable->dup_key )
      {
        CosmMemFree( new_key );
      }
      CosmMemFree( new_entry );
      return COSM_FAIL;
    }
  }
  else
  {
    new_value = value;
  }

  /* we have the needed memory, put it where it belongs */

  new_entry->key = new_key;
  new_entry->key_hashed = hash;
  new_entry->value = new_value;
  new_entry->next = hashtable->table[row];
  hashtable->table[row] = new_entry;

  return COSM_PASS;
}

s32 CosmHashTableUpdate( cosm_HASH_TABLE * hashtable,
  void * key, void * value )
{
  cosm_HASH_TABLE_ENTRY * entry;
  void * new_value;
  u64 hash, row;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state )
    || ( NULL == key ) || ( NULL == value ) )
  {
    return COSM_FAIL;
  }

  hash = (*hashtable->hash)( key );
  row = hash % hashtable->table_length;

  /* scan the row until we find it */
  entry = hashtable->table[row];
  while ( NULL != entry )
  {
    if ( ( entry->key_hashed == hash )
      && ( (*hashtable->equal)( key, entry->key ) ) )
    {
      if ( hashtable->dup_value )
      {
        /* try to duplicate value before we change hash table entry */
        if ( NULL == ( new_value = (*hashtable->dup_value)( value ) ) )
        {
          return COSM_FAIL;
        }
        CosmMemFree( entry->value );
        entry->value = new_value;
      }
      else
      {
        entry->value = value;
      }
      return COSM_PASS;
    }
    entry = entry->next;
  }

  return COSM_FAIL;
}

void * CosmHashTableValue( cosm_HASH_TABLE * hashtable, void * key )
{
  cosm_HASH_TABLE_ENTRY * entry;
  u64 hash, row;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state )
    || ( NULL == key ) )
  {
    return NULL;
  }

  hash = (*hashtable->hash)( key );
  row = hash % hashtable->table_length;

  /* scan the row until we find it */
  entry = hashtable->table[row];
  while ( NULL != entry )
  {
    if ( ( entry->key_hashed == hash )
      && ( (*hashtable->equal)( key, entry->key ) ) )
    {
      return entry->value;
    }
    entry = entry->next;
  }

  return NULL;
}

void CosmHashTableDelete( cosm_HASH_TABLE * hashtable, void * key )
{
  cosm_HASH_TABLE_ENTRY * entry, ** prev_entry;
  u64 hash, row;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state )
    || ( NULL == key ) )
  {
    return;
  }

  hash = (*hashtable->hash)( key );
  row = hash % hashtable->table_length;

  /* scan the row until we find it */
  prev_entry = &hashtable->table[row];
  entry = hashtable->table[row];
  while ( NULL != entry )
  {
    if ( ( entry->key_hashed == hash )
      && ( (*hashtable->equal)( key, entry->key ) ) )
    {
      /* fix chain */
      *prev_entry = entry->next;

      /* delete duplicated data and the entry */
      if ( hashtable->dup_key )
      {
        CosmMemFree( entry->key );
      }
      if ( hashtable->dup_value )
      {
        CosmMemFree( entry->value );
      }
      CosmMemFree( entry );

      hashtable->count--;
      return;
    }
    prev_entry = &entry->next;
    entry = entry->next;
  }

  return;
}

s32 CosmHashTableStart( cosm_HASH_TABLE * hashtable )
{
  u64 row;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state ) )
  {
    return COSM_FAIL;
  }

  for ( row = 0 ; row < hashtable->table_length ; row++ )
  {
    if ( NULL != hashtable->table[row] )
    {
      hashtable->iterator_row = row;
      hashtable->iterator_next = hashtable->table[row];
      return COSM_PASS;
    }
  }

  hashtable->iterator_row = hashtable->table_length;
  hashtable->iterator_next = NULL;
  return COSM_FAIL;
}

s32 CosmHashTableNext( void ** key, void ** value,
  cosm_HASH_TABLE * hashtable )
{
  cosm_HASH_TABLE_ENTRY * entry, * next;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state )
    || ( NULL == key ) || ( NULL == value ) )
  {
    return COSM_FAIL;
  }

  /* are we already done? */
  if ( ( hashtable->iterator_row > hashtable->table_length )
    || ( NULL == hashtable->iterator_next ) )
  {
    return COSM_FAIL;
  }

  entry = hashtable->iterator_next;

  /* find the next one */
  if ( NULL == entry->next )
  {
    /* this row is empty, try the next, we may be done */
    while ( ( hashtable->iterator_row < hashtable->table_length )
      && ( NULL != hashtable->table[hashtable->iterator_row] ) )
    {
      hashtable->iterator_row++;
    }
    /* we either found one, or we're done */
    if ( hashtable->iterator_row < hashtable->table_length )
    {
      next = hashtable->table[hashtable->iterator_row];
    }
    else
    {
      next = NULL;
    }
  }
  else
  {
    /* another entry in this row */
    next = entry->next;
  }

  hashtable->iterator_next = next;
  *key = entry->key;
  *value = entry->value;

  return COSM_PASS;
}

void CosmHashTableFree( cosm_HASH_TABLE * hashtable )
{
  cosm_HASH_TABLE_ENTRY * entry, * next;
  u64 row;

  if ( ( NULL == hashtable )
    || ( COSM_HASH_TABLE_STATE_INIT != hashtable->state ) )
  {
    return;
  }

  /* run through all array and chains, free duplicated data, entries */

  for ( row = 0 ; row < hashtable->table_length ; row++ )
  {
    entry = hashtable->table[row];
    while ( NULL != entry )
    {
      if ( hashtable->dup_key )
      {
        CosmMemFree( entry->key );
      }
      if ( hashtable->dup_value )
      {
        CosmMemFree( entry->value );
      }
      next = entry->next;
      CosmMemFree( entry );
      entry = next;
    }
  }

  /* free the entry table and clear the hashtable */
  CosmMemFree( hashtable->table );
  CosmMemSet( hashtable, sizeof( cosm_HASH_TABLE ), 0 );
}

void Cosm_HashTableGrow( cosm_HASH_TABLE * hashtable )
{
  cosm_HASH_TABLE_ENTRY ** new_table, * entry, * next, * temp;
  u64 row, new_length;

  if ( hashtable->table_index == ( PRIME_TABLE_LENGTH - 1 ) )
  {
    /* max size already */
    return;
  }
  else
  {
    /* we're ok to grow */
    new_length = prime_table[hashtable->table_index + 1];
  }

  if ( NULL == ( new_table =
    CosmMemAlloc( new_length * sizeof( void * ) ) ) )
  {
    /* out of memory */
    return;
  }

  /* move all entries to new chains */
  for ( row = 0 ; row < hashtable->table_length ; row++ )
  {
    entry = hashtable->table[row];
    while ( NULL != entry )
    {
      next = entry->next;
      temp = new_table[entry->key_hashed % new_length];
      new_table[entry->key_hashed % new_length] = entry;
      entry->next = temp;
      entry = next;
    }
  }

  /* free old table and replace it */
  CosmMemFree( hashtable->table );
  hashtable->table = new_table;
  hashtable->table_length = new_length;
  hashtable->table_index++;

  return;
}

/* testing */

s32 Cosm_TestHashTable( void )
{
  cosm_HASH_TABLE ht;
  
  CosmMemSet( &ht, sizeof( ht ), 0 );
  
  /*
  CosmHashTableInit( &ht, 4, NULL, NULL, NULL, NULL );
  */
  
  return COSM_PASS;
}
