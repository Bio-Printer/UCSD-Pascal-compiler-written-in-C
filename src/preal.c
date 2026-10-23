/* preal.c -- (the host build only) the P-machine's REAL, bit for bit: the Z80
   interpreter's floating-point package (FPL.TEXT / FPI.TEXT), ported from the
   emulator's UcsdReal.h, and PWROFTEN's table (TENTBL).  On the P-System the
   compiler's REAL is Tiny-C's float, which is this real, so the two builds make
   the same constants.

   A real is 4 bytes, as in memory: [0] the exponent (bias 128, 0: zero),
   [1] the sign and the top 7 mantissa bits (the leading 1 not stored), [2] [3]
   the rest of the mantissa: 0.1mmm...m * 2^(exp-128). */
#ifndef __TINYC__
#include "compglbl.h"
#include <stdint.h>

static const unsigned char TENTBL[156] = {
    0x81, 0x00, 0x00, 0x00, 0x84, 0x20, 0x00, 0x00, 0x87, 0x48, 0x00, 0x00, 0x8A, 0x7A, 0x00, 0x00,
    0x8E, 0x1C, 0x40, 0x00, 0x91, 0x43, 0x50, 0x00, 0x94, 0x74, 0x24, 0x00, 0x98, 0x18, 0x96, 0x80,
    0x9B, 0x3E, 0xBC, 0x20, 0x9E, 0x6E, 0x6B, 0x28, 0xA2, 0x15, 0x02, 0xF9, 0xA5, 0x3A, 0x43, 0xB7,
    0xA8, 0x68, 0xD4, 0xA5, 0xAC, 0x11, 0x84, 0xE7, 0xAF, 0x35, 0xE6, 0x21, 0xB2, 0x63, 0x5F, 0xA9,
    0xB6, 0x0E, 0x1B, 0xCA, 0xB9, 0x31, 0xA2, 0xBD, 0xBC, 0x5E, 0x0B, 0x6C, 0xC0, 0x0A, 0xC7, 0x24,
    0xC3, 0x2D, 0x78, 0xED, 0xC6, 0x58, 0xD7, 0x28, 0xCA, 0x07, 0x86, 0x79, 0xCD, 0x29, 0x68, 0x17,
    0xD0, 0x53, 0xC2, 0x1D, 0xD4, 0x04, 0x59, 0x52, 0xD7, 0x25, 0x6F, 0xA7, 0xDA, 0x4E, 0xCB, 0x91,
    0xDE, 0x01, 0x3F, 0x3B, 0xE1, 0x21, 0x8F, 0x0A, 0xE4, 0x49, 0xF2, 0xCD, 0xE7, 0x7C, 0x6F, 0x80,
    0xEB, 0x1D, 0xC5, 0xB0, 0xEE, 0x45, 0x37, 0x1C, 0xF1, 0x76, 0x84, 0xE3, 0xF5, 0x1A, 0x13, 0x0E,
    0xF8, 0x40, 0x97, 0xD2, 0xFB, 0x70, 0xBD, 0xC7, 0xFF, 0x16, 0x76, 0x9C,
};

static int und(unsigned char o[4]) { o[0] = o[1] = o[2] = o[3] = 0; return 0; }
static int ovr(unsigned char o[4]) { o[0] = 0xFF; o[1] = 0x7F; o[2] = 0xFF; o[3] = 0xFF; return 0; }

static int rnd(uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t L, uint8_t H, unsigned char o[4])
{
    uint8_t s, A;
    if (E & 0x80) {
        if (++D == 0 && ++C == 0 && ++B == 0) { B = 0x80; L++; }
    }
    if (L == 0)
        return ovr(o);
    s = (uint8_t)(H + 1);
    A = (uint8_t)(((s & 1) << 7) ^ B);
    o[0] = L; o[1] = A; o[2] = C; o[3] = D;
    return 1;
}

static int normx(uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t L, uint8_t H, unsigned char o[4])
{
    while (!(B & 0x80)) {
        L--;
        if (L == 0)
            return und(o);
        B = (uint8_t)((B << 1) | (C >> 7));
        C = (uint8_t)((C << 1) | (D >> 7));
        D = (uint8_t)((D << 1) | (E >> 7));
        E = (uint8_t)(E << 1);
    }
    return rnd(B, C, D, E, L, H, o);
}

