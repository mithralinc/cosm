/*
  This software is available as part of this Package under and subject to the
  terms of the Mithral License A or Mithral License B where "Creator" and
  "Package" are defined as:

  Creator(s): Mithral Communications & Design, Inc.
  Package: Cosm Libraries - Utility Layer

  A copy of the license(s) is enclosed with this Package and by using this
  Package you agree to the license terms. The Package is Copyright (C)
  1995-2007 by Creator. All rights reserved. Further information about the
  Package and pricing information can be found at the Creator's web site:
  http://www.mithral.com/
*/

#ifndef COSM_HASHTABLE_H
#define COSM_HASHTABLE_H

#include "cputypes.h"

typedef struct cosm_HASH_TABLE_ENTRY
{
  void * key;
  void * value;
  struct cosm_HASH_TABLE_ENTRY * next;
  u64 key_hashed;
} cosm_HASH_TABLE_ENTRY;

#define COSM_HASH_TABLE_STATE_NONE   0
#define COSM_HASH_TABLE_STATE_INIT   72874

typedef struct cosm_HASH_TABLE
{
  u32 state;
  u32 table_index;
  u64 count;
  u64 (*hash)( void * );
  s32 (*equal)( void *, void * );
  void * (*dup_key)( void * );
  void * (*dup_value)( void * );
  cosm_HASH_TABLE_ENTRY ** table;
  cosm_HASH_TABLE_ENTRY * iterator_next;
  u64 table_length;
  u64 iterator_row;
} cosm_HASH_TABLE;

s32 CosmHashTableInit( cosm_HASH_TABLE * hashtable, u64 minimum_size,
  u64 (*hash)( void * ), s32 (*equal)( void *, void * ),
  void * (*dup_key)( void * ), void * (*dup_value)( void * ) );
  /*
    Initialize the hash table with the expectation that at least minimum_size
    entries will be added. The table will grow automaticly, but if you can
    predict the needed size, then performance will be ideal.
    The hash function hashes a key into a u64 integer value.
    The equal function should completely compare two keys and return 1
    if they are equal, 0 if they are not.
    For hash tables pointing at existing keys and values, you do not need to
    have dup_key and dup_value. If you are planning to have the hash table
    manage either or both of the keys and values, then you need to have the
    duplication functions. When entries are deleted, duplicated data will
    also be freed.
    Each entry takes at least 20 bytes plus the keys and values, the practical
    limit is about 50M entries on a 32bit machine.
    Returns: COSM_PASS on success, or COSM_FAIL on parameter/memory failure.
  */

s32 CosmHashTableCount( u64 * count, cosm_HASH_TABLE * hashtable );
  /*
    Sets count to the number of entries in the hash table.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmHashTableAdd( cosm_HASH_TABLE * hashtable,
  void * key, void * value );
  /*
    Add the key/value entry to the hash table. If the new key is not
    uniquu and already exists in the hash table, the function fails.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmHashTableUpdate( cosm_HASH_TABLE * hashtable,
  void * key, void * value );
  /*
    Set a new value for the key. If the old value was duplicated, it
    is freed.
    Returns: COSM_PASS on success, or COSM_FAIL if key is not found.
  */

void * CosmHashTableValue( cosm_HASH_TABLE * hashtable, void * key );
  /*
    Get the value pointer for a key.
    Returns: pointer to value on success, or NULL if key is not found.
  */

void CosmHashTableDelete( cosm_HASH_TABLE * hashtable, void * key );
  /*
    Remove the entry associated with the key, and duplicated data if needed.
    If the key is not found no action is taken.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 CosmHashTableStart( cosm_HASH_TABLE * hashtable );
  /*
    Prepare the hash table to be iterated over. This must be called before
    CosmHashTableNext() is used.
    Returns: COSM_PASS on success, COSM_FAIL on invalid/empty hash table.
  */

s32 CosmHashTableNext( void ** key, void ** value,
  cosm_HASH_TABLE * hashtable );
  /*
    Get the next key/value as we iterate through the hash table.
    The order of the returned entries will appear random.
    Returns: nothing.
  */

void CosmHashTableFree( cosm_HASH_TABLE * hashtable );
  /*
    Free the hash table and all duplicated data.
    Returns: nothing.
  */

void Cosm_HashTableGrow( cosm_HASH_TABLE * hashtable );
  /*
    Grow the hash table to the next prime if memory allows.
    If this function fails, we'll just keep chaining values until memory
    runs out, so performance will degrade before it is full.
    Returns: nothing.
  */

/* testing */

s32 Cosm_TestHashTable( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
