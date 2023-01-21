#include "so_stdio.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <stdio.h>

#define READ_END 0
#define WRITE_END 1
#define BUFFER_SIZE (4096)

struct _so_file
{
    int fd;
    int eof;
    int maxRead;
    int isWrite;
    int isError;
    int isUpdate;
    int isAppend;
    int ChildPId;
    char buffer[BUFFER_SIZE];
    int bufferCount;
};

int WriteAll(int fd, const char *buffer, size_t nb)
{
    int bytesW = 0;
    int caspa = 0;

    while (caspa < nb)
    {
        bytesW = write(fd, buffer + caspa, nb - caspa);
        if (bytesW == (-1))
        {
            return (-1);
        }

        caspa += bytesW;
    }

    return caspa;
}

FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream)
{
    if (stream == NULL)
        return -1;

    int ret;
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

    int nrChr = WriteAll(so_fileno(stream), stream->buffer, stream->bufferCount);
    if (nrChr == (-1))
    {
        stream->eof = 1;
        stream->isError = 1;
        return (-1);
    }

    stream->bufferCount = 0;
    return 0;
}

FUNC_DECL_PREFIX int so_fileno(SO_FILE *stream)
{
    return stream->fd;
}

FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream)
{
    if (stream == NULL)
        return -1;

    if (stream->isWrite == (-1) || stream->bufferCount == 0)
    {
        stream->maxRead = read(so_fileno(stream), stream->buffer, BUFFER_SIZE);
        if (stream->maxRead == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }
        stream->isWrite = 0;
    }
    else if (stream->isWrite == 1)
    {
        int ret = so_fflush(stream);
        if (ret == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }

        stream->maxRead = read(so_fileno(stream), stream->buffer, BUFFER_SIZE);
        if (stream->maxRead == (-1))
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

        stream->maxRead = read(so_fileno(stream), stream->buffer, BUFFER_SIZE);
        if (stream->maxRead == (-1))
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

    char ch;
    ch = stream->buffer[stream->bufferCount];
    stream->bufferCount++;

    return ch;
}

FUNC_DECL_PREFIX size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    if (stream == NULL)
        return 0;

    char *cp = (char *)ptr;
    int c = 0;
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

FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream)
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
        int ret = so_fflush(stream);
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

FUNC_DECL_PREFIX size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    if (stream == NULL)
        return 0;

    char *cp = (char *)ptr;
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

FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream)
{
    if (stream == NULL)
        return -1;

    long int pos = lseek(so_fileno(stream), 0, SEEK_CUR);
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

FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if (stream == NULL)
        return -1;

    if (stream->isAppend == (-1))
    {
        stream->isAppend = 1;
        long int pos = lseek(so_fileno(stream), offset, whence);
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
        int ret = so_fflush(stream);
        if (ret == (-1))
        {
            stream->eof = 1;
            stream->isError = 1;
            return (-1);
        }
    }
    else if (stream->isWrite == 0)
        stream->bufferCount = 0;

    long int pos = lseek(so_fileno(stream), offset, whence);
    if (pos == (-1))
    {
        stream->eof = 1;
        stream->isError = 1;
        return -1;
    }
    return 0;
}

FUNC_DECL_PREFIX SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    SO_FILE *stream = (SO_FILE *)malloc(sizeof(SO_FILE));
    stream->eof = 0;
    stream->isError = 0;
    stream->isWrite = -1;
    stream->maxRead = 0;
    stream->isAppend = 0;
    stream->bufferCount = 0;

    if (strcmp(mode, "r") == 0)
    {
        stream->fd = open(pathname, O_RDONLY);
        if (stream->fd == (-1))
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
        stream->fd = open(pathname, O_RDWR);
        if (stream->fd == (-1))
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
        stream->fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (stream->fd == (-1))
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
        stream->fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (stream->fd == (-1))
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
        stream->fd = open(pathname, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (stream->fd == (-1))
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
        stream->fd = open(pathname, O_RDWR | O_APPEND | O_CREAT, 0644);
        if (stream->fd == (-1))
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

FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream)
{
    if (stream == NULL)
        return -1;

    int ret = 0;
    if (stream->isWrite == 1)
    {
        ret = so_fflush(stream);
        if (ret == (-1))
            return (-1);
    }

    ret = close(so_fileno(stream));
    if (ret == (-1))
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

FUNC_DECL_PREFIX int so_feof(SO_FILE *stream)
{
    if (stream == NULL)
        return -1;

    if (stream->eof == 1)
        return -1;
    else
        return 0;
}

FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream)
{
    if (stream == NULL)
        return 1;

    if (stream->isError != 0)
        return 1;
    return 0;
}

FUNC_DECL_PREFIX SO_FILE *so_popen(const char *command, const char *type)
{
    SO_FILE *stream = (SO_FILE *)malloc(sizeof(SO_FILE));
    stream->eof = 0;
    stream->isError = 0;
    stream->isWrite = -1;
    stream->maxRead = 0;
    stream->isAppend = 0;
    stream->bufferCount = 0;
    stream->isUpdate = 0;

    int ret;
    int pid;
    int pipe_fd[2];

    ret = pipe(pipe_fd);
    if (ret < 0)
    {
        stream->isError = 1;
        stream->eof = 1;
        free(stream);
        return NULL;
    }

    stream->ChildPId = fork();

    if (stream->ChildPId == -1)
    {
        free(stream);
        return NULL;
    }
    else if (stream->ChildPId == 0)
    {
        if (strcmp(type, "r") == 0)
        {
            close(pipe_fd[READ_END]);
            dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
        else if (strcmp(type, "w") == 0)
        {
            close(pipe_fd[WRITE_END]);
            dup2(pipe_fd[READ_END], STDIN_FILENO);
            execl("/bin/sh", "sh", "-c", command, NULL);
        }
    }
    else
    {
        if (strcmp(type, "r") == 0)
        {
            close(pipe_fd[WRITE_END]);
            stream->fd = pipe_fd[READ_END];
        }
        else if (strcmp(type, "w") == 0)
        {
            close(pipe_fd[READ_END]);
            stream->fd = pipe_fd[WRITE_END];
        }
        return stream;
    }

    if (stream != NULL)
        free(stream);

    return NULL;
}

FUNC_DECL_PREFIX int so_pclose(SO_FILE *stream)
{
    int status;
    int ret;

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

    close(so_fileno(stream));

    if (stream->ChildPId > 0)
    {
        ret = waitpid(stream->ChildPId, &status, 0);
        if (ret == (-1))
        {
            free(stream);
            return -1;
        }
    }
    if (stream != NULL)
        free(stream);
    return status;
}
