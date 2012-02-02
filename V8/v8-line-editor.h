////////////////////////////////////////////////////////////////////////////////
/// @brief V8 line editor
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2012 triagens GmbH, Cologne, Germany
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
/// @author Dr. Frank Celler
/// @author Copyright 2011-2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_V8_V8_LINE_EDITOR_H
#define TRIAGENS_V8_V8_LINE_EDITOR_H

#include "Basics/Common.h"

#include <v8.h>

// -----------------------------------------------------------------------------
// --SECTION--                                                class V8LineEditor
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8Shell
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor
////////////////////////////////////////////////////////////////////////////////

class V8LineEditor {
  public:

////////////////////////////////////////////////////////////////////////////////
/// @brief number of history entries
////////////////////////////////////////////////////////////////////////////////

    static const int MAX_HISTORY_ENTRIES = 1000;

  public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

    V8LineEditor (v8::Handle<v8::Context>, std::string const& history);

  public:

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor open
////////////////////////////////////////////////////////////////////////////////

    bool open ();

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor shutdown
////////////////////////////////////////////////////////////////////////////////

    bool close ();

////////////////////////////////////////////////////////////////////////////////
/// @brief get the history file path
///
/// the path is "$HOME/.avocado" if $HOME is set, else the local file ".avocado"
////////////////////////////////////////////////////////////////////////////////

    std::string getHistoryPath ();

////////////////////////////////////////////////////////////////////////////////
/// @brief line editor prompt
////////////////////////////////////////////////////////////////////////////////

    char* prompt (char const* prompt);

////////////////////////////////////////////////////////////////////////////////
/// @brief add to history
////////////////////////////////////////////////////////////////////////////////

    void addHistory (const char* string);

  private:

////////////////////////////////////////////////////////////////////////////////
/// @brief current text
////////////////////////////////////////////////////////////////////////////////

    std::string _current;

////////////////////////////////////////////////////////////////////////////////
/// @brief history filename
////////////////////////////////////////////////////////////////////////////////

    std::string _historyFilename;

////////////////////////////////////////////////////////////////////////////////
/// @brief context
////////////////////////////////////////////////////////////////////////////////

    v8::Handle<v8::Context> _context;
};

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#endif

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:
