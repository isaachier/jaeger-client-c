#include <stdint.h>

int main()
{
    int64_t x = 0;
    __atomic_add_fetch(&x, -1, __ATOMIC_RELAXED);
    __atomic_store_n(&x, 100, __ATOMIC_RELAXED);
    return 0;
}
