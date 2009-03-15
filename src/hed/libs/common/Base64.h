// -*- indent-tabs-mode: nil -*-

/* Base64 encoding and decoding, borrowed from Axis2c project.
 * Below is the license which is required by Axis2c.
 */

/*
 *   Copyright 2003-2004 The Apache Software Foundation.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef ARCLIB_BASE64
#define ARCLIB_BASE64

#include <string>

namespace Arc {
  class Base64 {
  public:
    Base64();
    ~Base64();
    static int encode_len(int len);
    static int encode(char *encoded, const char *string, int len);
    static int decode_len(const char *bufcoded);
    static int decode(char *bufplain, const char *bufcoded);
  };
} // namespace Arc

#endif // ARCLIB_BASE64
