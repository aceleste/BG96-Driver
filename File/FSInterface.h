/**
 * @file FSInterface.h
 * @author Alain CELESTE (alain.celeste@polaris-innovation.com)
 * @brief 
 * @version 0.1
 * @date 2019-08-14
 * 
 * @copyright Copyright (c) 2019 Polaris Innovation
 * 
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */

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
