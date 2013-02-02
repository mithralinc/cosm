/*
  Copyright 1995-2012 Mithral Communications & Design Inc.

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

#include "cosm/buffer.h"
#include "cosm/os_math.h"
#include "cosm/os_mem.h"

s32 CosmBufferInit( cosm_BUFFER * buffer, u64 size, u32 mode, u64 grow,
  const void * const data, u64 length )
{
  if ( buffer == NULL )
  {
    return COSM_BUFFER_ERROR_PARAM;
  }

  if ( buffer->mode != COSM_BUFFER_MODE_NONE )
  {
    return COSM_BUFFER_ERROR_FULL;
  }

  if ( ( mode != COSM_BUFFER_MODE_QUEUE ) && ( mode != COSM_BUFFER_MODE_STACK ) )
  {
    return COSM_BUFFER_ERROR_MODE;
  }

  CosmMemSet( buffer, sizeof( cosm_BUFFER ), 0 );

  if ( ( buffer->memory = CosmMemAlloc( size ) ) == NULL )
  {
    return COSM_BUFFER_ERROR_MEMORY;
  }

  /* Simply set the contents of the struct acording to arguments. */
  buffer->mem_length = size;
  buffer->mode = mode;
  buffer->head = (u64) 0;
  buffer->tail = (u64) 0;
  buffer->data_length = (u64) 0;
  buffer->grow_size = grow;

  if ( data != NULL )
  {
    return Cosm_BufferPut( buffer, data, length );
  }

  return COSM_PASS;
}

u64 CosmBufferLength( cosm_BUFFER * buffer )
{
  if ( buffer == NULL )
  {
    return (u64) 0;
  }

  return buffer->data_length;
}

s32 CosmBufferClear( cosm_BUFFER * buffer )
{
  if ( buffer == NULL )
  {
    return COSM_BUFFER_ERROR_PARAM;
  }

  buffer->data_length = (u64) 0;
  buffer->head = (u64) 0;
  buffer->tail = (u64) 0;

  return COSM_PASS;
}

s32 CosmBufferPut( cosm_BUFFER * buffer, const void * data, u64 length )
{
  if ( buffer == NULL )
  {
    return COSM_BUFFER_ERROR_PARAM;
  }

  return Cosm_BufferPut( buffer, data, length );
}

u64 CosmBufferGet( void * data, u64 length, cosm_BUFFER * buffer )
{
  u64 grab;
  u64 at_start, at_end, at_tail, at_head;
  u64 new_head;
  u8 * memory;

  if ( ( buffer == NULL ) || ( ( length == 0 ) ) )
  {
    return (u64) 0;
  }

  memory = (u8 *) buffer->memory;

  /* grab smaller of wanted and available */
  if ( ( length > buffer->data_length ) )
  {
    grab = buffer->data_length;
  }
  else
  {
    grab = length;
  }

  switch ( buffer->mode )
  {
    case COSM_BUFFER_MODE_QUEUE:
    {
      /* FIFO, take data starting at tail */
      if ( ( buffer->tail < buffer->head ) )
      {
        /* not wrapped */
        at_tail = grab;
        at_start = (u64) 0;
      }
      else
      {
        /* we are wrapped */
        at_tail = ( buffer->mem_length - buffer->tail );
        if ( ( at_tail < grab ) )
        {
          /* we need more from head */
          at_start = ( grab - at_tail );
        }
        else
        {
          /* only need grab from tail */
          at_tail = grab;
          at_start = (u64) 0;
        }
      }

      /* Start at tail */
      if ( CosmMemCopy( data, CosmMemOffset( memory, buffer->tail ), at_tail )
        == COSM_FAIL )
      {
        return 0;
      }

      /* move tail, then check for wrap */
      buffer->tail = ( buffer->tail + at_tail );
      if ( !( buffer->mem_length > buffer->tail ) )
      {
        buffer->tail = 0;
      }

      /* Add any wrapped data */
      if ( ( at_start > 0 ) )
      {
        if ( CosmMemCopy( CosmMemOffset( data, at_tail ),
          memory, at_start ) == COSM_FAIL )
        {
          /* return what we already copied out */
          buffer->data_length = ( buffer->data_length - at_tail );
          return at_tail;
        }

        /* We are wrapping our tail. */
        buffer->tail = at_start;
      }

      buffer->data_length = ( buffer->data_length - grab );
      return grab;
    }
    case COSM_BUFFER_MODE_STACK:
    {
      /* LIFO, take data from head */
      if ( ( grab > buffer->head ) )
      {
        /* we'll need more from the tail */
        at_head = buffer->head;
        at_end = ( grab - at_head );
      }
      else
      {
        /* all we need is at the head */
        at_head = grab;
        at_end = (u64) 0;
      }

      /* Move the at_head */
      if ( CosmMemCopy( CosmMemOffset( data, at_end ),
        CosmMemOffset( memory, ( buffer->head - at_head ) ), at_head )
        == COSM_FAIL )
      {
        return (u64) 0;
      }
      new_head = ( buffer->head - at_head );

      /* and do the at_end */
      if ( ( at_end > 0 ) )
      {
        new_head = ( buffer->mem_length - at_end );

        if ( CosmMemCopy( data, CosmMemOffset( memory, new_head ),
          at_end ) == COSM_FAIL )
        {
          /* !!! this is actually very very bad, buffer corrupted */
          return (u64) 0;
        }
      }

      buffer->head = new_head;
      buffer->data_length = ( buffer->data_length - grab );
      return grab;
    }
    default:
      return COSM_BUFFER_ERROR_MODE;
      break;
  }
}

