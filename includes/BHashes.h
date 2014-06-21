#pragma once

#include <array>
#include <Exceptions.h>
#include <tomcrypt.h>

namespace Balau {

template<const struct ltc_hash_descriptor * desc>
class Hash {
  public:
      Hash() { reset(); }
    void reset() {
        int r = desc->init(&m_state);
        IAssert(r == CRYPT_OK, "init for %s returned %i", desc->name, r);
    }
    void update(const uint8_t * data, const size_t len) {
        int r = desc->process(&m_state, data, len);
        IAssert(r == CRYPT_OK, "process for %s returned %i", desc->name, r);
    }
    void update(const String & data) { update((uint8_t *) data.to_charp(), data.strlen()); }
    template<size_t L>
    void update(const char (&str)[L]) { update((uint8_t *) str, L - 1); }
    template<size_t L>
    unsigned final(uint8_t (&digest)[L]) { return final(digest, L); }
    unsigned final(void * digest, unsigned outlen) {
        AAssert(outlen >= digestSize(), "digest size too small being passed on for %s: %u instead of %u", name(), outlen, digestSize());
        int r = desc->done(&m_state, (uint8_t *)digest);
        IAssert(r == CRYPT_OK, "done for %s returned %i", desc->name, r);
        return digestSize();
    }

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
    void prepare(const String & data) { prepare((uint8_t *) data.to_charp(), data.strlen()); }
    template<size_t L>
    void prepare(const char (&str)[L]) { prepare((uint8_t *) str, L - 1); }
    void update(const uint8_t * data, const size_t len) {
        int r = hmac_process(&m_state, data, len);
        IAssert(r == CRYPT_OK, "hmac_process for %s returned %i", Hash::name(), r);
    }
    void update(const String & data) { update((uint8_t *) data.to_charp(), data.strlen()); }
    template<size_t L>
    void update(const char (&str)[L]) { update((uint8_t *) str, L - 1); }
    template<size_t L>
    unsigned final(uint8_t(&digest)[L]) { return final(digest, L); }
    unsigned final(void * digest, unsigned outlen) {
        int r = hmac_done(&m_state, (uint8_t *) digest, &outlen);
        IAssert(r == CRYPT_OK, "hmac_done for %s returned %i", Hash::name(), r);
        return outlen;
    }

    static const unsigned digestSize() { return Hash::digestSize(); }
    static const unsigned blockSize() { return Hash::blockSize(); }

  private:
    hmac_state m_state;
};

class CHC       : public Hash<&chc_desc> { };
class Whirlpool : public Hash<&whirlpool_desc> { };
class SHA512    : public Hash<&sha512_desc> { };
class SHA384    : public Hash<&sha384_desc> { };
class SHA256    : public Hash<&sha256_desc> { };
class SHA224    : public Hash<&sha224_desc> { };
class SHA1      : public Hash<&sha1_desc> { };
class MD5       : public Hash<&md5_desc> { };
class MD4       : public Hash<&md4_desc> { };
class MD2       : public Hash<&md2_desc> { };
class Tiger     : public Hash<&tiger_desc> { };
class RMD128    : public Hash<&rmd128_desc> { };
class RMD160    : public Hash<&rmd160_desc> { };
class RMD256    : public Hash<&rmd256_desc> { };
class RMD320    : public Hash<&rmd320_desc> { };

};
