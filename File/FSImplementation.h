#ifndef _FS_IMPLEMENTATION_H_
#define _FS_IMPLEMENTATION_H_
#include "BG96.h"
#include "FSInterface.h"

class FSImplementation: public FSInterface
{
public: 
    FSImplementation(BG96 *bg96);
    ~FSImplementation(){};

    /** Returns the free space on UFS
     *
     *
     */
    virtual size_t fs_free_size();

    /** Returns the total space of UFS
     *
     *
     */
    virtual size_t fs_total_size();

    /** Returns the total number of files on UFS
     *
     *
     */
    virtual int fs_total_number_of_files();

    /** Returns the total space used by files on UFS
     *
     *
     */
    virtual size_t fs_total_size_of_files();

    /** Returns the free space on UFS
     *
     *  @param filename The full path and filename of the file
     */
    virtual size_t fs_file_size(const char *filename);

    virtual bool fs_file_exists(const char *filename);

    virtual int fs_delete_file(const char *filename);

    virtual int fs_upload_file(const char *filename, void *data, size_t size);

    virtual size_t fs_download_file(const char *filename, void* data, int16_t &checksum);

    virtual bool fs_open(const char *filename, FILE_MODE mode, FILE_HANDLE &fh); 

    virtual bool fs_read(FILE_HANDLE fh, size_t length, void *data);

    virtual bool fs_write(FILE_HANDLE fh, size_t length, void *data);

    virtual bool fs_seek(FILE_HANDLE fh, size_t offset);

    virtual bool fs_rewind(FILE_HANDLE fh);

    virtual bool fs_eof(FILE_HANDLE fh);

    virtual bool fs_get_offset(FILE_HANDLE fh, size_t &offset);

    virtual bool fs_truncate(FILE_HANDLE fh, size_t offset);

    virtual bool fs_close(FILE_HANDLE fh);

    virtual FS_ERROR fs_get_error();

private:
    BG96 *_bg96;
};

#endif //_FS_IMPLEMENTATION_H_

