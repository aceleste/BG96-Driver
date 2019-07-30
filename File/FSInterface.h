#ifndef __FS_INTERFACE_H__ 
#define __FS_INTERFACE_H__
#include "mbed.h"
#include <string.h>

#define MAX_ERROR_DESCRIPTION_LENGTH 40

#define FS_ERROR_NO_ERROR {.description = {0}, .errornum = 0}

typedef int32_t FILE_HANDLE;

typedef enum {START_OF_FILE, CURRENT_POSITION, END_OF_FILE} FILE_POS;

typedef enum {CREATE_RW, OVERWRITE_RW, EXISTONLY_RO} FILE_MODE;

typedef struct 
{
    char description[MAX_ERROR_DESCRIPTION_LENGTH]={0};
    int errornum=0;
    /* data */
} BG96_ERROR;

typedef BG96_ERROR FS_ERROR;

class FSInterface
{
public:
	virtual ~FSInterface() {}

    /** Returns the free space on UFS
     *
     *
     */
    virtual size_t fs_free_size()=0;

    /** Returns the total space of UFS
     *
     *
     */
    virtual size_t fs_total_size()=0;

    /** Returns the total number of files on UFS
     *
     *
     */
    virtual int fs_total_number_of_files()=0;

    /** Returns the total space used by files on UFS
     *
     *
     */
    virtual size_t fs_total_size_of_files()=0;

    /** Returns the free space on UFS
     *
     *  @param filename The full path and filename of the file
     */
    virtual size_t fs_file_size(const char *filename)=0;

    virtual bool fs_file_exists(const char *filename)=0;

    virtual int fs_delete_file(const char *filename)=0;

    virtual int fs_upload_file(const char *filename, void *data, size_t size)=0;

    virtual size_t fs_download_file(const char *filename, void* data, int16_t &checksum)=0;

    virtual bool fs_open(const char *filename, FILE_MODE mode, FILE_HANDLE &fh)=0;

    virtual bool fs_read(FILE_HANDLE fh, size_t length, void *data)=0;

    virtual bool fs_write(FILE_HANDLE fh, size_t length, void *data)=0;

    virtual bool fs_seek(FILE_HANDLE fh, size_t offset)=0;

    virtual bool fs_rewind(FILE_HANDLE fh)=0;

    virtual bool fs_eof(FILE_HANDLE fh)=0;

    virtual bool fs_get_offset(FILE_HANDLE fh, size_t &offset)=0;

    virtual bool fs_truncate(FILE_HANDLE fh, size_t offset)=0;

    virtual bool fs_close(FILE_HANDLE fh)=0;

    virtual FS_ERROR fs_get_error()=0;

protected:

    FS_ERROR _fs_error;
};

#endif //__FS_INTERFACE_H__
