//  gcc -Wall -O3 -shared -fPIC -DLITTLE_ENDIAN -DLTM_DESC -DLTC_SOURCE -DUSE_LTM -I/usr/include/tomcrypt -I/usr/include/tommath -lz -lutil -ltomcrypt -ltommath lcrypt.c -o /usr/lib64/lua/5.1/lcrypt.so
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <zlib.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "tomcrypt.h"

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#define ADD_FUNCTION(L,name) { lua_pushstring(L, #name); lua_pushcfunction(L, lcrypt_ ## name); lua_settable(L, -3); }
#define ADD_CONSTANT(L,name) { lua_pushstring(L, #name); lua_pushinteger(L, name); lua_settable(L, -3); }

static void lcrypt_error(lua_State *L, int err, void *tofree)
{
  if(unlikely(err != CRYPT_OK))
  {
    if(tofree != NULL) free(tofree);
    lua_pushstring(L, error_to_string(err));
    (void)lua_error(L);
  }
}

static void* lcrypt_malloc(lua_State *L, size_t size)
{
  void *ret = malloc(size);
  if(unlikely(ret == NULL))
  {
    lua_pushstring(L, "Out of memory");
    (void)lua_error(L);
  }
  memset(ret, 0, size);
  return ret;
}

#include "lcrypt_ciphers.c"
#include "lcrypt_hashes.c"
#include "lcrypt_math.c"
#include "lcrypt_bits.c"
#include "lcrypt_rsa.c"

static int lcrypt_tohex(lua_State *L)
{
  const char digits[] = "0123456789ABCDEF";
  size_t in_length = 0, spacer_length = 0, prepend_length = 0;
  const unsigned char *in;
  const char *spacer, *prepend;
  int i, j, pos = 0;
  if(unlikely(lua_isnil(L, 1))) { lua_pushlstring(L, "", 0); return 1; }
  in = (const unsigned char*)luaL_checklstring(L, 1, &in_length);
  if(unlikely(in_length == 0)) { lua_pushlstring(L, "", 0); return 1; }
  spacer = luaL_optlstring(L, 2, "", &spacer_length);
  prepend = luaL_optlstring(L, 3, "", &prepend_length);
  char *result = lcrypt_malloc(L, prepend_length + in_length * 2 + (in_length - 1) * spacer_length);
  for(j = 0; j < (int)prepend_length; j++) result[pos++] = prepend[j];
  result[pos++] = digits[(*in >> 4) & 0x0f];
  result[pos++] = digits[*in++ & 0x0f];
  for(i = 1; i < (int)in_length; i++)
  {
    for(j = 0; j < (int)spacer_length; j++) result[pos++] = spacer[j];
    result[pos++] = digits[(*in >> 4) & 0x0f];
    result[pos++] = digits[*in++ & 0x0f];
  }
  lua_pushlstring(L, result, pos);
  free(result);
  return 1;
}

static int lcrypt_fromhex(lua_State *L)
{
  size_t in_length;
  const unsigned char *in = (const unsigned char*)luaL_checklstring(L, 1, &in_length);
  unsigned char result[in_length];
  int i, d = -1, e = -1, pos = 0;
  for(i = 0; i < (int)in_length; i++)
  {
    if(d == -1)
    {
      if(*in >= '0' && *in <= '9')
        d = *in - '0';
      else if(*in >= 'A' && *in <= 'F')
        d = *in - 'A' + 10;
      else if(*in >= 'a' && *in <= 'f')
        d = *in - 'a' + 10;
    }
    else if(e == -1)
    {
      if(*in >= '0' && *in <= '9')
        e = *in - '0';
      else if(*in >= 'A' && *in <= 'F')
        e = *in - 'A' + 10;
      else if(*in >= 'a' && *in <= 'f')
        e = *in - 'a' + 10;
    }
    if(d >= 0 && e >= 0)
    {
      result[pos++] = d << 4 | e;
      d = e = -1;
    }
    in++;
  }
  lua_pushlstring(L, (char*)result, pos);
  return 1;
}

static int lcrypt_compress(lua_State *L)
{
  int err;
  size_t inlen;
  const unsigned char *in = (const unsigned char*)luaL_checklstring(L, 1, &inlen);
  uLongf outlen = compressBound(inlen);
  unsigned char *out = lcrypt_malloc(L, outlen);
  if(unlikely((err = compress(out, &outlen, in, inlen)) != Z_OK))
  {
    free(out);
    lua_pushstring(L, zError(err));
    return lua_error(L);
  }
  lua_pushlstring(L, (char*)out, outlen);
  free(out);
  return 1;
}