s32 CosmBufferUnget( cosm_BUFFER * buffer, const void * data, u64 length )
{
  u64 at_tail, at_end;
  s32 error;

  if ( buffer == NULL )
  {
    return COSM_BUFFER_ERROR_PARAM;
  }

  switch ( buffer->mode )
  {
    case COSM_BUFFER_MODE_STACK:
    {
      /* just put it back */
      return Cosm_BufferPut( buffer, data, length );
    }
    case COSM_BUFFER_MODE_QUEUE:
    {
      /* prepend data before tail */

      if ( ( error = Cosm_BufferGrow( buffer, length ) ) != COSM_PASS )
      {
        return error;
      }

      /* there is now room for the data */
      if ( ( length > buffer->tail ) )
      {
        /* No room before the tail, wrap it */
        at_tail = buffer->tail;
        at_end = ( length - at_tail );
      }
      else
      {
        /* Room before tail */
        at_tail = length;
        at_end = (u64) 0;
      }

      /* Add to the tail */
      buffer->tail = ( buffer->tail - at_tail );

      if ( CosmMemCopy( CosmMemOffset( buffer->memory, buffer->tail ),
        CosmMemOffset( data, at_end ), at_tail ) == COSM_FAIL )
      {
        return COSM_BUFFER_ERROR_MEMORY;
      }

      /* Add to the end of the buffer, we are now wrapped */
      if ( ( at_end > 0 ) )
      {
        buffer->tail = ( buffer->mem_length - at_end );

        if ( CosmMemCopy( CosmMemOffset( buffer->memory, buffer->tail ),
          data, at_end ) )
        {
          return COSM_BUFFER_ERROR_MEMORY;
        }
      }

      buffer->data_length = ( buffer->data_length + length );

      return COSM_PASS;
    }
    default:
    {
      return COSM_BUFFER_ERROR_MODE;
    }
  }
}

void CosmBufferFree( cosm_BUFFER * buffer )
{
  if ( buffer == NULL )
  {
    return;
  }

  CosmMemFree( buffer->memory );

  CosmMemSet( buffer, sizeof( cosm_BUFFER ), 0 );
}

/* low-level functions */

s32 Cosm_BufferGrow( cosm_BUFFER * buffer, u64 length )
{
  u64 div, mod, amount, add;
  void * mem_temp;

  amount = ( length + buffer->data_length );

  /* grow memory memory if we need to */
  if ( ( amount > buffer->mem_length ) )
  {
    if ( buffer->grow_size == 0 )
    {
      return COSM_BUFFER_ERROR_FULL;
    }

    /* grow in multiples of grow_size */
    CosmU64DivMod( amount, buffer->grow_size, &div, &mod );
    if ( mod != 0 )
    {
      div++;
    }
    amount = ( div * buffer->grow_size );

    if ( ( mem_temp = CosmMemRealloc( buffer->memory, amount ) ) == NULL )
    {
      return COSM_BUFFER_ERROR_MEMORY;
    }

    /* is the buffer was full, and head is 0, fix it */
    if ( ( ( buffer->head == 0 ) )
      && ( ( buffer->data_length > 0 ) ) )
    {
      buffer->head = buffer->mem_length;
    }

    /* put the tail section at the end again if we are wrapped */
    if ( !( buffer->tail < buffer->head ) )
    {
      add = ( amount - buffer->mem_length );

      CosmMemCopy( CosmMemOffset( mem_temp, ( buffer->tail + add ) ),
        CosmMemOffset( mem_temp, buffer->tail ),
        ( buffer->mem_length - buffer->tail ) );

      buffer->tail = ( buffer->tail + add );
    }

    buffer->memory = mem_temp;
    buffer->mem_length = amount;
  }

  return COSM_PASS;
}

