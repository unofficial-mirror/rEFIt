/**
 * \file fsw_core.h
 * Core file system wrapper abstraction layer header.
 */

/*-
 * Copyright (c) 2006 Christoph Pfisterer
 * Portions Copyright (c) The Regents of the University of California.
 * Portions Copyright (c) UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSW_CORE_H_
#define _FSW_CORE_H_

#include "fsw_base.h"


/** Maximum size for a path, specifically symlink target paths. */
#define FSW_PATH_MAX (4096)

/** Helper macro for token concatenation. */
#define FSW_CONCAT3(a,b,c) a##b##c
/** Expands to the name of a fstype dispatch table (fsw_fstype_table) for a named file system type. */
#define FSW_FSTYPE_TABLE_NAME(t) FSW_CONCAT3(fsw_,t,_table)

//
// The following evil hack avoids a lot of casts between generic and fstype-specific
// structures.
//

#ifndef VOLSTRUCTNAME
#define VOLSTRUCTNAME fsw_volume
#else
struct VOLSTRUCTNAME;
#endif
#ifndef DNODESTRUCTNAME
#define DNODESTRUCTNAME fsw_dnode
#else
struct DNODESTRUCTNAME;
#endif


/**
 * Status code type, returned from all functions that can fail.
 */
typedef int fsw_status_t;

/**
 * Possible status codes.
 */
enum {
    FSW_SUCCESS,
    FSW_OUT_OF_MEMORY,
    FSW_IO_ERROR,
    FSW_UNSUPPORTED,
    FSW_NOT_FOUND,
    FSW_VOLUME_CORRUPTED,
    FSW_UNKNOWN_ERROR
};


/**
 * Core: A string with explicit length and encoding information.
 */

struct fsw_string {
    int         type;               //!< Encoding of the string - empty, ISO-8859-1, UTF8, UTF16
    int         len;                //!< Length in characters
    int         size;               //!< Total data size in bytes
    void        *data;              //!< Data pointer (may be NULL if type is EMPTY or len is zero)
};

/**
 * Possible string types / encodings. In the case of FSW_STRING_TYPE_EMPTY,
 * all other members of the fsw_string structure may be invalid.
 */
enum {
    FSW_STRING_TYPE_EMPTY,
    FSW_STRING_TYPE_ISO88591,
    FSW_STRING_TYPE_UTF8,
    FSW_STRING_TYPE_UTF16
};

/** Static initializer for an empty string. */
#define FSW_STRING_INIT { FSW_STRING_TYPE_EMPTY, 0, 0, NULL }


/* forward declarations */

struct fsw_dnode;
struct fsw_host_table;
struct fsw_fstype_table;

/**
 * Core: Represents a mounted volume.
 */

struct fsw_volume {
    fsw_u32     phys_blocksize;     //!< Block size for disk access / file system structures
    fsw_u32     log_blocksize;      //!< Block size for logical file data
    
    struct DNODESTRUCTNAME *root;   //!< Root directory dnode
    struct fsw_string label;        //!< Volume label
    
    struct fsw_dnode *dnode_head;   //!< List of all dnodes allocated for this volume
    
    void        *host_data;         //!< Hook for a host-specific data structure
    struct fsw_host_table *host_table;      //!< Dispatch table for host-specific functions
    struct fsw_fstype_table *fstype_table;  //!< Dispatch table for file system specific functions
    int         host_string_type;   //!< String type used by the host environment
};

/**
 * Core: Represents a "directory node" - a file, directory, symlink, whatever.
 */

struct fsw_dnode {
    fsw_u32     refcount;           //!< Reference count
    
    struct VOLSTRUCTNAME *vol;      //!< The volume this dnode belongs to
    struct DNODESTRUCTNAME *parent; //!< Parent directory dnode
    struct fsw_string name;         //!< Name of this item in the parent directory
    
    fsw_u32     dnode_id;           //!< Unique id number (usually the inode number)
    int         type;               //!< Type of the dnode - file, dir, symlink, special
    fsw_u64     size;               //!< Data size in bytes
    
    struct fsw_dnode *next;         //!< Doubly-linked list of all dnodes: previous dnode
    struct fsw_dnode *prev;         //!< Doubly-linked list of all dnodes: next dnode
};

/**
 * Possible dnode types. FSW_DNODE_TYPE_UNKNOWN may only be used before
 * fsw_dnode_fill has been called on the dnode.
 */
enum {
    FSW_DNODE_TYPE_UNKNOWN,
    FSW_DNODE_TYPE_FILE,
    FSW_DNODE_TYPE_DIR,
    FSW_DNODE_TYPE_SYMLINK,
    FSW_DNODE_TYPE_SPECIAL
};

/**
 * Core: Stores the mapping of a region of a file to the data on disk.
 */

struct fsw_extent {
    int         type;               //!< Type of extent specification
    fsw_u32     log_start;          //!< Starting logical block number
    fsw_u32     log_count;          //!< Logical block count
    fsw_u32     phys_start;         //!< Starting physical block number (for FSW_EXTENT_TYPE_PHYSBLOCK only)
    void        *buffer;            //!< Allocated buffer pointer (for FSW_EXTENT_TYPE_BUFFER only)
};

