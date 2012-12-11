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

#include "cosm/http.h"
#include "cosm/os_mem.h"
#include "cosm/os_net.h"
#include "cosm/os_io.h"
#include "cosm/transform.h"

s32 CosmHTTPOpen( cosm_HTTP * http, const ascii * uri,
  const cosm_NET_ADDR * proxy, const ascii * proxy_name,
  const ascii * proxy_pass, const ascii * user_name,
  const ascii * user_pass )
{
  cosm_BUFFER buf;
  cosm_TRANSFORM base64, to_buf;
  ascii * host;
  ascii * port;
  ascii * proxy_auth;
  ascii * user_auth;
  u32 host_port;
  u32 i;
  u64 len;

  if ( ( http == NULL ) || ( uri == NULL ) )
  {
    return COSM_HTTP_ERROR_PARAM;
  }

  if ( http->status != COSM_HTTP_STATUS_NONE )
  {
    return COSM_HTTP_ERROR_ORDER;
  }

  if ( CosmStrBytes( uri ) < 9 )
  {
    return COSM_HTTP_ERROR_URI;
  }

  /* verify that we have a "http://" address */
  if ( ( ( uri[0] != 'h' ) && ( uri[0] != 'H' ) )
    || ( ( uri[1] != 't' ) && ( uri[1] != 'T' ) )
    || ( ( uri[2] != 't' ) && ( uri[2] != 'T' ) )
    || ( ( uri[3] != 'p' ) && ( uri[3] != 'P' ) )
    || ( uri[4] != ':' ) || ( uri[5] != '/' ) || ( uri[6] != '/' ) )
  {
    return COSM_HTTP_ERROR_URI;
  }
  host = (ascii *) &uri[7];

  /* copy path up to end or '/', to it manually to avoid a MemAlloc */
  i = 0;
  while ( ( *host != 0 ) && ( *host != '/' ) )
  {
    if ( i == COSM_HTTP_MAX_HOSTNAME )
    {
      return COSM_HTTP_ERROR_URI;
    }
    http->host_line[i++] = *(host++);
  }
  http->host_line[i] = 0;

  /* check the host for a port number */
  if ( ( port = CosmStrChar( &http->host_line[0], ':',
    (u64) COSM_HTTP_MAX_HOSTNAME ) ) != NULL )
  {
    port++;
    if ( CosmU32Str( &host_port, NULL, port, 10 ) != COSM_PASS )
    {
      return COSM_HTTP_ERROR_URI;
    }
    if ( host_port > 0xFFFF )
    {
      return COSM_HTTP_ERROR_URI;
    }
    port--;
  }

  /* using proxy? */
  if ( ( proxy != NULL ) && ( proxy->type != 0 ) )
  {
    http->proxy = *proxy;
    http->host.ip.v4 = 0;
  }
  else /* no proxy */
  {
    http->proxy.ip.v4 = 0;
    http->proxy.port = 0;

    if ( port != NULL )
    {
      /* port cannot be in the hostname string for lookup */
      *port = 0;
    }
    if ( CosmNetDNS( &http->host, 1, http->host_line ) != 1 )
    {
      return COSM_HTTP_ERROR_URI;
    }
    if ( port != NULL )
    {
      http->host.port = host_port;
    }
    else
    {
      http->host.port = 80;
    }
  }

  /* Proxy-Authorization: Basic base64("user:pass") */
  if ( ( proxy_name != NULL ) && ( proxy_pass != NULL ) )
  {
    CosmMemSet( &buf, sizeof( cosm_BUFFER ), 0 );
    CosmMemSet( &base64, sizeof( cosm_TRANSFORM ), 0 );
    CosmMemSet( &to_buf, sizeof( cosm_TRANSFORM ), 0 );

    if ( ( CosmBufferInit( &buf, 128, COSM_BUFFER_MODE_QUEUE, 128, NULL, 0 )
      != COSM_PASS )
      || ( CosmTransformInit( &to_buf, COSM_TRANSFORM_TO_BUFFER, NULL, &buf )
      != COSM_PASS )
      || ( CosmTransformInit( &base64, COSM_BASE64_ENCODE, &to_buf )
      != COSM_PASS ) )
    {
      return COSM_HTTP_ERROR_MEMORY;
    }

    if ( ( CosmTransform( &base64, proxy_name, CosmStrBytes( proxy_name ) )
      != COSM_PASS )
      || ( CosmTransform( &base64, ":", (u64) 1 ) != COSM_PASS )
      || ( CosmTransform( &base64, proxy_pass, CosmStrBytes( proxy_pass ) )
      != COSM_PASS ) )
    {
      CosmTransformEndAll( &base64 );
      CosmBufferFree( &buf );
      return COSM_HTTP_ERROR_MEMORY;
    }

    if ( CosmTransformEndAll( &base64 ) != COSM_PASS )
    {
      CosmBufferFree( &buf );
      return COSM_HTTP_ERROR_MEMORY;
    }

    len = CosmBufferLength( &buf );
    proxy_auth = CosmMemAlloc( ( len + 32LL ) );
    CosmPrintStr( proxy_auth, (u32) ( len + 32LL ),
     "Proxy-Authorization: Basic %.*s\r\n", len, buf.memory );
    CosmBufferFree( &buf );

    http->proxy_auth = proxy_auth;
  }

  /* Authorization: Basic base64("user:pass") */
  if ( ( user_name != NULL ) && ( user_pass != NULL ) )
  {
    CosmMemSet( &buf, sizeof( cosm_BUFFER ), 0 );
    CosmMemSet( &base64, sizeof( cosm_TRANSFORM ), 0 );
    CosmMemSet( &to_buf, sizeof( cosm_TRANSFORM ), 0 );

    if ( ( CosmBufferInit( &buf, 128, COSM_BUFFER_MODE_QUEUE, 128, NULL, 0 )
      != COSM_PASS )
      || ( CosmTransformInit( &to_buf, COSM_TRANSFORM_TO_BUFFER, NULL, &buf )
      != COSM_PASS )
      || ( CosmTransformInit( &base64, COSM_BASE64_ENCODE, &to_buf )
      != COSM_PASS ) )
    {
      return COSM_HTTP_ERROR_MEMORY;
    }

    if ( ( CosmTransform( &base64, user_name, CosmStrBytes( user_name ) )
      != COSM_PASS )
      || ( CosmTransform( &base64, ":", (u64) 1 ) != COSM_PASS )
      || ( CosmTransform( &base64, user_pass, CosmStrBytes( user_pass ) )
      != COSM_PASS ) )
    {
      CosmTransformEndAll( &base64 );
      CosmBufferFree( &buf );
      return COSM_HTTP_ERROR_MEMORY;
    }

    if ( CosmTransformEndAll( &base64 ) != COSM_PASS )
    {
      CosmBufferFree( &buf );
      return COSM_HTTP_ERROR_MEMORY;
    }

    len = CosmBufferLength( &buf );
    user_auth = CosmMemAlloc( ( len + 32LL ) );
    CosmPrintStr( user_auth, (u32) ( len + 32LL ),
     "Authorization: Basic %.*s\r\n", (u32) len, buf.memory );
    CosmBufferFree( &buf );

    http->user_auth = user_auth;
  }

  http->length = 0;
  http->status = COSM_HTTP_STATUS_CLOSED;

  return COSM_PASS;
}

