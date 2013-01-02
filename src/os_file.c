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

/* CPU/OS Layer - CPU and OS specific code is allowed */

#include "cosm/os_file.h"
#include "cosm/os_mem.h"
#include "cosm/os_io.h"

#include <errno.h>
#include <stdio.h>      /* for remove */
#include <string.h>

#include <sys/stat.h>   /* for stat and open */
#include <fcntl.h>      /* for open */
#include <sys/types.h>  /* for open and lseek */

#define COSM_FILE_PATH_UNIX 0
#define COSM_FILE_PATH_DOS  1
#define COSM_FILE_PATH_MAC  2

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#include <io.h>
#include <direct.h>
#define getcwd _getcwd
#define COSM_FILE_PATHMODE COSM_FILE_PATH_DOS
#else /* unixes */
#include <unistd.h>    /* for stat, close, read, write, lseek, ftruncate */
#include <sys/file.h>  /* for flock */
#include <sys/mman.h>  /* for mmap, munmap */
#define COSM_FILE_PATHMODE COSM_FILE_PATH_UNIX
#endif

/* Setup the size of file handle ( not the max size of a file ) */
#if 0
  /* int, off_t, size_t ans ssize_t are 64 bits */
#define COSM_FILE64
#else
  /* int, off_t, size_t ans ssize_t are 32 bits */
#undef COSM_FILE64
#endif

s32 CosmFileOpen( cosm_FILE * file, const ascii * filename,
  u32 mode, u32 lock )
{
  struct stat file_stats;
  cosm_FILENAME native_filename;
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE handle;
  DWORD share_mode, access_mode, create_mode, flags;
  LARGE_INTEGER zero;
#else
  int open_flags;
  int open_mode;
  int open_result;
#endif
  s32 error;

  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  if ( file->status != COSM_FILE_STATUS_CLOSED )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  /* Check if mode is valid */
  if ( mode & ~( COSM_FILE_MODE_EXIST | COSM_FILE_MODE_READ | COSM_FILE_MODE_WRITE |
    COSM_FILE_MODE_APPEND | COSM_FILE_MODE_CREATE | COSM_FILE_MODE_TRUNCATE |
    COSM_FILE_MODE_SYNC ) )
  {
    return COSM_FILE_ERROR_MODE;
  }

  /* Check if lock is valid */
  if ( lock & ~( COSM_FILE_LOCK_NONE | COSM_FILE_LOCK_READ |
    COSM_FILE_LOCK_WRITE ) )
  {
    return COSM_FILE_ERROR_MODE;
  }

  if ( ( ( mode & COSM_FILE_MODE_WRITE ) != COSM_FILE_MODE_WRITE ) &&
    ( ( mode & COSM_FILE_MODE_TRUNCATE ) == COSM_FILE_MODE_TRUNCATE ) )
  {
    /* must be writing to truncate */
    return COSM_FILE_ERROR_MODE;
  }

  if ( ( ( mode & COSM_FILE_MODE_READ ) == COSM_FILE_MODE_READ ) &&
    ( ( mode & COSM_FILE_MODE_APPEND ) == COSM_FILE_MODE_APPEND ) )
  {
    /* append is write only */
    return COSM_FILE_ERROR_MODE;
  }

  /* end parameter checking */

  /* translate filename */
  if ( Cosm_FileNativePath( native_filename, filename ) != COSM_PASS )
  {
    return COSM_FILE_ERROR_NAME;
  }

  /* Save filename, mode, and locks */
  CosmStrCopy( file->filename, filename, (u64) COSM_FILE_MAX_FILENAME );
  file->mode = mode;
  file->lockmode = lock;

  /* check for existance only */
  if ( ( mode & COSM_FILE_MODE_EXIST ) == COSM_FILE_MODE_EXIST )
  {
    /* We are using stat to test file existence */
    if ( stat( (const char *) native_filename, &file_stats ) == -1 )
    {
      if ( errno == ENOENT )
      {
        return COSM_FILE_ERROR_NOTFOUND;
      }
      /* another error as occured */
      return COSM_FILE_ERROR_DENIED;
    }

    return COSM_PASS;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )

  access_mode = ( ( mode & COSM_FILE_MODE_READ ) ? GENERIC_READ : 0 )
    | ( ( mode & COSM_FILE_MODE_WRITE ) ? GENERIC_WRITE : 0 )
    | ( ( mode & COSM_FILE_MODE_APPEND ) ? GENERIC_WRITE : 0 );

  share_mode = FILE_SHARE_DELETE
    | ( ( lock & COSM_FILE_LOCK_READ ) ? 0 : FILE_SHARE_READ )
    | ( ( lock & COSM_FILE_LOCK_WRITE ) ? 0 : FILE_SHARE_WRITE );

  create_mode = ( mode & COSM_FILE_MODE_CREATE ) ? OPEN_ALWAYS : OPEN_EXISTING;

  flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_POSIX_SEMANTICS
   | ( ( mode & COSM_FILE_MODE_NOBUFFER ) ? COSM_FILE_MODE_NOBUFFER : 0 )
   | ( ( mode & COSM_FILE_MODE_SYNC ) ? FILE_FLAG_WRITE_THROUGH : 0 );

  if ( ( handle = CreateFile( native_filename, access_mode,
    share_mode, NULL, create_mode, flags, NULL ) ) == INVALID_HANDLE_VALUE )
  {
    switch ( error = GetLastError() )
    {
      case ERROR_FILE_NOT_FOUND:
        return COSM_FILE_ERROR_NOTFOUND;
        break;
      case ERROR_SHARING_VIOLATION:
        if ( lock & COSM_FILE_LOCK_READ )
        {
          return COSM_FILE_ERROR_READLOCK;
        }
        else
        {
          return COSM_FILE_ERROR_WRITELOCK;
        }
        break;
      case ERROR_ACCESS_DENIED:
        return COSM_FILE_ERROR_DENIED;
        break;
      default:
        return COSM_FILE_ERROR_DENIED;
        break;
    }
  }

  if ( mode & COSM_FILE_MODE_APPEND )
  {
    zero.QuadPart = 0;
    if ( SetFilePointerEx( handle, zero, NULL, FILE_END ) == 0 )
    {
      return COSM_FILE_ERROR_SEEK;
    }
  }

  file->handle = handle;
#else /* not windows */
  /* convert mode to native value */
  if ( mode & COSM_FILE_MODE_READ )
  {
    if ( ( mode & COSM_FILE_MODE_WRITE )
      || ( mode & COSM_FILE_MODE_APPEND ) )
    {
      open_flags = O_RDWR;
    }
    else
    {
      open_flags = O_RDONLY;
    }
  }
  else
  {
    if ( ( mode & COSM_FILE_MODE_WRITE )
      || ( mode & COSM_FILE_MODE_APPEND ) )
    {
      open_flags = O_WRONLY;
    }
    else
    {
      return COSM_FILE_ERROR_MODE;
    }
  }

  /* Check any other flags */
  if ( mode & COSM_FILE_MODE_APPEND )
  {
    open_flags = open_flags | O_APPEND;
  }
  open_mode = 0;

  if ( mode & COSM_FILE_MODE_CREATE )
  {
    open_flags = open_flags | O_CREAT;
#if ( defined( S_IRGRP ) )
    /* platform handling user groups */
    open_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP;
#else
    /* platform not handling user groups -> use old style */
    open_mode = S_IREAD | S_IWRITE | S_IEXEC;
#endif
  }

#if ( defined( O_SYNC ) )
  if ( mode & COSM_FILE_MODE_SYNC )
  {
    open_flags = open_flags | O_SYNC;
  }
#endif

  /* force binary type open */
#if ( defined( O_BINARY ) )
  open_flags = open_flags | O_BINARY;
#endif

  /* Really try to open the file */
  open_result = open( (const char *) native_filename,
    open_flags, open_mode );
  if ( open_result == -1 )
  { /* Error during opening */
    switch( errno )
    {
      case ENOENT:
        error = COSM_FILE_ERROR_NOTFOUND;
        break;
      case EACCES:
        error = COSM_FILE_ERROR_DENIED;
        break;
#if ( defined( EROFS ) )
      case EROFS:
        error = COSM_FILE_ERROR_WRITEMODE;
        break;
#endif
      default:
        error = COSM_FILE_ERROR_DENIED;
    }
    return error;
  }

  /* File correctly open, store the handle */
  file->handle = (u64) open_result;

  /* Now lock if desired */
  if ( lock != COSM_FILE_LOCK_NONE )
  {
    if ( ( error = Cosm_FileLock( file, lock ) ) != COSM_PASS )
    {
      CosmFileClose( file );
      return error;
    }
  }

#endif /* OS */

  /* whipe and truncate if wanted */
  if ( mode & COSM_FILE_MODE_TRUNCATE )
  {
    if ( ( error = Cosm_FileTruncate( file, 0 ) ) != COSM_PASS )
    {
      CosmFileClose( file );
      return error;
    }
  }

  file->status = COSM_FILE_STATUS_OPEN;

  return COSM_PASS;
}

