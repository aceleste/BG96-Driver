#ifndef _FS_IMPLEMENTATION_H_
#define _FS_IMPLEMENTATION_H_
#include "BG96.h"
#include "FSInterface.h"

class FSImplementation: public FSInterface
{
public: 
    FSImplementation(BG96 *bg96);
    inline virtual ~FSImplementation(){};

    /** Returns the free space on UFS
     *
     *
     */
    size_t fs_free_size();

    /** Returns the total space of UFS
     *
     *
     */
    size_t fs_total_size();

    /** Returns the total number of files on UFS
     *
     *
     */
    int fs_total_number_of_files();

    /** Returns the total space used by files on UFS
     *
     *
     */
    size_t fs_total_size_of_files();

    /** Returns the free space on UFS
     *
     *  @param filename The full path and filename of the file
     */
    size_t fs_file_size(const char *filename);

    bool fs_file_exists(const char *filename);

    int fs_delete_file(const char *filename);

    int fs_upload_file(const char *filename, void *data, size_t size);

    size_t fs_download_file(const char *filename, void* data, int16_t &checksum);

    bool fs_open(const char *filename, FILE_MODE mode, FILE_HANDLE &fh);

    bool fs_read(FILE_HANDLE fh, size_t length, void *data);

    bool fs_write(FILE_HANDLE fh, size_t length, void *data);

    bool fs_seek(FILE_HANDLE fh, size_t offset);

    bool fs_rewind(FILE_HANDLE fh);

    bool fs_eof(FILE_HANDLE fh);

    bool fs_get_offset(FILE_HANDLE fh, size_t &offset);

    bool fs_truncate(FILE_HANDLE fh, size_t offset);

    bool fs_close(FILE_HANDLE fh);

    FS_ERROR fs_get_error();

private:
    BG96 *_bg96;
};

#endif //_FS_IMPLEMENTATION_H_

