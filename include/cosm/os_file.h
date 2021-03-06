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

/**
\file os_file.h
\brief Cosm file and directory operations.
*/

/* CPU/OS Layer - CPU and OS specific code is allowed */

#ifndef COSM_OS_FILE_H
#define COSM_OS_FILE_H

#include "cosm/cputypes.h"
#include "cosm/os_task.h"

#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
#else
#if ( OS_TYPE == OS_FREEBSD )
#include <sys/types.h>
#endif
#include <dirent.h>
#endif

/**
\defgroup FILE File Functions
Functions dealing with files.
\addtogroup FILE
@{
*/

/** File status closed. */
enum COSM_FILE_STATUS
{
  COSM_FILE_STATUS_CLOSED = 0,    /**< Closed */
  COSM_FILE_STATUS_OPEN   = 98712 /**< Open */
};

/** Modes for opening files. */
enum COSM_FILE_MODE
{
  COSM_FILE_MODE_NONE     = 0x00, /**< Invalid/missing file mode */
  COSM_FILE_MODE_EXIST    = 0x01, /**< Test for file existence only */
  COSM_FILE_MODE_READ     = 0x02, /**< Open for reading */
  COSM_FILE_MODE_WRITE    = 0x04, /**< Open for writing */
  COSM_FILE_MODE_APPEND   = 0x08, /**< Open at end of file, write only */
  COSM_FILE_MODE_CREATE   = 0x10, /**< Create the file if it doesn't exist */
  COSM_FILE_MODE_TRUNCATE = 0x20, /**< Truncate an existing file on open */
  COSM_FILE_MODE_SYNC     = 0x40, /**< Write to disk before return */
  COSM_FILE_MODE_NOBUFFER = 0x80  /**< No read/write buffering.
    This can have negative performance consequences for small read/writes.
    May requires sector and memory aligned read/writes on some platforms. */
};

/** Lock types for files. */
enum COSM_FILE_LOCK
{
  COSM_FILE_LOCK_NONE  = 0x00, /**< No file locking needed.
    Other locks will be observed. */
  COSM_FILE_LOCK_READ  = 0x01, /**< Shared read lock.
    Other processes can read the file, or add additional
    read locks, but none will be allowed to open the file for writing. */
  COSM_FILE_LOCK_WRITE = 0x02  /**< Exclusive write lock. No other readers or
    writers will be allowed access to the file while the file is open. */
};

#define COSM_FILE_ERROR_DENIED    -1   /* Access Denied */
#define COSM_FILE_ERROR_NOTFOUND  -2   /* File/Handle Not Found */
#define COSM_FILE_ERROR_READMODE  -3   /* Read Access denied */
#define COSM_FILE_ERROR_WRITEMODE -4   /* Write access denied */
#define COSM_FILE_ERROR_READLOCK  -5   /* Unable to read lock */
#define COSM_FILE_ERROR_WRITELOCK -6   /* Unable to write lock */
#define COSM_FILE_ERROR_EOF       -7   /* End Of File */
#define COSM_FILE_ERROR_APPEND    -8   /* Unable to append */
#define COSM_FILE_ERROR_CREATE    -9   /* Unable to create new file */
#define COSM_FILE_ERROR_CLOSED    -10  /* File is closed */
#define COSM_FILE_ERROR_SEEK      -11  /* Seek Error */
#define COSM_FILE_ERROR_NAME      -12  /* Invalid Filename */
#define COSM_FILE_ERROR_MODE      -13  /* Open flags conflict */
#define COSM_FILE_ERROR_UNLOCK    -14  /* Unable to unlock */
#define COSM_FILE_ERROR_TELL      -15  /* Tell Error */
#define COSM_FILE_ERROR_LENGTH    -16  /* Unable to discover file size */
#define COSM_FILE_ERROR_NOSPACE   -17  /* Device is full */
#define COSM_FILE_ERROR_PARAM     -18  /* Parameter error */

#define COSM_FILE_MAX_FILENAME    256

/**
\typedef cosm_FILENAME
\brief Internal filename type.
*/
typedef ascii cosm_FILENAME[COSM_FILE_MAX_FILENAME];

