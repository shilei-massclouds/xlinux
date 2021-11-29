#ifndef _LINUX_HASH_H
#define _LINUX_HASH_H

#define hash_64 hash_64_generic

#define hash_long(val, bits) hash_64(val, bits)

#define GOLDEN_RATIO_64 0x61C8864680B583EBull
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_64

static __always_inline u32 hash_64_generic(u64 val, unsigned int bits)
{
    /* 64x64-bit multiply is efficient on all 64-bit processors */
    return val * GOLDEN_RATIO_PRIME >> (64 - bits);
}

#endif /* _LINUX_HASH_H */