s32 CosmFileRead( void * buffer, u64 * bytes_read, cosm_FILE * file, u64 length )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  DWORD this_read, chunk;
  u64 remaining = length, total_read;
  u8 * ptr;

  *bytes_read = 0x0000000000000000LL;
  total_read = 0x0000000000000000LL;

  ptr = (u8 *) buffer;

  /* we can only write a DWORD at a time due to Win API*/
  while ( remaining > 0 )
  {
    /* use 1GiB chunks */
    chunk = ( remaining > 0x40000000LL ) ? 0x40000000 : (u32) remaining;
    if ( ReadFile( file->handle, ptr, chunk, &this_read, NULL ) == 0 )
    {
      *bytes_read = total_read;
      return COSM_FILE_ERROR_DENIED;
    }
    if ( this_read == 0 )
    {
      *bytes_read = total_read;
      return COSM_FILE_ERROR_EOF;
    }
    ptr += chunk;
    total_read += (u64) this_read;
    remaining -= (u64) chunk;
  }

  *bytes_read = total_read;
#else
  int real_read;
  u64 tmp;
  s32 error;

  *bytes_read = 0;

  if ( ( file == NULL ) || ( bytes_read == NULL ) )
  {
    return COSM_FILE_ERROR_PARAM;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  error = COSM_PASS;

#if ( defined( COSM_FILE64 ) )
  /* this should be valid since file handle is 64bit, int should be too */
  real_read = read( (int) file->handle, buffer, length );
#else
  tmp = 0x000000007FFFFFFFLL;
  if ( ( length > tmp ) )
  {
    return COSM_FILE_ERROR_SEEK;
  }
  real_read = read( (int) (u32) file->handle, buffer,
    (u32) length );
#endif
  if ( real_read == -1 )
  {
    if ( errno == EACCES )
    {
      error = COSM_FILE_ERROR_DENIED;
    }
    else
    {
      error = COSM_FILE_ERROR_NOTFOUND;
    }

    Cosm_FileTell( &tmp, file );

    return error;
  }

  *bytes_read = real_read;
#endif /* OS */

  return COSM_PASS;
}

s32 CosmFileWrite( cosm_FILE * file, u64 * bytes_written,
  const void * const buffer, u64 length )
{
  *bytes_written = (u64) 0;

  if ( ( file == NULL ) || ( bytes_written == NULL ) )
  {
    return COSM_FILE_ERROR_PARAM;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  return Cosm_FileWrite( file, bytes_written, buffer, length );
}

s32 CosmFileSeek( cosm_FILE * file, u64 offset )
{
  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  return Cosm_FileSeek( file, offset );
}

s32 CosmFileTell( u64 * offset, cosm_FILE * file )
{
  u64 len;
  s32 result;

  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  if ( ( result = Cosm_FileTell( &len, file ) ) == COSM_PASS )
  {
    *(offset) = len;
  }

  return result;
}

s32 CosmFileEOF( cosm_FILE * file )
{
  u64 current;
  u64 real;
  s32 result;

  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  if ( ( result = Cosm_FileTell( &current , file ) ) != COSM_PASS )
  {
    return result;
  }

  if ( ( result = Cosm_FileLength( &real, file ) ) != COSM_PASS )
  {
    return result;
  }

  /* We compare file size and current position to know if we are at EOF */
  if ( current == real )
  {
    return COSM_FILE_ERROR_EOF;
  }

  /* not EOF */
  return COSM_PASS;
}

s32 CosmFileLength( u64 * length, cosm_FILE * file )
{
  s32 result;
  u64 len;

  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  if ( ( result = Cosm_FileLength( &len, file ) ) != COSM_PASS )
  {
    return result;
  }

  *(length) = len;


  return COSM_PASS;
}

s32 CosmFileTruncate( cosm_FILE * file, u64 length )
{
  s32 result;

  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  if ( ( result = Cosm_FileTruncate( file, length ) ) != COSM_PASS )
  {
    return result;
  }

  return COSM_PASS;
}

s32 CosmFileDelete( const ascii * filename )
{
  cosm_FILENAME native_filename;
  cosm_FILE wipe_file;
  s32 error;

  /* Convert slashs to native format and put it into native_filename */
  if ( Cosm_FileNativePath( native_filename, filename ) != COSM_PASS )
  {
    return COSM_FILE_ERROR_NAME;
  }

  /* open the file for write only, exclusive access, and zero out */
  CosmMemSet( &wipe_file, sizeof( cosm_FILE ), 0 );
  error = CosmFileOpen( &wipe_file, filename, COSM_FILE_MODE_WRITE |
    COSM_FILE_MODE_SYNC, COSM_FILE_LOCK_WRITE );
  if ( error != COSM_PASS )
  {
    if ( error == COSM_FILE_ERROR_NOTFOUND )
    {
      /* no such file */
      return COSM_PASS;
    }
    else
    {
      return error;
    }
  }

  if ( ( error = Cosm_FileWipe( &wipe_file, 0 ) ) != COSM_PASS )
  {
    return error;
  }

  CosmFileClose( &wipe_file );

  if ( remove( (const char *) native_filename ) != 0 )
  {
    if ( errno == ENOENT )
    {
      /* this should have been cought above */
      return COSM_PASS;
    }
    else /* EACCES */
    {
      return COSM_FILE_ERROR_DENIED;
    }
  }

  return COSM_PASS;
}

s32 CosmFileInfo( cosm_FILE_INFO * info, const ascii * filename )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  struct __stat64 buf;
#else
#if ( ( OS_TYPE == OS_LINUX ) && defined( __USE_LARGEFILE64 ) )
#define _STAT stat64
#else
#define _STAT stat
#endif
  struct _STAT buf;
#endif
  cosm_FILENAME native_filename;
  cosmtime unix_epoc;

  if ( ( info == NULL ) || ( filename == NULL ) )
  {
    return COSM_FILE_ERROR_DENIED;
  }

  /* translate filename */
  if ( Cosm_FileNativePath( native_filename, filename ) != COSM_PASS )
  {
    return COSM_FILE_ERROR_NAME;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( _stat64( (const char *) native_filename, &buf ) != 0 )
#else
  if ( _STAT( (const char *) native_filename, &buf ) != 0 )
#endif
  {
    switch( errno )
    {
      case ENOENT:
        return COSM_FILE_ERROR_NOTFOUND;
        break;
      case EACCES:
      default:
        return COSM_FILE_ERROR_DENIED;
    }
  }

  /* length */
  info->length = buf.st_size;

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /* type */
  if ( ( buf.st_mode & _S_IFREG ) == _S_IFREG )
  {
    info->type = COSM_FILE_TYPE_FILE;
  }
  else if ( ( buf.st_mode & _S_IFDIR ) == _S_IFDIR )
  {
    info->type = COSM_FILE_TYPE_DIR;
  }
  else if ( ( buf.st_mode & _S_IFCHR ) == _S_IFCHR )
  {
    info->type = COSM_FILE_TYPE_DEVICE;
  }
  else
  {
    info->type = COSM_FILE_TYPE_SPECIAL;
  }

  /* rights */
  info->rights = 0;
  if ( ( buf.st_mode & _S_IREAD ) == _S_IREAD )
  {
    info->rights += COSM_FILE_RIGHTS_READ;
  }
  if ( ( buf.st_mode & _S_IWRITE ) == _S_IWRITE )
  {
    info->rights += COSM_FILE_RIGHTS_WRITE;
    info->rights += COSM_FILE_RIGHTS_APPEND;
  }
  if ( ( buf.st_mode & _S_IEXEC ) == _S_IEXEC )
  {
    info->rights += COSM_FILE_RIGHTS_EXEC;
  }
#else /* OS */
  /* type */
  if ( ( buf.st_mode & S_IFREG ) == S_IFREG )
  {
    info->type = COSM_FILE_TYPE_FILE;
  }
  else if ( ( buf.st_mode & S_IFDIR ) == S_IFDIR )
  {
    info->type = COSM_FILE_TYPE_DIR;
  }
  else if ( ( ( buf.st_mode & S_IFCHR ) == S_IFCHR ) ||
    ( ( buf.st_mode & S_IFBLK ) == S_IFBLK ) )
  {
    info->type = COSM_FILE_TYPE_DEVICE;
  }
  else
  {
    info->type = COSM_FILE_TYPE_SPECIAL;
  }

  /* rights */
  info->rights = 0;
  if ( ( buf.st_mode & S_IRUSR ) == S_IRUSR )
  {
    info->rights += COSM_FILE_RIGHTS_READ;
  }
  if ( ( buf.st_mode & S_IWUSR ) == S_IWUSR )
  {
    info->rights += COSM_FILE_RIGHTS_WRITE;
    info->rights += COSM_FILE_RIGHTS_APPEND;
  }
  if ( ( buf.st_mode & S_IXUSR ) == S_IXUSR )
  {
    info->rights += COSM_FILE_RIGHTS_EXEC;
  }
#endif /* OS */

  /* times */
  _COSM_SET128( unix_epoc, 00000000386D4380, 0000000000000000 );

  info->create.hi = (s64) buf.st_mtime;
  info->create.lo = 0x0000000000000000LL;
  info->create = CosmS128Sub( info->create, unix_epoc );

  info->modify.hi = (s64) buf.st_ctime;
  info->modify.lo = 0x0000000000000000LL;
  info->modify = CosmS128Sub( info->modify, unix_epoc );

  info->access.hi = (s64) buf.st_atime;
  info->access.lo = 0x0000000000000000LL;
  info->access = CosmS128Sub( info->access, unix_epoc );

  return COSM_PASS;
}

u64 CosmFileMapPageSize( void )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  SYSTEM_INFO info;
  
  GetSystemInfo( &info );
  return (u64) info.dwPageSize;
#else
  return (u64) getpagesize();
#endif
}

