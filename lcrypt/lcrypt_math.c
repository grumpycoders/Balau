typedef void* lcrypt_bigint;

static lcrypt_bigint* lcrypt_new_bigint(lua_State *L)
{
  lcrypt_bigint *bi = lua_newuserdata(L, sizeof(lcrypt_bigint));
  luaL_getmetatable(L, "LCRYPT_BIGINT");
  (void)lua_setmetatable(L, -2);
  *bi = NULL;
  lcrypt_error(L, ltc_mp.init(bi), NULL);
  return bi;
}

static int lcrypt_bigint_add(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.add(*bi_a, *bi_b, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_sub(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.sub(*bi_a, *bi_b, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_mul(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.mul(*bi_a, *bi_b, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_div(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.mpdiv(*bi_a, *bi_b, *bi, NULL), NULL);
  return 1;
}

static int lcrypt_bigint_divmod(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_q = lcrypt_new_bigint(L);
  lcrypt_bigint *bi_r = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.mpdiv(*bi_a, *bi_b, *bi_q, *bi_r), NULL);
  return 2;
}

static int lcrypt_bigint_mod(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.mpdiv(*bi_a, *bi_b, NULL, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_invmod(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.invmod(*bi_a, *bi_b, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_mulmod(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_c = luaL_checkudata(L, 3, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.mulmod(*bi_a, *bi_b, *bi_c, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_exptmod(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_c = luaL_checkudata(L, 3, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.exptmod(*bi_a, *bi_b, *bi_c, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_gcd(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.gcd(*bi_a, *bi_b, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_lcm(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.lcm(*bi_a, *bi_b, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_unm(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi = lcrypt_new_bigint(L);
  lcrypt_error(L, ltc_mp.neg(*bi_a, *bi), NULL);
  return 1;
}

static int lcrypt_bigint_eq(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lua_pushboolean(L, (ltc_mp.compare(*bi_a, *bi_b) == LTC_MP_EQ) ? 1 : 0);
  return 1;
}

static int lcrypt_bigint_lt(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lua_pushboolean(L, (ltc_mp.compare(*bi_a, *bi_b) == LTC_MP_LT) ? 1 : 0);
  return 1;
}

static int lcrypt_bigint_le(lua_State *L)
{
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  lcrypt_bigint *bi_b = luaL_checkudata(L, 2, "LCRYPT_BIGINT");
  lua_pushboolean(L, (ltc_mp.compare(*bi_a, *bi_b) == LTC_MP_GT) ? 0 : 1);
  return 1;
}

static int lcrypt_bigint_tostring(lua_State *L)
{
#if 0
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  size_t out_length = (size_t)ltc_mp.unsigned_size(*bi_a) + 1;
  unsigned char *out = lcrypt_malloc(L, out_length);
  out[0] = (ltc_mp.compare_d(*bi_a, 0) == LTC_MP_LT) ? (unsigned char)0x80 : (unsigned char)0x00;
  lcrypt_error(L, ltc_mp.unsigned_write(*bi_a, out+1), out);
  if(out[0] == 0 && out[1] < 0x7f)
    lua_pushlstring(L, (char*)out+1, out_length-1);
  else
    lua_pushlstring(L, (char*)out, out_length);
  free(out);
  return 1;
#else
  lcrypt_bigint *bi_a = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  char *out = lcrypt_malloc(L, ltc_mp.count_bits(*bi_a) / 3 + 3);
  lcrypt_error(L, ltc_mp.write_radix(*bi_a, out, 10), NULL);
  lua_pushstring(L, out);
  free(out);
  return 1;
#endif
}

static int lcrypt_bigint_index(lua_State *L)
{
  lcrypt_bigint *bi = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  const char *index = luaL_checkstring(L, 2);
  if(strcmp(index, "bits") == 0) { lua_pushinteger(L, ltc_mp.count_bits(*bi)); return 1; }
  if(strcmp(index, "isprime") == 0)
  {
    int ret = LTC_MP_NO;
    lcrypt_error(L, ltc_mp.isprime(*bi, &ret), NULL);
    lua_pushboolean(L, (ret == LTC_MP_YES) ? 1 : 0);
    return 1;
  }
  if(strcmp(index, "eq") == 0) { lua_pushcfunction(L, lcrypt_bigint_eq); return 1; }
  if(strcmp(index, "lt") == 0) { lua_pushcfunction(L, lcrypt_bigint_lt); return 1; }
  if(strcmp(index, "le") == 0) { lua_pushcfunction(L, lcrypt_bigint_le); return 1; }
  if(strcmp(index, "add") == 0) { lua_pushcfunction(L, lcrypt_bigint_add); return 1; }
  if(strcmp(index, "sub") == 0) { lua_pushcfunction(L, lcrypt_bigint_sub); return 1; }
  if(strcmp(index, "mul") == 0) { lua_pushcfunction(L, lcrypt_bigint_mul); return 1; }
  if(strcmp(index, "div") == 0) { lua_pushcfunction(L, lcrypt_bigint_div); return 1; }
  if(strcmp(index, "divmod") == 0) { lua_pushcfunction(L, lcrypt_bigint_divmod); return 1; }
  if(strcmp(index, "mod") == 0) { lua_pushcfunction(L, lcrypt_bigint_mod); return 1; }
  if(strcmp(index, "gcd") == 0) { lua_pushcfunction(L, lcrypt_bigint_gcd); return 1; }
  if(strcmp(index, "lcm") == 0) { lua_pushcfunction(L, lcrypt_bigint_lcm); return 1; }
  if(strcmp(index, "invmod") == 0) { lua_pushcfunction(L, lcrypt_bigint_invmod); return 1; }
  if(strcmp(index, "mulmod") == 0) { lua_pushcfunction(L, lcrypt_bigint_mulmod); return 1; }
  if(strcmp(index, "exptmod") == 0) { lua_pushcfunction(L, lcrypt_bigint_exptmod); return 1; }
  return 0;
}

static int lcrypt_bigint_gc(lua_State *L)
{
  lcrypt_bigint *bi = luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  if(likely(*bi != NULL))
  {
    ltc_mp.deinit(*bi);
    *bi = NULL;
  }
  return 0;
}

static int lcrypt_bigint_create(lua_State *L)
{
  if(lua_type(L, 1) == LUA_TNUMBER)
  {
    long n = luaL_checknumber(L, 1);
    lcrypt_bigint *bi = lcrypt_new_bigint(L);
    if(n < 0)
    {
      void *temp;
      int err = CRYPT_OK;
      lcrypt_error(L, ltc_mp.init(&temp), NULL);
      if((err = ltc_mp.set_int(temp, -n)) == CRYPT_OK)
      {
        err = ltc_mp.neg(temp, *bi);
      }
      ltc_mp.deinit(temp);
      lcrypt_error(L, err, NULL);
    }
    else
    {
      lcrypt_error(L, ltc_mp.set_int(*bi, n), NULL);
    }
  }
  else
  {
    size_t n_length = 0;
    unsigned char *n = (unsigned char*)luaL_optlstring(L, 1, "0", &n_length);
    lcrypt_bigint *bi = lcrypt_new_bigint(L);
    int radix = 10;
    if(lua_isnumber(L, 2) == 1)
    {
      radix = luaL_checknumber(L, 2);
    }
    if(radix <= 0)
    {
      lcrypt_error(L, ltc_mp.unsigned_read(*bi, n, n_length), NULL);
    }
    else
    {
      lcrypt_error(L, ltc_mp.read_radix(*bi, n, radix), NULL);
    }
  }
  return 1;
}

static int lcrypt_bigint_isbigint(lua_State *L)
{
  luaL_checkudata(L, 1, "LCRYPT_BIGINT");
  return 0;
}

static const struct luaL_reg lcrypt_bigint_flib[] =
{
  { "__index", lcrypt_bigint_index },
  { "__unm", lcrypt_bigint_unm },
  { "__tostring", lcrypt_bigint_tostring },
  { "__gc", lcrypt_bigint_gc },
  { NULL, NULL }
};

#define METAMETHOD_WRAPPER(L, f) \
    lua_pushstring(L, "__" #f); \
    luaL_dostring(L, "" \
"return function(a, b) \n" \
"    local aisb = pcall(lcrypt.isbigint, a) \n" \
"    local bisb = pcall(lcrypt.isbigint, b) \n" \
"    if not aisb then \n" \
"        a = lcrypt.bigint(a) \n" \
"    end \n" \
"    if not bisb then \n" \
"        b = lcrypt.bigint(b) \n" \
"    end \n" \
"    return a:" #f "(b) \n" \
"end \n" \
    ""); \
    lua_settable(L, -3)

static void lcrypt_start_math(lua_State *L)
{
  luaL_newmetatable(L, "LCRYPT_BIGINT");
  luaL_register(L, NULL, lcrypt_bigint_flib);
  METAMETHOD_WRAPPER(L, add);
  METAMETHOD_WRAPPER(L, sub);
  METAMETHOD_WRAPPER(L, mul);
  METAMETHOD_WRAPPER(L, div);
  METAMETHOD_WRAPPER(L, mod);
  METAMETHOD_WRAPPER(L, eq);
  METAMETHOD_WRAPPER(L, lt);
  METAMETHOD_WRAPPER(L, le);
  lua_pop(L, 1);

  ltc_mp = ltm_desc;

  lua_pushstring(L, "bigint");
  lua_pushcfunction(L, lcrypt_bigint_create);
  lua_settable(L, -3);
  lua_pushstring(L, "isbigint");
  lua_pushcfunction(L, lcrypt_bigint_isbigint);
  lua_settable(L, -3);
}
