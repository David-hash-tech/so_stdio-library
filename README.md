# README.md

In this project, the structure `SO_FILE` includes the following variables:
- `fd`: the file descriptor/handler corresponding to the file
- `eof`: a flag indicating the end of the file (1 if reached)
- `maxRead`: the number of characters present in the buffer after a `read`
- `isWrite`: (1) the last operation was a `write`; (0) the last operation was a `read`; (-1) no previous operation (file has just been opened)
- `isError`: (0) no error; (1) an error occurred
- `isUpdate`: (0) file opened in normal mode; (1) file opened in update mode
- `isAppend`: (0) file not opened in append mode; (1) file opened in append mode
- `buffer[BUFFER_SIZE]`: the buffer corresponding to the file
- `bufferCount`: the cursor position where the last read/write operation left off

In this implementation, the buffering method is used as follows:
- When reading a character, there are three situations: (`so_fgetc`)
  - `isWrite`=1, a `fflush` is first done to write the characters to the file, then a `read` is done to fill the buffer with characters, from which the first character is read;
  - `isWrite`=0, reading continues from where it left off; if all characters in the buffer have been read, another `read` is done to refill the buffer with characters;
  - `isWrite`=-1, a `read` is done to fill the buffer with characters, then the character from the buffer is read;
- When writing a character, there are three situations: (`so_fputc`)
  - `isWrite`=-1, the character is put in the buffer;
  - `isWrite`=0, the buffer is invalidated (`stream->bufferCursor`=0), then the character is put in the first position of the buffer;
  - `isWrite`=1, the character is put in the current position of the buffer (`stream->bufferCursor`); if the buffer is full of written characters, an `fflush` is done to the file, after which the buffer is invalidated and the first position is occupied.
- The `fread`/`fwrite` operations for reading/writing to the file use the `fgetc`/`fputc` functions, whose implementations are described above;

The implementations for Windows and Linux are similar. They only differ in the system calls used, as well as the implementation of the `so_popen()` and `so_pclose()` functions.

