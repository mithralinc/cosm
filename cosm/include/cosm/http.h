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

#ifndef COSM_HTTP_H
#define COSM_HTTP_H

#include "cosm/cputypes.h"
#include "cosm/os_net.h"
#include "cosm/buffer.h"
#include "cosm/log.h"

#define COSM_HTTP_ERROR_URI     -1 /* URI invalid */
#define COSM_HTTP_ERROR_NET     -2 /* Host/proxy unreachable */
#define COSM_HTTP_ERROR_MEMORY  -3 /* Buffer/memory error */
#define COSM_HTTP_ERROR_PARAM   -4 /* Parameter error */
#define COSM_HTTP_ERROR_CLOSED  -5 /* Connection closed or aborted */
#define COSM_HTTP_ERROR_ORDER   -6 /* Functions called in wrong order */
#define COSM_HTTP_ERROR_VERSION -7 /* Unsupported versions of HTTPD protcol */

#define COSM_HTTP_STATUS_NONE    0
#define COSM_HTTP_STATUS_CLOSED  1 /* connection closed but settings valid */
#define COSM_HTTP_STATUS_IDLE    2 /* no get/post in progress */
#define COSM_HTTP_STATUS_BODY    3 /* ready to read body */

#define COSM_HTTP_VERSION_0_9    9 /* HTTP/0.9 and before */
#define COSM_HTTP_VERSION_1_0   10 /* HTTP/1.0 */
#define COSM_HTTP_VERSION_1_1   11 /* HTTP/1.1 */

#define COSM_HTTP_CHUNKING_CLOSE    1 /* length unknown, read till close */
#define COSM_HTTP_CHUNKING_FIXED    2 /* Known length, unused by Cosm */
#define COSM_HTTP_CHUNKING_CHUNKED  3 /* chunked encoding */

#define COSM_HTTP_MAX_HOSTNAME ( COSM_NET_MAX_HOSTNAME + 32 )

typedef struct cosm_HTTP
{
  cosm_NET net;
  ascii host_line[COSM_HTTP_MAX_HOSTNAME + 1];
  u32 status;
  u32 version;
  u32 chunking;
  u32 persistent;
  u32 http_status;
  u32 length;
  cosm_NET_ADDR host;
  cosm_NET_ADDR proxy;
  ascii * proxy_auth;
  ascii * user_auth;
  cosm_BUFFER header;
  u32 header_flag;
} cosm_HTTP;

#define COSM_HTTPD_ERROR_ADDRESS  -1 /* Unable to listen on host/addr */
#define COSM_HTTPD_ERROR_PARAM    -2 /* Parameter error */
#define COSM_HTTPD_ERROR_ORDER    -3 /* Functions called in wrong order */
#define COSM_HTTPD_ERROR_LOGFILE  -4 /* Unable to open logfile */
#define COSM_HTTPD_ERROR_TIMEOUT  -5 /* Unable to start/stop in time */
#define COSM_HTTPD_ERROR_NET      -6 /* Network Error */
#define COSM_HTTPD_ERROR_MEMORY   -7 /* Buffer/memory error */

#define COSM_HTTPD_STATUS_NONE      0
#define COSM_HTTPD_STATUS_IDLE      1 /* not running and no handlers */
#define COSM_HTTPD_STATUS_STOPPED   2 /* valid setup, not running */
#define COSM_HTTPD_STATUS_STARTING  3 /* starting up */
#define COSM_HTTPD_STATUS_STOPPING  4 /* shutting down */
#define COSM_HTTPD_STATUS_RUNNING   5 /* running */

#define COSM_HTTPD_THREAD_STOPPED  0
#define COSM_HTTPD_THREAD_RUNNING  1
#define COSM_HTTPD_THREAD_EXIT     2
#define COSM_HTTPD_THREAD_DEAD     3

#define COSM_HTTPD_REQUEST_GET   0
#define COSM_HTTPD_REQUEST_POST  1

