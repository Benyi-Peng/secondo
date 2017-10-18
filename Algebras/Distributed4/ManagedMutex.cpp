/*
----
This file is part of SECONDO.

Copyright (C) 2017, Faculty of Mathematics and Computer Science, Database
Systems for New Applications.

SECONDO is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

SECONDO is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
SECONDO; if not, write to the Free Software Foundation, Inc., 59 Temple Place,
Suite 330, Boston, MA  02111-1307  USA
----

//paragraph [10] title: [{\Large \bf] [}]
//characters [1] tt: [\texttt{] [}]
//[secondo] [{\sc Secondo}]

[10] Implementation of Class ManagedMutex

2017-08-14: Sebastian J. Bronner $<$sebastian@bronner.name$>$

\tableofcontents

1 Preliminary Setup

*/
#include "ManagedMutex.h"
#include "SecondoSystem.h"

namespace distributed4 {
  using boost::filesystem::directory_iterator;
  using boost::filesystem::filesystem_error;
  using boost::filesystem::path;
  using boost::filesystem::read_symlink;
  using boost::interprocess::named_sharable_mutex;
  using boost::interprocess::open_only;
  using boost::interprocess::open_or_create;
  using std::all_of;
  using std::runtime_error;
  using std::stoi;
  using std::string;

  const directory_iterator dit_end;
  const path map_files{"/proc/self/map_files"};
/*
2 Constructors

2.1 "ManagedMutex(const string\&)"[1]

This constructor takes the name of an object as a parameter. That name is used
as a basis for the name of the underlying named sharable mutex (and its shared
memory segment). The ManagedMutex is instantiated and left unlocked.

*/
  ManagedMutex::ManagedMutex(const string& name): shm_target{getSHMPath(name)}
  {
    const char* shm_name{shm_target.filename().c_str()};
/*
A race condition exists where it is possible for this process to map the shared
memory segment for the mutex and another process to remove that same shared
memory segment. That happens when the other process checked for mapped
references before the shared memery segment was mapped in this process. This
race condition is mitigated here. After mapping the shared memory segment, the
mutex is locked and a check is performed to see if the mapping is still valid.
If it isn't, the race condition happened, and the mutex is re-mapped.

*/
    directory_iterator it;
    while(it == dit_end) {
      if(mutex)
        delete mutex;
      mutex = new named_sharable_mutex{open_or_create, shm_name, 0600};
      lock(false);
      for(it = directory_iterator{map_files}; it != dit_end; ++it)
        if(read_symlink(*it) == shm_target)
          break;
      unlock();
    }
  }
/*
2.1 "ManagedMutex(const string\&, bool)"[1]

In addition to the name of an object, this constructor also takes a boolean
value, "exclusive"[1]. The ManagedMutex is instantiated and locked for
exclusive ownership when "exclusive"[1] is "true"[1], and for sharable
ownership when "false"[1].

*/
  ManagedMutex::ManagedMutex(const string& name, bool exclusive):
    shm_target{getSHMPath(name)} {
    const string shm_name{shm_target.filename().c_str()};
/*
The race condition mentioned in the first constructor is handled slightly
differently here: (1) The mutex is not unlocked at the end of the loop, and
instead unlocked at the beginning of the loop starting with the second
iteration. (2) The mutex is directly locked for the type of ownership specified
by the parameter "exclusive"[1].

*/
    directory_iterator it;
    while(it == dit_end) {
      if(mutex) {
        unlock();
        delete mutex;
      }
      mutex = new named_sharable_mutex{open_or_create, shm_name.c_str(), 0600};
      lock(exclusive);
      for(it = directory_iterator{map_files}; it != dit_end; ++it)
        if(read_symlink(*it) == shm_target)
          break;
    }
  }
/*
3 Destructor

*/
  ManagedMutex::~ManagedMutex() {
/*
Checking and removing the shared memory segment requires locking the mutex to
avoid most race conditions. If a lock can't be acquired, the mutex is being
used and may not be removed. In that case, no further checking is necessary and
the underlying shared memory segment will not be removed.

*/
    if(!mutex->try_lock()) {
      delete mutex;
      return;
    }
/*
Having acquired a lock on the mutex, all running processes of the process uid
are checked to see if the shared memory segment is mapped to any of them. The
processes checked are limited to the process uid by the kernel's permissions on
"/proc/.../map\_files"[1].

*/
    directory_iterator mit;  // map_files iterator
    for(directory_iterator it{"/proc"}; it != dit_end; ++it) {
      path map_files{*it};
      string filename{map_files.filename().native()};
      if(!all_of(filename.begin(), filename.end(), isdigit))
        continue;
      if(stoi(filename) == getpid())
        continue;
      map_files += "/map_files";
      try {
        for(mit = directory_iterator{map_files}; mit != dit_end; ++mit)
          if(read_symlink(*mit) == shm_target)
            break;
      } catch(filesystem_error&) {}
      if(mit != dit_end)
        break;
    }
/*
The value of "mit"[1] tells whether a mapped reference was found, or not.  When
"mit"[1] is equal to "dit\_end"[1], all directories were searched and the
iterator reached the end of the directory. In that case, no mapped reference
was found. When "mit"[1] has some other value, it points to the directory
entry of a mapped reference, thus allowing us to conclude that such a mapped
reference exists. In the absence of a mapped reference, the shared memory
segment for the mutex is removed.

*/
    if(mit == dit_end)
      named_sharable_mutex::remove(shm_target.c_str());
    mutex->unlock();
    delete mutex;
  }
/*
4 Member Methods

4.1 "lock"[1]

Unlike other mutex implementations that offer ~seperate methods~ for exclusive
and sharable locking, this "lock"[1] method takes an ~argument~ indicating the
desired lock type: "true"[1] for an exclusive lock or "false"[1] for a sharable
lock. If a process tries to lock a mutex more than once, a "runtime\_error"[1]
exception is thrown.

*/
  void ManagedMutex::lock(bool exclusive) {
    if(owned)
      throw runtime_error("Attempted to lock a mutex that is already owned.");

    if(exclusive) {
      if(!mutex->try_lock()) {
        cmsg.info() << "The mutex at " << shm_target << " is currently "
          "locked. Waiting for exclusive ownership." << endl;
        cmsg.send();
        mutex->lock();
      }
    } else {
      if(!mutex->try_lock_sharable()) {
        cmsg.info() << "The mutex at " << shm_target << " is currently "
          "locked. Waiting for sharable ownership." << endl;
        cmsg.send();
        mutex->lock_sharable();
      }
    }

    this->exclusive = exclusive;
    owned = true;
  }
/*
4.2 "unlock"[1]

The state of the lock is tracked. Therefore, it is not necessary to distinguish
between unlocking a sharable lock and an exclusive one when calling
"unlock()"[1]. There is just this one method to unlock both lock types. If a
process tries to unlock a mutex that is not locked, a "runtime\_error"[1]
exception is thrown.

*/
  void ManagedMutex::unlock() {
    if(!owned)
      throw runtime_error("Attempted to unlock an unowned mutex.");

    if(exclusive)
      mutex->unlock();
    else
      mutex->unlock_sharable();

    owned = false;
  }
/*
4.3 "static unlock"[1]

This static member function is for the situation where the current process does
not already have an instance of ManagedMutex for the named object, the
corresponding mutex is known to be locked, and the process is responsible for
unlocking it (i.e. when the user calls "query unlock(objname)"[1].

*/
  void ManagedMutex::unlock(const string& name) {
    const path shm_target{getSHMPath(name)};
/*
Make sure that the current process does not hold a reference to the
corresponding shared memory segment.

*/
    for(directory_iterator it{map_files}; it != dit_end; ++it)
      if(read_symlink(*it) == shm_target)
        throw runtime_error("Illegal use of ManagedMutex::unlock while "
            "holding a reference to \"" + string{shm_target.c_str()} + "\".");
/*
Map the corresponding "named\_sharable\_mutex"[1], throwing an
"interprocess\_exception"[1] if it doesn't exist.

*/
    named_sharable_mutex mutex{open_only, shm_target.filename().c_str()};
/*
Make sure that the mutex is in fact already locked.

*/
    if(mutex.try_lock())
    {
      mutex.unlock();
      throw runtime_error("Attempted to use ManagedMutex::unlock to unlock \""
          + string{shm_target.c_str()} + "\", which wasn't locked.");
    }
/*
Figure out if it is locked for exclusive or sharable ownership and unlock it
accordingly.

*/
    if(mutex.try_lock_sharable())
    {
      mutex.unlock_sharable();
      mutex.unlock_sharable();
    } else {
      mutex.unlock();
    }
  }
/*
4.4 "getSHMPath"[1]

This member function is for internal use only. Its primary purpose is to allow
assigning a value to shm\_target in the initializer list of the constructor.

*/
  string ManagedMutex::getSHMPath(const string& name) {
    string dbname{SecondoSystem::GetInstance()->GetDatabaseName()};
    string dbdir{SmiEnvironment::GetSecondoHome()};
    if(dbdir[0] == '/')
      dbdir.erase(0, 1);
    replace(dbdir.begin(), dbdir.end(), '/', '_');
    return "/dev/shm/secondo:" + dbdir + "_" + dbname + ":" + name;
  }
}