s32 Cosm_BufferPut( cosm_BUFFER * buffer, const void * data, u64 length )
{
  u64 space, at_start, at_head;
  s32 error;

  /* check if we're already full */
  if ( ( error = Cosm_BufferGrow( buffer, length ) ) != COSM_PASS )
  {
    return error;
  }

  /* how much space is after head? */
  space = ( buffer->mem_length - buffer->head );

  if ( ( space < length ) )
  {
    /* Wrap */
    at_head = space;
    at_start = ( length - at_head );
  }
  else
  {
    /* No wrap */
    at_head = length;
    at_start = (u64) 0;
  }

  /* Add to head */
  if ( CosmMemCopy( CosmMemOffset( buffer->memory, buffer->head ),
    data, at_head ) == COSM_FAIL )
  {
    return COSM_BUFFER_ERROR_MEMORY;
  }

  /* check for head wrap */
  buffer->head = ( buffer->head + at_head );
  if ( !( buffer->mem_length > buffer->head ) )
  {
    buffer->head = 0;
  }

  /* Add to buffer start if needed */
  if ( ( at_start > 0 ) )
  {
    if ( CosmMemCopy( buffer->memory, CosmMemOffset( data, at_head ),
      at_start ) == COSM_FAIL )
    {
      return COSM_BUFFER_ERROR_MEMORY;
    }

    buffer->head = at_start;
  }

  buffer->data_length = ( buffer->data_length + length );

  return COSM_PASS;
}

/* testing */

