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

#include "cosm/transform.h"
#include "cosm/os_mem.h"
#include "cosm/os_net.h"
#include "cosm/os_file.h"
#include "cosm/buffer.h"

s32 CosmTransformInit( cosm_TRANSFORM * transform,
  s32 (*init)( cosm_TRANSFORM *, va_list ),
  s32 (*encode)( cosm_TRANSFORM *, const void * const, u64 ),
  s32 (*end)( cosm_TRANSFORM * ),
  cosm_TRANSFORM * next_transform, ... )
{
  va_list params;
  s32 result;

  if ( ( transform == NULL ) || ( init == NULL ) || ( encode == NULL )
    || ( end == NULL ) )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  /* if we have a next, it has to be setup/running already */
  if ( ( next_transform != NULL )
    && ( ( next_transform->cookie != COSM_TRANSFORM_COOKIE )
    || ( ( next_transform->state != COSM_TRANSFORM_STATE_INIT )
    && ( next_transform->state != COSM_TRANSFORM_STATE_DATA ) ) ) )
  {
    return COSM_TRANSFORM_ERROR_NEXT;
  }

  if ( transform->tmp_data != NULL )
  {
    /* dont want to lose a working stream */
    return COSM_TRANSFORM_ERROR_STATE;
  }

  transform->cookie = COSM_TRANSFORM_COOKIE;
  transform->state = COSM_TRANSFORM_STATE_INIT;
  transform->init = (s32 (*)( void *, va_list )) init;
  transform->encode = (s32 (*)( void *, const void * const, u64 )) encode;
  transform->end = (s32 (*)( void * )) end;
  transform->next_transform = next_transform;

  va_start( params, next_transform );
  if ( ( result = (*init)( transform, params ) ) != COSM_PASS )
  {
    transform->state = COSM_TRANSFORM_STATE_ERROR;
    va_end( params );
    return result;
  }
  va_end( params );

  return COSM_PASS;
}

s32 CosmTransform( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  s32 result;

  if ( ( transform == NULL ) || ( transform->cookie != COSM_TRANSFORM_COOKIE )
    || ( data == NULL ) )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  if ( ( transform->state != COSM_TRANSFORM_STATE_DATA )
    && ( transform->state != COSM_TRANSFORM_STATE_INIT ) )
  {
    return COSM_TRANSFORM_ERROR_STATE;
  }

  if ( ( result = (*transform->encode)( transform, data, length ) )
    != COSM_PASS )
  {
    transform->state = COSM_TRANSFORM_STATE_ERROR;
    return result;
  }

  transform->state = COSM_TRANSFORM_STATE_DATA;

  return COSM_PASS;
}

s32 CosmTransformEnd( cosm_TRANSFORM * transform )
{
  s32 result;

  if ( ( transform == NULL )
    || ( transform->cookie != COSM_TRANSFORM_COOKIE ) )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  /* we had a bad error, need to cleanup */
  if ( transform->state == COSM_TRANSFORM_STATE_ERROR )
  {
    (*transform->end)( transform );

    transform->state = COSM_TRANSFORM_STATE_DONE;
    CosmMemFree( transform->tmp_data );
    transform->tmp_data = NULL;
    return COSM_PASS;
  }

  if ( ( ( transform->state != COSM_TRANSFORM_STATE_INIT )
    && ( transform->state != COSM_TRANSFORM_STATE_DATA ) )
    || ( transform->tmp_data == NULL ) )
  {
    transform->state = COSM_TRANSFORM_STATE_ERROR;
    return COSM_TRANSFORM_ERROR_STATE;
  }

  if ( ( result = (*transform->end)( transform ) ) != COSM_PASS )
  {
    transform->state = COSM_TRANSFORM_STATE_ERROR;
    return result;
  }

  transform->state = COSM_TRANSFORM_STATE_DONE;
  transform->tmp_data = NULL;

  return COSM_PASS;
}

s32 CosmTransformEndAll( cosm_TRANSFORM * transform )
{
  cosm_TRANSFORM * next;
  s32 error, tmp;

  error = COSM_PASS;
  next = transform;
  while ( next != NULL )
  {
    if ( ( tmp = CosmTransformEnd( next ) ) != COSM_PASS )
    {
      if ( error == COSM_PASS )
      {
        error = tmp;
      }
    }
    next = next->next_transform;
  }

  return COSM_PASS;
}

/* Base64 code */

typedef struct cosm_BASE64_TMP
{
  u32 count;
  u32 value;
  ascii line[80]; /* 57 or 4 used */
} cosm_BASE64_TMP;

/* tables for fast encode and decode */

static const ascii base64_encode[64] =
{
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};