void * CosmFileMap( cosm_FILE_MEMORY_MAP * map, cosm_FILE * file,
  u64 length, u64 offset )
{
  void * addr;
  int access;
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  int map_mode;
#endif
 
  if ( ( NULL == map ) || ( NULL == file ) || ( 0 == length ) )
  {
    return NULL;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( ( file->mode & COSM_FILE_MODE_READ )
    && ( file->mode & COSM_FILE_MODE_WRITE ) )
  {
    access = PAGE_READWRITE;
    map_mode = FILE_MAP_WRITE;
  }
  else if ( file->mode & COSM_FILE_MODE_READ )
  {
    access = PAGE_READONLY;
    map_mode = FILE_MAP_READ;
  }
  else
  {
    /* not a valid mode for the open file */
    return NULL;
  }

  if ( NULL == ( map->file_mapping = CreateFileMapping(
    file->handle, NULL, access, 0xFFFFFFFF, 0xFFFFFFFF, NULL ) ) )
  {
    /* bad failure */
    return NULL;
  }

  if ( NULL == ( addr = MapViewOfFile( map->file_mapping,
    map_mode, (u32) ( offset >> 32 ), (u32) offset, (SIZE_T) length ) ) )
  {
    CloseHandle( map->file_mapping );
    return NULL;
  }
#else
  access = ( ( file->mode & COSM_FILE_LOCK_READ ) ? PROT_READ : 0 )
    | ( ( file->mode & COSM_FILE_LOCK_WRITE ) ? PROT_WRITE : 0 )
    | ( ( file->mode & COSM_FILE_MODE_APPEND ) ? PROT_WRITE : 0 );
  
  if ( NULL == ( addr = mmap( NULL, length, access, MAP_SHARED,
    file->handle, offset ) ) )
  {
    return NULL;
  }
#endif

  map->memory = addr;
  map->length = length;
  return addr;
}

s32 CosmFileUnmap( cosm_FILE_MEMORY_MAP * map )
{
 if ( ( NULL == map ) || ( NULL == map->memory ) )
 {
   return COSM_FAIL;
 }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  UnmapViewOfFile( map->memory );
  CloseHandle( map->file_mapping );
#else
  munmap( map->memory, map->length );
#endif

  return COSM_PASS;
}


s32 CosmFileClose( cosm_FILE * file )
{
  if ( file == NULL )
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  /* First check if the file is open */
  if ( file->status != COSM_FILE_STATUS_OPEN )
  {
    return COSM_FILE_ERROR_CLOSED;
  }

  /* Release lock */
  Cosm_FileUnLock( file );
  /* !!! deal with failed case, not good, very very not good */

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( CloseHandle( file->handle ) == 0 )
#else
#if ( defined( COSM_FILE64 ) )
  if ( close( (int) file->handle ) != 0 )
#else
  if ( close( (int) (u32) file->handle ) != 0 )
#endif
#endif /* OS */
  {
    return COSM_FILE_ERROR_NOTFOUND;
  }

  file->status = COSM_FILE_STATUS_CLOSED;


  return COSM_PASS;
}

/* Directory functions. */

s32 CosmDirOpen( cosm_DIR * dir, const ascii * dirname, u32 mode )
{
  cosm_FILENAME native_dirname;
  s32 error;
  s32 last_error;
  u32 higher_levels;
  cosm_FILENAME tmp_native_dirname;
  cosm_FILENAME tmp_dirname;
  ascii * currpos, * nativepos;
  u32 size, i;

  if ( dir == NULL )
  {
    return COSM_DIR_ERROR_NOTFOUND;
  }


  if ( dir->status != COSM_DIR_STATUS_CLOSED )
  {
    return COSM_DIR_ERROR_CLOSED;
  }

  if ( ( ( mode & COSM_DIR_MODE_EXIST ) == COSM_DIR_MODE_EXIST )
    && ( ( mode & COSM_DIR_MODE_CREATE ) == COSM_DIR_MODE_CREATE ) )
  {
    /* mode combination is invalid */
    return COSM_DIR_ERROR_MODE;
  }

  if ( mode & ~( COSM_DIR_MODE_EXIST | COSM_DIR_MODE_CREATE ) )
  {
    /* Undefined modes used */
    return COSM_DIR_ERROR_MODE;
  }

  /* Convert dirname and put it into native_dirname */
  if ( Cosm_FileNativePath( native_dirname, dirname ) != COSM_PASS )
  {
    return COSM_DIR_ERROR_NAME;
  }

  if ( ( mode & COSM_DIR_MODE_CREATE ) == COSM_DIR_MODE_CREATE )
  {
    /* We are trying to create the directory */

    /*
      Find the first | to find the native part of the filename, if any.
      We do not try to create the native part.
    */
    i = size = CosmStrBytes( dirname );
    nativepos = CosmStrChar( dirname, '|', i );

    /* Make a copy of the path. */
    size++;
    if ( CosmStrCopy( tmp_dirname, dirname, size ) != COSM_PASS )
    {
      return COSM_DIR_ERROR_CREATE;
    }

    /* Remove a trailing slash. */
    currpos = CosmMemOffset( tmp_dirname,
      ( i - 1LL ) );
    if ( *currpos == '/' )
    {
      *currpos = '\0';
    }

    /* translate to native path type */
    Cosm_FileNativePath( tmp_native_dirname, tmp_dirname );

    higher_levels = 1; /* We should make atleast one directory. */

    do
    {
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
      error = _mkdir( (char *) tmp_native_dirname );
#else /* OS */
#if ( defined( S_IRGRP ) )
      /* platform handling user groups */
      error = mkdir( (const char *) tmp_native_dirname, S_IRUSR | S_IWUSR |
        S_IXUSR | S_IRGRP | S_IXGRP );
#else
      /* platform not handling user groups -> use old style */
      error = mkdir( (const char *) tmp_native_dirname, S_IREAD |
        S_IWRITE | S_IEXEC );
#endif /* S_IRGRP */
#endif /* OS */

      if ( error != 0 )
      {
        /*
          Check if mkdir failed because of lacking earlier elements
          in the path.
        */
        if ( errno == ENOENT )
        {
          higher_levels++;
          /* Remove one level from path and try creating it.  */
          do
          {
            i--;
            currpos = CosmMemOffset( tmp_dirname, i );
          }
          while ( ( *currpos != '/' ) && ( currpos != nativepos ) &&
            ( ( i  > 0 ) ) );

          /*
            If we hit 'bottom' of the tree without managing to
            create a directory we have to give up.
          */
          if ( ( currpos == nativepos ) || ( ( i == 0 ) ) )
          {
            return COSM_DIR_ERROR_CREATE;
          }

          /* Snub the path. */
          *currpos = '\0';
          /* Convert to native. */
          if ( Cosm_FileNativePath( tmp_native_dirname, tmp_dirname )
            != COSM_PASS )
          {
            return COSM_DIR_ERROR_NAME;
          }
        }
        else if ( errno == EEXIST )
        {
          /* already exists, we're fine */
          return COSM_PASS;
        }
        else
        {
          /* mkdir failed for some other reason then missing parent. */
          return COSM_DIR_ERROR_CREATE;
        }
      }
      else
      {
        /*
          We managed to create a directory, so we can take another shot at
          the next higher level in the tree.
        */
        higher_levels--;
        if ( higher_levels > 0 )
        {
          do
          {
            currpos = CosmMemOffset( tmp_dirname, i );
            i++;
          } while ( ( *currpos != '\0' ) && ( ( i < size ) ) );

          i--;
          *currpos = '/';

          /*
            This means that we reached the end of the list.
            This should not happend.
          */
          if ( i == size )
          {
            return COSM_DIR_ERROR_CREATE;
          }

          if ( Cosm_FileNativePath( tmp_native_dirname, tmp_dirname )
            != COSM_PASS )
          {
            return COSM_DIR_ERROR_NAME;
          }
        } /* if (higher_levels > 0 ) */
      }
    /* While we need to create directories higher up in the directory tree. */
    } while ( higher_levels > 0 );

    /* End of mkdir routines. */

    if ( error != 0 )
    {
      /* mkdir failed */
      return COSM_DIR_ERROR_CREATE;
    }

    return COSM_PASS;
  }

  /* we are trying to test existence or read the directory */

  CosmStrCopy( dir->dirname, dirname, (u64) COSM_FILE_MAX_FILENAME );

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  /*
    add the "\*" to the end of the native if we can, check for MAX_PATH
  */
  i = CosmStrBytes( native_dirname );
  if ( ( i > (u64) COSM_FILE_MAX_FILENAME - 3 ) || ( i > (u64) MAX_PATH - 3 ) )
  {
    /* too long */
    return COSM_DIR_ERROR_NAME;
  }
  else
  {
    /* add the "\*" */
    currpos = CosmMemOffset( native_dirname, i );
    *currpos++ = '\\';
    *currpos++ = '*';
    *currpos = '\0';
  }
  /* open it */
  if ( ( dir->handle = FindFirstFile( (LPCTSTR) native_dirname,
    &dir->find_data ) ) == INVALID_HANDLE_VALUE )
  {
    switch( GetLastError() )
    {
      case ERROR_DIRECTORY:
      case ERROR_INVALID_NAME:
        last_error = COSM_DIR_ERROR_NOTFOUND;
        break;
      default:
        last_error = COSM_DIR_ERROR_DENIED;
    }
    return last_error;
  }

#else
  /* Really try to open the directory */
  dir->handle = opendir( (const char *) native_dirname );
  if ( dir->handle == NULL )
  {
    switch( errno )
    {
      case ENOENT:
        last_error = COSM_DIR_ERROR_NOTFOUND;
        break;
      case ENOTDIR:
      case EACCES:
      default:
        last_error = COSM_DIR_ERROR_DENIED;
    }
    return last_error;
  }
#endif
  if ( ( mode & COSM_DIR_MODE_EXIST ) == COSM_DIR_MODE_EXIST )
  {
    /* We were just testing for the directory existence -> close dir */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
    FindClose( dir->handle );
#else
    closedir( dir->handle );
#endif
    return COSM_PASS;
  }

  dir->offset = (u64) 0;
  dir->status = COSM_DIR_STATUS_OPEN;
  return COSM_PASS;
}

s32 CosmDirRead( cosm_FILENAME * buffer, u64 * names_read, u64 length,
  cosm_DIR * dir )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  u32 result;
#else
  struct dirent * readdir_result;
#endif
  u32 ok_to_exit;

  *names_read = (u64) 0;

  if ( dir == NULL )
  {
    return COSM_DIR_ERROR_NOTFOUND;
  }


  if ( dir->status != COSM_DIR_STATUS_OPEN )
  {
    return COSM_DIR_ERROR_CLOSED;
  }

  ok_to_exit = 0;

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  while ( ok_to_exit == 0 )
  {
    if ( dir->offset == 0 )
    {
      /* copy the filename given by readdir into the buffer */
      CosmStrCopy( CosmMemOffset( buffer, *names_read * sizeof( cosm_FILENAME ) ),
        (utf8 *) &dir->find_data.cFileName, COSM_FILE_MAX_FILENAME );
      (*names_read)++;
      dir->offset++;

      if ( ( length == *names_read ) )
      {
        /* Buffer is full */
        ok_to_exit = 1;
      }
    }
    else
    {
      result = FindNextFile( dir->handle, &dir->find_data );
      if ( result == 0 )
      {
        /* failed, probably out of files, either way we are done */
        return COSM_DIR_ERROR_EOF;
      }
      else
      {
        CosmStrCopy( CosmMemOffset( buffer,
          *names_read * sizeof( cosm_FILENAME ) ),
          (utf8 *) &dir->find_data.cFileName, COSM_FILE_MAX_FILENAME );
        (*names_read)++;
        dir->offset++;

        if ( ( length == *names_read ) )
        {
          /* done for now */
          ok_to_exit = 1;
        }
      }
    }
  }
#else /* OS */
  while ( ok_to_exit == 0 )
  {
    readdir_result = readdir( dir->handle );

    if ( readdir_result == NULL )
    {
      return COSM_DIR_ERROR_EOF;
    }
    else
    {
      /* copy the filename given by readdir into the buffer */
      CosmStrCopy( CosmMemOffset( buffer,
        *names_read * sizeof( cosm_FILENAME ) ),
        readdir_result->d_name, (u64) COSM_FILE_MAX_FILENAME );
      (*names_read)++;
      dir->offset++;

      if ( length == *names_read )
      {
        /* done for now */
        ok_to_exit = 1;
      }
    }
  }
#endif /* OS */

  return COSM_PASS;
}