s32 CosmHTTPGet( cosm_HTTP * http, u32 * status, const ascii * uri_path,
  u32 wait_ms )
{
  u32 len;
  ascii * request;
  s32 result;
  u32 bytes;

  *status = 0;

  if ( ( http == NULL ) || ( uri_path == NULL ) )
  {
    return COSM_HTTP_ERROR_PARAM;
  }

  /* reopen the connection if we need to */
  if ( http->persistent != 1 )
  {
    CosmNetClose( &http->net );
    http->status = COSM_HTTP_STATUS_CLOSED;
  }

  if ( http->status == COSM_HTTP_STATUS_CLOSED )
  {
    if ( Cosm_HTTPNetOpen( http ) != COSM_PASS )
    {
      return COSM_HTTP_ERROR_NET;
    }
  }
  else if ( http->status != COSM_HTTP_STATUS_IDLE )
  {
    return COSM_HTTP_ERROR_ORDER;
  }

  /* find out how much request string we need */
  len = CosmStrBytes( http->proxy_auth ) + CosmStrBytes( http->user_auth )
    + CosmStrBytes( uri_path ) + COSM_HTTP_MAX_HOSTNAME + 129;


  if ( ( request = CosmMemAlloc( len ) ) == NULL )
  {
    return COSM_HTTP_ERROR_MEMORY;
  }

  CosmPrintStr( request, len,
    "\
GET http://%.*s%.*s HTTP/1.1\r\n\
Host: %.*s\r\n\
%.*s\
%.*s\
Connection: Keep-Alive\r\n\r\n",
    COSM_HTTP_MAX_HOSTNAME, http->host_line,
    CosmStrBytes( uri_path ), uri_path,
    COSM_HTTP_MAX_HOSTNAME, http->host_line,
    ( http->proxy_auth != NULL ) ? CosmStrBytes( http->proxy_auth ) : 0,
    ( http->proxy_auth != NULL ) ? http->proxy_auth : NULL,
    ( http->user_auth != NULL ) ? CosmStrBytes( http->user_auth ) : 0,
    ( http->user_auth != NULL ) ? http->user_auth : NULL );

  if ( CosmNetSend( &http->net, &bytes, request,
    CosmStrBytes( request ) ) != COSM_PASS )
  {
    CosmMemFree( request );
    return COSM_HTTP_ERROR_NET;
  }

  CosmMemFree( request );

  /* get header */
  if ( ( result = Cosm_HTTPParseHeader( http, wait_ms ) ) != COSM_PASS )
  {
    return result;
  }

  *status = http->http_status;

  return COSM_PASS;
}

s32 CosmHTTPPost( cosm_HTTP * http, u32 * status, const ascii * uri_path,
  const void * data, u32 length, u32 wait_ms )
{
  u32 len;
  ascii * request;
  s32 result;
  u32 bytes;

  *status = 0;

  if ( ( http == NULL ) || ( uri_path == NULL )
    || ( ( data == NULL ) && ( length != 0 ) ) )
  {
    return COSM_HTTP_ERROR_PARAM;
  }

  /* reopen the connection if we need to */
  if ( http->persistent != 1 )
  {
    CosmNetClose( &http->net );
    http->status = COSM_HTTP_STATUS_CLOSED;
  }
  if ( http->status == COSM_HTTP_STATUS_CLOSED )
  {
    if ( Cosm_HTTPNetOpen( http ) != COSM_PASS )
    {
      return COSM_HTTP_ERROR_NET;
    }
  }
  else if ( http->status != COSM_HTTP_STATUS_IDLE )
  {
    return COSM_HTTP_ERROR_ORDER;
  }

  /* find out how much request string we need */
  len = CosmStrBytes( http->proxy_auth ) + CosmStrBytes( http->user_auth )
    + CosmStrBytes( uri_path ) + COSM_HTTP_MAX_HOSTNAME + 129;

  if ( ( request = CosmMemAlloc( len ) ) == NULL )
  {
    return COSM_HTTP_ERROR_MEMORY;
  }

  CosmPrintStr( request, len,
    "\
POST http://%.*s%.*s HTTP/1.1\r\n\
Host: %.*s\r\n\
%.*s\
%.*s\
Connection: Keep-Alive\r\n\
Content-Length: %u\r\n\r\n",
    COSM_HTTP_MAX_HOSTNAME, http->host_line,
    CosmStrBytes( uri_path ), uri_path,
    COSM_HTTP_MAX_HOSTNAME, http->host_line,
    ( http->proxy_auth != NULL ) ? CosmStrBytes( http->proxy_auth ) : 0,
    ( http->proxy_auth != NULL ) ? http->proxy_auth : NULL,
    ( http->user_auth != NULL ) ? CosmStrBytes( http->user_auth ) : 0,
    ( http->user_auth != NULL ) ? http->user_auth : NULL, length );

  if ( CosmNetSend( &http->net, &bytes, request,
    CosmStrBytes( request ) ) != COSM_PASS )
  {
    CosmMemFree( request );
    return COSM_HTTP_ERROR_NET;
  }

  CosmMemFree( request );

  /* send data */
  if ( CosmNetSend( &http->net, &bytes, data, length ) != COSM_PASS )
  {
    return COSM_HTTP_ERROR_NET;
  }

  /* get header */
  if ( ( result = Cosm_HTTPParseHeader( http, wait_ms ) ) != COSM_PASS )
  {
    return result;
  }

  *status = http->http_status;

  return COSM_PASS;
}