/*
  Paths are always in the cosm "native|/path/path2/filename" format
  This is important for the system because any platform that can will do a
  chroot() before running, non-chroot systems will tack addition path on the
  front at runtime. The 'other' slash is translated on the fly if needed, but
  internally, it's all stored in the same format. The native portion of the
  path (everything before and including the '|') is platform specific for
  devices/nodes on weird OSes. e.g. "e:|/temp/a.txt"(win32), or
 "//4|/tmp/a.txt" (qnx). If paths are ever sent over the net, this native
  portion will be stripped.
*/

/** Structure used by CosmFile functions. */
typedef struct cosm_FILE
{
  cosm_FILENAME filename;  /**< File name. */
  /**
    @var cosm_FILE::handle
    Internal file handle in an OS specific format.
  */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE handle;
#else
  u64 handle;
#endif
  u32 status;    /**< Status of file. */
  u32 mode;      /**< Mode opened in. */
  u32 lockmode;  /**< Mode locked with. */
} cosm_FILE;

/** Memory mapped file structure. */
typedef struct cosm_FILE_MEMORY_MAP
{
  void * memory;        /**< Memory pointer */
  u64 length;           /**< Status of file. */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE file_mapping;  /**< Status of file. */
#endif
} cosm_FILE_MEMORY_MAP;

#define COSM_FILE_TYPE_UNKNOWN   0  /* Unknown, filessytem error, etc */
#define COSM_FILE_TYPE_FILE      1  /* Normal file */
#define COSM_FILE_TYPE_DIR       2  /* Directory */
#define COSM_FILE_TYPE_DEVICE    3  /* OS device file/handle */
#define COSM_FILE_TYPE_SPECIAL   4  /* Something valid, but not listed */

#define COSM_FILE_RIGHTS_NONE    0x00 /* No access */
#define COSM_FILE_RIGHTS_READ    0x01 /* Readable */
#define COSM_FILE_RIGHTS_WRITE   0x02 /* Writeable (implies append) */
#define COSM_FILE_RIGHTS_APPEND  0x04 /* Appendable */
#define COSM_FILE_RIGHTS_EXEC    0x08 /* Executeable program */

/** File information structure. */
typedef struct cosm_FILE_INFO
{
  u64 length;
  u32 type;
  u32 rights;
  cosmtime create;
  cosmtime modify;
  cosmtime access;
} cosm_FILE_INFO;

/*
  File Functions
*/

/**
Attempt to open the \a filename with the \a mode(s) and \a lock(s) specified.
\param file File handle to create.
\param filename Name of file to open.
\param mode Combination of #COSM_FILE_MODE modes.
\param lock Combination of #COSM_FILE_LOCK types.
\return COSM_PASS on success, or an error code on failure.
\code
  cosm_FILE * file;
  s32 result;

  file = CosmMemAlloc( sizeof( cosm_FILE ) );

  result = CosmFileOpen( file, "foo.txt",
    COSM_FILE_MODE_READ | COSM_FILE_MODE_WRITE,
    COSM_FILE_LOCK_NONE );

  if ( result == COSM_PASS )
    CosmPrint( "File open for reading.\n" );
  else
    CosmPrint( "Unable to open /etc/fstab for reading.\n" );
\endcode
*/
s32 CosmFileOpen( cosm_FILE * file, const ascii * filename,
  u32 mode, u32 lock );

s32 CosmFileRead( void * buffer, u64 * bytes_read, cosm_FILE * file,
  u64 length );
  /*
    Read length bytes from the file into the buffer. bytes_read is set to the
    number of bytes actually read, which may be non-zero even on an error.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileWrite( cosm_FILE * file, u64 * bytes_written,
  const void * const buffer, u64 length );
  /*
    Write length bytes to the file from the buffer. bytes_written is set to
    the number of bytes actually written, which may be non-zero even on
    an error.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileSeek( cosm_FILE * file, u64 offset );
  /*
    Move the current file offset (in bytes).
    If the file is open in mode COSM_FILE_MODE_APPEND, you can't seek.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileTell( u64 * offset, cosm_FILE * file );
  /*
    Get the current file offset.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileEOF( cosm_FILE * file );
  /*
    Test EOF state.
    Returns: COSM_PASS if there is more to read, COSM_FILE_ERROR_EOF if EOF,
      or an error code on failure.
  */

s32 CosmFileLength( u64 * length, cosm_FILE * file );
  /*
    Set length to the length of the open file.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileTruncate( cosm_FILE * file, u64 length );
  /*
    Truncate the file at length bytes.
    Current offset will be moved to end of file.
    Returns: COSM_PASS on success, or an error code on failure.
  */

u64 CosmFileMapPageSize( void );
  /*
    Get the system memory page granularity needed for CosmFileMap.
    Returns: The system page granularity, or zero on error.
  */

void * CosmFileMap( cosm_FILE_MEMORY_MAP * map, cosm_FILE * file,
  u64 length, u64 offset );
  /*
    Memory map a region of the already open file length bytes long beginning
    at offset into memory. Both length and offset must be multiples of
    CosmFileMapPageSize().
    The read/write permission of the mapping will be the same as the file,
    so the file must be opened in mode with COSM_FILE_MODE_READ alone or with
    COSM_FILE_MODE_WRITE. Writing to a read-only mapping result in segfaults.
    Executable code is not allowed to be mapped - for that see the
    CosmDynamicLib functions.
    You can create multiple mappings to different regions of the same file,
    but all must be unmapped before the file is closed.
    Note that the space availbale for memory mapped files varies not only by
    operating system, but by the system configuration. If your application
    uses memory mapped files, you must tell users how to enable and expand
    the space available for memory mapped files.
    Returns: Address of mapped file on success, NULL on failure.
  */

s32 CosmFileUnmap( cosm_FILE_MEMORY_MAP * map );
  /*
    Unmap the memory mapped region.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileClose( cosm_FILE * file );
  /*
    Close the file.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileDelete( const ascii * filename );
  /*
    Attempt to delete the file.
    Deletion should involve a full wipe of the file data.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmFileInfo( cosm_FILE_INFO * info, const ascii * filename );
  /*
    Get Information about a file.
    Returns: COSM_PASS on success, or an error code on failure.
  */

/**
@}
*/

/**
\defgroup DIRECTORY Directory Functions
Functions dealing with directories.
\addtogroup DIRECTORY
@{
*/

/*
  Directory Functions
*/

#define COSM_DIR_STATUS_CLOSED   0
#define COSM_DIR_STATUS_OPEN     1

#define COSM_DIR_MODE_NONE       0x00
#define COSM_DIR_MODE_EXIST      0x01 /* Test for dir existance only */
#define COSM_DIR_MODE_CREATE     0x02 /* Create the dir if it doesnt exist */

#define COSM_DIR_ERROR_DENIED    -1  /* Access Denied */
#define COSM_DIR_ERROR_NOTFOUND  -2  /* Directory not found */
#define COSM_DIR_ERROR_READMODE  -3  /* Read access denied */
#define COSM_DIR_ERROR_EOF       -4  /* End of Directory */
#define COSM_DIR_ERROR_CREATE    -5  /* Unable to create new directory */
#define COSM_DIR_ERROR_NOTEMPTY  -6  /* Directory is not empty (no delete) */
#define COSM_DIR_ERROR_MODE      -7  /* Open flags conflict */
#define COSM_DIR_ERROR_CLOSED    -8  /* Directory is closed */
#define COSM_DIR_ERROR_NAME      -9  /* Invalid Dirname */
#define COSM_DIR_ERROR_PARAM     -10 /* Parameter error */

typedef struct cosm_DIR
{
  cosm_FILENAME dirname;
  u64 offset;
  u32 status;
/* dirctories are handled far differently by every OS */
#if ( ( OS_TYPE == OS_WIN32 ) || ( OS_TYPE == OS_WIN64 ) )
  HANDLE handle;
  WIN32_FIND_DATA find_data;
#else
  DIR * handle;
#endif
} cosm_DIR;

/**
Attempt to open the dirname with mode.
\return COSM_PASS on success, or an error code on failure.
*/
s32 CosmDirOpen( cosm_DIR * dir, const ascii * dirname, u32 mode );

s32 CosmDirRead( cosm_FILENAME * buffer, u64 * names_read, u64 length,
  cosm_DIR * dir );
  /*
    Read length filenames from the dir into the buffer. Set num_read
    to the number of filenames read, zero on error.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmDirClose( cosm_DIR * dir );
  /*
    Close the directory.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmDirDelete( const ascii * dirname );
  /*
    Attempt to delete the dir. Will only work if the directory is empty.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmDirGet( cosm_FILENAME * dirname );
  /*
    Set dirname to the current working directory.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 CosmDirSet( const ascii * dirname );
  /*
    Set the current working directory to dirname.
    Returns: COSM_PASS on success, or an error code on failure.
  */