s32 CosmDirClose( cosm_DIR * dir )
{
  if ( dir == NULL )
  {
    return COSM_DIR_ERROR_NOTFOUND;
  }

  if ( dir->status != COSM_DIR_STATUS_OPEN )
  {
    return COSM_DIR_ERROR_CLOSED;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( FindClose( dir->handle ) == 0 )
#else
  if ( closedir( dir->handle ) != 0 )
#endif
  {
    return COSM_DIR_ERROR_NOTFOUND;
  }

  dir->status = COSM_DIR_STATUS_CLOSED;

  return COSM_PASS;
}

s32 CosmDirDelete( const ascii * dirname )
{
  cosm_FILENAME native_dirname;

  /* Convert slashs to native format and put it into native_dirname */
  if ( Cosm_FileNativePath( native_dirname, dirname ) != COSM_PASS )
  {
    return COSM_DIR_ERROR_NAME;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( RemoveDirectory( (LPCTSTR) native_dirname ) == 0 )
  {
    switch( GetLastError() )
    {
      case ERROR_DIR_NOT_EMPTY:
      case WSAENOTEMPTY:
        return COSM_DIR_ERROR_NOTEMPTY;
      case ERROR_DIRECTORY:
      case ERROR_INVALID_NAME:
        return COSM_DIR_ERROR_NOTFOUND;
      default:
        return COSM_DIR_ERROR_DENIED;
    }
  }
#else /* OS */
  if ( rmdir( (const char *) native_dirname ) != 0 )
  {
    switch( errno )
    {
      case EACCES:
        return COSM_DIR_ERROR_DENIED;
      case ENOENT:
        return COSM_DIR_ERROR_NOTFOUND;
#if ( defined( ENOTEMPTY ) )
      case ENOTEMPTY:
        return COSM_DIR_ERROR_NOTEMPTY;
#endif
      default:
        return COSM_DIR_ERROR_DENIED;
    }
  }
#endif /* OS */

  return COSM_PASS;
}

s32 CosmDirGet( cosm_FILENAME * dirname )
{
  cosm_FILENAME native_dirname;

  if ( dirname == NULL )
  {
    return COSM_DIR_ERROR_PARAM;
  }

  if ( NULL == getcwd( (char *) native_dirname, COSM_FILE_MAX_FILENAME - 1 ) )
  {
    return COSM_FAIL;
  }

  /* !!! translate back */
  CosmMemCopy( dirname, native_dirname, (u64) COSM_FILE_MAX_FILENAME );

  return COSM_PASS;
}

s32 CosmDirSet( const ascii * dirname )
{
  cosm_FILENAME native_dirname;

  if ( dirname == NULL )
  {
    return COSM_DIR_ERROR_PARAM;
  }

  /* Convert slashs to native format and put it into native_dirname */
  if ( Cosm_FileNativePath( native_dirname, dirname ) != COSM_PASS )
  {
    return COSM_DIR_ERROR_NAME;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( _chdir( (const char *) native_dirname ) != 0 )
  {
    return COSM_DIR_ERROR_DENIED;
  }
#else /* OS */
  if ( chdir( (const char *) native_dirname ) != 0 )
  {
    switch( errno )
    {
      case ENOENT:
        return COSM_DIR_ERROR_NOTFOUND;
      default:
        return COSM_DIR_ERROR_DENIED;
    }
  }
#endif /* OS */

  return COSM_PASS;
}

/* byte order functions */

#if ( COSM_ENDIAN == COSM_ENDIAN_LITTLE )

void CosmU16Load( u16 * num, const void * bytes )
{
  const u8 * mem;

  mem = bytes;
  *num = (u16) ( ( (u16) mem[0] << 8 ) | (u16) mem[1] );
}

void CosmU16Save( void * bytes, const u16 * num )
{
  u8 * mem;
  u16 tmp;

  tmp = *num;
  mem = bytes;
  mem[0] = (u8) ( tmp >> 8 );
  mem[1] = (u8) tmp;
}

void CosmU32Load( u32 * num, const void * bytes )
{
  const u8 * mem;

  mem = bytes;
  *num = ( (u32) mem[0] << 24 ) | ( (u32) mem[1] << 16 )
    | ( (u32) mem[2] << 8 ) | (u32) mem[3];
}

void CosmU32Save( void * bytes, const u32 * num )
{
  u8 * mem;
  u32 tmp;

  tmp = *num;
  mem = bytes;
  mem[0] = (u8) ( tmp >> 24 );
  mem[1] = (u8) ( tmp >> 16 );
  mem[2] = (u8) ( tmp >> 8 );
  mem[3] = (u8) tmp;
}

#endif /* little endian u16/u32 save/loads */

#if ( ( COSM_ENDIAN == COSM_ENDIAN_LITTLE ) \
  || ( defined( CPU_32BIT ) ) )

void CosmU64Load( u64 * num, const void * bytes )
{
#if ( COSM_ENDIAN == COSM_ENDIAN_BIG )
  *num = *( (u64 *) bytes );
#else
  const u8 * mem;
  u32 hi, lo;

  mem = bytes;
  hi = ( (u32) mem[0] << 24 ) | ( (u32) mem[1] << 16 )
    | ( (u32) mem[2] << 8 ) | (u32) mem[3];
  lo = ( (u32) mem[4] << 24 ) | ( (u32) mem[5] << 16 )
    | ( (u32) mem[6] << 8 ) | (u32) mem[7];

  *num = ( (u64) hi << 32 ) | lo;
#endif
}

void CosmU64Save( void * bytes, const u64 * num )
{
#if ( COSM_ENDIAN == COSM_ENDIAN_BIG )
  *( (u64 *) bytes ) = *num;
#else
  u8 * mem;
  u32 hi, lo;

  hi = (u32) ( *num >> 32 );
  lo = (u32) *num;

  mem = bytes;
  mem[0] = (u8) ( hi >> 24 );
  mem[1] = (u8) ( hi >> 16 );
  mem[2] = (u8) ( hi >>  8 );
  mem[3] = (u8) hi;
  mem[4] = (u8) ( lo >> 24 );
  mem[5] = (u8) ( lo >> 16 );
  mem[6] = (u8) ( lo >>  8 );
  mem[7] = (u8) lo;
#endif
}

#endif /* little endian u64 save/loads */

void CosmU128Load( u128 * num, const void * bytes )
{
#if ( COSM_ENDIAN == COSM_ENDIAN_BIG )
  const u8 * mem;

  mem = bytes;
  num->hi = *( (u64 *) mem );
  num->lo = *( (u64 *) &mem[8] );
#else
  const u8 * mem;
  u32 hi_hi, hi_lo, lo_hi, lo_lo;

  mem = bytes;
  hi_hi = ( (u32) mem[0] << 24 ) | ( (u32) mem[1] << 16 )
    | ( (u32) mem[2] << 8 ) | (u32) mem[3];
  hi_lo = ( (u32) mem[4] << 24 ) | ( (u32) mem[5] << 16 )
    | ( (u32) mem[6] << 8 ) | (u32) mem[7];
  lo_hi = ( (u32) mem[8] << 24 ) | ( (u32) mem[9] << 16 )
    | ( (u32) mem[10] << 8 ) | (u32) mem[11];
  lo_lo = ( (u32) mem[12] << 24 ) | ( (u32) mem[13] << 16 )
    | ( (u32) mem[14] << 8 ) | (u32) mem[15];

  num->hi = ( (u64) hi_hi << 32 ) | hi_lo;
  num->lo = ( (u64) lo_hi << 32 ) | lo_lo;
#endif
}

void CosmU128Save( void * bytes, const u128 * num )
{
#if ( COSM_ENDIAN == COSM_ENDIAN_BIG )
  u8 * mem;

  mem = bytes;
  *( (u64 *) mem ) = num->hi;
  *( (u64 *) &mem[8] ) = num->lo;
#else
  u8 * mem;
  u32 hi, lo;

  mem = bytes;

  hi = (u32) ( num->hi >> 32 );
  lo = (u32) num->hi;
  mem[0]  = (u8) ( hi >> 24 );
  mem[1]  = (u8) ( hi >> 16 );
  mem[2]  = (u8) ( hi >>  8 );
  mem[3]  = (u8) hi;
  mem[4]  = (u8) ( lo >> 24 );
  mem[5]  = (u8) ( lo >> 16 );
  mem[6]  = (u8) ( lo >>  8 );
  mem[7]  = (u8) lo;

  hi = (u32) ( num->lo >> 32 );
  lo = (u32) num->lo;
  mem[8]  = (u8) ( hi >> 24 );
  mem[9]  = (u8) ( hi >> 16 );
  mem[10] = (u8) ( hi >>  8 );
  mem[11] = (u8) hi;
  mem[12] = (u8) ( lo >> 24 );
  mem[13] = (u8) ( lo >> 16 );
  mem[14] = (u8) ( lo >>  8 );
  mem[15] = (u8) lo;
#endif
}

/* low level functions */

s32 Cosm_FileNativePath( void * native, const utf8 * path )
{
  /* map to utf8char, then to whatever we need */
  ascii * ptr, * src, * dest;
  u32 length;

  length = (u64) COSM_FILE_MAX_FILENAME;

  if ( ( ptr = CosmStrChar( path, (ascii) '|', length ) ) != NULL )
  {
    if ( CosmStrBytes( path ) > length )
    {
      return COSM_FAIL;
    }
    src = (ascii *) path;
    dest = native;
    while ( *src != 0 )
    {
      if ( *src == '|' )
      {
        src++;
      }
      else
      {
        *(dest)++ = *(src)++;
      }
    }
    *dest = 0;
  }
  else
  {
    if ( CosmStrCopy( native, path, length + 1 ) != COSM_PASS )
    {
      return COSM_FAIL;
    }
  }

  /* perform post copy substitutions */
#if ( COSM_FILE_PATHMODE == COSM_FILE_PATH_DOS )
  ptr = native;
  while ( ( ptr = CosmStrChar( ptr, '/', length ) ) != NULL )
  {
    *(ptr) = '\\';
  }
#elif ( COSM_FILE_PATHMODE == COSM_FILE_PATH_MAC )
  ptr = native;
  while ( ( ptr = CosmStrChar( ptr, '/', length ) ) != NULL )
  {
    *(ptr) = ':';
  }
#endif

  return COSM_PASS;
}

s32 Cosm_FileWrite( cosm_FILE * file, u64 * bytes_written,
  const void * const buffer, u64 length )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  DWORD this_write, chunk;
  u64 remaining = length, total_write;
  u8 * ptr;

  *bytes_written = 0x0000000000000000LL;
  total_write = 0x0000000000000000LL;

  ptr = (u8 *) buffer;

  /* we can only write a DWORD at a time due to Win API */
  while ( remaining > 0 )
  {
    /* use 1GiB chunks */
    chunk = ( remaining > 0x40000000LL ) ? 0x40000000 : (u32) remaining;
    if ( WriteFile( file->handle, ptr, chunk, &this_write, NULL ) == 0 )
    {
      *bytes_written = total_write;
      return COSM_FILE_ERROR_DENIED;
    }
    ptr += chunk;
    total_write += (u64) this_write;
    remaining -= (u64) chunk;
  }

  *bytes_written = total_write;
#else
  int real_write;
  u64 tmp;
  s32 error;

  *bytes_written = (u64) 0;

#if ( defined( COSM_FILE64 ) )
  real_write = write( (int) file->handle, buffer, length );
#else
  tmp = 0x000000007FFFFFFFLL;
  if ( ( length > tmp ) )
  {
    return COSM_FILE_ERROR_SEEK;
  }
  real_write = write( (int) (u32) file->handle, buffer,
    (u32) length );
#endif
  if ( real_write == -1 )
  {
    if ( errno == EACCES )
    {
      error = COSM_FILE_ERROR_DENIED;
    }
    else
    {
      error = COSM_FILE_ERROR_NOTFOUND;
    }

    return error;
  }

  if ( ( file->mode & COSM_FILE_MODE_SYNC ) == COSM_FILE_MODE_SYNC )
  {
#if ( defined( COSM_FILE64 ) )
    fsync( (int) file->handle );
#else
    fsync( (int) (u32) file->handle );
#endif
  }

  *bytes_written = real_write;
#endif /* OS */

  return COSM_PASS;
}

s32 Cosm_FileSeek( cosm_FILE * file, u64 offset )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  LARGE_INTEGER position;

  position.QuadPart = offset;
  if ( SetFilePointerEx( file->handle, position, NULL, FILE_BEGIN ) == 0 )
  {
    return COSM_FILE_ERROR_SEEK;
  }
#else
  off_t real_offset;

  if ( ( file->mode & COSM_FILE_MODE_APPEND ) == COSM_FILE_MODE_APPEND )
  {
    /* Seek in a file open for append is Forbidden */
    return COSM_FILE_ERROR_DENIED;
  }

#if ( defined( COSM_FILE64 ) )
  real_offset = lseek( (int) file->handle, (off_t) offset, SEEK_SET );
#else
  if ( offset > 0x7FFFFFFFLL )
  {
    return COSM_FILE_ERROR_SEEK;
  }
  real_offset = lseek( (int) (u32) file->handle,
    (off_t) (u32) offset, SEEK_SET );
#endif

  if ( real_offset == -1 )
  {
    /* error during the lseek */
    return COSM_FILE_ERROR_SEEK;
  }

#endif /* OS */

  return COSM_PASS;
}