static int lcrypt_uncompress(lua_State *L)
{
  int i, err;
  size_t inlen;
  const unsigned char *in = (const unsigned char*)luaL_checklstring(L, 1, &inlen);
  uLongf outlen = inlen << 1;
  unsigned char *out = NULL;
  for(i = 2; i < 16; i++)
  {
    out = lcrypt_malloc(L, outlen);
    if(likely((err = uncompress(out, &outlen, in, inlen)) == Z_OK)) break;
    if(unlikely(err != Z_BUF_ERROR))
    {
      free(out);
      lua_pushstring(L, zError(err));
      return lua_error(L);
    }
    free(out);
    outlen = inlen << i;
  }
  if(unlikely(err == Z_BUF_ERROR))
  {
    free(out);
    lua_pushstring(L, zError(err));
    return lua_error(L);
  }
  lua_pushlstring(L, (char*)out, outlen);
  free(out);
  return 1;
}

static int lcrypt_base64_encode(lua_State *L)
{
  size_t inlen;
  const unsigned char *in = (const unsigned char*)luaL_checklstring(L, 1, &inlen);
  unsigned long outlen = (inlen + 3) * 4 / 3;
  unsigned char *out = malloc(outlen);
  if(out == NULL) return 0;
  lcrypt_error(L, base64_encode(in, inlen, out, &outlen), out);
  lua_pushlstring(L, (char*)out, outlen);
  free(out);
  return 1;
}

static int lcrypt_base64_decode(lua_State *L)
{
  size_t inlen;
  const unsigned char *in = (const unsigned char*)luaL_checklstring(L, 1, &inlen);
  unsigned long outlen = inlen * 3 / 4;
  unsigned char *out = malloc(outlen);
  if(out == NULL) return 0;
  lcrypt_error(L, base64_decode(in, inlen, out, &outlen), out);
  lua_pushlstring(L, (char*)out, outlen);
  free(out);
  return 1;
}

static int lcrypt_xor(lua_State *L)
{
  int i;
  size_t a_length, b_length;
  const unsigned char *a = (const unsigned char*)luaL_checklstring(L, 1, &a_length);
  const unsigned char *b = (const unsigned char*)luaL_checklstring(L, 2, &b_length);
  unsigned char *c = NULL;
  if(a_length > b_length)
  {
    size_t temp = a_length;
    a_length = b_length;
    b_length = temp;
    c = (void*)a; a = b; b = c;
  }
  c = lcrypt_malloc(L, b_length);
  for(i = 0; i < a_length; i++) c[i] = a[i] ^ b[i];
  for(; i < b_length; i++) c[i] = b[i];
  lua_pushlstring(L, (char*)c, b_length);
  free(c);
  return 1;
}

static int lcrypt_time(lua_State *L)
{
  double ret;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ret = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
  lua_pushnumber(L, ret);
  return(1);
}

static int lcrypt_random(lua_State *L)
{
  int len = luaL_checkint(L, 1);
  char *buffer = lcrypt_malloc(L, len);
  #ifdef _WIN32
    HMODULE hLib = LoadLibrary("ADVAPI32.DLL");
    if (unlikely(!hLib))
    {
      lua_pushstring(L, "Unable to open ADVAPI32.DLL");
      (void)lua_error(L);
    }
    BOOLEAN (APIENTRY *pfn)(void *, ULONG) =
      (BOOLEAN (APIENTRY *)(void *, ULONG)) GetProcAddress(hLib, "SystemFunction036");
    if (unlikely(!pfn))
    {
      lua_pushstring(L, "Unable to open ADVAPI32.DLL");
      (void)lua_error(L);
    }
    ULONG ulCbBuff = len;
    if (unlikely(!pfn(buffer, ulCbBuff)))
    {
      lua_pushstring(L, "Call to SystemFunction036 failed.");
      (void)lua_error(L);
    }
  #else
    FILE *fp;
    if(unlikely((fp = fopen("/dev/urandom", "rb")) == NULL))
    {
      lua_pushstring(L, "Unable to open /dev/urandom.");
      (void)lua_error(L);
    }
    if(unlikely(fread(buffer, len, 1, fp) != 1))
    {
      fclose(fp);
      lua_pushstring(L, "Unable to read /dev/urandom.");
      (void)lua_error(L);
    }
    fclose(fp);
  #endif
  lua_pushlstring(L, buffer, len);
  free(buffer);
  return 1;
}

static const luaL_Reg lcryptlib[] =
{
  {"tohex",         lcrypt_tohex},
  {"fromhex",       lcrypt_fromhex},
  {"compress",      lcrypt_compress},
  {"uncompress",    lcrypt_uncompress},
  {"base64_encode", lcrypt_base64_encode},
  {"base64_decode", lcrypt_base64_decode},
  {"xor",           lcrypt_xor},
  {"time",          lcrypt_time},
  {"random",        lcrypt_random},
  {NULL, NULL}
};

int luaopen_lcrypt(lua_State *L)
{
  luaL_register(L, "lcrypt", lcryptlib);

  lua_getglobal(L, "lcrypt");

  lcrypt_start_ciphers(L);
  lcrypt_start_hashes(L);
  lcrypt_start_math(L);
  lcrypt_start_bits(L);

  lua_pop(L, 1);
  return 1;
}
