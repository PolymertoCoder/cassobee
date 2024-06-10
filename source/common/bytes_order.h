#pragma once
#include <endian.h>

#define encode_base16(x) htobe16(x)
#define encode_base32(x) htobe16(x)
#define encode_base64(x) htobe16(x)

#define decode_base16(x) be16toh(x)
#define decode_base32(x) be32toh(x)
#define decode_base64(x) be64toh(x)