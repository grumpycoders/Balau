#include <functional>
#include "Base64.h"

static char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char lookup[] = {
//  x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xA  xB  xC  xD  xE  xF
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 0x
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 1x
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,  // 2x
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,  // 3x
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  // 4x
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,  // 5x
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,  // 6x
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,  // 7x
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 8x
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // 9x
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // Ax
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // Bx
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // Cx
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // Dx
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // Ex
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  // Fx
};

const double Balau::Base64::ratio = 4 / 3;

void Balau::Base64::encode_block(unsigned char in_tab[3], int len, char out[5]) {
    out[0] = cb64[in_tab[0] >> 2];
    out[1] = cb64[((in_tab[0] & 3) << 4) | ((in_tab[1] & 240) >> 4)];
    out[2] = len > 1 ? cb64[((in_tab[1] & 15) << 2) | ((in_tab[2] & 192) >> 6)] : '=';
    out[3] = len > 2 ? cb64[in_tab[2] & 63] : '=';
    out[4] = 0;
}

Balau::String Balau::Base64::encode(const uint8_t * data, int stream_size) {
    String encoded;
    encoded.reserve((size_t) (stream_size * ratio + 1));
    unsigned char in_tab[3];
    int len, i, s_pos;

    s_pos = 0;

    while (stream_size > 0) {
        in_tab[0] = 0;
        in_tab[1] = 0;
        in_tab[2] = 0;

        len = stream_size >= 3 ? 3 : stream_size;

        for (i = 0; i < len; i++) {
            in_tab[i] = data[s_pos + i];
        }

        char block[5];
        encode_block(in_tab, len, block);

        encoded += block;

        s_pos += 3;
        stream_size -= 3;
    }

    return encoded;
}

int Balau::Base64::stri(char x) {
    return lookup[(unsigned char) x];
}

int Balau::Base64::decode_block(char s1, char s2, char s3, char s4, unsigned char * out_tab) {
    int len, sb1, sb2, sb3, sb4;

    len = s3 == '=' ? 1 : s4 == '=' ? 2 : 3;
    s3 = (s3 == '=') || (s3 == 0) ? 'A' : s3;
    s4 = (s4 == '=') || (s4 == 0) ? 'A' : s4;
    
    sb1 = stri(s1);
    sb2 = stri(s2);
    sb3 = stri(s3);
    sb4 = stri(s4);
    
    out_tab[0] = (sb1 << 2) | (sb2 >> 4);
    out_tab[1] = ((sb2 << 4) & 255) | (sb3 >> 2);
    out_tab[2] = ((sb3 << 6) & 240) | sb4;
    
    return len;
}

ssize_t Balau::Base64::decode(const String & str_in, uint8_t * data_out, size_t outLen) {
    size_t s_len = str_in.strlen(), len = 0, i, t_len, idx;
    char s1, s2, s3, s4;
    unsigned char t_out[3];
    unsigned char * p = data_out;

    bool failure = false;

    std::function<char()> readNext = [&]() {
        char r = '=';

        if (idx >= s_len)
            return r;

        do {
            r = str_in[idx++];
        } while (r == '\r' || r == '\n' || r == ' ' || r == '\t');

        if (isdigit(r) || isalpha(r) || r == '+' || r == '/')
            return r;

        failure = true;

        return '=';
    };
    
    for (idx = 0; idx < s_len || failure; len += t_len) {
        s1 = readNext();
        s2 = readNext();
        s3 = readNext();
        s4 = readNext();
        t_len = decode_block(s1, s2, s3, s4, t_out);
        
        for (i = 0; i < t_len; i++) {
            if (outLen == 0)
                return -1;
            *(p++) = t_out[i];
            outLen--;
        }
    }

    return failure ? -1 : len;
}