s32 CosmHTTPRecv( void * buffer, u32 * bytes_received, cosm_HTTP * http,
  u32 length, u32 wait_ms )
{
  u32 result;
  u32 buffer_get;
  u32 get;
  u32 chunk_total;

  *bytes_received = 0;

  if ( http->status != COSM_HTTP_STATUS_BODY )
  {
    return COSM_HTTP_ERROR_ORDER;
  }

  /* empty buffer if no header */
  if ( ( http->version == COSM_HTTP_VERSION_0_9 )
    && ( CosmBufferLength( &http->header ) > 0 ) )
  {
    buffer_get = (u32) CosmBufferGet( buffer, (u64) length, &http->header );

    if ( buffer_get == length )
    {
      *bytes_received = buffer_get;
      return COSM_PASS;
    }
    else
    {
      buffer = CosmMemOffset( buffer, (u64) buffer_get );
      length = length - buffer_get;
      *bytes_received = buffer_get;
    }
  }

  switch ( http->chunking )
  {
    case COSM_HTTP_CHUNKING_FIXED:
      if ( http->length == 0 )
      {
        http->status = COSM_HTTP_STATUS_IDLE;
        return COSM_PASS;
      }
      get = ( length > http->length ) ? http->length : length;
      if ( ( CosmNetRecv( buffer, &result, &http->net, get, wait_ms )
        != COSM_PASS ) && ( result == 0 ) )
      {
        CosmNetClose( &http->net );
        http->status = COSM_HTTP_STATUS_CLOSED;
      }
      http->length -= result;
      *bytes_received = result;
      break;
    case COSM_HTTP_CHUNKING_CHUNKED:
      chunk_total = 0;
      while ( length > 0 )
      {
        if ( http->length > 0 )
        {
          get = ( length > http->length ) ? http->length : length;
          if ( ( CosmNetRecv( buffer, &result, &http->net, get, wait_ms )
            != COSM_PASS ) && ( result == 0 ) )
          {
            CosmNetClose( &http->net );
            http->status = COSM_HTTP_STATUS_CLOSED;
            return COSM_PASS;
          }
          buffer = CosmMemOffset( buffer, (u64) result );
          http->length -= result;
          length -= result;
          chunk_total += result;
        }

        if ( http->length == 0 )
        {
          /* need a new length */
          if ( Cosm_HTTPChunkLength( http, wait_ms ) == COSM_PASS )
          {
            if ( http->length == 0 )
            {
              /* we're done */
              http->status = COSM_HTTP_STATUS_IDLE;
              *bytes_received = chunk_total;
              return COSM_PASS;
            }
          }
          else
          {
            CosmNetClose( &http->net );
            http->status = COSM_HTTP_STATUS_CLOSED;
            break;
          }
        }
      }
      *bytes_received = chunk_total;
      break;
    case COSM_HTTP_CHUNKING_CLOSE:
    default:
      /* read and close if nothing left */
      if ( ( CosmNetRecv( buffer, &result, &http->net, length, wait_ms )
        != COSM_PASS ) && ( result == 0 ) )
      {
        CosmNetClose( &http->net );
        http->status = COSM_HTTP_STATUS_CLOSED;
        return COSM_PASS;
      }
      *bytes_received += result;
      break;
  }

  return COSM_PASS;
}

s32 CosmHTTPClose( cosm_HTTP * http )
{
  if ( http == NULL )
  {
    return COSM_HTTP_ERROR_PARAM;
  }

  CosmNetClose( &http->net );
  if ( http->header_flag )
  {
    CosmBufferFree( &http->header );
  }

  if ( http->proxy_auth != NULL )
  {
    CosmMemFree( http->proxy_auth );
  }

  if ( http->user_auth != NULL )
  {
    CosmMemFree( http->user_auth );
  }

  CosmMemSet( http, sizeof( cosm_HTTP ), 0 );

  return COSM_PASS;
}

s32 CosmHTTPDInit( cosm_HTTPD * httpd, ascii * log_path, u32 log_level,
  u32 threads, u32 stack_size, const cosm_NET_ADDR * host, u32 wait_ms )
{
  if ( ( httpd == NULL ) || ( threads == 0 ) || ( host == NULL ) )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  CosmMutexInit( &httpd->lock );
  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( httpd->status != COSM_HTTPD_STATUS_NONE )
  {
    CosmMutexUnlock( &httpd->lock );
    return COSM_HTTPD_ERROR_ORDER;
  }

  if ( log_path != NULL )
  {
    if ( CosmLogOpen( &httpd->log, log_path, log_level, COSM_LOG_MODE_NUMBER )
      != COSM_PASS )
    {
      return COSM_HTTPD_ERROR_LOGFILE;
    }
    httpd->log_active = 1;
  }
  else
  {
    httpd->log_active = 0;
  }

  httpd->handler_count = 0;
  httpd->thread_count = threads;
  httpd->stack_size = stack_size;
  httpd->paths = NULL;
  httpd->handlers = NULL;
  httpd->host = *host;
  httpd->wait_ms = wait_ms;
  httpd->status = COSM_HTTPD_STATUS_IDLE;

  CosmMutexUnlock( &httpd->lock );

  return COSM_PASS;
}

s32 CosmHTTPDSetHandler( cosm_HTTPD * httpd, const ascii * path,
  cosm_NET_ACL * acl, s32 (*handler)( cosm_HTTPD_REQUEST * request ) )
{
  u32 i, j;
  u32 max;

  if ( ( httpd == NULL ) || ( path == NULL ) )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( ( httpd->status != COSM_HTTPD_STATUS_IDLE )
    && ( ( httpd->status != COSM_HTTPD_STATUS_STOPPED ) ) )
  {
    CosmMutexUnlock( &httpd->lock );
    return COSM_HTTPD_ERROR_ORDER;
  }

  /* already exists? replace handler */
  for ( i = 0 ; i < httpd->handler_count ; i++ )
  {
    max = ( CosmStrBytes( path ) > CosmStrBytes( httpd->paths[i] ) )
      ? CosmStrBytes( path ) : CosmStrBytes( httpd->paths[i] );
    if ( CosmStrCmp( httpd->paths[i], path, max ) == 0 )
    {
      /* if the new handler is NULL, then remove it */
      if ( handler == NULL )
      {
        for ( j = i ; j < ( httpd->handler_count - 1 ) ; j++ )
        {
          httpd->paths[j] = httpd->paths[j + 1];
          httpd->handlers[j] = httpd->handlers[j + 1];
        }
        httpd->handler_count--;
      }
      else
      {
        httpd->handlers[i] = handler;
      }
      CosmMutexUnlock( &httpd->lock );
      return COSM_PASS;
    }
  }

  /* if we didn't match something, a NULL handler doesn't do anything */
  if ( handler == NULL )
  {
    CosmMutexUnlock( &httpd->lock );
    return COSM_PASS;
  }

  httpd->paths = CosmMemRealloc( httpd->paths,
    sizeof( ascii * ) * ( httpd->handler_count + 1 ) );
  httpd->handlers = CosmMemRealloc( httpd->handlers,
    sizeof( s32 (*)( cosm_HTTPD_THREAD * thread ) )
    * ( httpd->handler_count + 1 ) );

  /* not found, add it, longer matching paths before shorter ones */
  for ( i = 0 ; i < httpd->handler_count ; i++ )
  {
    /* if current one is a substr of path, path goes before it */
    if ( CosmStrStr( path, httpd->paths[i], CosmStrBytes( path ) + 1 ) == path )
    {
      for ( j = httpd->handler_count ; j > i ; j-- )
      {
        httpd->paths[j] = httpd->paths[j - 1];
        httpd->handlers[j] = httpd->handlers[j - 1];
      }
      httpd->paths[i] = (ascii *) path;
      httpd->handlers[i] = handler;
      break;
    }
  }

  /* if we didn't insert and break out of last loop, it goes at end */
  if ( i == httpd->handler_count )
  {
    httpd->paths[i] = (ascii *) path;
    httpd->handlers[i] = handler;
  }

  httpd->handler_count++;
  httpd->status = COSM_HTTPD_STATUS_STOPPED;

  CosmMutexUnlock( &httpd->lock );

  return COSM_PASS;
}

