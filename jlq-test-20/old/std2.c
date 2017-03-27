
#include <inttypes.h>

uint32_t
__udivsi3(uint32_t n, uint32_t d)
{
	/*
    const unsigned n_uword_bits = sizeof(su_int) * CHAR_BIT;
    su_int q;
    su_int r;
    unsigned sr;
    // special cases 
    if (d == 0)
        return 0; // ?! 
    if (n == 0)
        return 0;
    sr = __builtin_clz(d) - __builtin_clz(n);
    // 0 <= sr <= n_uword_bits - 1 or sr large 
    if (sr > n_uword_bits - 1)  // d > r 
        return 0;
    if (sr == n_uword_bits - 1)  // d == 1 
        return n;
    ++sr;
    // 1 <= sr <= n_uword_bits - 1 
    // Not a special case 
    q = n << (n_uword_bits - sr);
    r = n >> sr;
    su_int carry = 0;
    for (; sr > 0; --sr)
    {
        // r:q = ((r:q)  << 1) | carry 
        r = (r << 1) | (q >> (n_uword_bits - 1));
        q = (q << 1) | carry;
        //carry = 0;
        //  if (r.all >= d.all)
        //  {
        //       r.all -= d.all;
        //       carry = 1;
        //  }
         
        const si_int s = (si_int)(d - r - 1) >> (n_uword_bits - 1);
        carry = s & 1;
        r -= d & s;
    }
    q = (q << 1) | carry;
    return q;
	*/
	return 0;
}

uint32_t
__umodsi3(uint32_t a, uint32_t b)
{
    return a - __udivsi3(a, b) * b;
}
