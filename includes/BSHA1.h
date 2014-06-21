#pragma once

#include <Exceptions.h>
#include <tomcrypt.h>

namespace Balau {

class SHA1 {
  public:
      SHA1() { reset(); }
    void reset() { sha1_init(&m_state); }
    void update(const uint8_t * data, const size_t len) { sha1_process(&m_state, data, len); }
    void final(uint8_t * digest) { sha1_done(&m_state, digest); }

    enum { DIGEST_SIZE = 20 };

  private:
    hash_state m_state;
};

};