s32 Cosm_FileTell( u64 * offset, cosm_FILE * file )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  LARGE_INTEGER zero;
  LARGE_INTEGER position;

  zero.QuadPart = 0;
  if ( SetFilePointerEx( file->handle, zero, &position, FILE_CURRENT ) == 0 )
  {
    return COSM_FILE_ERROR_TELL;
  }

  *offset = position.QuadPart;
#else
  int real_offset;

#if ( defined( COSM_FILE64 ) )
  real_offset = lseek( (int) file->handle, 0 , SEEK_CUR );
#else
  real_offset = lseek( (int) (u32) file->handle, 0 , SEEK_CUR );
#endif
  if ( real_offset == -1 )
  {
    return COSM_FILE_ERROR_TELL;
  }

#if ( defined( COSM_FILE64 ) )
  *(offset) = real_offset;
#else
  *(offset) = (u64) real_offset;
#endif

#endif /* OS */

  return COSM_PASS;
}

s32 Cosm_FileLength( u64 * length, cosm_FILE * file )
{
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  LARGE_INTEGER size;

  if ( GetFileSizeEx( file->handle, &size ) == 0 )
  {
    return COSM_FILE_ERROR_LENGTH;
  }

  *length = size.QuadPart;
#else
  struct stat file_stats;

#if ( defined( COSM_FILE64 ) )
  if ( fstat( file->handle, &file_stats ) == -1 )
#else
  if ( fstat( (u32) file->handle, &file_stats ) == -1 )
#endif
  {
    return COSM_FILE_ERROR_LENGTH;
  }

  *(length) = (u64) file_stats.st_size;

#endif /* OS */

  return COSM_PASS;
}