s32 CosmHTTPDStart( cosm_HTTPD * httpd, u32 timeout_ms )
{
  u32 running;
  u32 sleep;

  /* start the server thread */

  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( ( httpd->status != COSM_HTTPD_STATUS_STOPPED )
    && ( httpd->status != COSM_HTTPD_STATUS_STARTING ) )
  {
    CosmMutexUnlock( &httpd->lock );
    return COSM_HTTPD_ERROR_ORDER;
  }

  /* start main thread */
  if ( httpd->status != COSM_HTTPD_STATUS_STARTING )
  {
    httpd->httpd_thread_stop = 0;
    httpd->httpd_thread = 0x0000000000000000LL;

    if ( CosmThreadBegin( &httpd->httpd_thread, Cosm_HTTPDMain,
      (void *) httpd, httpd->thread_count * 4096 ) != COSM_PASS )
    {
      CosmMutexUnlock( &httpd->lock );
      return COSM_HTTPD_ERROR_ADDRESS; /* !!! */
    }
    httpd->status = COSM_HTTPD_STATUS_STARTING;

    CosmMutexUnlock( &httpd->lock );
  }

  /* wait till timeout expires, and report status */
  running = 0;
  while ( timeout_ms > 0 )
  {
    sleep = ( timeout_ms > 100 ) ? 100 : timeout_ms;
    timeout_ms -= sleep;
    CosmSleep( sleep );

    if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
    {
      return COSM_HTTPD_ERROR_PARAM;
    }
    if ( httpd->status == COSM_HTTPD_STATUS_RUNNING )
    {
      running = 1;
    }
    CosmMutexUnlock( &httpd->lock );

    if ( running == 1 )
    {
      return COSM_PASS;
    }
  }

  /* if we make it here, flag it to die */
  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }
  httpd->httpd_thread_stop = 1;
  CosmMutexUnlock( &httpd->lock );

  return COSM_HTTPD_ERROR_TIMEOUT;
}

s32 CosmHTTPDStop( cosm_HTTPD * httpd, u32 timeout_ms )
{
  u32 stopped;
  u32 sleep;
  cosm_NET net;

  /* stop the server thread */

  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( ( httpd->status != COSM_HTTPD_STATUS_RUNNING )
    && ( httpd->status != COSM_HTTPD_STATUS_STOPPING ) )
  {
    CosmMutexUnlock( &httpd->lock );
    return COSM_HTTPD_ERROR_ORDER;
  }

  httpd->httpd_thread_stop = 1;

  CosmMutexUnlock( &httpd->lock );

  /* trigger an accept if we're waiting */
  CosmMemSet( &net, sizeof( cosm_NET ), 0 );
  if ( _COSM_NETOPEN( &net, &httpd->host ) == COSM_PASS )
  {
    CosmNetClose( &net );
  }

  /* close the network port so we can immediately start a new server */
  CosmNetClose( &httpd->net );

  /* wait till timeout expires, and report status */
  stopped = 0;
  while ( timeout_ms > 0 )
  {
    sleep = timeout_ms > 100 ? 100 : timeout_ms;
    timeout_ms -= sleep;
    CosmSleep( sleep );

    if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
    {
      return COSM_HTTPD_ERROR_PARAM;
    }
    if ( httpd->status == COSM_HTTPD_STATUS_STOPPED )
    {
      stopped = 1;
    }
    CosmMutexUnlock( &httpd->lock );

    if ( stopped == 1 )
    {
      return COSM_PASS;
    }
  }

  return COSM_HTTPD_ERROR_TIMEOUT;
}

s32 CosmHTTPDSendInit( cosm_HTTPD_REQUEST * request, u32 status_code,
  ascii * status_string, ascii * mime_type )
{
  u32 sent;
  u32 bytes;
  ascii * header;
  
  if ( ( request == NULL ) || ( status_code < 100 ) || ( status_code > 999 )
    || ( mime_type == NULL ) || ( mime_type == NULL ) )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( request->state != COSM_HTTPD_REQUEST_START )
  {
    return COSM_HTTPD_ERROR_ORDER;
  }
  request->state = COSM_HTTPD_REQUEST_HEADER;

  /* 0.9 has no header */
  if ( request->version == COSM_HTTP_VERSION_0_9 )
  {
    return COSM_PASS;
  }

  /*
    HTTP/1.x <status_code> <status_string>\r\n
    Content-Type: <mime_type>\r\n
    
    length = 9 + 3 + 1 + status_string + 2
      + 14 + mime_type + 2 ( + 1 for the \0 )
  */
  bytes = 32 + CosmStrBytes( status_string ) + CosmStrBytes( mime_type );
  if ( ( header = CosmMemAlloc( bytes ) ) == NULL )
  {
    return COSM_HTTPD_ERROR_MEMORY;
  }

  CosmPrintStr( header, bytes,
    "HTTP/1.%c %u %.*s\r\n"
    "Content-Type: %.*s\r\n",
    ( request->version == COSM_HTTP_VERSION_1_1 ) ? '1' : '0', status_code,
    CosmStrBytes( status_string ), status_string,
    CosmStrBytes( mime_type ), mime_type );

  if ( CosmNetSend( request->net, &sent, header, bytes - 1 ) != COSM_PASS )
  {
    CosmMemFree( header );
    return COSM_HTTPD_ERROR_NET;
  }
  CosmMemFree( header );

  return COSM_PASS;
}


