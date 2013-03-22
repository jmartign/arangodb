////////////////////////////////////////////////////////////////////////////////
/// @brief Read-Write Lock
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2013 triAGENS GmbH, Cologne, Germany
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
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Frank Celler
/// @author Achim Brandt
/// @author Copyright 2010-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_BASICS_READ_WRITE_LOCK_H
#define TRIAGENS_BASICS_READ_WRITE_LOCK_H 1

#include "Basics/Common.h"

#include "BasicsC/locks.h"

#undef TRI_READ_WRITE_LOCK_COUNTER

namespace triagens {
  namespace basics {

// -----------------------------------------------------------------------------
// --SECTION--                                               class ReadWriteLock
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Threading
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief read-write lock
////////////////////////////////////////////////////////////////////////////////

    class ReadWriteLock {
        ReadWriteLock (ReadWriteLock const&);
        ReadWriteLock& operator= (ReadWriteLock const&);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                      constructors and destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Threading
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructs a read-write lock
////////////////////////////////////////////////////////////////////////////////

        ReadWriteLock ();

////////////////////////////////////////////////////////////////////////////////
/// @brief deletes read-write lock
////////////////////////////////////////////////////////////////////////////////

        ~ReadWriteLock ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                    public methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Threading
/// @{
////////////////////////////////////////////////////////////////////////////////

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief check for read locked
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_READ_WRITE_LOCK_COUNTER
        bool isReadLocked () const;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief locks for reading
////////////////////////////////////////////////////////////////////////////////

        void readLock ();

////////////////////////////////////////////////////////////////////////////////
/// @brief check for write locked
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_READ_WRITE_LOCK_COUNTER
        bool isWriteLocked () const;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief locks for writing
////////////////////////////////////////////////////////////////////////////////

        void writeLock ();

////////////////////////////////////////////////////////////////////////////////
/// @brief releases the read-lock or write-lock
////////////////////////////////////////////////////////////////////////////////

        void unlock ();

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                 private variables
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Threading
/// @{
////////////////////////////////////////////////////////////////////////////////

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief read-write lock variable
////////////////////////////////////////////////////////////////////////////////

        TRI_read_write_lock_t _rwlock;

////////////////////////////////////////////////////////////////////////////////
/// @brief write lock marker
////////////////////////////////////////////////////////////////////////////////

        bool _writeLocked;

////////////////////////////////////////////////////////////////////////////////
/// @brief mutex for read-write counter
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_READ_WRITE_LOCK_COUNTER
        TRI_mutex_t _mutex;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief read counter
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_READ_WRITE_LOCK_COUNTER
        int32_t _readLockedCounter;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief write counter
////////////////////////////////////////////////////////////////////////////////

#ifdef TRI_READ_WRITE_LOCK_COUNTER
        int32_t _writeLockedCounter;
#endif
    };
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#endif

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
