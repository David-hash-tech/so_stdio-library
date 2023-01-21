#include "so_stdio.h"

#include <stdio.h>

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE (4096)

struct _so_file
{
    HANDLE fd;
    DWORD eof;
    DWORD maxRead;
    DWORD isWrite;
    DWORD isError;
    DWORD isUpdate;
    DWORD isAppend;
    CHAR buffer[BUFFER_SIZE];
    DWORD bufferCount;
    PROCESS_INFORMATION pi;
};

SO_FILE* so_fopen(const char* pathname, const char* mode)
{
    SO_FILE* stream = (SO_FILE*)malloc(sizeof(SO_FILE));
    stream->eof = 0;
    stream->isError = 0;
    stream->isWrite = -1;
    stream->maxRead = 0;
    stream->isAppend = 0;
    stream->bufferCount = 0;

    if (strcmp(mode, "r") == 0)
    {
        stream->fd = CreateFileA(pathname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (stream->fd == INVALID_HANDLE_VALUE)
        {
            stream->isError = 1;
            free(stream);
            return NULL;
        }
        stream->isUpdate = 0;
        return stream;
    }
    else if (strcmp(mode, "r+") == 0)
    {
        stream->fd = CreateFileA(pathname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (stream->fd == INVALID_HANDLE_VALUE)
        {
            stream->isError = 1;
            free(stream);
            return NULL;
        }
        stream->isUpdate = 1;
        return stream;
    }
    else if (strcmp(mode, "w") == 0)
    {
        stream->fd = CreateFileA(pathname, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (stream->fd == INVALID_HANDLE_VALUE)
        {
            stream->isError = 1;
            free(stream);
            return NULL;
        }
        stream->isUpdate = 0;
        return stream;
    }
    else if (strcmp(mode, "w+") == 0)
    {
        stream->fd = CreateFileA(pathname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (stream->fd == INVALID_HANDLE_VALUE)
        {
            stream->isError = 1;
            free(stream);
            return NULL;
        }
        stream->isUpdate = 1;
        return stream;
    }
    else if (strcmp(mode, "a") == 0)
    {
        stream->fd = CreateFileA(pathname, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (stream->fd == INVALID_HANDLE_VALUE)
        {
            stream->isError = 1;
            free(stream);
            return NULL;
        }
        stream->isUpdate = 0;
        stream->isAppend = 1;
        return stream;
    }
    else if (strcmp(mode, "a+") == 0)
    {
        stream->fd = CreateFileA(pathname, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW | OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (stream->fd == INVALID_HANDLE_VALUE)
        {
            stream->isError = 1;
            free(stream);
            return NULL;
        }
        stream->isUpdate = 1;
        stream->isAppend = 1;
        return stream;
    }
    else
    {
        free(stream);
        return NULL;
    }
}

int so_fclose(SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    DWORD ret = 0;
    if (stream->isWrite == 1)
    {
        ret = so_fflush(stream);
        if (ret == (-1))
            return (-1);
    }
    ret = CloseHandle(so_fileno(stream));
    if (ret == 0)
    {
        stream->eof = 1;
        stream->isError = 1;
        free(stream);
        return -1;
    }
    if (stream != NULL)
        free(stream);
    return 0;
}

int so_feof(SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    if (stream->eof == 1)
        return -1;
    else
        return 0;
}

int so_ferror(SO_FILE* stream)
{
    if (stream == NULL)
        return 1;

    if (stream->isError != 0)
        return 1;
    return 0;
}

int so_fseek(SO_FILE* stream, long offset, int whence)
{
    if (stream == NULL)
        return -1;

    if (stream->isAppend == (-1))
    {
        stream->isAppend = 1;
        DWORD pos = SetFilePointer(so_fileno(stream), offset, NULL, whence);
        if (pos == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return pos;
        }
        return 0;
    }

    if (stream->isWrite == 1)
    {
        DWORD ret = so_fflush(stream);
        if (ret == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }
    }
    else if (stream->isWrite == 0)
        stream->bufferCount = 0;

    DWORD pos = SetFilePointer(so_fileno(stream), offset, NULL, whence);
    if (pos == (-1))
    {
        stream->eof = 1;
        stream->isError = 1;
        return -1;
    }
    return 0;
}

long so_ftell(SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    DWORD pos = SetFilePointer(so_fileno(stream), 0, NULL, FILE_CURRENT);
    if (pos == (-1))
    {
        stream->eof = 1;
        stream->isError = 1;
        return (-1);
    }

    if (stream->isWrite == (-1))
        return pos;
    else if (stream->isWrite == 1)
    {
        pos = pos + stream->bufferCount;
        return pos;
    }

    pos = pos - stream->maxRead + stream->bufferCount;
    return pos;
}

int so_fputc(int c, SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    if (stream->isWrite == (-1))
    {
        stream->isWrite = 1;
    }
    else if (stream->isWrite == 0)
    {
        stream->bufferCount = 0;
        stream->isWrite = 1;
    }
    else if (stream->bufferCount == BUFFER_SIZE && stream->isWrite == 1)
    {
        DWORD ret = so_fflush(stream);
        if (ret == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }
        stream->bufferCount = 0;
    }

    stream->buffer[stream->bufferCount] = c;
    stream->bufferCount++;

    return c;
}

size_t so_fwrite(const void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{
    if (stream == NULL)
        return 0;

    CHAR* cp = (char*)ptr;
    unsigned char c;
    unsigned char aux;
    size_t writed = 0;

    while (writed < size * nmemb)
    {
        memcpy(&aux, cp + writed, sizeof(char));
        c = so_fputc(aux, stream);
        if (c == -1)
        {
            stream->eof = 1;
            stream->isError = 1;
            return 0;
        }
        writed++;
    }

    return writed / size;
}

int so_fgetc(SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    DWORD ret = 0;
    if (stream->isWrite == (-1) || stream->bufferCount == 0)
    {
        ret = ReadFile(so_fileno(stream), stream->buffer, BUFFER_SIZE, &(stream->maxRead), NULL);
        if (ret == 0)
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }
        stream->isWrite = 0;
    }
    else if (stream->isWrite == 1)
    {
        ret = so_fflush(stream);
        if (ret == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }

        ret = ReadFile(so_fileno(stream), stream->buffer, BUFFER_SIZE, &(stream->maxRead), NULL);
        if (ret == 0)
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }

        stream->isWrite = 0;
    }
    else if (stream->bufferCount >= BUFFER_SIZE && stream->isWrite == 0)
    {
        stream->bufferCount = 0;

        ret = ReadFile(so_fileno(stream), stream->buffer, BUFFER_SIZE, &(stream->maxRead), NULL);
        if (ret == 0)
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }
    }

    if (stream->bufferCount == stream->maxRead) // a ajuns la EOF in fisier
    {
        stream->eof = 1;
        return 0;
    }

    CHAR ch;
    ch = stream->buffer[stream->bufferCount];
    stream->bufferCount++;

    return ch;
}

size_t so_fread(void* ptr, size_t size, size_t nmemb, SO_FILE* stream)
{
    if (stream == NULL)
        return 0;

    CHAR* cp = (CHAR*)ptr;
    DWORD c = 0;
    size_t readed = 0;
    unsigned char chr = 0;

    while (readed < size * nmemb)
    {
        c = so_fgetc(stream);

        if (so_feof(stream) != 0)
        {
            break;
        }

        *cp++ = (unsigned char)c;
        readed++;
    }
    return (readed / size);
}

HANDLE so_fileno(SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    return stream->fd;
}

int WriteAll(HANDLE fd, const char* buffer, size_t nb)
{
    DWORD bytesW = 0;
    DWORD caspa = 0;
    DWORD ret = 0;

    while (caspa < nb)
    {
        ret = WriteFile(fd, buffer + caspa, nb - caspa, &bytesW, NULL);
        if (ret == 0)
        {
            return (-1);
        }

        caspa += bytesW;
    }

    return caspa;
}

int so_fflush(SO_FILE* stream)
{
    if (stream == NULL)
        return -1;

    DWORD ret;
    if (stream->isWrite == 0 || stream->isWrite == (-1))
        return 0;
    if (stream->isAppend == 1 && stream->isWrite == 1)
    {
        stream->isAppend = (-1);
        ret = so_fseek(stream, 0, SEEK_END);
        if (ret == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return -1;
        }
    }

    DWORD nrChr = WriteAll(so_fileno(stream), stream->buffer, stream->bufferCount);
    if (nrChr == (-1))
    {
        stream->eof = 1;
        stream->isError = 1;
        return (-1);
    }

    stream->bufferCount = 0;
    return 0;
}

SO_FILE* so_popen(const char* command, const char* type)
{
    SO_FILE* stream = (SO_FILE*)malloc(sizeof(SO_FILE));
    stream->eof = 0;
    stream->isError = 0;
    stream->isWrite = -1;
    stream->maxRead = 0;
    stream->isAppend = 0;
    stream->bufferCount = 0;
    stream->isUpdate = 0;

    BOOL ret;
    STARTUPINFO si;
    HANDLE rhandle, whandle;
    SECURITY_ATTRIBUTES si_pipe;
    
    CHAR command_line[100] = "C:\\Windows\\System32\\cmd.exe /C ";
    strcat_s(command_line, sizeof(command_line), command);

    ZeroMemory(&(stream->pi), sizeof(stream->pi));

    ZeroMemory(&si_pipe, sizeof(si_pipe));
    si_pipe.nLength = sizeof(si_pipe);
    si_pipe.bInheritHandle = TRUE;

    GetStartupInfo(&si);

    CreatePipe(&rhandle, &whandle, &si_pipe, 0);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (strcmp(type, "r") == 0)
    {
        stream->fd = rhandle;
        si.hStdOutput = whandle;
        ret = SetHandleInformation(rhandle, HANDLE_FLAG_INHERIT, 0);
    }
    else if (strcmp(type, "w") == 0)
    {
        stream->fd = whandle;
        si.hStdInput = rhandle;
        ret = SetHandleInformation(whandle, HANDLE_FLAG_INHERIT, 0);
    }
    else
    {
        CloseHandle(stream->pi.hProcess);
        CloseHandle(stream->pi.hThread);
        free(stream);
    }

    ret = CreateProcessA(NULL, command_line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &(stream->pi));

    if (strcmp(type, "r") == 0)
        CloseHandle(whandle);

    if (strcmp(type, "w") == 0)
        CloseHandle(rhandle);

    if (ret == 0)
    {
        free(stream);
        return NULL;
    }

    return stream;
}

int so_pclose(SO_FILE* stream)
{
    DWORD ret = 0;
    DWORD WaitRet = 0;
    DWORD exitCode = 0;

    if (stream == NULL)
        return -1;

    if (stream->isWrite == 1)
    {
        ret = so_fflush(stream);
        if (ret == -1)
        {
            stream->isError = 1;
            stream->eof = 1;
            free(stream);
            return -1;
        }
    }

    CloseHandle(so_fileno(stream));

    WaitRet = WaitForSingleObject(stream->pi.hProcess, INFINITE);
    if (WaitRet == WAIT_FAILED)
    {
        return -1;
    }

    GetExitCodeProcess(stream->pi.hProcess, &exitCode);

    CloseHandle(stream->pi.hProcess);
    CloseHandle(stream->pi.hThread);

    if (stream != NULL)
        free(stream);

    return exitCode;
}