static int norm(uint8_t B, uint8_t C, uint8_t D, uint8_t E, uint8_t L, uint8_t H, unsigned char o[4])
{
    uint8_t A;
    if (B != 0)
        return normx(B, C, D, E, L, H, o);
    if ((E | D | C) == 0) { o[0] = o[1] = o[2] = o[3] = 0; return 1; }
    A = L;
    for (;;) {
        if (A < 9)
            return und(o);
        A = (uint8_t)(A - 9 + 1);
        B = C; C = D; D = E; E = 0;
        if (B != 0)
            return normx(B, C, D, E, A, H, o);
    }
}

static int addsub(const unsigned char *a1, const unsigned char *a2, int subtract, unsigned char o[4])
{
    uint8_t B = subtract ? 0x7F : 0x02, A, C, E, x, D, H, mb, mc, md, me, L, bb, n, br;
    const unsigned char *greater = a1, *lesser = a2;
    unsigned s;
    int t;
    A = (uint8_t)(a1[0] - a2[0]);
    if (a1[0] < a2[0]) { greater = a2; lesser = a1; B++; A = (uint8_t)(0 - A); }
    C = A;
    if (lesser[0] == 0) {
        if (C == 0) { o[0] = o[1] = o[2] = o[3] = 0; return 1; }
        C = 25;
    }
    E = greater[1];
    x = (uint8_t)(E ^ B);
    D = (uint8_t)((x << 1) | (x >> 7));
    B++;
    x = (uint8_t)(E ^ B ^ lesser[1] ^ D);
    E = (uint8_t)((x & 0x80) ^ D);
    H = E;
    mb = (uint8_t)(lesser[1] | 0x80); mc = lesser[2]; md = lesser[3]; me = 0;
    A = C;
    if (A >= 26) A = 25;
    while (A >= 8) { me = md; md = mc; mc = mb; mb = 0; A = (uint8_t)(A - 8); }
    for (n = A; n; n--) {
        me = (uint8_t)((me >> 1) | (md << 7));
        md = (uint8_t)((md >> 1) | (mc << 7));
        mc = (uint8_t)((mc >> 1) | (mb << 7));
        mb = (uint8_t)(mb >> 1);
    }
    L = greater[0];
    bb = (uint8_t)(greater[1] | 0x80);
    if (!(H & 0x80)) {
        s = (unsigned)md + greater[3]; md = (uint8_t)s;
        s = (unsigned)mc + greater[2] + (s >> 8); mc = (uint8_t)s;
        s = (unsigned)bb + mb + (s >> 8); mb = (uint8_t)s;
        if (s >> 8) {
            me = (uint8_t)((me >> 1) | (md << 7));
            md = (uint8_t)((md >> 1) | (mc << 7));
            mc = (uint8_t)((mc >> 1) | (mb << 7));
            mb = (uint8_t)((mb >> 1) | 0x80);
            L++;
        }
        return rnd(mb, mc, md, me, L, H, o);
    }
    t = 0 - me;                 br = t < 0; me = (uint8_t)t;
    t = greater[3] - md - br;   br = t < 0; md = (uint8_t)t;
    t = greater[2] - mc - br;   br = t < 0; mc = (uint8_t)t;
    t = bb - mb - br;           br = t < 0; mb = (uint8_t)t;
    if (br) {
        H = (uint8_t)(H + 1);
        t = 0 - me;             br = t < 0; me = (uint8_t)t;
        t = 0 - md - br;        br = t < 0; md = (uint8_t)t;
        t = 0 - mc - br;        br = t < 0; mc = (uint8_t)t;
        t = 0 - mb - br;        mb = (uint8_t)t;
    }
    return norm(mb, mc, md, me, L, H, o);
}

static uint32_t mant(const unsigned char *a)
{
    return ((uint32_t)(a[1] | 0x80) << 16) | ((uint32_t)a[2] << 8) | a[3];
}

