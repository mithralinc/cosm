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

#include "cosm/email.h"
#include "cosm/os_io.h"
#include "cosm/os_mem.h"

s32 CosmEmailSMTP( const cosm_NET_ADDR * smtp_server,
  const ascii * to, const ascii * from, const ascii * subject,
  const ascii * message, u32 length )
{
  cosm_NET net;
  ascii reply[4], buffer[64];
  cosm_NET_ADDR my_ip;
  u64 buffer_length;
  u32 last_idx, idx;
  u32 bytes;

  if ( to == NULL )
  {
    return COSM_EMAIL_ERROR_TO;
  }
  if ( from == NULL )
  {
    return COSM_EMAIL_ERROR_FROM;
  }
  if ( ( message == NULL ) && ( length != 0 ) )
  {
    return COSM_EMAIL_ERROR_MESSAGE;
  }

  if ( CosmMemSet( &net, sizeof( cosm_NET ), 0 ) != COSM_PASS )
  {
    return COSM_EMAIL_ERROR_ABORTED;
  }

  my_ip.type = COSM_NET_IPV4;
  my_ip.ip.v4 = 0;
  my_ip.port = smtp_server->port;

  if ( COSM_PASS != CosmNetOpen( &net, &my_ip, smtp_server,
    COSM_NET_MODE_TCP ) )
  {
    return COSM_EMAIL_ERROR_HOST;
  }

  if ( Cosm_EmailGetReply( reply, &net ) != COSM_PASS )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }
  if ( reply[0] != '2' )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( CosmNetMyIP( &my_ip, 1 ) == COSM_PASS )
  {
    buffer_length = CosmPrintStr( buffer, 64, "HELO %u.%u.%u.%u\r\n",
      ( my_ip.ip.v4 >> 24 ) & 0xFF, ( my_ip.ip.v4 >> 16 ) & 0xFF,
      ( my_ip.ip.v4 >> 8 ) & 0xFF, my_ip.ip.v4 & 0xFF );
  }
  else
  {
    /* Can't get my IP, just write something */
    buffer_length = CosmPrintStr( buffer, 64, "HELO Unknown\r\n" );
  }

  if ( CosmNetSend( &net, &bytes, buffer, (u32) buffer_length )
    != COSM_PASS )
  {
    /* Can't send HELO */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( Cosm_EmailGetReply( reply, &net ) != COSM_PASS )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }
  if ( reply[0] != '2' )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( ( CosmNetSend( &net, &bytes, "MAIL FROM:", 10 ) != COSM_PASS )
    || ( CosmNetSend( &net, &bytes, from, CosmStrBytes( from ) )
    != COSM_PASS ) || ( CosmNetSend( &net, &bytes, "\r\n", 2 ) != COSM_PASS ) )
  {
    /* Can't send FROM: command */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( Cosm_EmailGetReply( reply, &net ) != COSM_PASS )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( reply[0] != '2' )
  {
    if ( ( reply[0] == '5' ) && ( reply[1] == '0' ) && ( reply[2] == '1' ) )
    {
      CosmNetClose( &net );
      return COSM_EMAIL_ERROR_FROM;
    }
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( ( CosmNetSend( &net, &bytes, "RCPT TO:", 8 ) != COSM_PASS )
    || ( CosmNetSend( &net, &bytes, to, CosmStrBytes( to ) )
    != COSM_PASS ) || ( CosmNetSend( &net, &bytes, "\r\n", 2 ) != COSM_PASS ) )
  {
    /* Can't send TO: command */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( Cosm_EmailGetReply( reply, &net ) != COSM_PASS )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( reply[0] != '2' )
  {
    if ( ( reply[0] == '5' ) && ( reply[1] == '5' ) && ( reply[2] == '1' ) )
    {
      CosmNetClose( &net );
      return COSM_EMAIL_ERROR_NORELAY;
    }
    if ( ( reply[0] == '5' ) && ( reply[1] == '5' ) && ( reply[2] == '0' ) )
    {
      CosmNetClose( &net );
      return COSM_EMAIL_ERROR_TO;
    }
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( CosmNetSend( &net, &bytes, "DATA\r\n", 6 ) != COSM_PASS )
  {
    /* Can't send DATA command */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( Cosm_EmailGetReply( reply, &net ) != COSM_PASS )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( reply[0] != '3' )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  /* Sending headers */
  if ( ( CosmNetSend( &net, &bytes, "From: ", 6 ) != COSM_PASS )
    || ( CosmNetSend( &net, &bytes, from, CosmStrBytes( from ) )
    != COSM_PASS ) || ( CosmNetSend( &net, &bytes, "\r\n", 3 ) != COSM_PASS ) )
  {
    /* Can't send From: line */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( ( CosmNetSend( &net, &bytes, "To: ", 4 ) != COSM_PASS )
    || ( CosmNetSend( &net, &bytes, to, CosmStrBytes( to ) )
    != COSM_PASS ) || ( CosmNetSend( &net, &bytes, "\r\n", 3 ) != COSM_PASS ) )
  {
    /* Can't send To: line */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  /* send subject only if there is one */
  if ( subject != NULL )
  {
    if ( ( CosmNetSend( &net, &bytes, "Subject: ", 9 ) != COSM_PASS )
      || ( CosmNetSend( &net, &bytes, subject,
      CosmStrBytes( subject ) ) != COSM_PASS )
      || ( CosmNetSend( &net, &bytes, "\r\n", 3 ) != COSM_PASS ) )
    {
      /* Can't send Subject: line */
      CosmNetClose( &net );
      return COSM_EMAIL_ERROR_ABORTED;
    }
  }

  /* Sending message */
  last_idx = idx = 0;
  while ( idx < length )
  {
    if ( ( idx == last_idx ) && ( message[idx] == '.' ) )
    {
      /* Must double a dot at the beginning of a line */
      if ( CosmNetSend( &net, &bytes, ".", 1 ) != COSM_PASS )
      {
        /* Can't send the extra dot */
        CosmNetClose( &net );
        return COSM_EMAIL_ERROR_ABORTED;
      }
    }
    if ( message[idx] == '\r' )
    {
      if ( CosmNetSend( &net, &bytes, &message[last_idx],
        ( idx - last_idx + 1 ) ) != COSM_PASS )
      {
        /* Can't send that line of text */
        CosmNetClose( &net );
        return COSM_EMAIL_ERROR_ABORTED;
      }
      if ( CosmNetSend( &net, &bytes, "\n", 1 ) != COSM_PASS )
      {
        /* Can't send the carriage return */
        CosmNetClose( &net );
        return COSM_EMAIL_ERROR_ABORTED;
      }
      if ( ( idx < length - 1 ) && ( message[idx+1] == '\n' ) )
      {
        idx++;
      }
      last_idx = idx + 1;
    }
    else if ( message[idx] == '\n' )
    {
      if ( CosmNetSend( &net, &bytes, &message[last_idx], ( idx - last_idx ) )
        != COSM_PASS )
      {
        /* Can't send that line of text */
        CosmNetClose( &net );
        return COSM_EMAIL_ERROR_ABORTED;
      }
      if ( CosmNetSend( &net, &bytes, "\r\n", 2 ) != COSM_PASS )
      {
        /* Can't send the end of line */
        CosmNetClose( &net );
        return COSM_EMAIL_ERROR_ABORTED;
      }
      if ( ( idx < length - 1 ) && ( message[idx+1] == '\r' ) )
      {
        idx++;
      }
      last_idx = idx + 1;
    }
    idx++;
  }
  if ( last_idx < idx )
  {
    /* Send remaining text */
    if ( CosmNetSend( &net, &bytes, &message[last_idx], ( idx - last_idx ) )
      != COSM_PASS )
    {
      /* Can't send that line of text */
      CosmNetClose( &net );
      return COSM_EMAIL_ERROR_ABORTED;
    }
  }

  /* Close the DATA channel */
  if ( CosmNetSend( &net, &bytes, "\r\n.\r\n", 5 ) != COSM_PASS )
  {
    /* Can't send the end of DATA command */
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  if ( Cosm_EmailGetReply( reply, &net ) != COSM_PASS )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }
  if ( reply[0] != '2' )
  {
    CosmNetClose( &net );
    return COSM_EMAIL_ERROR_ABORTED;
  }

  CosmNetSend( &net, &bytes, "QUIT\r\n", 6 );
  CosmNetClose( &net );
  return COSM_PASS;
}

/* Low level functions */

s32 Cosm_EmailGetReply( ascii * reply, cosm_NET * net )
{
  ascii buffer[64];
  u32 size, timeout, wait = 120000;
  u32 i, j, part, lastline = 0;

  if ( ( reply == NULL ) || ( net == NULL ) )
  {
    return COSM_FAIL;
  }

  if ( ( CosmNetRecv( reply, &size, net, 4, wait ) != COSM_PASS )
    || ( size != 4 ) )
  {
    /* Reply code not received */
    return COSM_FAIL;
  }
  if ( reply[3] == ' ' )
  {
    /* Last line to receive */
    lastline = 1;
  }
  reply[3] = 0;

  timeout = 0;
  part = 0;
  for ( ; ; )
  {
    CosmNetRecv( &buffer[part], &size, net, 64 - part, timeout );
    if ( size == 0 )
    {
      /* No rest of line received */
      return COSM_FAIL;
    }
    part = 0;
    if ( lastline == 0 )
    {
      for ( i = 0 ; i < ( size - 1 ) ; i++ )
      {
        if ( buffer[i] == '\r' )
        {
          if ( i > ( size - 6 ) )
          {
            part = size - i;
            for ( j = 0 ; i < size ; i++ )
            {
              buffer[j] = buffer[i];
              j++;
            }
            break;
          }
          if ( buffer[i+1] == '\n' )
          {
            i += 5;
            if ( buffer[i] == ' ' )
            {
              /* Only one line left to receive */
              lastline = 1;
              break;
            }
          }
        }
      }
    }
    if ( lastline == 1 )
    {
      if ( buffer[size-2] == '\r' && buffer[size-1] == '\n' )
      {
        /* Reply text all received */
        return COSM_PASS;
      }
      if ( buffer[size-1] == '\r' )
      {
        buffer[0] = '\r';
        part = 1;
      }
    }

    if ( size < 64 )
    {
      /* Leave time for more text to arrive */
      timeout = 30000;
    }
    else
    {
      timeout = 0;
    }
  }
}

/* testing */

s32 Cosm_TestEmail( void )
{
  /* No automatic test can be done for now */
  return COSM_PASS;
}