s32 Cosm_TestBuffer( void )
{
  cosm_BUFFER srcQ1, srcQ2, srcS3, dstQ1, dstS2;
  u32 item1, item2, item3, item4; /* Items for the queue */
  u64 large_item1, large_item2; /* Larger items. */
  u32 i;

  /* Tests totally rewritten.. */

  /* Philsophy of the tests.
   *   A Create some source buffers.
   *   B Fill those buffers.
   *   C Examine them.
   *   D Shuffle from source to destination
   *   E Examine them.
   *   F Get data out of them.
   *   G Check for sizes
   *   H Destruct it all.
   *   I Test some special cases, BufferCreate with item for one..
   *  ... And yes I know they are incomplete.. But they will always
   *   be..
   **/

  /* Assumptions :
   *  1) We will not run out of memory for any natural reasons.
   *   A machine that runs  out of memory during these tests is
   *   useless to us anyhow.
   *  2) The machine is sane. If the machine is insane there is no
   *   way of knowing if the code is.
   */

  /* Test section A - Create some source buffers.*/

  CosmMemSet( &srcQ1, sizeof( cosm_BUFFER ), 0 );
  CosmMemSet( &srcQ2, sizeof( cosm_BUFFER ), 0 );
  CosmMemSet( &srcS3, sizeof( cosm_BUFFER ), 0 );
  CosmMemSet( &dstQ1, sizeof( cosm_BUFFER ), 0 );
  CosmMemSet( &dstS2, sizeof( cosm_BUFFER ), 0 );

  if ( ( CosmBufferInit( &srcQ1, (u64) 10, COSM_BUFFER_MODE_QUEUE,
    (u64) 10, NULL, 0 ) != COSM_PASS ) )
  {
    return -1;
  }

  if ( ( CosmBufferInit( &srcQ2, (u64) 10, COSM_BUFFER_MODE_QUEUE,
    (u64) 20, NULL, 0 ) != COSM_PASS ) )
  {
    return -2;
  }

  if ( ( CosmBufferInit( &srcS3, (u64) 80, COSM_BUFFER_MODE_STACK,
    (u64) 0, NULL, 0 ) != COSM_PASS ) )
    /* This one cannot grow. */
  {
    return -3;
  }

  if ( ( CosmBufferInit( &dstQ1, (u64) 1000, COSM_BUFFER_MODE_QUEUE,
    (u64) 0, NULL, 0 ) != COSM_PASS ) )
  {
    return -4;
  }

  if ( ( CosmBufferInit( &dstS2, (u64) 20, COSM_BUFFER_MODE_STACK,
    (u64) 2, NULL, 0 ) != COSM_PASS ) )
  {
    return -5;
  }

  /*
   * Test section B - Fill those buffers.
   */

  /* First stuff some small data into the first and third pipe.
   * and some slightly larger data down the second. */
  for ( i = 1; i < 21; i++ )
  {
    item1 = i;
    item2 = i + 100; /* Let data differ so we can separate it.*/
    large_item1 = (u64) i * 23000LL;

    if ( CosmBufferPut( &srcQ1, &item1, sizeof( item1 ) )
      != COSM_PASS )
    {
      return -6;
    }

    if ( CosmBufferPut( &srcQ2, &large_item1,
      sizeof( large_item1 ) )  != COSM_PASS )
    {
      return -7;
    }

    if ( CosmBufferPut( &srcS3, &item2,
      sizeof( item2 ) )  != COSM_PASS )
    {
      return -8;
    }


  }
  /*
   * Test section C - Examine them.
   */

  /* First sizes.*/
  if ( CosmBufferLength( &srcQ1 ) != 80LL )
  {
    return -9;
  }
  if ( CosmBufferLength( &srcQ2 ) != 160LL )
  {
    return -10;
  }
  if ( CosmBufferLength( &srcS3 ) != 80LL )
  {
    return -11;
  }


  /* srcS3 should be full, we test that.*/
  if ( CosmBufferPut( &srcS3, &item2,
    sizeof( item2 ) ) != COSM_BUFFER_ERROR_FULL  )
  {
    return -12;
  }
  if ( CosmBufferLength( &srcS3 ) != 80LL )
  {
    return -13;
  }

  /* Then we sample some data from each. */
  if ( CosmBufferGet( &item1, sizeof( item1 ), &srcQ1 ) != sizeof( item1 ) )
  {
    return -14;
  }
  if ( item1 != 1 ) /* First item entered. */
  {
    return -15;
  }
  if ( CosmBufferUnget( &srcQ1, &item1, sizeof( item1 ) )
    != COSM_PASS )
  {
    return -16;
  }

  if ( CosmBufferGet( &large_item1, sizeof( large_item1 ), &srcQ2 )
    != sizeof( large_item1 ) )
  {
    return -17;
  }
  if ( large_item1 != 23000LL ) /* First item entered. */
  {
    return -18;
  }
  if ( CosmBufferUnget( &srcQ2, &large_item1,
    sizeof( large_item1 ) ) != COSM_PASS )
  {
    return -19;
  }

  if ( CosmBufferGet( &item1, sizeof( item1 ), &srcS3 )
    != sizeof( item1 ) )
  {
    return -20;
  }
  if ( item1 != 120 ) /* Last item entered. */
  {
    return -21;
  }
  if ( CosmBufferUnget( &srcS3, &item1, sizeof( item1 ) )
    != COSM_PASS )
  {
    return -22;
  }

  /*
   * Test section D - Shuffle from source to destination
   */
  /* There are 20 items in each buffer.*/
  for ( i=0; i<20; i++ )
  {
    if ( CosmBufferGet( &item1, sizeof( item1 ), &srcQ1 )
      != sizeof( item1 ) )
    {
      return -23;
    }
    if ( CosmBufferGet( &large_item1, sizeof( large_item1 ), &srcQ2 )
      != sizeof( large_item1 ) )
    {
      return -24;
    }
   if ( CosmBufferGet( &item2, sizeof( item2 ), &srcS3 )
      != sizeof( item2 ) )
    {
      return -25;
    }

    if ( item1 != ( i + 1 ) )
    {
      return -26;
    }
    if ( item2 != ( (20-i) + 100 ) )
    {
      return -27;
    }
    if ( large_item1 != (u64) (i+1) * 23000LL )
    {
      return -28;
    }

    /* Everything is fine. Push into dsts.*/
    if ( CosmBufferPut( &dstQ1, &item1, sizeof( item1 ) ) !=
      COSM_PASS )
    {
      return -29;
    }
    if ( CosmBufferPut( &dstS2, &item1, sizeof( item1 ) ) !=
      COSM_PASS )
    {
      return -30;
    }

    if ( CosmBufferPut( &dstQ1, &item2, sizeof( item2 ) ) !=
      COSM_PASS )
    {
      return -31;
    }
    if ( CosmBufferPut( &dstS2, &item2, sizeof( item2 ) ) !=
      COSM_PASS )
    {
      return -32;
    }

    if ( CosmBufferPut( &dstQ1, &large_item1,
       sizeof( large_item1 ) ) != COSM_PASS )
    {
      return -33;
    }
    if ( CosmBufferPut( &dstS2, &large_item1,
       sizeof( large_item1 ) ) != COSM_PASS )
    {
      return -34;
    }
  }

  /*
   * Test section E - Examine them.
   */

  /* src buffers should now be empty! And the destinations
   * quite loaded. */
  if ( CosmBufferLength( &srcQ1 ) != 0 )
  {
    return -35;
  }
  if ( CosmBufferLength( &srcQ2 ) != 0 )
  {
    return -36;
  }
  if ( CosmBufferLength( &srcS3 ) != 0 )
  {
    return -37;
  }
  if ( CosmBufferLength( &dstQ1 ) != 80LL + 80 + 160 )
  {
    return -38;
  }
  if ( CosmBufferLength( &dstS2 ) != 80LL + 80 + 160 )
  {
    return -39;
  }

  /*
   * Test section F -  Shuffle some data crosswise
   *   We move from dst buffers back to src, using unget's.
   */
  for ( i = 0; i < 20; i++ )
  {

    if ( CosmBufferGet( &item1, sizeof( item1 ), &dstQ1 )
      != sizeof( item1 ) )
    {
      return -40;
    }
    if ( CosmBufferGet( &item2, sizeof( item2 ), &dstQ1 )
      != sizeof( item2 ) )
    {
      return -41;
    }
    if ( CosmBufferGet( &large_item1, sizeof( large_item1 ), &dstQ1 )
      != sizeof( large_item1 ) )
    {
      return -42;
    }

    /* This set of gets is backwards since it is a LIFO. */
    if ( CosmBufferGet( &large_item2, sizeof( large_item2 ), &dstS2 )
      != sizeof( large_item2 ) )
    {
      return -43;
    }
    if ( CosmBufferGet( &item4, sizeof( item3 ), &dstS2 )
      != sizeof( item3 ) )
    {
      return -44;
    }
    if ( CosmBufferGet( &item3, sizeof( item4 ), &dstS2 )
      != sizeof( item4 ) )
    {
      return -45;
    }

    /*
     * Check that data is as expected.
     */

    /* item1,item2 and large_item1 belong to dstQ1  */
    if ( item1 != ( i + 1 ) )
    {
      return -46;
    }

    /* Backwards since it has been stored in srcS3. */
    if ( item2 != ( ( 20 - i ) + 100 ) )
    {
      return -47;
    }

    if ( large_item1 != (u64) ( i + 1 ) * 23000LL )
    {
      return -48;
    }

    /* item3, item4 and large_item2 belong to dstS2.
     * These are backwards. */
    if ( item3 != ( 20 - i ) )
    {
      return -49;
    }

    /* Backwards backwards. Been in both dstS2 and srcS3 */
    if ( item4 != ( i + 101 ) )
    {
      return -50;
    }
    if ( large_item2 != ( 20ULL - i ) * 23000ULL )
    {
      return -51;
    }
  }

  /*
   * Test section G - Check for sizes.
   */

  /* We checked src buffers above, no changes have been made to them. */
  if ( CosmBufferLength( &dstQ1 ) != 0 )
  {
    return -52;
  }
  if ( CosmBufferLength( &dstS2 ) != 0 )
  {
    return -53;
  }

  /*
   * Test section H - Destruct it all
   */
  CosmBufferFree( &srcQ1 );
  CosmBufferFree( &srcQ2 );
  CosmBufferFree( &srcS3 );
  CosmBufferFree( &dstQ1 );
  CosmBufferFree( &dstS2 );

  /*
   * Test section I - Test some special cases
   */

  /* I.1 Testing BufferCreate with item as argument.*/
  large_item1 = 321LL;
  if ( CosmBufferInit( &srcS3, (u64) 10, COSM_BUFFER_MODE_STACK,
    (u64) 29, &large_item1, sizeof( large_item1 ) )
    != COSM_PASS )
  {
    return -54;
  }

  for ( i = 0 ; i < 10 ; i++ )
  {
    item1 = i * 3;
    if ( CosmBufferPut( &srcS3, &item1, sizeof( item1 ) )
      != COSM_PASS )
    {
      return -55;
    }
  }

  for ( i = 0; i < 10; i++ )
  {
    if ( CosmBufferGet( &item1, sizeof( item1 ), &srcS3 )
      != sizeof( item1 ) )
    {
      return -56;
    }
  }

  CosmBufferFree( &srcS3 );

  return COSM_PASS;
}