static int mul(const unsigned char *a1, const unsigned char *a2, unsigned char o[4])
{
    uint8_t x, H, A, L;
    unsigned sum;
    int carry;
    uint64_t P;
    if (a1[0] == 0 || a2[0] == 0) { o[0] = o[1] = o[2] = o[3] = 0; return 1; }
    x = (uint8_t)(a1[1] ^ a2[1]);
    H = (uint8_t)((x << 1) | (x >> 7));
    sum = (unsigned)(uint8_t)(a1[0] - 1) + a2[0];
    A = (uint8_t)sum;
    carry = sum > 0xFF;
    if (A & 0x80) { if (carry) return ovr(o); }
    else          { if (!carry) return und(o); }
    L = (uint8_t)(A + 0x81);
    P = (uint64_t)mant(a1) * mant(a2);
    return normx((uint8_t)(P >> 40), (uint8_t)(P >> 32), (uint8_t)(P >> 24), (uint8_t)(P >> 16), L, H, o);
}

static int dvd(const unsigned char *a1, const unsigned char *a2, unsigned char o[4])
{
    uint8_t x, H, A, L;
    int borrow;
    uint64_t Q;
    if (a2[0] == 0)
        return ovr(o);
    if (a1[0] == 0) { o[0] = o[1] = o[2] = o[3] = 0; return 1; }
    x = (uint8_t)(a1[1] ^ a2[1]);
    H = (uint8_t)((x << 1) | (x >> 7));
    A = (uint8_t)(a1[0] - a2[0]);
    borrow = a1[0] < a2[0];
    if (A & 0x80) { if (!borrow) return ovr(o); }
    else          { if (borrow) return und(o); }
    L = (uint8_t)(A + 0x81);
    Q = ((uint64_t)mant(a1) << 25) / mant(a2);
    return normx((uint8_t)(Q >> 18), (uint8_t)(Q >> 10), (uint8_t)(Q >> 2), (uint8_t)((Q & 3) << 6), L, H, o);
}

REAL RFLOAT(INTEGER i)
{
    REAL r;
    uint16_t v = (uint16_t)i;
    uint8_t B = (uint8_t)(v >> 8), C = (uint8_t)v, D = 0, E = 0, H = 0;
    if (B & 0x80) {
        uint32_t m = ((uint32_t)B << 24) | ((uint32_t)C << 16);
        H = 1;
        m = 0u - m;
        B = (uint8_t)(m >> 24); C = (uint8_t)(m >> 16); D = (uint8_t)(m >> 8); E = (uint8_t)m;
    }
    norm(B, C, D, E, 0x90, H, r.b);
    return r;
}

REAL RADD(REAL a, REAL b) { REAL r; addsub(a.b, b.b, 0, r.b); return r; }
REAL RSUB(REAL a, REAL b) { REAL r; addsub(a.b, b.b, 1, r.b); return r; }
REAL RMUL(REAL a, REAL b) { REAL r; mul(a.b, b.b, r.b); return r; }
REAL RDIVR(REAL a, REAL b) { REAL r; dvd(a.b, b.b, r.b); return r; }
REAL RNEG(REAL a) { if (a.b[0]) a.b[1] ^= 0x80; return a; }

REAL RPOT(INTEGER i)
{
    REAL r;
    if (i < 0 || i >= 39) { r.b[0] = r.b[1] = r.b[2] = r.b[3] = 0; return r; }
    memcpy(r.b, TENTBL + 4 * i, 4);
    return r;
}

static INTEGER fix(const unsigned char *a)
{
    uint8_t ex = a[0], A;
    uint16_t m, mag;
    int sh;
    if (ex >= 0x90)
        return (INTEGER)((a[1] & 0x80) ? 0x8000 : 0x7FFF);
    m = (uint16_t)(((a[1] | 0x80) << 8) | a[2]);
    A = (uint8_t)(ex - 0x90);
    sh = (A >= 0xF0) ? (int)(uint8_t)(0 - A) : 16;
    mag = (sh >= 16) ? 0 : (uint16_t)(m >> sh);
    return (INTEGER)((a[1] & 0x80) ? (uint16_t)(0 - mag) : mag);
}

INTEGER RTRUNC(REAL a) { return fix(a.b); }

INTEGER RROUND(REAL a)
{
    unsigned char half[4], s[4];
    half[0] = 0x80; half[1] = (unsigned char)(a.b[1] & 0x80); half[2] = half[3] = 0;
    addsub(a.b, half, 0, s);
    return fix(s);
}

/* a < b: the interpreter's real comparison (the difference's sign) */
BOOLEAN RLT(REAL a, REAL b)
{
    REAL d = RSUB(a, b);
    return d.b[0] != 0 && (d.b[1] & 0x80) != 0;
}
#endif
