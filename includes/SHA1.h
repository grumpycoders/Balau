#pragma once

#include <Exceptions.h>

namespace Balau {

class SHA1 {
  public:
      SHA1() { reset(); }
    void reset();
    void update(const uint8_t* data, const size_t len);
    void final(uint8_t * digest);

    enum { DIGEST_SIZE = 20 };

  private:
    void transform(uint32_t state[5], const uint8_t buffer[64]);

    uint32_t m_state[5];
    uint32_t m_count[2];
    uint8_t m_buffer[64];
};

};