s32 CosmHTTPDSendHead( cosm_HTTPD_REQUEST * request, const void * string )
{
  u32 sent;

  if ( ( request == NULL ) || ( string == NULL ) )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( request->state != COSM_HTTPD_REQUEST_HEADER )
  {
    return COSM_HTTPD_ERROR_ORDER;
  }

  if ( request->version == COSM_HTTP_VERSION_0_9 )
  {
    return COSM_PASS;
  }
  else
  {
    if ( ( CosmNetSend( request->net, &sent, string,
      CosmStrBytes( string ) ) != COSM_PASS )
      || ( CosmNetSend( request->net, &sent, "\r\n", 2 ) != COSM_PASS ) )
    {
      return COSM_HTTPD_ERROR_NET;
    }
  }

  return COSM_PASS;
}

s32 CosmHTTPDSend( cosm_HTTPD_REQUEST * request, const void * data, u32 length )
{
  u32 sent;
  ascii str[16];
  u32 len_length;

  if ( ( request == NULL ) || ( ( data == NULL ) && ( length > 0 ) ) )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( request->state == COSM_HTTPD_REQUEST_HEADER )
  {
    /* end the header */
    if ( request->version == COSM_HTTP_VERSION_1_1 )
    {
      CosmNetSend( request->net, &sent,
       "Transfer-Encoding: chunked\r\n\r\n", 30 );
    }
    else if ( request->version == COSM_HTTP_VERSION_1_0 )
    {
      CosmNetSend( request->net, &sent, "\r\n", 2 );
    }
    request->state = COSM_HTTPD_REQUEST_BODY;
  }

  if ( request->state != COSM_HTTPD_REQUEST_BODY )
  {
    return COSM_HTTPD_ERROR_ORDER;
  }

  if ( request->version == COSM_HTTP_VERSION_1_1 )
  {
    /*
      chunk  =>  %X\r\n<data>\r\n
      end    =>  0\r\n\r\n
    */
    if ( length > 0 )
    {
      len_length = CosmPrintStr( str, 16, "%X\r\n", length );
      if ( ( CosmNetSend( request->net, &sent, str, len_length ) != COSM_PASS )
        || ( CosmNetSend( request->net, &sent, data, length ) != COSM_PASS )
        || ( CosmNetSend( request->net, &sent, "\r\n", 2 ) != COSM_PASS ) )
      {
        return COSM_HTTPD_ERROR_NET;
      }
    }
    else
    {
      if ( CosmNetSend( request->net, &sent, "0\r\n\r\n", 5 ) != COSM_PASS )
      {
        return COSM_HTTPD_ERROR_NET;
      }
    }
  }
  else
  {
    if ( CosmNetSend( request->net, &sent, data, length ) != COSM_PASS )
    {
      return COSM_HTTPD_ERROR_NET;
    }
  }

  return COSM_PASS;
}

s32 CosmHTTPDRecv( void * buffer, u32 * bytes_received,
  cosm_HTTPD_REQUEST * request, u32 length, u32 wait_ms )
{
  u32 get;
  u32 result;

  *bytes_received = 0;

  if ( request->post_length > 0 )
  {
    get = ( length > request->post_length ) ? request->post_length : length;
    if ( ( CosmNetRecv( buffer, &result, request->net, get, wait_ms )
      != COSM_PASS ) && ( result == 0 ) )
    {
      CosmNetClose( request->net );
      return COSM_HTTPD_ERROR_NET;
    }
    *bytes_received += result;
    request->post_length -= result;
  }

  return COSM_PASS;
}

s32 CosmHTTPDFree( cosm_HTTPD * httpd )
{
  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_PARAM;
  }

  if ( ( httpd->status != COSM_HTTPD_STATUS_IDLE )
    && ( ( httpd->status != COSM_HTTPD_STATUS_STOPPED ) ) )
  {
    CosmMutexUnlock( &httpd->lock );
    return COSM_HTTPD_ERROR_ORDER;
  }

  CosmMemFree( httpd->paths );
  CosmMemFree( httpd->handlers );
  httpd->status = COSM_HTTPD_STATUS_NONE;
  if ( httpd->log_active )
  {
    CosmLogClose( &httpd->log );
  }

  CosmMutexUnlock( &httpd->lock );
  CosmMutexFree( &httpd->lock );

  return COSM_PASS;
}

s32 Cosm_HTTPNetOpen( cosm_HTTP * http )
{
  if ( ( http->proxy.ip.v4 != 0 ) && ( http->proxy.port != 0 ) )
  {
    if ( _COSM_NETOPEN( &http->net, &http->proxy )
      != COSM_PASS )
    {
      return COSM_FAIL;
    }
  }
  else
  {
    if ( _COSM_NETOPEN( &http->net, &http->host ) != COSM_PASS )
    {
      return COSM_FAIL;
    }
  }

  http->status = COSM_HTTP_STATUS_IDLE;
  return COSM_PASS;
}

