#pragma once

#include <Exceptions.h>
#include <tomcrypt.h>

namespace Balau {

template<const struct ltc_prng_descriptor * desc>
class PRNG {
  public:
      PRNG() { start(); }
      ~PRNG() { done(); }
    void reset() {
        done();
        start();
    }
    void setKey(const uint8_t * key, size_t len) {
        int r = desc->add_entropy(key, len, &m_state);
        IAssert(r == CRYPT_OK, "add_entropy for %s returned %i", desc->name, r);
        r = desc->ready(&m_state);
        IAssert(r == CRYPT_OK, "ready for %s returned %i", desc->name, r);
    }
    size_t generate(uint8_t * out, size_t len) {
        size_t actualLen = desc->read(out, len, &m_state);
        IAssert(len == actualLen, "Couldn't get enough bytes for %s", desc->name);
        return actualLen;
    }

    template<size_t L>
    void setKey(const char (&key)[L]) { setKey((const uint8_t *) key, L - 1); }
    void setKey(const String & key) { setKey((const uint8_t *) key.to_charp(), key.strlen()); }
    template<size_t L>
    void generate(const uint8_t (&out)[L]) { generate(out, L); }

    static const char * name() { return desc->name; }

  private:
    void start() {
        int r = desc->start(&m_state);
        IAssert(r == CRYPT_OK, "start for %s returned %i", desc->name, r);
    }
    void done() {
        int r = desc->done(&m_state);
        IAssert(r == CRYPT_OK, "done for %s returned %i", desc->name, r);
    }
    prng_state m_state;
};

template<class PRNG, size_t BlockLen = 64>
class Cypher : public PRNG {
  public:
    void process(uint8_t * buf, size_t len) {
        while (len) {
            if (!m_len) {
                m_ptr = m_buffer;
                m_len = BlockLen;
                generate(m_buffer, BlockLen);
            }

            ssize_t chunkLen = std::min(m_len, len);
            m_len -= chunkLen;
            len -= chunkLen;

            if ((((uintptr_t) m_ptr) & 7) && (((uintptr_t) buf) & 7)) {
                uint64_t * buf64 = (uint64_t *) buf;
                uint64_t * ptr64 = (uint64_t *) m_ptr;
                chunkLen -= 7;
                while (chunkLen > 0) {
                    chunkLen -= 8;
                    *buf64++ ^= *ptr64++;
                }
                m_ptr = (uint8_t *) ptr64;
                buf = (uint8_t *) buf64;
                chunkLen += 7;
            }

            if ((((uintptr_t) m_ptr) & 3) && (((uintptr_t) buf) & 3)) {
                uint64_t * buf32 = (uint32_t *) buf;
                uint64_t * ptr32 = (uint32_t *) m_ptr;
                chunkLen -= 3;
                while (chunkLen > 0) {
                    chunkLen -= 4;
                    *buf32++ ^= *ptr32++;
                }
                m_ptr = (uint8_t *) ptr32;
                buf = (uint8_t *) buf32;
                chunkLen += 3;
            }

            while (chunkLen--)
                *buf++ ^= *m_ptr++;
        }
    }
  private:
    uint8_t m_buffer[BlockLen], * m_ptr = m_buffer;
    size_t m_len = 0;
};

typedef PRNG<&yarrow_desc>   Yarrow;
typedef PRNG<&fortuna_desc>  Fortuna;
typedef PRNG<&rc4_desc>      RC4;
typedef PRNG<&sprng_desc>    SPRNG;
typedef PRNG<&sober128_desc> Sober128;

};
