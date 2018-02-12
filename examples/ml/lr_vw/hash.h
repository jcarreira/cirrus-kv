uint64_t uniform_hash(const void * key, size_t len, uint64_t seed);

uint64_t hashstring (std::string s, uint64_t h) {
  std::cout << "hashing string: " << s << " h: " << h << std::endl;
  //trim leading whitespace but not UTF-8
  std::string::iterator it = s.begin();
  for(; it != s.end() && *it <= 0x20 && (int)*it>= 0; it++);
  //trim trailing white space but not UTF-8
  std::string::iterator it_end = s.end() - 1;
  for(; it_end > it && *(it_end-1) <= 0x20 && (int)*(it_end-1) >=0; it_end--);

  size_t ret = 0;
  char *p = &*it;
  while (p != &*it_end) {
    if (*p >= '0' && *p <= '9')
      ret = 10*ret + *(p++) - '0';
    else
      return uniform_hash((unsigned char *)&*it, (&*it_end) - (&*it), h); 
  }
  return ret + h;
}

static inline uint32_t getblock(const uint32_t * p, int i) {
  return p[i];
}

//-----------------------------------------------------------------------------

uint64_t uniform_hash(const void * key, size_t len, uint64_t seed) {
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = (int)len / 4;

  uint32_t h1 = (uint32_t)seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  // --- body
  const uint32_t * blocks = (const uint32_t *)(data + nblocks * 4); 

  for (int i = -nblocks; i; i++)
  {
    uint32_t k1 = MURMUR_HASH_3::getblock(blocks, i); 

    k1 *= c1; 
    k1 = ROTL32(k1, 15);
    k1 *= c2; 

    h1 ^= k1; 
    h1 = ROTL32(h1, 13);
    h1 = h1 * 5 + 0xe6546b64;
  }

  // --- tail
  const uint8_t * tail = (const uint8_t*)(data + nblocks * 4); 

  uint32_t k1 = 0;

  switch (len & 3)
  {
  case 3: k1 ^= tail[2] << 16; 
  case 2: k1 ^= tail[1] << 8;
  case 1: k1 ^= tail[0];
    k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1; 
  }

  // --- finalization
  h1 ^= len;

  return MURMUR_HASH_3::fmix(h1);
}

