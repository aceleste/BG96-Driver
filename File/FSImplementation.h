/**
 * @file FSImplementation.h
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

