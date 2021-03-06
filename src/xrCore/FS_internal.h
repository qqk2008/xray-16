#ifndef FS_internalH
#define FS_internalH
#pragma once

#include "lzhuf.h"
#if defined(XR_PLATFORM_WINDOWS)
#include <io.h>
#endif
#include <fcntl.h>
#if defined(XR_PLATFORM_WINDOWS)
#include <sys\stat.h>
#include <share.h>
#endif

#if defined(XR_PLATFORM_FREEBSD)
#define _sys_errlist sys_errlist
#endif

void* FileDownload(pcstr fn, size_t* pdwSize = nullptr);
void FileCompress(pcstr fn, pcstr sign, void* data, size_t size);
void* FileDecompress(pcstr fn, pcstr sign, size_t* size = nullptr);

class CFileWriter : public IWriter
{
private:
    FILE* hf;

public:
    CFileWriter(const char* name, bool exclusive)
    {
        R_ASSERT(name && name[0]);
        fName = name;
        VerifyPath(fName.c_str());
        pstr conv_fn = xr_strdup(name);
        convert_path_separators(conv_fn);
        if (exclusive)
        {
            const int handle = _sopen(conv_fn, _O_WRONLY | _O_TRUNC | _O_CREAT | _O_BINARY, SH_DENYWR);
#ifdef _EDITOR
            if (handle == -1)
                Msg("!Can't create file: '%s'. Error: '%s'.", conv_fn, _sys_errlist[errno]);
#endif
            hf = _fdopen(handle, "wb");
        }
        else
        {
            hf = fopen(conv_fn, "wb");
            if (hf == 0)
                Msg("!Can't write file: '%s'. Error: '%s'.", conv_fn, _sys_errlist[errno]);
        }
        xr_free(conv_fn);
    }

    virtual ~CFileWriter()
    {
        if (0 != hf)
        {
            fclose(hf);
            // release RO attrib
#if defined(XR_PLATFORM_WINDOWS)
            DWORD dwAttr = GetFileAttributes(fName.c_str());
            if ((dwAttr != u32(-1)) && (dwAttr & FILE_ATTRIBUTE_READONLY))
            {
                dwAttr &= ~FILE_ATTRIBUTE_READONLY;
                SetFileAttributes(fName.c_str(), dwAttr);
            }
#endif
        }
    }
    // kernel
    void w(const void* _ptr, size_t count) override
    {
        if ((0 != hf) && (0 != count))
        {
            const size_t mb_sz = 0x1000000; // 2 MB
            u8* ptr = (u8*)_ptr;
            size_t req_size;
            for (req_size = count; req_size > mb_sz; req_size -= mb_sz, ptr += mb_sz)
            {
                size_t W = fwrite(ptr, mb_sz, 1, hf);
                R_ASSERT3(W == 1, "Can't write mem block to file. Disk maybe full.", _sys_errlist[errno]);
            }
            if (req_size)
            {
                size_t W = fwrite(ptr, req_size, 1, hf);
                R_ASSERT3(W == 1, "Can't write mem block to file. Disk maybe full.", _sys_errlist[errno]);
            }
        }
    };

    void seek(size_t pos) override
    {
        if (0 != hf)
            fseek(hf, pos, SEEK_SET);
    };
    size_t tell() override { return (0 != hf) ? ftell(hf) : 0; };
    bool valid() override { return (0 != hf); }

    void flush() override
    {
        if (hf)
            fflush(hf);
    };
};

// It automatically frees memory after destruction
class CTempReader : public IReader
{
public:
    CTempReader(void* _data, size_t _size, size_t _iterpos) : IReader(_data, _size, _iterpos) {}
    virtual ~CTempReader();
};
class CPackReader : public IReader
{
    void* base_address;

public:
    CPackReader(void* _base, void* _data, size_t _size) : IReader(_data, _size), base_address(_base) {}
    virtual ~CPackReader();
};
class XRCORE_API CFileReader : public IReader
{
public:
    CFileReader(pcstr name);
    virtual ~CFileReader();
};
class CCompressedReader : public IReader
{
public:
    CCompressedReader(const char* name, const char* sign);
    virtual ~CCompressedReader();
};
class CVirtualFileReader : public IReader
{
private:
#if defined(XR_PLATFORM_WINDOWS)
    void *hSrcFile, *hSrcMap;
#elif defined(XR_PLATFORM_LINUX)
    int hSrcFile;
#endif

public:
    CVirtualFileReader(pcstr cFileName);
    virtual ~CVirtualFileReader();
};

#endif
