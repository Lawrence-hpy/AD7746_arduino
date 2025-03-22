#include "no_os_util.h"

uint32_t no_os_find_first_set_bit(uint32_t word) {
    if (word == 0)
        return 32;
    uint32_t pos = 0;
    while (!(word & (1 << pos)))
        pos++;
    return pos;
}

uint64_t no_os_find_first_set_bit_u64(uint64_t word) {
    if (word == 0)
        return 64;
    uint64_t pos = 0;
    while (!(word & ((uint64_t)1 << pos)))
        pos++;
    return pos;
}

uint32_t no_os_find_last_set_bit(uint32_t word) {
    if (word == 0)
        return 32;
    uint32_t pos = 31;
    while (!(word & (1 << pos)))
        pos--;
    return pos;
}

uint32_t no_os_find_closest(int32_t val, const int32_t *array, uint32_t size) {
    int32_t best_diff = abs(val - array[0]);
    uint32_t best_index = 0;
    for (uint32_t i = 1; i < size; i++) {
        int32_t diff = abs(val - array[i]);
        if (diff < best_diff) {
            best_diff = diff;
            best_index = i;
        }
    }
    return best_index;
}

uint32_t no_os_field_prep(uint32_t mask, uint32_t val) {
    while ((mask & 1) == 0) {
        mask >>= 1;
        val <<= 1;
    }
    return val & mask;
}

uint64_t no_os_field_prep_u64(uint64_t mask, uint64_t val) {
    while ((mask & 1) == 0) {
        mask >>= 1;
        val <<= 1;
    }
    return val & mask;
}

uint32_t no_os_field_get(uint32_t mask, uint32_t word) {
    while ((mask & 1) == 0) {
        mask >>= 1;
        word >>= 1;
    }
    return word & mask;
}

uint32_t no_os_field_max(uint32_t mask) {
    return no_os_field_get(mask, mask);
}

uint64_t no_os_field_max_u64(uint64_t mask) {
    return no_os_field_get(mask, mask);
}

int32_t no_os_log_base_2(uint32_t x) {
    int32_t res = -1;
    while (x) {
        x >>= 1;
        res++;
    }
    return res;
}

uint32_t no_os_greatest_common_divisor(uint32_t a, uint32_t b) {
    while (b != 0) {
        uint32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

uint64_t no_os_greatest_common_divisor_u64(uint64_t a, uint64_t b) {
    while (b != 0) {
        uint64_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

uint32_t no_os_lowest_common_multiple(uint32_t a, uint32_t b) {
    return (a / no_os_greatest_common_divisor(a, b)) * b;
}

unsigned int no_os_hweight8(uint8_t word) {
    unsigned int count = 0;
    while (word) {
        count += word & 1;
        word >>= 1;
    }
    return count;
}

unsigned int no_os_hweight16(uint16_t word) {
    unsigned int count = 0;
    while (word) {
        count += word & 1;
        word >>= 1;
    }
    return count;
}

unsigned int no_os_hweight32(uint32_t word) {
    unsigned int count = 0;
    while (word) {
        count += word & 1;
        word >>= 1;
    }
    return count;
}