static const u8 base64_decode[128] =
{
  99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 62, 99, 99, 99, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 99, 99, 99,  0, 99, 99,
  99,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 99, 99, 99, 99, 99,
  99, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 99, 99, 99, 99, 99
};

s32 Cosm_Base64CInit( cosm_TRANSFORM * transform, va_list params )
{
  transform->tmp_data = CosmMemAlloc( sizeof( cosm_BASE64_TMP ) );
  if ( transform->tmp_data == NULL )
  {
    return COSM_TRANSFORM_ERROR_MEMORY;
  }

  /* we require an outlet */
  if ( transform->next_transform == NULL )
  {
    return COSM_TRANSFORM_ERROR_NEXT;
  }

  return COSM_PASS;
}

s32 Cosm_Base64Comp( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_BASE64_TMP * base;
  u32 count;
  const u8 * ptr;
  const ascii * in;
  ascii tmp_out[80];
  ascii * out;
  u32 value, i;
  s32 result;

  base = transform->tmp_data;
  count = base->count;

  /* check for less then a whole line */
  if ( ( (u64) count + length ) < 57LL )
  {
    CosmMemCopy( &base->line[count], data, length );
    base->count += (u32) length;
    return COSM_PASS;
  }

  ptr = (u8 *) data;

  while ( ( (u64) count + length ) > 56LL )
  {
    /* fill tmp buffer */
    for ( i = count ; i < 57 ; i++ )
    {
      base->line[i] = *(ptr++);
    }
    length = ( length - 57LL - count );
    count = 0;

    /* encode and write to buffer */
    in = base->line;
    out = tmp_out;
    for ( i = 0 ; i < 19 ; i++ )
    {
      /* read 3 bytes */
      value = ( (u32) in[0] << 16 ) + ( (u32) in[1] << 8 ) + (u32) in[2];
      in = &in[3];

      /* make into 4 */
      out[3] = base64_encode[value & 0x3F];
      value >>= 6;
      out[2] = base64_encode[value & 0x3F];
      value >>= 6;
      out[1] = base64_encode[value & 0x3F];
      value >>= 6;
      out[0] = base64_encode[value & 0x3F];
      out = &out[4];
    }

    /* we need to feed the data to the next transform */
    if ( ( result = CosmTransform( transform->next_transform,
      tmp_out, 76LL ) ) != COSM_PASS )
    {
      return result;
    }
  }

  CosmMemCopy( &base->line[base->count], ptr, length );
  base->count = (u32) length;

  return COSM_PASS;
}

s32 Cosm_Base64CEnd( cosm_TRANSFORM * transform )
{
  cosm_BASE64_TMP * base;
  const ascii * in;
  ascii tmp_out[80];
  ascii * out;
  u32 len;
  u32 value;
  s32 result;

  base = transform->tmp_data;
  len = 0;

  in = base->line;
  out = tmp_out;
  while ( base->count > 2 )
  {
    value = ( (u32) in[0] << 16 ) + ( (u32) in[1] << 8 ) + (u32) in[2];
    base->count -= 3;
    in = &in[3];

    /* make into 4 */
    out[3] = base64_encode[value & 0x3F];
    value >>= 6;
    out[2] = base64_encode[value & 0x3F];
    value >>= 6;
    out[1] = base64_encode[value & 0x3F];
    value >>= 6;
    out[0] = base64_encode[value & 0x3F];
    out = &out[4];
    len += 4;
  }

  if ( base->count > 0 )
  {
    /* terminate and pad */
    if ( base->count == 2 )
    {
      value = ( (u32) in[0] << 16 ) + ( (u32) in[1] << 8 );
      out[3] = '=';
      value >>= 6;
      out[2] = base64_encode[value & 0x3F];
    }
    else /* 1 */
    {
      value = (u32) in[0] << 16;
      out[3] = '=';
      out[2] = '=';
      value >>= 6;
    }

    value >>= 6;
    out[1] = base64_encode[value & 0x3F];
    value >>= 6;
    out[0] = base64_encode[value & 0x3F];
    out = &out[4];

    len += 4;
  }

  /* we need to feed the data to the next transform */
  if ( ( result = CosmTransform( transform->next_transform, tmp_out,
    (u64) len ) ) != COSM_PASS )
  {
    return result;
  }

  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}

s32 Cosm_Base64DInit( cosm_TRANSFORM * transform, va_list params )
{
  transform->tmp_data = CosmMemAlloc( sizeof( cosm_BASE64_TMP ) );
  if ( transform->tmp_data == NULL )
  {
    return COSM_TRANSFORM_ERROR_STATE;
  }

  /* we require an outlet */
  if ( transform->next_transform == NULL )
  {
    return COSM_TRANSFORM_ERROR_NEXT;
  }

  return COSM_PASS;
}

