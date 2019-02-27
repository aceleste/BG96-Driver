#include "FSInterface.h"
#include "BG96.h"
#include "FSImplementation.h"
#include "mbed.h"
#include "nsapi_types.h"

//TODO: error mechanism to be reviewed. It may happen that getError will return an invalid error because another AT command
// might have been fired which would have also failed for another reason after the initial fs related command failed. fs error should then 
// be maintained in the BG96 core driver class and updated at each fs commande failure inside the mutex lock/unlock.

FSImplementation::FSImplementation(BG96 *bg96)
{
    _bg96 = bg96;
}

size_t FSImplementation::fs_free_size()
{
    size_t freesize;
    size_t totalsize;
    if (_bg96->fs_size(freesize, totalsize) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return freesize;
    } else {
        _bg96->getError(_fs_error);
        return 0;
    }
}

size_t FSImplementation::fs_total_size()
{
    size_t freesize;
    size_t totalsize;
    if (_bg96->fs_size(freesize, totalsize) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return totalsize;
    } else {
        _bg96->getError(_fs_error);
        return 0;
    }  
}

int FSImplementation::fs_total_number_of_files()
{
    int nfiles;
    size_t sfiles;
    if (_bg96->fs_nfiles(nfiles, sfiles) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return nfiles;
    } else {
        _bg96->getError(_fs_error);
        return 0;
    }
}

size_t FSImplementation::fs_total_size_of_files()
{
    int nfiles;
    size_t sfiles;
    if (_bg96->fs_nfiles(nfiles, sfiles) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return sfiles;
    } else {
        _bg96->getError(_fs_error);
        return 0;
    }   
}

size_t FSImplementation::fs_file_size(const char *filename)
{
    size_t filesize;
    if (_bg96->fs_file_size(filename, filesize) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return filesize;
    } else {
        _bg96->getError(_fs_error);
        return 0;
    }
}

bool FSImplementation::fs_file_exists(const char *filename)
{
    size_t filesize;
    if (_bg96->fs_file_size(filename, filesize) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    } else {
        _bg96->getError(_fs_error);
        return false;
    }
}

int FSImplementation::fs_delete_file(const char *filename)
{
    int rc;
    if ((rc = _bg96->fs_delete_file(filename))==0) {
        _fs_error = FS_ERROR_NO_ERROR;
        return 0;
    } else {
        _bg96->getError(_fs_error);
        return -1;
    }
}

int FSImplementation::fs_upload_file(const char *filename, void *data, size_t size)
{
    size_t lsize = size;
    if (_bg96->fs_upload_file(filename, data, lsize)==NSAPI_ERROR_OK && lsize == size) {
        _fs_error = FS_ERROR_NO_ERROR;
        return 0;
    } else {
        _bg96->getError(_fs_error);
        return -1;
    }
}

size_t FSImplementation::fs_download_file(const char *filename, void* data, int16_t &checksum)
{
    size_t fsize = fs_file_size(filename);
    if (_bg96->fs_download_file(filename, data, fsize, checksum) == NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return fsize;
    } else {
        _bg96->getError(_fs_error);
        return 0;
    }
}

bool FSImplementation::fs_open(const char *filename, FILE_MODE mode, FILE_HANDLE &fh)
{
    if(_bg96->fs_open(filename,mode, fh)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    } else {
        _bg96->getError(_fs_error);
        return false;
    }
}

bool FSImplementation::fs_read(FILE_HANDLE fh, size_t length, void *data)
{
    if (_bg96->fs_read(fh,length,data)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    } else {
        _bg96->getError(_fs_error);
        return false;
    }
}

bool FSImplementation::fs_write(FILE_HANDLE fh, size_t length, void *data)
{
    if (_bg96->fs_write(fh,length,data)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;
}

bool FSImplementation::fs_seek(FILE_HANDLE fh, size_t offset)
{
    if (_bg96->fs_seek(fh,offset,START_OF_FILE)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;   
}

bool FSImplementation::fs_rewind(FILE_HANDLE fh)
{
    if (_bg96->fs_seek(fh,0,START_OF_FILE)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;   
}

bool FSImplementation::fs_eof(FILE_HANDLE fh)
{
    if (_bg96->fs_seek(fh,0,END_OF_FILE)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;   
}

bool FSImplementation::fs_get_offset(FILE_HANDLE fh, size_t &offset)
{
    if (_bg96->fs_get_offset(fh, offset)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;
}

bool FSImplementation::fs_truncate(FILE_HANDLE fh, size_t offset)
{
    if (_bg96->fs_truncate(fh, offset)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;
}

bool FSImplementation::fs_close(FILE_HANDLE fh)
{
    if (_bg96->fs_close(fh)==NSAPI_ERROR_OK) {
        _fs_error = FS_ERROR_NO_ERROR;
        return true;
    }
    _bg96->getError(_fs_error);
    return false;
}

FS_ERROR FSImplementation::fs_get_error()
{
    return _fs_error;
}