#define COSM_HTTPD_REQUEST_NONE    0
#define COSM_HTTPD_REQUEST_START   1
#define COSM_HTTPD_REQUEST_HEADER  2
#define COSM_HTTPD_REQUEST_BODY    3

typedef struct cosm_HTTPD_REQUEST
{
  u32 type;
  u32 version;
  u32 state;
  u32 persistent;
  ascii * path;
  cosm_NET * net;
  cosm_BUFFER header;
  u32 header_flag;
  u32 post_length;
  ascii * get_data;
  u32 thread_number;
} cosm_HTTPD_REQUEST;

typedef struct cosm_HTTPD_THREAD
{
  u64 id;
  u32 state;
  u32 thread_number;
  void * httpd;
  cosm_NET net;
  cosm_SEMAPHORE semaphore;
} cosm_HTTPD_THREAD;

typedef struct cosm_HTTPD
{
  cosm_NET net;
  u32 status;
  u32 handler_count;
  ascii **paths;
  s32 (**handlers)( cosm_HTTPD_REQUEST * request );
  u32 log_active;
  cosm_LOG log;
  cosm_NET_ADDR host;
  u32 wait_ms;
  u32 thread_count;
  u32 stack_size;
  u64 httpd_thread;
  u32 httpd_thread_stop;
  cosm_MUTEX lock;
} cosm_HTTPD;

/* High level functions */

s32 CosmHTTPOpen( cosm_HTTP * http, const ascii * uri,
  const cosm_NET_ADDR * proxy, const ascii * proxy_name,
  const ascii * proxy_pass, const ascii * user_name,
  const ascii * user_pass );
  /*
    Open an HTTP/1.1 (RFC 2616) connection to the given uri.
    The uri must be of the standard "http://host[:port][/][ignored]"
    form, anything past and including the single slash is ignored.
    proxy_host and proxy_port are the address of the web proxy, and are used
    if both are non-zero. The name and pass parameters are for the proxy and
    web page passwords.
    Returns: COSM_PASS on success, or an error code on failure.
  */

#define _COSM_HTTPOPEN( http, uri ) \
  CosmHTTPOpen( http, uri, NULL, NULL, NULL, NULL, NULL )
  /*
    Since CosmHTTPOpen is rather complex, a macro for simpler cases is also
    defined. _COSM_HTTPOPEN(...) allows just the first 2 parameters to be
    specified for when you do not need to worry about proxies or passwords.
  */

