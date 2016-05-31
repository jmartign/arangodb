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
////////////////////////////////////////////////////////////////////////////////

#include "VocbaseContext.h"

#include <chrono>

#include <velocypack/Builder.h>
#include <velocypack/Exception.h>
#include <velocypack/Parser.h>
#include <velocypack/velocypack-aliases.h>

#include "Basics/MutexLocker.h"
#include "Basics/tri-strings.h"
#include "Cluster/ServerState.h"
#include "Endpoint/ConnectionInfo.h"
#include "Logger/Logger.h"
#include "Ssl/SslInterface.h"
#include "VocBase/auth.h"
#include "VocBase/server.h"
#include "VocBase/vocbase.h"

using namespace arangodb;
using namespace arangodb::basics;
using namespace arangodb::rest;

////////////////////////////////////////////////////////////////////////////////
/// @brief sid lock
////////////////////////////////////////////////////////////////////////////////

static arangodb::Mutex SidLock;

////////////////////////////////////////////////////////////////////////////////
/// @brief sid cache
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
// turn off warnings about too long type name for debug symbols blabla in MSVC
// only...
#pragma warning(disable : 4503)
#endif

typedef std::unordered_map<std::string, std::pair<std::string, double>>
    DatabaseSessionsType;

static std::unordered_map<std::string, DatabaseSessionsType> SidCache;

////////////////////////////////////////////////////////////////////////////////
/// @brief time-to-live for aardvark server sessions
////////////////////////////////////////////////////////////////////////////////

double VocbaseContext::ServerSessionTtl =
    60.0 * 60.0 * 2;  // 2 hours session timeout

////////////////////////////////////////////////////////////////////////////////
/// @brief defines a sid
////////////////////////////////////////////////////////////////////////////////