/**
 * Possible extent representation types. FSW_EXTENT_TYPE_INVALID is for shandle's
 * internal use only, it must not be returned from a get_extent function.
 */
enum {
    FSW_EXTENT_TYPE_INVALID,
    FSW_EXTENT_TYPE_SPARSE,
    FSW_EXTENT_TYPE_PHYSBLOCK,
    FSW_EXTENT_TYPE_BUFFER
};

/**
 * Core: An access structure to a dnode's raw data. There can be multiple
 * shandles per dnode, each of them has its own position pointer.
 */

struct fsw_shandle {
    struct fsw_dnode *dnode;        //!< The dnode this handle reads data from
    
    fsw_u64     pos;                //!< Current file pointer in bytes
    struct fsw_extent extent;       //!< Current extent
};

/**
 * Core: Used in gathering detailed information on a volume.
 */

struct fsw_volume_stat {
    fsw_u64     total_bytes;        //!< Total size of data area size in bytes
    fsw_u64     free_bytes;         //!< Bytes still available for storing file data
};

/**
 * Core: Used in gathering detailed information on a dnode.
 */

struct fsw_dnode_stat {
    fsw_u64     used_bytes;         //!< Bytes actually used by the file on disk
    void        (*store_time_posix)(struct fsw_dnode_stat *sb, int which, fsw_u32 posix_time);   //!< Callback for storing a Posix-style timestamp
    void        (*store_attr_posix)(struct fsw_dnode_stat *sb, fsw_u16 posix_mode);   //!< Callbock for storing a Posix-style file mode
    void        *host_data;         //!< Hook for a host-specific data structure
};

/**
 * Type of the timestamp passed into store_time_posix.
 */
enum {
    FSW_DNODE_STAT_CTIME,
    FSW_DNODE_STAT_MTIME,
    FSW_DNODE_STAT_ATIME
};

/**
 * Core: Function table for a host environment.
 */

struct fsw_host_table
{
    int         native_string_type; //!< String type used by the host environment
    
    void         (*change_blocksize)(struct fsw_volume *vol,
                                     fsw_u32 old_phys_blocksize, fsw_u32 old_log_blocksize,
                                     fsw_u32 new_phys_blocksize, fsw_u32 new_log_blocksize);
    fsw_status_t (*read_block)(struct fsw_volume *vol, fsw_u32 phys_bno, void **buffer_out);
};

/**
 * Core: Function table for a file system driver.
 */

struct fsw_fstype_table
{
    struct fsw_string name;         //!< String giving the name of the file system
    fsw_u32     volume_struct_size; //!< Size for allocating the fsw_volume structure
    fsw_u32     dnode_struct_size;  //!< Size for allocating the fsw_dnode structure
    
    fsw_status_t (*volume_mount)(struct VOLSTRUCTNAME *vol);
    void         (*volume_free)(struct VOLSTRUCTNAME *vol);
    fsw_status_t (*volume_stat)(struct VOLSTRUCTNAME *vol, struct fsw_volume_stat *sb);
    
    fsw_status_t (*dnode_fill)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno);
    void         (*dnode_free)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno);
    fsw_status_t (*dnode_stat)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno,
                               struct fsw_dnode_stat *sb);
    fsw_status_t (*get_extent)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno,
                               struct fsw_extent *extent);
    
    fsw_status_t (*dir_lookup)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno,
                               struct fsw_string *lookup_name, struct DNODESTRUCTNAME **child_dno);
    fsw_status_t (*dir_read)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno,
                             struct fsw_shandle *shand, struct DNODESTRUCTNAME **child_dno);
    fsw_status_t (*readlink)(struct VOLSTRUCTNAME *vol, struct DNODESTRUCTNAME *dno,
                             struct fsw_string *link_target);
};


/**
 * \name Volume Functions
 */
/*@{*/

fsw_status_t fsw_mount(void *host_data,
                       struct fsw_host_table *host_table,
                       struct fsw_fstype_table *fstype_table,
                       struct fsw_volume **vol_out);
void         fsw_unmount(struct fsw_volume *vol);
fsw_status_t fsw_volume_stat(struct fsw_volume *vol, struct fsw_volume_stat *sb);

void         fsw_set_blocksize(struct VOLSTRUCTNAME *vol, fsw_u32 phys_blocksize, fsw_u32 log_blocksize);
fsw_status_t fsw_read_block(struct VOLSTRUCTNAME *vol, fsw_u32 phys_bno, void **buffer_out);

/*@}*/


/**
 * \name dnode Functions
 */
/*@{*/

fsw_status_t fsw_dnode_create_root(struct VOLSTRUCTNAME *vol, fsw_u32 dnode_id, struct DNODESTRUCTNAME **dno_out);
fsw_status_t fsw_dnode_create(struct DNODESTRUCTNAME *parent_dno, fsw_u32 dnode_id, int type,
                              struct fsw_string *name, struct DNODESTRUCTNAME **dno_out);
