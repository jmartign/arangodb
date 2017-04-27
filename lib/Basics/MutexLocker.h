////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Achim Brandt
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGODB_BASICS_MUTEX_LOCKER_H
#define ARANGODB_BASICS_MUTEX_LOCKER_H 1

#include "Basics/Common.h"
#include "Basics/Mutex.h"

/// @brief construct locker with file and line information
///
/// Ones needs to use macros twice to get a unique variable based on the line
/// number.
#define MUTEX_LOCKER_VAR_B(a) MUTEX_LOCKER_VAR_A(a)

#ifdef TRI_SHOW_LOCK_TIME

#define MUTEX_LOCKER(obj, lock) \
  arangodb::basics::MutexLocker obj(&lock, __FILE__, __LINE__)

#else

#define MUTEX_LOCKER(obj, lock) arangodb::basics::MutexLocker obj(&lock)

#endif

#define TRY_MUTEX_LOCKER(obj, lock) arangodb::basics::TryMutexLocker obj(&lock)

namespace arangodb {
namespace basics {

/// @brief mutex locker
///
/// A MutexLocker locks a mutex during its lifetime und unlocks the mutex
/// when it is destroyed.
class MutexLocker {
  MutexLocker(MutexLocker const&) = delete;
  MutexLocker& operator=(MutexLocker const&) = delete;

 public:
/// @brief aquires a lock
///
/// The constructor aquires a lock, the destructor releases the lock.
#ifdef TRI_SHOW_LOCK_TIME

  MutexLocker(Mutex* mutex, char const* file, int line);

#else

  explicit MutexLocker(Mutex* mutex);

#endif


  ~MutexLocker();
  
  bool isLocked() const { return _isLocked; }
  
  /// @brief locks the mutex
  void lock();
  
  /// @brief releases the mutex
  void unlock();

 private:
  /// @brief the mutex
  Mutex* _mutex;
  
  /// @brief whether or not the mutex is locked
  bool _isLocked;

#ifdef TRI_SHOW_LOCK_TIME

  /// @brief file
  char const* _file;

  /// @brief line number
  int _line;

  /// @brief lock time
  double _time;

#endif
};
  

class TryMutexLocker {
    TryMutexLocker(MutexLocker const&) = delete;
    TryMutexLocker& operator=(MutexLocker const&) = delete;
    
  public:
    /// @brief tries to aquire a lock
    ///
    /// The constructor aquires a lock, the destructor releases the lock.
    explicit TryMutexLocker(Mutex* mutex);
    
    ~TryMutexLocker();
    
    bool isLocked() const { return _isLocked; }
    
    /// @brief releases the lock
    void unlock();
    
  private:
    /// @brief the mutex
    Mutex* _mutex;
    
    /// @brief whether or not the mutex is locked
    bool _isLocked;
  };
}
}

#endif