void VocbaseContext::createSid(std::string const& database,
                               std::string const& sid,
                               std::string const& username) {
  MUTEX_LOCKER(mutexLocker, SidLock);

  // find entries for database first
  auto it = SidCache.find(database);

  if (it == SidCache.end()) {
    it = SidCache.emplace(database, DatabaseSessionsType()).first;
  }

  // now insert a database-specific sid
  double const now = TRI_microtime() * 1000.0;
  (*it).second.emplace(sid, std::make_pair(username, now));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clears all sid entries for a database
////////////////////////////////////////////////////////////////////////////////

void VocbaseContext::clearSid(std::string const& database) {
  MUTEX_LOCKER(mutexLocker, SidLock);

  SidCache.erase(database);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clears a sid
////////////////////////////////////////////////////////////////////////////////

void VocbaseContext::clearSid(std::string const& database,
                              std::string const& sid) {
  MUTEX_LOCKER(mutexLocker, SidLock);

  auto it = SidCache.find(database);

  if (it == SidCache.end()) {
    // database not found. no need to go on
    return;
  }

  (*it).second.erase(sid);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief gets the last access time
////////////////////////////////////////////////////////////////////////////////

double VocbaseContext::accessSid(std::string const& database,
                                 std::string const& sid) {
  MUTEX_LOCKER(mutexLocker, SidLock);

  auto it = SidCache.find(database);

  if (it == SidCache.end()) {
    // database not found. no need to go on
    return 0.0;
  }

  auto const& sids = (*it).second;
  auto it2 = sids.find(sid);

  if (it2 == sids.end()) {
    return 0.0;
  }

  return (*it2).second.second;
}

VocbaseContext::VocbaseContext(HttpRequest* request, TRI_server_t* server,
                               TRI_vocbase_t* vocbase, std::string const& jwtSecret)
    : RequestContext(request), _server(server), _vocbase(vocbase), _jwtSecret(jwtSecret) {
  TRI_ASSERT(_server != nullptr);
  TRI_ASSERT(_vocbase != nullptr);
}

VocbaseContext::~VocbaseContext() { TRI_ReleaseVocBase(_vocbase); }

////////////////////////////////////////////////////////////////////////////////
/// @brief whether or not to use special cluster authentication
////////////////////////////////////////////////////////////////////////////////

bool VocbaseContext::useClusterAuthentication() const {
  auto role = ServerState::instance()->getRole();

  if (ServerState::instance()->isDBServer(role)) {
    return true;
  }

  if (ServerState::instance()->isCoordinator(role)) {
    std::string const& s = _request->requestPath();

    if (s == "/_api/shard-comm" || s == "/_admin/shutdown") {
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief return authentication realm
////////////////////////////////////////////////////////////////////////////////

std::string VocbaseContext::realm() const {
  if (_vocbase == nullptr) {
    return std::string("");
  }

  return _vocbase->_name;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief checks the authentication
////////////////////////////////////////////////////////////////////////////////

GeneralResponse::ResponseCode VocbaseContext::authenticate() {
  TRI_ASSERT(_vocbase != nullptr);

  if (!_vocbase->_settings.requireAuthentication) {
    // no authentication required at all
    return GeneralResponse::ResponseCode::OK;
  }

#ifdef ARANGODB_HAVE_DOMAIN_SOCKETS
  // check if we need to run authentication for this type of
  // endpoint
  ConnectionInfo const& ci = _request->connectionInfo();

  if (ci.endpointType == Endpoint::DomainType::UNIX &&
      !_vocbase->_settings.requireAuthenticationUnixSockets) {
    // no authentication required for unix socket domain connections
    return GeneralResponse::ResponseCode::OK;
  }
#endif

  std::string const& path = _request->requestPath();

  if (_vocbase->_settings.authenticateSystemOnly) {
    // authentication required, but only for /_api, /_admin etc.

    if (!path.empty()) {
      // check if path starts with /_
      if (path[0] != '/') {
        return GeneralResponse::ResponseCode::OK;
      }

      if (path[0] != '\0' && path[1] != '_') {
        return GeneralResponse::ResponseCode::OK;
      }
    }
  }

  if (StringUtils::isPrefix(path, "/_open/") ||
      StringUtils::isPrefix(path, "/_admin/aardvark/") || path == "/") {
    return GeneralResponse::ResponseCode::OK;
  }

  // .............................................................................
  // authentication required
  // .............................................................................

  bool found;
  char cn[4096];

  cn[0] = '\0';
  strncat(cn, "arango_sid_", 11);
  strncat(cn + 11, _vocbase->_name, sizeof(cn) - 12);

  // extract the sid
  std::string const& sid = _request->cookieValue(cn, found);

  if (found) {
    MUTEX_LOCKER(mutexLocker, SidLock);

    auto it = SidCache.find(_vocbase->_name);

    if (it != SidCache.end()) {
      auto& sids = (*it).second;
      auto it2 = sids.find(sid);

      if (it2 != sids.end()) {
        _request->setUser((*it2).second.first);
        double const now = TRI_microtime() * 1000.0;
        // fetch last access date of session
        double const lastAccess = (*it2).second.second;

        // check if session has expired
        if (lastAccess + (ServerSessionTtl * 1000.0) < now) {
          // session has expired
          sids.erase(sid);
          return GeneralResponse::ResponseCode::UNAUTHORIZED;
        }

        (*it2).second.second = now;
        return GeneralResponse::ResponseCode::OK;
      }
    }

    // no cookie found. fall-through to regular HTTP authentication
  }

  std::string const& authStr = _request->header(StaticStrings::Authorization, found);
  
  if (!found) {
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }

  size_t methodPos = authStr.find_first_of(' ');
  if (methodPos == std::string::npos) {
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }
  
  // skip over "basic "
  char const* auth = authStr.c_str() + methodPos;
  while (*auth == ' ') {
    ++auth;
  }

  LOG(DEBUG) << "Authorization header: " << authStr;

  if (TRI_CaseEqualString(authStr.c_str(), "basic ", 6)) {
    return basicAuthentication(auth);
  } else if (TRI_CaseEqualString(authStr.c_str(), "bearer ", 7)) {
    return jwtAuthentication(std::string(auth));
  } else {
    // mop: hmmm is 403 the correct status code? or 401? or 400? :S
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief checks the authentication via basic
////////////////////////////////////////////////////////////////////////////////

GeneralResponse::ResponseCode VocbaseContext::basicAuthentication(const char* auth) {
  if (useClusterAuthentication()) {
    std::string const expected = ServerState::instance()->getAuthentication();

    if (expected.substr(6) != std::string(auth)) {
      return GeneralResponse::ResponseCode::UNAUTHORIZED;
    }

    std::string const up = StringUtils::decodeBase64(auth);
    std::string::size_type n = up.find(':', 0);

    if (n == std::string::npos || n == 0 || n + 1 > up.size()) {
      LOG(TRACE) << "invalid authentication data found, cannot extract "
                    "username/password";

      return GeneralResponse::ResponseCode::BAD;
    }

    _request->setUser(up.substr(0, n));

    return GeneralResponse::ResponseCode::OK;
  }

  // look up the info in the cache first
  bool mustChange;
  std::string username = TRI_CheckCacheAuthInfo(_vocbase, auth, &mustChange);

  if (username.empty()) {
    // no entry found in cache, decode the basic auth info and look it up
    std::string const up = StringUtils::decodeBase64(auth);
    std::string::size_type n = up.find(':', 0);

    if (n == std::string::npos || n == 0 || n + 1 > up.size()) {
      LOG(TRACE) << "invalid authentication data found, cannot extract "
                    "username/password";
      return GeneralResponse::ResponseCode::BAD;
    }

    username = up.substr(0, n);

    LOG(TRACE) << "checking authentication for user '" << username << "'";
    bool res =
        TRI_CheckAuthenticationAuthInfo(_vocbase, auth, username.c_str(),
                                        up.substr(n + 1).c_str(), &mustChange);

    if (!res) {
      return GeneralResponse::ResponseCode::UNAUTHORIZED;
    }
  }

  _request->setUser(std::move(username));

  if (mustChange) {
    if ((_request->requestType() == GeneralRequest::RequestType::PUT ||
         _request->requestType() == GeneralRequest::RequestType::PATCH) &&
        StringUtils::isPrefix(_request->requestPath(), "/_api/user/")) {
      return GeneralResponse::ResponseCode::OK;
    }

    return GeneralResponse::ResponseCode::FORBIDDEN;
  }

  return GeneralResponse::ResponseCode::OK;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief checks the authentication via jwt
////////////////////////////////////////////////////////////////////////////////

GeneralResponse::ResponseCode VocbaseContext::jwtAuthentication(std::string const& auth) {
  std::vector<std::string> const parts = StringUtils::split(auth, '.');
  
  if (parts.size() != 3) {
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }

  std::string const& header = parts[0];
  std::string const& body = parts[1];
  std::string const& signature = parts[2];
  
  std::string const message = header + "." + body;

  if (!validateJwtHeader(header)) {
    LOG(DEBUG) << "Couldn't validate jwt header " << header;
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }
  
  if (!validateJwtBody(body)) {
    LOG(DEBUG) << "Couldn't validate jwt body " << body;
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }
  
  if (!validateJwtHMAC256Signature(message, signature)) {
    LOG(DEBUG) << "Couldn't validate jwt signature " << signature;
    return GeneralResponse::ResponseCode::UNAUTHORIZED;
  }

  return GeneralResponse::ResponseCode::OK;
}

std::shared_ptr<VPackBuilder> VocbaseContext::parseJson(std::string const& str, std::string const& hint) {
  std::shared_ptr<VPackBuilder> result;
  VPackParser parser;
  try {
    parser.parse(str);
    result = parser.steal();
  } catch (std::bad_alloc const&) {
    LOG(ERR) << "Out of memory parsing " << hint << "!";
  } catch (VPackException const& ex) {
    LOG(DEBUG) << "Couldn't parse " << hint << ": " << ex.what();
  } catch (...) {
    LOG(ERR) << "Got unknown exception trying to parse " << hint;
  }
  
  return result;
}

bool VocbaseContext::validateJwtHeader(std::string const& header) {
  std::shared_ptr<VPackBuilder> headerBuilder = parseJson(StringUtils::decodeBase64(header), "jwt header");
  if (headerBuilder.get() == nullptr) {
    return false;
  }

  VPackSlice const headerSlice = headerBuilder->slice();
  if (!headerSlice.isObject()) {
    return false;
  }

  VPackSlice const algSlice = headerSlice.get("alg");
  VPackSlice const typSlice = headerSlice.get("typ");

  if (!algSlice.isString()) {
    return false;
  }
  
  if (!typSlice.isString()) {
    return false;
  }
  
  if (algSlice.copyString() != "HS256") {
    return false;
  }
  
  std::string typ = typSlice.copyString();
  std::transform(typ.begin(), typ.end(), typ.begin(), ::tolower);
  if (typ != "jwt") {
    return false;
  }

  return true;
}

bool VocbaseContext::validateJwtBody(std::string const& body) {
  std::shared_ptr<VPackBuilder> bodyBuilder = parseJson(StringUtils::decodeBase64(body), "jwt body");
  if (bodyBuilder.get() == nullptr) {
    return false;
  }

  VPackSlice const bodySlice = bodyBuilder->slice();
  if (!bodySlice.isObject()) {
    return false;
  }

  VPackSlice const issSlice = bodySlice.get("iss");
  if (!issSlice.isString()) {
    return false;
  }
  
  if (issSlice.copyString() != "arangodb") {
    return false;
  }
  
  // mop: optional exp (cluster currently uses non expiring jwts)
  if (bodySlice.hasKey("exp")) {
    VPackSlice const expSlice = bodySlice.get("exp");
    
    if (!expSlice.isNumber()) {
      return false;
    }
    
    std::chrono::system_clock::time_point expires(std::chrono::seconds(expSlice.getNumber<uint64_t>()));
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

    if (now >= expires) {
      return false;
    }
  }
  return true;
}

bool VocbaseContext::validateJwtHMAC256Signature(std::string const& message, std::string const& signature) {
  std::string decodedSignature = StringUtils::decodeBase64(signature);
  return verifyHMAC(_jwtSecret.c_str(), _jwtSecret.length(), message.c_str(), message.length(), decodedSignature.c_str(), decodedSignature.length(), SslInterface::Algorithm::ALGORITHM_SHA256);
}