void         fsw_dnode_retain(struct fsw_dnode *dno);
void         fsw_dnode_release(struct fsw_dnode *dno);

fsw_status_t fsw_dnode_fill(struct fsw_dnode *dno);
fsw_status_t fsw_dnode_stat(struct fsw_dnode *dno, struct fsw_dnode_stat *sb);

fsw_status_t fsw_dnode_lookup(struct fsw_dnode *dno,
                              struct fsw_string *lookup_name, struct fsw_dnode **child_dno_out);
fsw_status_t fsw_dnode_lookup_path(struct fsw_dnode *dno,
                                   struct fsw_string *lookup_path, char separator,
                                   struct fsw_dnode **child_dno_out);
fsw_status_t fsw_dnode_dir_read(struct fsw_shandle *shand, struct fsw_dnode **child_dno_out);
fsw_status_t fsw_dnode_readlink(struct fsw_dnode *dno, struct fsw_string *link_target);
fsw_status_t fsw_dnode_readlink_data(struct DNODESTRUCTNAME *dno, struct fsw_string *link_target);
fsw_status_t fsw_dnode_resolve(struct fsw_dnode *dno, struct fsw_dnode **target_dno_out);

/*@}*/


/**
 * \name shandle Functions
 */
/*@{*/

fsw_status_t fsw_shandle_open(struct DNODESTRUCTNAME *dno, struct fsw_shandle *shand);
void         fsw_shandle_close(struct fsw_shandle *shand);
fsw_status_t fsw_shandle_read(struct fsw_shandle *shand, fsw_u32 *buffer_size_inout, void *buffer);

/*@}*/


/**
 * \name Memory Functions
 */
/*@{*/

fsw_status_t fsw_alloc_zero(int len, void **ptr_out);
fsw_status_t fsw_memdup(void **dest_out, void *src, int len);

/*@}*/


/**
 * \name String Functions
 */
/*@{*/

int          fsw_strlen(struct fsw_string *s);
int          fsw_streq(struct fsw_string *s1, struct fsw_string *s2);
int          fsw_streq_cstr(struct fsw_string *s1, const char *s2);
fsw_status_t fsw_strdup_coerce(struct fsw_string *dest, int type, struct fsw_string *src);
void         fsw_strsplit(struct fsw_string *lookup_name, struct fsw_string *buffer, char separator);

void         fsw_strfree(struct fsw_string *s);

/*@}*/


/**
 * \name Posix Mode Macros
 * These macros can be used globally to test fields and bits in
 * Posix-style modes.
 *
 * Taken from FreeBSD sys/stat.h.
 */
/*@{*/
#ifndef S_IRWXU

#define	S_ISUID	0004000			/* set user id on execution */
#define	S_ISGID	0002000			/* set group id on execution */
#define	S_ISTXT	0001000			/* sticky bit */

#define	S_IRWXU	0000700			/* RWX mask for owner */
#define	S_IRUSR	0000400			/* R for owner */
#define	S_IWUSR	0000200			/* W for owner */
#define	S_IXUSR	0000100			/* X for owner */

#define	S_IRWXG	0000070			/* RWX mask for group */
#define	S_IRGRP	0000040			/* R for group */
#define	S_IWGRP	0000020			/* W for group */
#define	S_IXGRP	0000010			/* X for group */

#define	S_IRWXO	0000007			/* RWX mask for other */
#define	S_IROTH	0000004			/* R for other */
#define	S_IWOTH	0000002			/* W for other */
#define	S_IXOTH	0000001			/* X for other */

#define	S_IFMT	 0170000		/* type of file mask */
#define	S_IFIFO	 0010000		/* named pipe (fifo) */
#define	S_IFCHR	 0020000		/* character special */
#define	S_IFDIR	 0040000		/* directory */
#define	S_IFBLK	 0060000		/* block special */
#define	S_IFREG	 0100000		/* regular */
#define	S_IFLNK	 0120000		/* symbolic link */
#define	S_IFSOCK 0140000		/* socket */
#define	S_ISVTX	 0001000		/* save swapped text even after use */
#define	S_IFWHT  0160000		/* whiteout */

#define	S_ISDIR(m)	(((m) & 0170000) == 0040000)	/* directory */
#define	S_ISCHR(m)	(((m) & 0170000) == 0020000)	/* char special */
#define	S_ISBLK(m)	(((m) & 0170000) == 0060000)	/* block special */
#define	S_ISREG(m)	(((m) & 0170000) == 0100000)	/* regular file */
#define	S_ISFIFO(m)	(((m) & 0170000) == 0010000)	/* fifo or socket */
#define	S_ISLNK(m)	(((m) & 0170000) == 0120000)	/* symbolic link */
#define	S_ISSOCK(m)	(((m) & 0170000) == 0140000)	/* socket */
#define	S_ISWHT(m)	(((m) & 0170000) == 0160000)	/* whiteout */

#define S_BLKSIZE	512		/* block size used in the stat struct */

#endif
/*@}*/


#endif
