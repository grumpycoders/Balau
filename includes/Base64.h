#pragma once

#include <Exceptions.h>

namespace Balau {

class Base64 {
public:
    static String encode(const uint8_t * data, int len);
    static int decode(const String & str_in, uint8_t * data_out);
    static const double ratio;

private:
    static void encode_block(unsigned char in_tab[3], int len, char out[4]);
    static int stri(char);
    static int decode_block(char s1, char s2, char s3, char s4, unsigned char * out_tab);
};

};