s32 Cosm_FileTruncate( cosm_FILE * file, u64 length )
{
  s32 error;

  if ( ( error = Cosm_FileWipe( file, length ) ) != COSM_PASS )
  {
    return error;
  }

  if ( ( error = Cosm_FileSeek( file, length ) ) != COSM_PASS )
  {
    return error;
  }

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  if ( SetEndOfFile( file->handle ) == 0 )
  {
    return COSM_FILE_ERROR_EOF;
  }
#else
#if ( defined( COSM_FILE64 ) )
  if ( ftruncate( (int) file->handle, (size_t) length ) != 0 )
#else
  if ( ftruncate( (int) (u32) file->handle,
    (size_t) (u32) length ) != 0 )
#endif /* file64 */
  {
    /* error during the ftruncate */
    if ( errno == EBADF )
    {
      return COSM_FILE_ERROR_NOTFOUND;
    }
    else if ( errno == EINVAL )
    {
      return COSM_FILE_ERROR_WRITEMODE;
    }
  }
#endif /* OS */

  return COSM_PASS;
}

s32 Cosm_FileLock( cosm_FILE * file, u32 lock )
{
#if ( OS_TYPE == OS_LINUX )
  if ( lock == COSM_FILE_LOCK_READ )
  {
#if ( defined( COSM_FILE64 ) )
    if ( flock( (int) file->handle, LOCK_SH | LOCK_NB ) == -1 )
#else
    if ( flock( (int) (u32) file->handle, LOCK_SH | LOCK_NB ) == -1 )
#endif
    {
      return COSM_FILE_ERROR_READLOCK;
    }
  }
  else
  {
    if ( lock == COSM_FILE_LOCK_WRITE )
    {
#if ( defined( COSM_FILE64 ) )
      if ( flock( (int) file->handle, LOCK_EX | LOCK_NB ) == -1 )
#else
      if ( flock( (int) (u32) file->handle, LOCK_EX | LOCK_NB ) == -1 )
#endif
      {
        return COSM_FILE_ERROR_WRITELOCK;
      }
    }
  }
#elif ( OS_TYPE == OS_SOLARIS )
  struct flock lock_desc;

  if ( lock == COSM_FILE_LOCK_READ )
  {
    /* With thus values, we should lock all the file */
    lock_desc.l_type = F_RDLCK;
    lock_desc.l_whence = 0;
    lock_desc.l_start = 0;
    lock_desc.l_len = 0;

#if ( defined( COSM_FILE64 ) )
    if ( fcntl( (int) file->handle, F_SETLK, &lock_desc ) == -1 )
#else
    if ( fcntl( (int) (u32) file->handle, F_SETLK, &lock_desc ) == -1 )
#endif
    {
      return COSM_FILE_ERROR_READLOCK;
    }
  }
  else
  {
    if ( lock == COSM_FILE_LOCK_WRITE )
    {
      /* With thus values, we should lock all the file */
      lock_desc.l_type = F_WRLCK;
      lock_desc.l_whence = 0;
      lock_desc.l_start = 0;
      lock_desc.l_len = 0;

#if ( defined( COSM_FILE64 ) )
      if ( fcntl( (int) file->handle, F_SETLK, &lock_desc ) == -1 )
#else
      if ( fcntl( (int) (u32) file->handle, F_SETLK,
        &lock_desc ) == -1 )
#endif
      {
        return COSM_FILE_ERROR_WRITELOCK;
      }
    }
  }
#else /* !!! no OS locks, we need to do them ourselves */
  /* open a file with ~ at the end, place ProcessID and lock type in it */

#endif

  file->lockmode = lock;
  return COSM_PASS;
}

s32 Cosm_FileUnLock( cosm_FILE * file )
{
#if ( OS_TYPE == OS_LINUX )
#if ( defined( COSM_FILE64 ) )
  if ( flock( (int) file->handle, LOCK_UN ) == -1 )
#else
  if ( flock( (int) (u32) file->handle, LOCK_UN ) == -1 )
#endif
  {
    return COSM_FILE_ERROR_UNLOCK;
  }
#elif ( OS_TYPE == OS_SOLARIS )
  struct flock lock_desc;

  /* With thus values, we should lock all the file */
  lock_desc.l_type = F_UNLCK;
  lock_desc.l_whence = 0;
  lock_desc.l_start = 0;
  lock_desc.l_len = 0;
#if ( defined( COSM_FILE64 ) )
  if ( fcntl( (int) file->handle, F_SETLK, &lock_desc ) == -1 )
#else
  if ( fcntl( (int) (u32) file->handle, F_SETLK, &lock_desc ) == -1 )
#endif
  {
    return COSM_FILE_ERROR_READLOCK;
  }
#else /* !!! no OS locks, we need to do them ourselves */

#endif

  file->lockmode = COSM_FILE_LOCK_NONE;
  return COSM_PASS;
}

s32 Cosm_FileWipe( cosm_FILE * file, u64 length )
{
  u64 file_size;       /* Size of the file to wipe */
  void * zero_land;    /* Zero full memory to wiped the file with */
  u64 zero_land_size;  /* Size of zero_land space */
  u64 zero;            /* Use to wipe if can't allocate zero_land */
  u64 to_wipe;
  u64 really_write;
  s32 error;

  if ( ( error = Cosm_FileLength( &file_size, file ) ) != COSM_PASS )
  {
    return error;
  }

  if ( ( error = Cosm_FileSeek( file, length ) ) != COSM_PASS )
  {
    return error;
  }

  zero_land_size = 0x0000000000001000LL;
  if ( ( zero_land = CosmMemAlloc( zero_land_size ) ) == NULL )
  {
    zero_land_size = 0x0000000000000008LL;
    zero = 0;
    zero_land = &zero;
  }

/* !!! fix error handling */

  /* seek to length, while < file_size write data */
  to_wipe = ( file_size - length );
  do
  {
    /* !!! problem with lock status and write... */
    if ( ( zero_land_size > to_wipe ) )
    {
      Cosm_FileWrite( file, &really_write, zero_land, to_wipe );
    }
    else
    {
      Cosm_FileWrite( file, &really_write, zero_land, zero_land_size );
    }
    to_wipe = ( to_wipe - really_write );
  } while ( really_write != 0 );

  CosmMemFree( zero_land );
  return COSM_PASS;
}

/* testing */