s32 CosmHTTPGet( cosm_HTTP * http, u32 * status, const ascii * uri_path,
  u32 wait_ms );
  /*
    Using the open http connection, open the uri_path given for reading
    and place the resulting HTTP status code into status. CGI
    parameters may be passed as part of the uri_path. This function is
    only used to read static web data. wait_ms is the network timeout
    while looking for a response to see if anything is coming back or not.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPPost( cosm_HTTP * http, u32 * status, const ascii * uri_path,
  const void * data, u32 length, u32 wait_ms );
  /*
    Using the open http connection, send length bytes of data to the
    uri_path which is then opened for reading and the resulting HTTP status
    code is put into status. CGI parameter data may be sent as data, but
    none is allowed in the uri_path with this method. This function is
    primarily for communication when network traffic of blocked, but is
    fully compatable with web servers. wait_ms is the network timeout
    while looking for a response to see if anything is coming back or not.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPRecv( void * buffer, u32 * bytes_received, cosm_HTTP * http,
  u32 length, u32 wait_ms );
  /*
    Read up to length bytes of the resulting get/post data from the server
    into the buffer, waiting up to wait_ms milliseconds for data to arrive.
    bytes_read is set to the number of bytes actually read.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPClose( cosm_HTTP * http );
  /*
    Close the http connection and free any remaining data.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDInit( cosm_HTTPD * httpd, ascii * log_path, u32 log_level,
  u32 threads, u32 stack_size, const cosm_NET_ADDR * host, u32 wait_ms );
  /*
    Initialize a server with threads threads, running on the ip and port
    given. log_path and log_level are for the server log using a CosmLog.
    wait_ms is the network timeout while parsing the request.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDSetHandler( cosm_HTTPD * httpd, const ascii * path,
  cosm_NET_ACL * acl, s32 (*handler)( cosm_HTTPD_REQUEST * request ) );
  /*
    A handler will be called when a client mathching the ACL requests a
    URL starting with path. The handler with the longest matching path
    will be called, and the order handlers are added does not matter.
    if handler is NULL, the handler is removed. At minimum you must
    have a handler set for the path "/" which will be called if no other
    handler matches.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDStart( cosm_HTTPD * httpd, u32 timeout_ms );
  /*
    Starts the httpd server once it is initialized and handlers are set.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDStop( cosm_HTTPD * httpd, u32 timeout_ms );
  /*
    Stops the httpd server, possibly to change the handlers. If the
    threads cannot be shutdown within timeout_ms milliseconds, then
    COSM_HTTPD_ERROR_TIMEOUT is returned and you can attempt to stop it again.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDSendInit( cosm_HTTPD_REQUEST * request, u32 status_code,
  ascii * status_string, ascii * mime_type );
  /*
    This is the first function that should be called in any handler.
    status_code is the 3 digit HTTP status code. status_string is the text
    associated with the that code - for 200 it is "OK".
    mime_type is the media type of data to be sent based on the content.
    Further information on these parameters can be found in the HTTP/1.1
    documentation (RFC 2616).
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDSendHead( cosm_HTTPD_REQUEST * request, const void * string );
  /*
    Send the string to the client as part of the HTTP header sent in
    handler functions. This function must be used after CosmHTTPDSendInit
    but before any CosmHTTPDSend are called.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDSend( cosm_HTTPD_REQUEST * request, const void * data,
  u32 length );
  /*
    Send length bytes of data to the client in a handler function.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDRecv( void * buffer, u32 * bytes_received,
  cosm_HTTPD_REQUEST * request, u32 length, u32 wait_ms );
  /*
    Read up to length bytes of the posted data from the client
    into the buffer, waiting up to wait_ms milliseconds for data to arrive.
    bytes_received is set to the number of bytes actually read.
    For use in handler functions.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmHTTPDFree( cosm_HTTPD * httpd );
  /*
    Free the httpd and any remaining data. A server must not be running
    in order to free it.
    Returns: COSM_PASS on success, or an error code on failure.
  */

/* low level */

s32 Cosm_HTTPNetOpen( cosm_HTTP * http );
  /*
    Open the needed network connection to the host or proxy.
    Returns: COSM_PASS on success, COSM_FAIL on failure.
  */

s32 Cosm_HTTPParseHeader( cosm_HTTP * http, u32 wait_ms );
  /*
    Parse the returned HTTP header beyond the status line and setup for Recv.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_HTTPChunkLength( cosm_HTTP * http, u32 wait_ms );
  /*
    Read one line and get the hex encoded chunk length into length.
    Returns: COSM_PASS on success, COSM_FAIL on failure.
  */

s32 Cosm_HTTPDParseRequest( cosm_HTTPD_REQUEST * request, cosm_NET * net,
  u32 wait_ms );
  /*
    Parse the HTTP request.
    Returns: COSM_PASS on success, or an error code on failure.
  */

void Cosm_HTTPDThread( void * arg );
  /*
    HTTPD thread, handles one connection and calls handlers.
    Returns: Nothing.
  */

void Cosm_HTTPDMain( void * arg );
  /*
    Main HTTPD engine, accepts connections and triggers threads.
    Returns: Nothing.
  */

/* testing */

s32 Cosm_TestHTTP( void );
  /*
    Test functions in this header.
    Returns: COSM_PASS on success, or a negative number corresponding to the
      test that failed.
  */

#endif
