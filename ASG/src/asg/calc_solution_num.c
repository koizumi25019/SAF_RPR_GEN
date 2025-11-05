#include "./calc_solution_num.h"
#include <string.h>

ThresholdNums g_thresh;

/* big = big * 2 (—á: "123" ¨ "246") */
void bignum_mul2(char* big) {
    int carry = 0;
    size_t len = strlen(big);
    for (int i = (int)len - 1; i >= 0; --i) {
        int d = (big[i] - '0') * 2 + carry;
        big[i] = '0' + (d % 10);
        carry = d / 10;
    }
    if (carry) {
        memmove(big + 1, big, len + 1);  // '\0' ‚àƒVƒtƒg
        big[0] = '0' + carry;        // carry ‚Í 1 ‚© 0
    }
}

// sum = sum + add
void bignum_add(char* sum, const char* add) {
    int carry = 0;
    size_t len_s = strlen(sum), len_a = strlen(add);
    int i = (int)len_s - 1, j = (int)len_a - 1;
    while (j >= 0 || carry) {
        int sv = (i >= 0 ? sum[i] - '0' : 0);
        int av = (j >= 0 ? add[j] - '0' : 0);
        int d = sv + av + carry;
        if (i >= 0) {
            sum[i] = '0' + (d % 10);
        }
        else {
            // sum ‚Ìæ“ª‚ğ‰z‚¦‚½‚çŠg’£
            memmove(sum + 1, sum, len_s + 1);
            sum[0] = '0' + (d % 10);
            len_s++;
            i = 0;
        }
        carry = d / 10;
        i--; j--;
    }
}

/* quotient = num / divisor (—á: "246" / 10 ¨ "24") */
void bignum_div_small(const char* num, int divisor, char* quotient) {
    int rem = 0, qi = 0;
    int len = (int)strlen(num);
    for (int i = 0; i < len; ++i) {
        int d = rem * 10 + (num[i] - '0');
        int q = d / divisor;
        rem = d % divisor;
        if (qi > 0 || q > 0) {
            quotient[qi++] = '0' + q;
        }
    }
    if (qi == 0) {
        quotient[qi++] = '0';
    }
    quotient[qi] = '\0';
}

/* ‘å‚«‚È 10i•¶š—ñ“¯m‚ğ«‘‡E’·‚³‡‚Å”äŠr */
int bignum_cmp(const char* a, const char* b) {
    // æ“ª‚Ì '0' ‚Í”ò‚Î‚·
    while (*a == '0') ++a;
    while (*b == '0') ++b;
    int la = (int)strlen(a), lb = (int)strlen(b);
    if (la < lb) return -1;
    if (la > lb) return +1;
    // “¯‚¶’·‚³‚È‚ç•¶š’PˆÊ‚Å”äŠr
    for (int i = 0; i < la; ++i) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return +1;
    }
    return 0;
}