/**
@}
*/

/**
\defgroup ENDIAN Endian Safe Functions
\addtogroup ENDIAN
Utility Functions to deal with endian issues and keep date portable.
ALL data must be run through these when moving from the machine to the
network or a file. Functions handle case where the pointers are the same
or overlapping memory location.
@{
*/

#if ( COSM_ENDIAN == COSM_ENDIAN_BIG )
#define CosmU16Load( num, bytes ) ( *( (u16 *) num ) = *( (u16 *) bytes ) )
#define CosmU16Save( bytes, num ) ( *( (u16 *) bytes ) = *( (u16 *) num ) )
#define CosmU32Load( num, bytes ) ( *( (u32 *) num ) = *( (u32 *) bytes ) )
#define CosmU32Save( bytes, num ) ( *( (u32 *) bytes ) = *( (u32 *) num ) )
#else
void CosmU16Load( u16 * num, const void * bytes );
void CosmU16Save( void * bytes, const u16 * num );
void CosmU32Load( u32 * num, const void * bytes );
void CosmU32Save( void * bytes, const u32 * num );
#endif

#if ( ( COSM_ENDIAN == COSM_ENDIAN_BIG ) \
  && ( defined( CPU_64BIT ) ) )
#define CosmU64Load( num, bytes ) ( *( (u64 *) num ) = *( (u64 *) bytes ) )
#define CosmU64Save( bytes, num ) ( *( (u64 *) bytes ) = *( (u64 *) num ) )
#else
void CosmU64Load( u64 * num, const void * bytes );
void CosmU64Save( void * bytes, const u64 * num );
#endif

void CosmU128Load( u128 * num, const void * bytes );
void CosmU128Save( void * bytes, const u128 * num );

#define CosmS16Load( num, bytes ) CosmU16Load( (u16 *) num, bytes )
#define CosmS16Save( bytes, num ) CosmU16Save( bytes, (u16 *) num )
#define CosmS32Load( num, bytes ) CosmU32Load( (u32 *) num, bytes )
#define CosmS32Save( bytes, num ) CosmU32Save( bytes, (u32 *) num )
#define CosmS64Load( num, bytes ) CosmU64Load( (u64 *) num, bytes )
#define CosmS64Save( bytes, num ) CosmU64Save( bytes, (u64 *) num )
#define CosmS128Load( num, bytes ) CosmU128Load( (u128 *) num, bytes )
#define CosmS128Save( bytes, num ) CosmU128Save( bytes, (u128 *) num )

/**
@}
*/

/* Low level Functions */

s32 Cosm_FileNativePath( void * native, const utf8 * path );
  /*
    Translate path into a native system formatted path.
    Returns: COSM_PASS on success, or COSM_FAIL on failure.
  */

s32 Cosm_FileWrite( cosm_FILE * file, u64 * bytes_written,
  const void * const buffer, u64 length );
  /*
    Write length bytes to the file from the buffer. bytes_written is set
    to the number of bytes actually written. Mutex already set.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileSeek( cosm_FILE * file, u64 offset );
  /*
    Move the current file offset (in bytes). Mutex already set.
    If the file is open in mode COSM_FILE_MODE_APPEND, you can't seek.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileTell( u64 * offset, cosm_FILE * file );
  /*
    Get the current file offset. Mutex already set.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileLength( u64 * length, cosm_FILE * file );
  /*
    Set length to the length of the open file. Mutex already set.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileTruncate( cosm_FILE * file, u64 length );
  /*
    Truncate the file at length bytes. Mutex already set.
    Current offset will be moved to end of file.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileLock( cosm_FILE * file, u32 lock );
  /*
    Lock an open file. Mutex already set.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileUnLock( cosm_FILE * file );
  /*
    Unlock an open file. Mutex already set.
    Returns: COSM_PASS on success, or an error code on failure.
  */

s32 Cosm_FileWipe( cosm_FILE * file, u64 length );
  /*
    Securely wipe all file data past length bytes into the file which
    must be open and writeable.
    Current offset will be moved to new end of file.
    Returns: COSM_PASS on success, or an error code on failure.
  */

/* testing */

/**
Test functions in os_file.
\return COSM_PASS, or a negative number corresponding to the test that failed.
*/
s32 Cosm_TestOSFile( void );

#endif