s32 Cosm_HTTPParseHeader( cosm_HTTP * http, u32 wait_ms )
{
  static ascii http_magic[2][8] = { "HTTP/1.1", "http/1.0" };
  utf8 tmp_buffer[8];
  u32 result;
  u32 i, count, length;
  ascii * ptr;
  ascii ch;

  /*
  Example format:

  HTTP/1.1 200 OK<CRLF>
  Cache-Control: no-cache<CRLF>
  Expires: Tue, 01 Jan 1980 00:00:00 GMT<CRLF>
  Content-Type: application/octet-stream<CRLF>
  Content-Length: <byte-count><CRLF>
  <CRLF>
  <raw-bytes>

  ?Date: Tue, 01 Jan 1980 00:00:00 GMT<CRLF>
  ?Transfer-Encoding: chunked<CRLF>
  ?Connection: close<CRLF>
  */

  /* clean up the last header */
  if ( http->header_flag )
  {
    CosmBufferFree( &http->header );
    http->header_flag = 0;
  }

  if ( CosmBufferInit( &http->header, (u64) 1024,
    COSM_BUFFER_MODE_QUEUE, (u64) 1024, NULL, 0 )
    == COSM_FAIL )
  {
    return COSM_HTTP_ERROR_MEMORY;
  }
  http->header_flag = 1;

  /* look for the version at start of header */
  count = 0;
  for ( i = 0 ; i < 8 ; i++ )
  {
    if ( ( CosmNetRecv( &tmp_buffer[i], &result, &http->net, 1, wait_ms )
      != COSM_PASS ) && ( result == 0 ) )
    {
      if ( i == 0 )
      {
        /* nothing read yet */
        CosmNetClose( &http->net );
        http->status = COSM_HTTP_STATUS_CLOSED;
        return COSM_HTTP_ERROR_NET;
      }
      else
      {
        /* we have some bytes already */
        http->version = COSM_HTTP_VERSION_0_9;
        http->persistent = 0;
        break;
      }
    }
    count = i + 1;

    if ( i == 7 )
    {
      if ( tmp_buffer[7] == '1' )
      {
        http->version = COSM_HTTP_VERSION_1_1;
        http->persistent = 1;
      }
      else if ( tmp_buffer[7] == '0' )
      {
        http->version = COSM_HTTP_VERSION_1_0;
        http->persistent = 0;
      }
      else
      {
        return COSM_HTTP_ERROR_VERSION;
      }
    }
    else
    {
      if ( ( ( tmp_buffer[i] != http_magic[0][i] )
        && ( tmp_buffer[i] != http_magic[1][i] ) ) )
      {
        http->version = COSM_HTTP_VERSION_0_9;
        http->persistent = 0;
        break;
      }
    }
  }

  if ( CosmBufferPut( &http->header, tmp_buffer, (u64) count )
    != COSM_PASS )
  {
    return COSM_HTTP_ERROR_MEMORY;
  }

  /* if it's a legacy connection we're done looking for headers */
  if ( http->version == COSM_HTTP_VERSION_0_9 )
  {
    http->http_status = 200;
    http->status = COSM_HTTP_STATUS_BODY;
    http->chunking = COSM_HTTP_CHUNKING_CLOSE;
    return COSM_PASS;
  }

  /* get the status */
  if ( ( CosmNetRecv( tmp_buffer, &result, &http->net, 4, wait_ms ) != COSM_PASS )
    || ( result != 4 ) )
  {
    CosmNetClose( &http->net );
    http->status = COSM_HTTP_STATUS_CLOSED;
    return COSM_HTTP_ERROR_NET;
  }
  tmp_buffer[4] = 0;

  if ( CosmBufferPut( &http->header, tmp_buffer, (u64) result )
    != COSM_PASS )
  {
    return COSM_HTTP_ERROR_MEMORY;
  }

  if ( CosmU32Str( &http->http_status, NULL, tmp_buffer, 10 ) != COSM_PASS )
  {
    /* wasnt a valid header after all */
    http->version = COSM_HTTP_VERSION_0_9;
    http->chunking = COSM_HTTP_CHUNKING_CLOSE;
    http->persistent = 0;
    return COSM_PASS;
  }
  else
  {
    /* read from net until \r\n\r\n, stuff into buffer */
    i = 0;
    result = 1;
    while ( ( i < 4 ) && ( result == 1 ) )
    {
      if ( ( CosmNetRecv( &ch, &result, &http->net, 1, wait_ms ) != COSM_PASS )
        && ( result == 0 ) )
      {
        CosmNetClose( &http->net );
        http->status = COSM_HTTP_STATUS_CLOSED;
        return COSM_HTTP_ERROR_NET;
      }

      if ( CosmBufferPut( &http->header, &ch, (u64) 1 ) != COSM_PASS )
      {
        return COSM_HTTP_ERROR_MEMORY;
      }

      if ( ( i % 2 ) == 0 )
      {
        if ( ch == '\r' )
        {
          i++;
        }
        else
        {
          i = 0;
        }
      }
      else
      {
        if ( ch == '\n' )
        {
          i++;
        }
        else
        {
          i = 0;
        }
      }
    }

    /* terminate buffer string */
    ch = 0;
    if ( CosmBufferPut( &http->header, &ch, (u64) 1 ) != COSM_PASS )
    {
      return COSM_HTTP_ERROR_MEMORY;
    }

    /* look for "Content-Length: " or "Transfer-Encoding: chunked" */
    if ( ( ( ptr = CosmStrStr( http->header.memory,
      "Content-Length: ", (u32) http->header.data_length ) ) != NULL )
      && ( ( CosmU32Str( &length, NULL, &ptr[16], 10 ) == COSM_PASS ) ) )
    {
      http->length = length;
      http->chunking = COSM_HTTP_CHUNKING_FIXED;
    }
    else if ( CosmStrStr( http->header.memory,
      "Transfer-Encoding: chunked", (u32) http->header.data_length )
      != NULL )
    {
      http->length = 0;
      http->chunking = COSM_HTTP_CHUNKING_CHUNKED;
    }
    else
    {
      /* read until close */
      http->chunking = COSM_HTTP_CHUNKING_CLOSE;
      http->persistent = 0;
    }
  }

  http->status = COSM_HTTP_STATUS_BODY;

  return COSM_PASS;
}

s32 Cosm_HTTPChunkLength( cosm_HTTP * http, u32 wait_ms )
{
  ascii tmp_buffer[16];
  ascii ch;
  u32 count;
  u32 result;

  /* get the length line, but midstream we have a blank line first */
  do
  {
    ch = 0;
    count = 0;
    /* should only be 8 hex characters */
    while ( ( ch != '\n' ) && ( count < 15 ) )
    {
      CosmNetRecv( &ch, &result, &http->net, 1, wait_ms );
      if ( result != 1 )
      {
        /* abort */
        break;
      }
      tmp_buffer[count++] = ch;
    }
  } while ( count == 2 );

  if ( ( count > 2 ) && ( ch == '\n' ) && ( tmp_buffer[count-2] == '\r' ) )
  {
    result = CosmU32Str( &http->length, NULL, tmp_buffer, 16 );
    if ( ( result == COSM_PASS ) && ( http->length == 0 ) )
    {
      /* we need to get 2 more bytes, yet another "\r\n" we know is coming */
      if ( ( CosmNetRecv( tmp_buffer, &result, &http->net, 2, wait_ms )
        == COSM_PASS ) && ( result == 2 ) )
      {
        return COSM_PASS;
      }
      else
      {
        return COSM_FAIL;
      }
    }
    else
    {
      return result;
    }
  }

  return COSM_FAIL;
}

