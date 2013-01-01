#include "adler32.h"

uint32_t rsync_adler32(unsigned char *buf, uint32_t len)
{
    uint32_t a = 0;
    for (int i = 0; i < len; i++)
    {
        a += buf[i];
    }
    MOD(a);
    uint32_t b = 0;
    for (int i = 0; i < len; i++)
    {
        b += (len - i) * buf[i];
    }
    MOD(b);
    uint32_t adler = a | (b << 16);
    return adler;
}

uint32_t rsync_rolling_adler32(uint32_t rsync_rolling_adler32, uint32_t len, unsigned char old_ch, unsigned char new_ch)
{
    uint32_t a_old = (rsync_rolling_adler32 << 16) >> 16;
    uint32_t a_new = a_old - old_ch + new_ch;
    MOD(a_new);
    
    uint32_t b_old = (rsync_rolling_adler32 >> 16);
    uint32_t b_new = b_old - len*old_ch + a_new;
    MOD(b_new);
    
    rsync_rolling_adler32 = a_new | b_new << 16;
    
    return rsync_rolling_adler32;
}
