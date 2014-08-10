#pragma once

#include <Exceptions.h>
#include <tomcrypt.h>

#undef max

namespace Balau {

template<const struct ltc_hash_descriptor * desc>
class Hash {
  public:
      Hash() { reset(); }
    void reset() {
        int r = desc->init(&m_state);
        IAssert(r == CRYPT_OK, "init for %s returned %i", desc->name, r);
    }
    void update(const uint8_t * data, size_t len) {
        while (len) {
            unsigned long blockLen = std::numeric_limits<unsigned long>::max();
            if (blockLen > len)
                blockLen = (unsigned long) len;
            int r = desc->process(&m_state, data, blockLen);
            data += blockLen;
            len -= blockLen;
            IAssert(r == CRYPT_OK, "process for %s returned %i", desc->name, r);
        }
    }
    unsigned final(void * digest, unsigned outlen) {
        AAssert(outlen >= digestSize(), "digest size too small being passed on for %s: %u instead of %u", name(), outlen, digestSize());
        int r = desc->done(&m_state, (uint8_t *) digest);
        IAssert(r == CRYPT_OK, "done for %s returned %i", desc->name, r);
        return digestSize();
    }

    void update(const String & data) { update((uint8_t *) data.to_charp(), data.strlen()); }
    template<size_t L>
    void update(const char (&str)[L]) { update((uint8_t *) str, L - 1); }
    template<size_t L>
    unsigned final(uint8_t (&digest)[L]) { return final(digest, L); }

    static const unsigned digestSize() { return desc->hashsize; }
    static const unsigned blockSize() { return desc->blocksize; }
    static const char * name() { return desc->name; }
    static char hashID() { return desc->ID; }

  private:
    hash_state m_state;
};

template<class Hash>
class HMAC {
  public:
    void prepare(const uint8_t * key, const size_t len) {
        int r = hmac_init(&m_state, find_hash_id(Hash::hashID()), key, len);
        IAssert(r == CRYPT_OK, "hmac_init for %s returned %i", Hash::name(), r);
    }
    void update(const uint8_t * data, const size_t len) {
        int r = hmac_process(&m_state, data, len);
        IAssert(r == CRYPT_OK, "hmac_process for %s returned %i", Hash::name(), r);
    }
    unsigned long final(void * digest, unsigned long outlen) {
        int r = hmac_done(&m_state, (uint8_t *) digest, &outlen);
        IAssert(r == CRYPT_OK, "hmac_done for %s returned %i", Hash::name(), r);
        return outlen;
    }

    void prepare(const String & data) { prepare((uint8_t *)data.to_charp(), data.strlen()); }
    template<size_t L>
    void prepare(const char (&str)[L]) { prepare((uint8_t *) str, L - 1); }
    void update(const String & data) { update((uint8_t *) data.to_charp(), data.strlen()); }
    template<size_t L>
    void update(const char (&str)[L]) { update((uint8_t *) str, L - 1); }
    template<size_t L>
    unsigned final(uint8_t(&digest)[L]) { return final(digest, L); }

    static const unsigned digestSize() { return Hash::digestSize(); }
    static const unsigned blockSize() { return Hash::blockSize(); }

  private:
    hmac_state m_state;
};

typedef Hash<&chc_desc>       CHC;
typedef Hash<&whirlpool_desc> Whirlpool;
typedef Hash<&sha512_desc>    SHA512;
typedef Hash<&sha384_desc>    SHA384;
typedef Hash<&sha256_desc>    SHA256;
typedef Hash<&sha224_desc>    SHA224;
typedef Hash<&sha1_desc>      SHA1;
typedef Hash<&md5_desc>       MD5;
typedef Hash<&md4_desc>       MD4;
typedef Hash<&md2_desc>       MD2;
typedef Hash<&tiger_desc>     Tiger;
typedef Hash<&rmd128_desc>    RMD128;
typedef Hash<&rmd160_desc>    RMD160;
typedef Hash<&rmd256_desc>    RMD256;
typedef Hash<&rmd320_desc>    RMD320;

};
