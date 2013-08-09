const char * lcrypt_rsa = ""
"rsa = {}\n"
"\n"
"function rsa:pkcs1_pad(data, out_length)\n"
"  local asn1 = string.char(0x00, 0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14)\n"
"  return string.char(0x00, 0x01) .. string.char(0xff):rep(out_length - #asn1 - #data - 2) .. asn1 .. data\n"
"end\n"
"\n"
"function rsa:encode_int(value, len)\n"
"  local ret = ''\n"
"  for i=1,len do\n"
"    ret = string.char(value % 256) .. ret\n"
"    value = math.floor(value / 256)\n"
"  end\n"
"  return ret\n"
"end\n"
"\n"
"function rsa:oaep_g(data, out_length)\n"
"  local out,counter = '', 0\n"
"  while #out < out_length do\n"
"    out = out .. lcrypt.hashes.sha1:hash(data .. self:encode_int(counter, 4)):done()\n"
"    counter = counter + 1\n"
"  end\n"
"  return out:sub(1, out_length)\n"
"end\n"
"\n"
"function rsa:oaep_pad(data, param, out_length)\n"
"  out_length = out_length - 1\n"
"  local h_length = #data\n"
"  local g_length = out_length - h_length\n"
"  local seed = lcrypt.random(h_length)\n"
"  local c = lcrypt.hashes.sha1:hash(param):done()\n"
"  c = c .. string.rep(string.char(0), g_length - h_length - 2 - #c) .. string.char(0, 1) .. data\n"
"  local x = lcrypt.xor(c, self:oaep_g(seed, g_length))\n"
"  local y = lcrypt.xor(seed, self:oaep_g(x, h_length))\n"
"  return string.char(0) .. x .. y\n"
"end\n"
"\n"
"function rsa:oaep_unpad(data, param, out_length)\n"
"  data = data:sub(2, #data)\n"
"  local g_length = #data - out_length\n"
"  local x = data:sub(1, g_length)\n"
"  local seed = lcrypt.xor(self:oaep_g(x, out_length), data:sub(g_length +1, #data))\n"
"  local c = lcrypt.xor(x, self:oaep_g(seed, g_length))\n"
"  local v = lcrypt.hashes.sha1:hash(param):done()\n"
"  if c:sub(1,#v) == v then return c:sub(g_length - out_length + 1, #c) end\n"
"end\n"
"\n"
"function rsa:prime(bits)\n"
"  bits = math.floor(bits)\n"
"  if bits < 24 then return end\n"
"  local ret, high, bytes = nil, 1, math.floor((bits - 7) / 8)\n"
"  for i=1,bits-bytes*8-1 do high = 1 + high + high end\n"
"  high = string.char(high)\n"
"  low = lcrypt.random(1):byte()\n"
"  if low / 2 == math.floor(low / 2) then low = low + 1 end\n"
"  low = string.char(low)\n"
"  bytes = bytes - 1\n"
"  repeat\n"
"    ret = lcrypt.bigint(high .. lcrypt.random(bytes) .. low)\n"
"  until ret.isprime\n"
"  return ret\n"
"end\n"
"\n"
"function rsa:gen_key(bits, e)\n"
"  local key,one,p1,q1 = { e=lcrypt.bigint(e) }, lcrypt.bigint(1), nil, nil\n"
"  bits = bits / 2\n"
"  repeat\n"
"    key.p = self:prime(bits)\n"
"    p1 = key.p - one\n"
"  until p1:gcd(key.e) == one\n"
"  repeat\n"
"    key.q = self:prime(bits)\n"
"    q1 = key.q - one\n"
"  until q1:gcd(key.e) == one\n"
"  key.d = key.e:invmod(p1:lcm(q1))\n"
"  key.n = key.p * key.q\n"
"  key.dp = key.d % p1\n"
"  key.dq = key.d % q1\n"
"  key.qp = key.q:invmod(key.p)\n"
"  return key\n"
"end\n"
"\n"
"function rsa:private(msg, key)\n"
"  msg = lcrypt.bigint(msg)\n"
"  local a,b = msg:exptmod(key.dp, key.p), msg:exptmod(key.dq, key.q)\n"
"  local ret = tostring(key.qp:mulmod(a - b, key.p) * key.q + b)\n"
"  if ret:byte(1) == 0 then ret = ret:sub(2, #ret) end\n"
"  return ret\n"
"end\n"
"\n"
"function rsa:public(msg, key)\n"
"  return tostring(lcrypt.bigint(msg):exptmod(key.e, key.n))\n"
"end\n"
"\n"
"function rsa:sign_pkcs1(msg, key)\n"
"  return self:private(self:pkcs1_pad(lcrypt.hashes.sha1:hash(msg):done(), key.n.bits / 8), key)\n"
"end\n"
"\n"
"function rsa:verify_pkcs1(signature, msg, key)\n"
"  msg = lcrypt.hashes.sha1:hash(msg):done()\n"
"  local tmp = self:public(signature, key)\n"
"  if tmp:sub(#tmp - #msg + 1, #tmp) == msg then return true end\n"
"end\n"
"\n"
"function rsa:sign_oaep(msg, param, key)\n"
"  return self:private(self:oaep_pad(lcrypt.hashes.sha1:hash(msg):done(), param, key.n.bits / 8), key)\n"
"end\n"
"\n"
"function rsa:verify_oaep(signature, msg, param, key)\n"
"  local tmp = self:public(signature, key)\n"
"  local h = self:oaep_unpad(tmp, param, 20)\n"
"  if h == lcrypt.hashes.sha1:hash(msg):done() then return true end\n"
"end\n"
"";