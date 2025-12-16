#include <openssl/md5.h>
#include <openssl/evp.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>

// MD-5 algorithm
extern "C" const char* md5_hash(const char* str) 
{
  static char result[33];
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();

  if(!ctx) return nullptr;

  const EVP_MD* algorithm = EVP_md5();
  unsigned char digest[EVP_MD_size(algorithm)];
  unsigned int digest_len;

  EVP_DigestInit_ex(ctx,algorithm,nullptr);
  EVP_DigestUpdate(ctx,str,strlen(str));
  EVP_DigestFinal_ex(ctx,digest,&digest_len);
  EVP_MD_CTX_free(ctx);

  for(int i = 0;i < digest_len;i++) {
    sprintf(&result[i*2], "%02x", (unsigned int)digest[i]);

  }
  result[digest_len*2] = '\0';

  return result;
}

// SHA3-256 algorithm
extern "C" const char* sha3_256_hash(const char* str) 
{
  static char result[65];
  EVP_MD_CTX* ctx = EVP_MD_CTX_new();

  if(!ctx) return nullptr;

  const EVP_MD* algorithm = EVP_sha3_256();
  unsigned char digest[32];

  EVP_DigestInit_ex(ctx,algorithm,nullptr);
  EVP_DigestUpdate(ctx,str,strlen(str));
  EVP_DigestFinal_ex(ctx,digest,nullptr);
  EVP_MD_CTX_free(ctx);

  for(int i = 0;i < 32;i++) {
    sprintf(&result[i*2], "%02x", (unsigned int)digest[i]);
  }

  return result;
}

// murmur2 algorithm
extern "C" const char* murmur2_hash(const char* str) 
{
  static char result[17];
  const uint64_t m = 0xc6a4a7935bd1e995ULL;
  const int r = 47;
  const uint64_t seed = 0x9747b28c9c4e6c9bULL;

  size_t len = strlen(str);
  uint64_t h = seed ^ (len*m);

  const uint8_t* data = reinterpret_cast<const uint8_t*>(str);

  while(len >= 8) {
    uint64_t k;
    memcpy(&k, data, 8);
    k *= m;
    k ^= k >> r;
    k *= m;

    h^=k;
    h*=m;
    data += 8;
    len -= 8;
  }

  switch(len) {
    case 7: h^= static_cast<uint64_t>(data[6]) << 48;
    case 6: h^= static_cast<uint64_t>(data[5]) << 40;
    case 5: h^= static_cast<uint64_t>(data[4]) << 32;
    case 4: h^= static_cast<uint64_t>(data[3]) << 24;
    case 3: h^= static_cast<uint64_t>(data[2]) << 16;
    case 2: h^= static_cast<uint64_t>(data[1]) << 8;
    case 1: h^= static_cast<uint64_t>(data[0]);
            h *= m;

  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  snprintf(result,sizeof(result), "%016lx", h);
  return result;

}