s32 Cosm_Base64Decomp( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_BASE64_TMP * base;
  const u8 * ptr;
  u8 tmp_out[4];
  u32 pad = 0;
  s32 result;

  base = transform->tmp_data;
  ptr = (u8 *) data;

  while ( ( length > 0 ) )
  {
    /* we ignore any wierd characters in the stream including linefeeds */
    if ( ( *ptr < 128 ) && ( base64_decode[*ptr] != 99 ) )
    {
      base->value <<= 6;
      base->value += base64_decode[*ptr];
      base->count++;
      /* any padding? */
      if ( *ptr == '=' )
      {
        pad++;
      }
    }

    if ( base->count == 4 )
    {
      tmp_out[2] = (u8) base->value;
      tmp_out[1] = (u8) ( base->value >> 8 );
      tmp_out[0] = (u8) ( base->value >> 16 );

      /* we need to feed the data to the next transform */
      if ( ( result = CosmTransform( transform->next_transform, tmp_out,
        (u64) 3 - pad ) ) != COSM_PASS )
      {
        return result;
      }
      base->count = 0;
    }

    ptr++;
    length--;
  }

  return COSM_PASS;
}

s32 Cosm_Base64DEnd( cosm_TRANSFORM * transform )
{
  cosm_BASE64_TMP * base;

  base = transform->tmp_data;

  if ( base->count != 0 )
  {
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  CosmMemFree( transform->tmp_data );

  return COSM_PASS;
}

/* very common transforms */

s32 Cosm_TransformToMemInit( cosm_TRANSFORM * transform, va_list params )
{
  transform->tmp_data = va_arg( params, void * );

  if ( transform->tmp_data == NULL )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  return COSM_PASS;
}

s32 Cosm_TransformToMem( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  u8 * memory = (u8 *) transform->tmp_data;
  s32 result;

  if ( CosmMemCopy( memory, data, length ) != COSM_PASS )
  {
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  transform->tmp_data = CosmMemOffset( memory, length );

  if ( transform->next_transform != NULL )
  {
    if ( ( result = CosmTransform( transform->next_transform, data, length ) )
      != COSM_PASS )
    {
      return result;
    }
  }

  return COSM_PASS;
}

s32 Cosm_TransformToMemEnd( cosm_TRANSFORM * transform )
{
  return COSM_PASS;
}

s32 Cosm_TransformToFileInit( cosm_TRANSFORM * transform, va_list params )
{
  cosm_FILE * file = va_arg( params, cosm_FILE * );

  /* check the file is open */
  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  transform->tmp_data = file;

  return COSM_PASS;
}

s32 Cosm_TransformToFile( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_FILE * file = (cosm_FILE *) transform->tmp_data;
  u64 written;
  s32 result;

  if ( CosmFileWrite( file, &written, data, length ) != COSM_PASS )
  {
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  if ( transform->next_transform != NULL )
  {
    if ( ( result = CosmTransform( transform->next_transform, data, length ) )
      != COSM_PASS )
    {
      return result;
    }
  }

  return COSM_PASS;
}

s32 Cosm_TransformToFileEnd( cosm_TRANSFORM * transform )
{
  return COSM_PASS;
}

s32 Cosm_TransformToNetInit( cosm_TRANSFORM * transform, va_list params )
{
  cosm_NET * net = va_arg( params, cosm_NET * );

  /* check the connect is open */
  if ( net->status != COSM_NET_STATUS_OPEN )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  transform->tmp_data = net;

  return COSM_PASS;
}

s32 Cosm_TransformToNet( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_NET * net = (cosm_NET *) transform->tmp_data;
  u32 sent;
  s32 result;
  u32 chunk;

  while ( length > 0 )
  {
    chunk = ( length > 0x10000000 ) ? 0x10000000 : (u32) length;
    if ( ( CosmNetSend( net, &sent, data, chunk ) != COSM_PASS )
      || ( sent != chunk ) )
    {
      return COSM_TRANSFORM_ERROR_FATAL;
    }
    length -= chunk;
  }

  if ( transform->next_transform != NULL )
  {
    if ( ( result = CosmTransform( transform->next_transform, data, length ) )
      != COSM_PASS )
    {
      return result;
    }
  }

  return COSM_PASS;
}

s32 Cosm_TransformToNetEnd( cosm_TRANSFORM * transform )
{
  return COSM_PASS;
}

s32 Cosm_TransformToBufferInit( cosm_TRANSFORM * transform, va_list params )
{
  cosm_BUFFER * buffer = va_arg( params, cosm_BUFFER * );

  if ( buffer->mode != COSM_BUFFER_MODE_QUEUE )
  {
    return COSM_TRANSFORM_ERROR_PARAM;
  }

  transform->tmp_data = buffer;

  return COSM_PASS;
}

s32 Cosm_TransformToBuffer( cosm_TRANSFORM * transform,
  const void * const data, u64 length )
{
  cosm_BUFFER * buffer = (cosm_BUFFER *) transform->tmp_data;
  s32 result;

  if ( CosmBufferPut( buffer, data, length ) != COSM_PASS )
  {
    return COSM_TRANSFORM_ERROR_FATAL;
  }

  if ( transform->next_transform != NULL )
  {
    if ( ( result = CosmTransform( transform->next_transform,
      data, length ) ) != COSM_PASS )
    {
      return result;
    }
  }

  return COSM_PASS;
}

s32 Cosm_TransformToBufferEnd( cosm_TRANSFORM * transform )
{
  return COSM_PASS;
}

/* self-test */

s32 Cosm_TestTransform( void )
{
  cosm_BUFFER buf;
  cosm_TRANSFORM trans_buff;
  cosm_TRANSFORM transform1;
  cosm_TRANSFORM transform2;
  ascii question[24] = "Aladdin:open sesame";
  ascii answer[32] = "QWxhZGRpbjpvcGVuIHNlc2FtZQ==";
  ascii answer2[48] = "UVd4aFpHUnBianB2Y0dWdUlITmxjMkZ0WlE9PQ==";
  ascii text[48];

  /* test an encoding base64 */
  CosmMemSet( &buf, sizeof( cosm_BUFFER ), 0 );
  CosmBufferInit( &buf, 1024, COSM_BUFFER_MODE_QUEUE, 1024, NULL, 0 );

  /* setup a buffer transform */
  CosmMemSet( &trans_buff, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &trans_buff, COSM_TRANSFORM_TO_BUFFER,
    NULL, &buf ) != COSM_PASS )
  {
    return -1;
  }

  /* now run our transforms, and chain to buffer */
  CosmMemSet( &transform1, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform1, COSM_BASE64_ENCODE, &trans_buff )
    != COSM_PASS )
  {
    return -2;
  }

  if ( CosmTransform( &transform1, question, 19LL ) != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -3;
  }

  if ( CosmTransformEnd( &transform1 ) != COSM_PASS )
  {
    CosmTransformEnd( &transform1 );
    CosmBufferFree( &buf );
    return -4;
  }

  CosmMemSet( text, sizeof( text ), 0 );
  if ( CosmBufferGet( text, (u64) 48, &buf ) != 28LL )
  {
    CosmBufferFree( &buf );
    return -5;
  }

  if ( CosmMemCmp( answer, text, 28LL ) != 0 )
  {
    CosmBufferFree( &buf );
    return -6;
  }

  /* test a decoding base64 */
  CosmMemSet( &transform2, sizeof( cosm_TRANSFORM ), 0 );
  if ( CosmTransformInit( &transform2, COSM_BASE64_DECODE, &trans_buff )
    != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -7;
  }

  if ( CosmTransform( &transform2, answer, 30LL ) != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -8;
  }

  if ( CosmTransformEnd( &transform2 ) != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -9;
  }

  CosmMemSet( text, sizeof( text ), 0 );
  if ( CosmBufferGet( text, (u64) 48, &buf ) != 19LL )
  {
    CosmBufferFree( &buf );
    return -10;
  }

  if ( CosmMemCmp( question, text, 19LL ) != 0 )
  {
    CosmBufferFree( &buf );
    return -11;
  }

  /* test a chain of 2 base64 transforms =  in -> 1 -> 2 - > buffer  */
  CosmMemSet( &transform1, sizeof( cosm_TRANSFORM ), 0 );
  CosmMemSet( &transform2, sizeof( cosm_TRANSFORM ), 0 );

  /* init 2 then 1 */
  if ( CosmTransformInit( &transform2, COSM_BASE64_ENCODE, &trans_buff )
    != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -12;
  }

  if ( CosmTransformInit( &transform1, COSM_BASE64_ENCODE, &transform2 )
    != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -13;
  }

  /* feed 1 */
  if ( CosmTransform( &transform1, question, 19LL ) != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -14;
  }

  /* end 1 then 2 */
  if ( CosmTransformEndAll( &transform1 ) != COSM_PASS )
  {
    CosmBufferFree( &buf );
    return -15;
  }

  CosmMemSet( text, sizeof( text ), 0 );
  if ( CosmBufferGet( text, (u64) 48, &buf ) != 40LL )
  {
    CosmBufferFree( &buf );
    return -16;
  }

  CosmBufferFree( &buf );

  if ( CosmMemCmp( answer2, text, 40LL ) != 0 )
  {
    return -17;
  }

  return COSM_PASS;
}