s32 Cosm_TestOSFile( void )
{
  cosm_DIR * testdir;
  cosm_FILE * testfile;
  u8 * testfileimg;  /* Image of the file as it is wrote */
  u8   testpattern1[31] = "1234568790\nabcdefghi\nABCDEFGHI";
  u8   buffer[100];
  s32  i, j;
  u64  token_len;  /* len of the tokens we are reading/writting */
  u64  real_write;
  u64  real_read;
  u64  real_length;
  u64  real_offset;
  u8 * hello = (u8*) "Hello World !";
  cosm_FILENAME testfilenames [16];
  s32 error;

  const u8 bytes1[16] = /* the number 0x0102030405060708090A0B0C0D0E0F10 */
  {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
  };
  u8 bytes2[16];
  u16  b_u16;
  u32  b_u32;
  u64  b_u64, e_u64;
  u128 b_u128, e_u128;

/***********************************************************************/
/* File test 1                                                         */
/*                                                                     */
/* Test functions : CosmFileOpen, CosmFileRead, CosmFileWrite, CosmFileSeek,   */
/*   CosmFileTell, CosmFileEOF, CosmFileLength, CosmFileTruncate, CosmFileClose  */
/*                                                                     */
/* Methodology : ;)                                                    */
/* -------------                                                       */
/* 01. Create the file os_file.tst (read and write, exclusive lock)    */
/* 02. Write 20 patterns of 10 bytes (bytes = u8)                      */
/* 03. Check we are at end of file                                     */
/* 04. Check current size of the file (should be 200)                  */
/* 05. Move to the begining of the file                                */
/* 06. Read and compare 8 patterns of 25 bytes                         */
/* 07. Check we are at end of file                                     */
/* 08. Truncate file at 100 bytes                                      */
/* 09. Check current size of the file (should be 100)                  */
/* 10. Move to the begining of the file                                */
/* 11. Read and compare 1 pattern of 100 bytes                         */
/* 12. Check we are at end of file                                     */
/* 13. Write 26 bytes                                                  */
/* 14. Check current size of the file (should be 126)                  */
/* 15. Check we are at end of file                                     */
/* 16. Read 25 patterns of 6 bytes, with an offset of (-5), from EOF   */
/* 17. Checking position in file (offset should be 5)                  */
/* 18. Close file                                                      */
/* 19. Open file os_file.tst (write only, exclusive lock)              */
/* 20. Write the string 'Hello world !' to file                        */
/* 21. Close file                                                      */
/* 22. Open file os_file.tst (read only, shared read lock)             */
/* 23. Read and compare 10 patterns of 12 bytes                        */
/* 24. Read and compare 1 pattern of 6 bytes                           */
/* 25. Check we are at end of file                                     */
/* 26. Close file                                                      */
/* 27. Delete file                                                     */
/***********************************************************************/

  /* Allocate memory for all tests */

  testfile = (cosm_FILE *) CosmMemAlloc( sizeof( cosm_FILE ) );

  if ( testfile == NULL )
  {
    return 1; /* we already tested CosmMemAlloc! */
  }

  testfileimg = (u8 *) CosmMemAlloc( 200LL );
  if ( testfileimg == NULL )
  {
    return 1;
  }

  testdir = (cosm_DIR *) CosmMemAlloc( sizeof( cosm_DIR ) );
  if ( testdir == NULL )
  {
    return 1;
  }

/* Test 1. Phase 1. Open file 'os_file.tst' */

  error = CosmFileOpen( testfile, "os_file.tst",
    COSM_FILE_MODE_CREATE | COSM_FILE_MODE_READ | COSM_FILE_MODE_WRITE,
    COSM_FILE_LOCK_WRITE );
  if ( error != COSM_PASS )
  { /* Open was not succesfull */
    return -1;
  }

/* Test 1. Phase 2. Writing tokens */

  token_len = 10LL;

  for ( i = 0 ; i < 20 ; i++ )
  {
    if ( COSM_PASS != CosmFileWrite( testfile, &real_write,
      testpattern1 + i, token_len ) )
    {
      return -2;
    }
    CosmMemCopy ( &testfileimg[i * 10], &testpattern1[i], token_len );
  }

/* Test 1. Phase 3. Check we are at end of file */

  error = CosmFileEOF( testfile );
  if ( error != COSM_FILE_ERROR_EOF )
  { /* Even a real error, or the library think there is more to read */
    return -3;
  }

/* Test 1. Phase 4. Check current size of the file (should be 200) */

  error = CosmFileLength( &real_length, testfile );
  if ( error != COSM_PASS )
  { /* Length was not succesfull */
    return -4;
  }
  if ( real_length != 200LL )
  { /* Wrong length returned */
    return -5;
  }

/* Test 1. Phase 5. Move to the beginning of file */

  error = CosmFileSeek( testfile, 0 );
  if ( error != COSM_PASS )
  { /* Seek was not successfull */
    return -6;
  }

/* Test 1. Phase 6. Read and compare tokens */
  token_len = 25LL;

  for ( i = 0 ; i < 8 ; i++ )
  {
    CosmFileRead( buffer, &real_read, testfile, token_len );
    if ( real_read != token_len )
    { /* Read was not succesfull or read wrong size */
       return -7;
    }
    for ( j = 0 ; j < 25 ; j++ )
    {
      if ( buffer[j]!= testfileimg[j + (i * 25)] )
      {
        CosmFileClose( testfile );
        return -8;
      }
    }
  }

/* Test 1. Phase 7. Check we are at end of file */

  error = CosmFileEOF( testfile );
  if ( error != COSM_FILE_ERROR_EOF )
  { /* Even a real error, or the library think there is more to read */
    return -9;
  }

/* Test 1. Phase 8. Truncate file at 100 bytes */

  error = CosmFileTruncate( testfile, 100LL );
  if ( error != COSM_PASS )
  { /* Truncate was not succesfull */
    return -10;
  }

/* Test 1. Phase 9. Check current size of the file (should be 100) */

  error = CosmFileLength( &real_length, testfile );
  if ( error != COSM_PASS )
  { /* Length was not succesfull */
    return -11;
  }

  if ( real_length != 100LL )
  { /* Wrong length returned */
    return -12;
  }

/* Test 1. Phase 10. Move to the beginning of file */

  error = CosmFileSeek( testfile, 0 );
  if ( error != COSM_PASS )
  { /* Seek was not successfull */
    return -13;
  }

/* Test 1. Phase 11. Read and compare tokens */

  token_len = 100LL;

  CosmFileRead( buffer, &real_read, testfile, token_len );
  if ( real_read != token_len )
  { /* Read was not succesfull or read wrong size */
     return -14;
  }
  for ( j = 0 ; j < 100 ; j++ )
  {
    if ( buffer[j] != testfileimg[j] )
    {
      CosmFileClose( testfile );
      return -15;
    }
  }

/* Test 1. Phase 12. Check we are at end of file */

  error = CosmFileEOF( testfile );
  if ( error != COSM_FILE_ERROR_EOF )
  { /* Even a real error, or the library think there is more to read */
    return -16;
  }

/* Test 1. Phase 13. Writing 26 bytes */

  token_len = 26LL;

  if ( COSM_PASS != CosmFileWrite( testfile, &real_write,
    &testfileimg[100], token_len ) )
  {
    return -17;
  }

/* Test 1. Phase 14. Check current size of the file (should be 126) */

  error = CosmFileLength( &real_length, testfile );
  if ( error != COSM_PASS )
  { /* Length was not succesfull */
    return -18;
  }
  if ( real_length != 126LL )
  { /* Wrong length returned */
    return -19;
  }

/* Test 1. Phase 15. Check we are at end of file */

  error = CosmFileEOF( testfile );
  if ( error != COSM_FILE_ERROR_EOF )
  { /* Even a real error, or the library think there is more to read */
    return -20;
  }

/* Test 1. Phase 16. Read and compare tokens from end of file */

  token_len = (u64) 5;

  for ( i = 24 ; i > -1 ; i-- )
  {
    error = CosmFileSeek( testfile, (u64) i * 5 );
    if ( error != COSM_PASS )
    { /* Seek was not successfull */
      return -21;
    }

    CosmFileRead( buffer, &real_read, testfile, token_len );
    if ( real_read != token_len )
    { /* Read was not succesfull or read wrong size */
       return -22;
    }
    for ( j = 0 ; j < 5 ; j++ )
    {
      if ( buffer[j]!= testfileimg[j + (i * 5)] )
      {
        CosmFileClose( testfile );
        return -23;
      }
    }
  }

/* Test 1. Phase 17. Checking position (offset should be 5) */

  error = CosmFileTell( &real_offset , testfile );
  if ( error != COSM_PASS )
  { /* Tell was not succesfull */
    return -24;
  }

  if ( real_offset != 5LL )
  { /* Returned position is wrong */
    return -25;
  }

/* Test 1. Phase 18. Close file */

  if ( CosmFileClose( testfile ) != COSM_PASS )
  {
    return -26;
  }

/* Test 1. Phase 19. Open file 'os_file.tst' */

  error = CosmFileOpen( testfile, "os_file.tst", COSM_FILE_MODE_WRITE,
    COSM_FILE_LOCK_WRITE );
  if ( error != COSM_PASS )
  { /* Open was not succesfull */
    return -27;
  }

/* Test 1. Phase 20. Write the Hello phrase */

  token_len = 13LL;

  if ( COSM_PASS != CosmFileWrite( testfile, &real_write, hello,
    token_len ) )
  {
    return -28;
  }
  CosmMemCopy( testfileimg, hello, token_len );

/* Test 1. Phase 21. Close file */

  if ( CosmFileClose( testfile ) != COSM_PASS )
  {
    return -29;
  }

/* Test 1. Phase 22. Open file 'os_file.tst' */

/*
Hello World !568790
34568790
a4568790
ab568790
abc68790
abcd8790
abcde790
abcdef90
abcdefg0
abcdefgh
abcdefghiabcdefghi
bcdefg
*/
  error = CosmFileOpen( testfile, "os_file.tst", COSM_FILE_MODE_READ,
    COSM_FILE_LOCK_READ );
  if ( error != COSM_PASS )
  { /* Open was not succesfull */
    return -30;
  }

/* Test 1. Phase 23. Read and compare tokens */

  token_len = 12LL;

  for ( i = 0 ; i < 10 ; i++ )
  {
    CosmFileRead( buffer, &real_read, testfile, token_len );
    if ( real_read != token_len )
    {
      /* Read was not succesfull or read wrong size */
      CosmFileClose( testfile );
      return -31;
    }
    for ( j = 0 ; j < 12 ; j++ )
    {
      if ( buffer[j]!= testfileimg[j + (i * 12)] )
      {
        CosmFileClose( testfile );
        return -32;
      }
    }
  }

/* Test 1. Phase 24. Read and compare 6 bytes */

  token_len = (u64) 6;

  CosmFileRead( buffer, &real_read, testfile, token_len );
  if ( real_read != token_len )
  {
    /* Read was not succesfull or read wrong size */
    return -33;
  }
  for ( j = 0 ; j < 6 ; j++ )
  {
    if ( buffer[j] != testfileimg[j + 120] )
    {
      CosmFileClose( testfile );
      return -34;
    }
  }

/* Test 1. Phase 25. Check we are at end of file */

  error = CosmFileEOF( testfile );
  if ( error != COSM_FILE_ERROR_EOF )
  { /* Even a real error, or the library think there is more to read */
    return -35;
  }

/* Test 1. Phase 26. Close file */

  if ( CosmFileClose( testfile ) != COSM_PASS )
  {
    return -36;
  }

/* Test 1. Phase 27. Delete file 'os_file.tst' */

  if ( CosmFileDelete( "os_file.tst" ) != COSM_PASS )
  { /* Delete was not succesfull */
    return -37;
  }

/***********************************************************************/
/* File test 2                                                         */
/*                                                                     */
/* Test functions : CosmDirOpen, CosmDirRead, CosmDirClose                   */
/*                                                                     */
/* Methodology :                                                       */
/* -------------                                                       */
/* 01. Open current directory                                          */
/* 02. Read current directory content                                  */
/* 03. Close the directory                                             */
/* 04. Create a directory 'testdir.tmp'                                */
/* 05. Create a directory 'testdir.tmp/level2'                         */
/* 06. Open directory 'testdir.tmp'                                    */
/* 07. Create file 'testdir.tmp/tiny.tmp' (write only, exclusive lock) */
/* 08. Write the 'Hello world !' string in it                          */
/* 09. Close the file                                                  */
/* 10. Read content of the directory and check                         */
/* 11. Close directory 'testdir.tmp'                                   */
/* 12. Delete directory 'testdir.tmp/level2'                           */
/* 13. Try to delete directory 'testdir.tmp', should fail (not empty)  */
/* 14. Delete file 'testdir.tmp/tiny.tmp'                              */
/* 15. Delete directory 'testdir.tmp'                                  */
/***********************************************************************/

/* Test 2. Phase 1. Open current directory */

  if ( CosmDirOpen( testdir, ".", 0 ) != COSM_PASS )
  { /* Open a directory was not succesfull */
    return -38;
  }

/* Test 2. Phase 2. Read current directory content */

  do
  {
    if ( ( CosmDirRead( testfilenames, &real_read, (u64) 16, testdir )
      != COSM_DIR_ERROR_EOF ) && ( ( real_read == 0 ) ) )
    {
      /* We didn't reach EOF but get an error instead */
      return -40;
    }
  } while ( real_read != 0 );

/* Test 2. Phase 3. Close directory */

  if ( CosmDirClose( testdir ) != COSM_PASS )
  { /* Close a directory was not succesfull */
    return -41;
  }

/* Test 2. Phase 4. Create directory 'testdir.tmp' */

  if ( CosmDirOpen( testdir, "testdir.tmp", COSM_DIR_MODE_CREATE )
    != COSM_PASS )
  { /* Create a directory was not succesfull */
    return -42;
  }

/* Test 2. Phase 5. Create directory 'testdir.tmp/level2/level3/level4/' */

  if ( CosmDirOpen( testdir, "testdir.tmp/level2/level3/level4/",
    COSM_DIR_MODE_CREATE ) != COSM_PASS )
  { /* Create a directory was not succesfull */
    return -43;
  }

/* Test 2. Phase 6. Open directory 'testdir.tmp' */

  if ( CosmDirOpen( testdir, "testdir.tmp", 0 )
    != COSM_PASS )
  { /* Open a directory was not succesfull */
    return -44;
  }

/* Test 2. Phase 7. Open file 'testdir.tmp/tiny.tmp' */

  error = CosmFileOpen( testfile, "testdir.tmp/tiny.tmp",
    COSM_FILE_MODE_CREATE | COSM_FILE_MODE_WRITE, COSM_FILE_LOCK_WRITE );
  if ( error != COSM_PASS )
  { /* Open was not succesfull */
    return -45;
  }

/* Test 2. Phase 8. Write the Hello phrase */

  token_len = 13LL;

  if ( COSM_PASS != CosmFileWrite( testfile, &real_write, hello,
    token_len ) )
  {
    /* Write was not succesful */
    return -46;
  }

/* Test 2. Phase 9. Close file */

  if ( CosmFileClose( testfile ) != COSM_PASS )
  {
    return -47;
  }

/* Test 2. Phase 10. Read content of the directory and check */

/* Test 2. Phase 11. Close directory */
  if ( CosmDirClose( testdir ) != COSM_PASS )
  { /* Close a directory was not succesfull */
    return -48;
  }

/* Test 2. Phase 12. Delete directory 'testdir.tmp/level2/level3/level4' */

  if ( CosmDirDelete( "testdir.tmp/level2/level3/level4/" )
    != COSM_PASS )
  { /* Delete a directory was not succesfull */
    return -49;
  }

/* Test 2. Phase 13. Try to delete directory 'testdir.tmp', should fail */

  if ( CosmDirDelete( "testdir.tmp" ) == COSM_PASS )
  { /* Delete a directory was succesfull -> Wrong thing */
    return -50;
  }

/* Test 2. Phase 14. Delete file 'testdir.tmp/tiny.tmp' */

  if ( CosmFileDelete( "testdir.tmp/tiny.tmp" ) != COSM_PASS )
  { /* Delete was not succesfull */
    return -51;
  }

/* Test 2. Phase 15. Delete 'testdir.tmp tree' */

  if ( ( CosmDirDelete( "testdir.tmp/level2/level3" ) != COSM_PASS ) ||
    ( CosmDirDelete( "testdir.tmp/level2" ) != COSM_PASS ) ||
    ( CosmDirDelete( "testdir.tmp" ) != COSM_PASS ) )
  { /* Delete a directory was not succesfull */
    return -52;
  }

  CosmMemFree( testdir );
  CosmMemFree( testfile );
  CosmMemFree( testfileimg );

/*
  Test functions : Cosm*Load, Cosm*Save
*/

  CosmU16Load( &b_u16, bytes1 );
  if ( b_u16 != 0x0102 )
  {
    return -53;
  }

  CosmU16Save( bytes2, &b_u16 );
  for ( i = 0 ; i < 2 ; i++ )
  {
    if ( bytes1[i] != bytes2[i] )
    {
      return -54;
    }
  }

  CosmU32Load( &b_u32, bytes1 );
  if ( b_u32 != 0x01020304 )
  {
    return -55;
  }

  CosmU32Save( bytes2, &b_u32 );
  for ( i = 0 ; i < 4 ; i++ )
  {
    if ( bytes1[i] != bytes2[i] )
    {
      return -56;
    }
  }

  CosmU64Load( &b_u64, bytes1 );
  e_u64 = 0x0102030405060708LL;
  if ( b_u64 != e_u64 )
  {
    return -57;
  }

  CosmU64Save( bytes2, &b_u64 );
  for ( i = 0 ; i < 8 ; i++ )
  {
    if ( bytes1[i] != bytes2[i] )
    {
      return -58;
    }
  }

  CosmU128Load( &b_u128, bytes1 );
  _COSM_SET128( e_u128, 0102030405060708, 090A0B0C0D0E0F10 );
  if ( !CosmU128Eq( b_u128, e_u128 ) )
  {
    return -59;
  }

  CosmU128Save( bytes2, &b_u128 );
  for ( i = 0 ; i < 16 ; i++ )
  {
    if ( bytes1[i] != bytes2[i] )
    {
      return -60;
    }
  }

  return COSM_PASS;
}
