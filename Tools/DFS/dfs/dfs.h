/*
----
This file is part of SECONDO.
Realizing a simple distributed filesystem for master thesis of stephan scheide

Copyright (C) 2015,
Faculty of Mathematics and Computer Science,
Database Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

SECONDO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SECONDO; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
----


//[$][\$]

*/

#ifndef DFS_H
#define DFS_H

#include "../define.h"
#include <exception>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>

namespace dfs {

#define FILEBUFFER const char*
#define FILEPATH const char*
#define CATEGORY const char*
#define FILEID FILEPATH
#define FEATURE const char*
#define NUMBER unsigned long

/*
1 Class ~SimpleFilesystem~

describes a general interface to a file system

*/

class SimpleFilesystem {
  public:
    /**
     * creates a new file on filesystem
     * @param fileId - the id of the file
     * @param content - the content of the file
     * @param length - the size of content
     * @param category - an optional category
     */
    virtual void storeFile(FILEID fileId, FILEBUFFER content, long length,
                           CATEGORY category = 0) = 0;

    /**
     * deletes a file from filesystem
     */
    virtual void deleteFile(FILEID fileId) = 0;

    /**
     * copies a file from filesystem to new file identified by local path
     * @param fileId
     * @param localPath
     */
    virtual void receiveFileToLocal(FILEID fileId, FILEPATH localPath) = 0;

    /**
     * receives a part of the content of the file and stores it to given buffer
     * @param fileId - the id of the file
     * @param startIndex - the start index of the content
     * @param length - the length (amount of bytes) to read
     * @param targetBuffer - the target buffer to store the data to
     * @param targetBufferStartIndex - the position first byte will be
     * written in target buffer, typically zero
     */
    virtual void
    receiveFilePartially(FILEID fileId, NUMBER startIndex, NUMBER length,
                         char *targetBuffer,
                         NUMBER targetBufferStartIndex = 0) = 0;

    /**
 * appends content to given file
 * if file not exists new one is created without any category
 * @param fileId
 * @param appendix
 * @param length
 */
    virtual void
    appendToFile(FILEID fileId, FILEBUFFER appendix, long length) = 0;

    /**
     * returns the next position of the file where is appended to
     * usually this is currentfilesize since indices are zerobased
     * @param fileId
     * @return
     */
    virtual UI64 nextWritePosition(FILEID fileId) = 0;

    /**
     * returns TRUE if the fileid by given id exists in the file system
     * @param fileId
     * @return
     */
    virtual bool hasFile(FILEID fileId) = 0;

    virtual ~SimpleFilesystem() {}
  };

  /**
   * class helping reading from filesystem stream-like
   */
  class SimpleFilesystemReader {
  public:
    SimpleFilesystemReader();

    ~SimpleFilesystemReader();

    /**
     * tells the reader to operate on file on given DFS
     * @param pNewDfs
     * @param fileId
     */
    void useFile(SimpleFilesystem *pNewDfs, FILEID fileId);

    /**
     * changes internal position
     * upcoming operations starts reading from this position
     * @param newPosition
     */
    void changePosition(UI64 newPosition);

    /**
     * reads data to buffer beginning from current position
     * @param length
     * @param targetBuffer
     */
    void receiveBuffer(char *targetBuffer, NUMBER length);

    /**
     * tells the current position, the position where the next byte would
     * be read from
     * @return
     */
    UI64 getCurrentPosition();

  private:


    SimpleFilesystem *pDfs;
    char *currentFileId;
    UI64 curPosition;
  };


  /**
   * extended the simple filesystem to a command file system with more
   * operations
   */
  class Filesystem : public SimpleFilesystem {
  public:


    /**
     * deletes all files from filesystem
     */
    virtual void deleteAllFiles() = 0;

    /**
     * deletes all file belonging to given category
     * @param c
     */
    virtual UI64 deleteAllFilesOfCategory(CATEGORY c) = 0;

    /**
     * returns amount of files present in filesystem
     * @return
     */
    virtual int countFiles() = 0;

    /**
     * returns sum of all file sizes within filesystem
     * @return
     */
    virtual UI64 totalSize() = 0;

    /**
     * returns the size of a given file
     * if file is not present, 0 is returned
     * @param fileId
     * @return
     */
    virtual UI64 fileSize(FILEID fileId) = 0;

    /**
     * creates a new file on filesystem sourcing the local file
     * @param fileId - the id of the file
     * @param localPath - the path to local file
     * @param category - an optional category
     */
    virtual void storeFileFromLocal(FILEID fileId, FILEPATH localPath,
                                    CATEGORY category = 0) = 0;

    /**
     * indicates whether filesystem provides a given feature
     * @param name
     * @return
     */
    virtual bool hasFeature(FEATURE name) = 0;

    /**
     * returns the ids of the files stored on file system
     * @return
     */
    virtual std::vector<std::string> listFileNames() = 0;

    /**
     * changes a setting of the DFS
     * @param key
     * @param value
     */
    virtual void changeSetting(const char *key, const char *newValue) = 0;

    /**
     * changes chunksize of DFS
     * only affects new files
     * @param newValue
     */
    virtual void changeChunkSize(int newValue) = 0;

    virtual ~Filesystem() {}
  };

  class BaseException : public std::exception {
  public:
    char message[128];

    BaseException() {
      memset(message, 0, 128);
      strncpy(message, "common error occurred", 127);
    }

    BaseException(const char *msg) {
      memset(message, 0, 128);
      strncpy(message, msg, 127);
    }

    virtual const char *what() {
      return message;
    }
  };

  class ResultException : public BaseException {
  public:
    ResultException(const char *msg) : BaseException(msg) {
    }
  };

  class CallerException : public BaseException {
  public:
    virtual const char *what() {
      return "user called interface in a wrong way";
    }
  };

};

#endif