s32 Cosm_HTTPDParseRequest( cosm_HTTPD_REQUEST * request, cosm_NET * net,
  u32 wait_ms )
{
  u32 i, result;
  ascii ch;
  ascii * tmp;
  ascii * tmp_path;
  utf8 tmp_num[3] = { 0x00, 0x00, 0x00 };

  /*
  Incoming request looks like...

  GET [http://..]/ HTTP/1.1<CRLF>
  ?Host: %.*s<CRLF>
  <CRLF>

  POST [http://..]/ HTTP/1.1<CRLF>
  ?Host: %.*s<CRLF>
  Content-Length: <decimal><CRLF>
  <CRLF>
  <raw-bytes>

  ?Connection: Keep-Alive<CRLF>
  */

  /* clean up the last header */
  if ( request->header_flag )
  {
    CosmBufferFree( &request->header );
    CosmMemFree( request->path );
    request->header_flag = 0;
  }

  /* init the new one */
  CosmMemSet( request, sizeof( cosm_HTTPD_REQUEST ), 0 );
  if ( CosmBufferInit( &request->header, 1024LL,
    COSM_BUFFER_MODE_QUEUE, 1024LL, NULL, 0 )
    == COSM_FAIL )
  {
    return COSM_HTTPD_ERROR_MEMORY;
  }
  request->header_flag = 1;

  /* read in first line */
  ch = 0;
  result = 1;
  while ( ( result == 1 ) && ( ch != '\n' ) )
  {
    if ( ( CosmNetRecv( &ch, &result, net, 1, wait_ms ) != COSM_PASS )
      && ( result == 0 ) )
    {
      CosmNetClose( net );
      return COSM_HTTPD_ERROR_NET;
    }

    if ( CosmBufferPut( &request->header, &ch, 1LL ) != COSM_PASS )
    {
      return COSM_HTTPD_ERROR_MEMORY;
    }
  }

  /* terminate buffer string */
  ch = 0;
  if ( CosmBufferPut( &request->header, &ch, 1LL ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_MEMORY;
  }

  /* get request type, get/post */
  tmp = request->header.memory;
  if ( ( ( tmp[0] == 'P' ) || ( tmp[0] == 'p' ) )
    && ( tmp[4] == ' ' ) )
  {
    request->type = COSM_HTTPD_REQUEST_POST;
    tmp_path = &tmp[5];
  }
  else if ( ( ( tmp[0] == 'G' ) || ( tmp[0] == 'g' ) )
    && ( tmp[3] == ' ' ) )
  {
    request->type = COSM_HTTPD_REQUEST_GET;
    tmp_path = &tmp[4];
  }
  else
  {
    /* invalid request */
    CosmNetClose( net );
    return COSM_HTTPD_ERROR_NET;
  }

  if ( ( tmp = CosmStrChar( tmp_path, ' ',
    (u32) request->header.data_length ) )!= NULL )
  {
    if ( CosmStrStr( tmp, "/1.1", (u32) request->header.data_length )
      != NULL )
    {
      request->version = COSM_HTTP_VERSION_1_1;
      request->persistent = 1;
    }
    else if ( CosmStrStr( tmp, "/1.0", (u32) request->header.data_length )
      != NULL )
    {
      request->version = COSM_HTTP_VERSION_1_0;
    }
    /* terminate the path */
    *tmp = 0;
  }
  else
  {
    request->version = COSM_HTTP_VERSION_0_9;
  }

  /* allocate path space */
  if ( ( request->path = CosmMemAlloc( request->header.data_length ) ) == NULL )
  {
    return COSM_HTTPD_ERROR_MEMORY;
  }

  /* copy path, unencode path part */
  if ( *tmp_path != '/' )
  {
    /* skip past the http:// and stop at the 3rd '/' */
    i = 0;
    do
    {
      if ( *tmp_path == '/' )
      {
        i++;
      }
    } while ( ( i < 3 ) && ( *tmp_path++ != 0 ) );
  }
  tmp = request->path;
  while ( ( *tmp_path != 0 ) && ( *tmp_path != 13 ) )
  {
    if ( *tmp_path == '%' )
    {
      /* hex encoded value */
      tmp_num[0] = *++tmp_path;
      tmp_num[1] = *++tmp_path;
      CosmU32Str( &i, NULL, tmp_num, 16 );
      *tmp++ = (ascii) i;
    }
    else if ( *tmp_path == '?' )
    {
      *tmp++ = 0;
      request->get_data = tmp;
      tmp_path++;
      while ( *tmp_path != 0 )
      {
        *tmp++ = *tmp_path++;
      }
      break;
    }
    else
    {
      *tmp++ = *tmp_path;
    }
    tmp_path++;
  }
  *tmp = 0;

  /* clear the buffer */
  if ( CosmBufferClear( &request->header ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_MEMORY;
  }

  if ( request->version >= COSM_HTTP_VERSION_1_0 )
  {
    /* read remaining header until \r\n\r\n into buffer */
    i = 2; /* already read one \r\n */
    result = 1;
    while ( ( i < 4 ) && ( result == 1 ) )
    {
      if ( ( CosmNetRecv( &ch, &result, net, 1, wait_ms ) != COSM_PASS )
        && ( result == 0 ) )
      {
        CosmNetClose( net );
        return COSM_HTTPD_ERROR_NET;
      }

      if ( CosmBufferPut( &request->header, &ch, (u64) 1 ) != COSM_PASS )
      {
        return COSM_HTTPD_ERROR_MEMORY;
      }

      if ( ( i % 2 ) == 0 )
      {
        if ( ch == '\r' )
        {
          i++;
        }
        else
        {
          i = 0;
        }
      }
      else
      {
        if ( ch == '\n' )
        {
          i++;
        }
        else
        {
          i = 0;
        }
      }
    }
  }

  /* terminate buffer string */
  ch = 0;
  if ( CosmBufferPut( &request->header, &ch, (u64) 1 ) != COSM_PASS )
  {
    return COSM_HTTPD_ERROR_MEMORY;
  }

  /* keepalive in 1.0? */
  if ( ( request->version == COSM_HTTP_VERSION_1_0 )
    && ( ( CosmStrStr( request->header.memory, "Connection: Keep-Alive",
    (u32) request->header.data_length ) != NULL )
    || ( CosmStrStr( request->header.memory, "Connection: keep-alive",
    (u32) request->header.data_length ) != NULL ) ) )
  {
    request->persistent = 1;
  }

  /* fixed post data length? */
  if ( ( request->type == COSM_HTTPD_REQUEST_POST )
    && ( ( tmp = CosmStrStr( request->header.memory, "Content-Length: ",
    (u32) request->header.data_length ) ) != NULL )
    && ( ( CosmU32Str( &i, NULL, &tmp[16], 10 ) == COSM_PASS ) ) )
  {
    request->post_length = i;
  }

  request->net = net;
  request->state = COSM_HTTPD_REQUEST_START;

  return COSM_PASS;
}

void Cosm_HTTPDThread( void * arg )
{
  cosm_HTTPD_THREAD * thread;
  cosm_HTTPD * httpd;
  cosm_HTTPD_REQUEST request;
  u32 i;

  thread = (cosm_HTTPD_THREAD *) arg;
  httpd = (cosm_HTTPD *) thread->httpd;
  CosmMemSet( &request, sizeof( cosm_HTTPD_REQUEST ), 0 );

  for ( ; ; )
  {
    /* wait for thread to be told to run */
    if ( CosmSemaphoreDown( &thread->semaphore, COSM_SEMAPHORE_WAIT )
      != COSM_PASS )
    {
      thread->state = COSM_HTTPD_THREAD_DEAD;
      CosmThreadEnd();
    }

    /* check if we should exit now */
    if ( thread->state == COSM_HTTPD_THREAD_EXIT )
    {
      CosmNetClose( &thread->net );
      if ( request.header_flag )
      {
        CosmBufferFree( &request.header );
        request.header_flag = 0;
        CosmMemFree( request.path );
      }
      thread->state = COSM_HTTPD_THREAD_DEAD;
      CosmThreadEnd();
    }

    /*
      we have a network connection, decode the header
      and call the right handlers. repeat until closed.
    */
    request.persistent = 0;
    while ( Cosm_HTTPDParseRequest( &request, &thread->net, httpd->wait_ms )
      == COSM_PASS )
    {
      request.thread_number = thread->thread_number;
      /* call correct handler, no matches means no call */
      for ( i = 0 ; i < httpd->handler_count ; i++ )
      {
        if ( CosmStrCmp( request.path, httpd->paths[i],
          CosmStrBytes( httpd->paths[i] ) ) == 0 )
        {
          if ( ( (*httpd->handlers[i])( &request ) ) != COSM_PASS )
          {
            /* failed */
            CosmNetClose( &thread->net );
          }
          break;
        }
      }

      if ( request.persistent != 1 )
      {
        /* dont keep connection open for another request */
        break;
      }
    }
    CosmNetClose( &thread->net );

    /* flag and wait for another SemaphoreUp */
    thread->state = COSM_HTTPD_THREAD_STOPPED;
  }
}

void Cosm_HTTPDMain( void * arg )
{
  cosm_HTTPD * httpd;
  cosm_HTTPD_THREAD * threads;
  cosm_NET tmp_net;
  u32 seek, found;
  u32 i, j;
  u32 sent;

  httpd = (cosm_HTTPD *) arg;

  /* open and listen on network */
  if ( CosmNetListen( &httpd->net, &httpd->host, COSM_NET_MODE_TCP,
    httpd->thread_count * 2 ) != COSM_PASS )
  {
    return;
  }

  /* create thread pool */
  if ( ( threads = (cosm_HTTPD_THREAD *) CosmMemAlloc(
    sizeof( cosm_HTTPD_THREAD ) * (u64) httpd->thread_count ) ) == NULL )
  {
    CosmNetClose( &httpd->net );
    return;
  }

  /* start threads and put into an idle state */
  for ( i = 0 ; i < httpd->thread_count ; i++ )
  {
    /* start them idle */
    if ( CosmSemaphoreInit( &threads[i].semaphore, 0 ) != COSM_PASS )
    {
      /* !!! this is a possible error under load */
    }
    threads[i].httpd = httpd;
    threads[i].thread_number = i;

    /* Test for bad thread creation */
    if ( CosmThreadBegin( &threads[i].id, Cosm_HTTPDThread, &threads[i],
      httpd->stack_size ) != COSM_PASS )
    {
      /* a thread failed to start, abort */
      CosmNetClose( &httpd->net );
      for ( j = 0 ; j < i ; j++ )
      {
        threads[j].state = COSM_HTTPD_THREAD_EXIT;
        (void) CosmSemaphoreUp( &threads[i].semaphore );
      }
      return;
    }
  }

  /* mark as running, and start listening */
  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    CosmThreadEnd();
    /* !!! need cleanup */
  }
  httpd->status = COSM_HTTPD_STATUS_RUNNING;
  CosmMutexUnlock( &httpd->lock );

  /* seek is incrementing, found is the last one we matched */
  seek = 0;
  found = 0;
  for ( ; ; )
  {
    CosmMemSet( &tmp_net, sizeof( cosm_NET ), 0 );
    if ( ( CosmNetAccept( &tmp_net, &httpd->net, NULL,
      COSM_NET_ACCEPT_WAIT ) != COSM_PASS )
      || ( httpd->httpd_thread_stop == 1 ) )
    {
      /* our main socket died, or we've been told to die, shut it down */
      CosmNetClose( &threads[seek].net );
      break;
    }
    else
    {
      while ( ( threads[seek].state != COSM_HTTPD_THREAD_STOPPED )
        && ( seek != found ) )
      {
        seek = ( seek + 1 ) % httpd->thread_count;
      }

      if ( threads[seek].state == COSM_HTTPD_THREAD_STOPPED )
      {
        /* found an available thread */
        threads[seek].state = COSM_HTTPD_THREAD_RUNNING;
        threads[seek].net = tmp_net;
        if ( CosmSemaphoreUp( &threads[seek].semaphore ) != COSM_PASS )
        {
          /* !!! oh oh */
        }
        found = seek;
        seek = ( seek + 1 ) % httpd->thread_count;
      }
      else
      {
        /* send a 503 */
        CosmNetSend( &tmp_net, &sent,
         "HTTP/1.1 503 Busy\r\nContent-Length: 0\r\n\r\n", 40 );
        CosmNetClose( &tmp_net );
        if ( found == seek )
        {
          seek = ( seek + 1 ) % httpd->thread_count;
        }
      }
    }
  }

  /* shut down all threads and exit */
  CosmNetClose( &httpd->net );
  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    CosmThreadEnd();
    /* !!! need cleanup */
  }
  httpd->status = COSM_HTTPD_STATUS_STOPPING;
  CosmMutexUnlock( &httpd->lock );

  /* this should finish as soon as the slowest thread is done */
  for ( i = 0 ; i < httpd->thread_count ; i++ )
  {
    while ( threads[i].state != COSM_HTTPD_THREAD_STOPPED )
    {
      CosmSleep( 10 );
    }
    threads[i].state = COSM_HTTPD_THREAD_EXIT;
    (void) CosmSemaphoreUp( &threads[i].semaphore );
    while ( threads[i].state != COSM_HTTPD_THREAD_DEAD )
    {
      CosmSleep( 10 );
    }
    /* need to free up the semaphores */
    CosmSemaphoreFree( &threads[i].semaphore );
  }

  /* threads are now all dead */
  if ( CosmMutexLock( &httpd->lock, COSM_MUTEX_WAIT ) != COSM_PASS )
  {
    CosmThreadEnd();
    /* !!! need cleanup */
  }
  httpd->status = COSM_HTTPD_STATUS_STOPPED;
  CosmMutexUnlock( &httpd->lock );
  CosmThreadEnd();
}

s32 Cosm_TestHTTP( void )
{
  return COSM_PASS;
}
