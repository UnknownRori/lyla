#include <stdio.h>
#include <rstb_common.h>

#include "prime.c"
#include "raylib.h"
#include "types.h"
#include "randomizer.h"

static u32 lower_bound(u32 n)
{
    u32 lo = 0, hi = PRIMES_COUNT;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2;
        if (primes[mid] < n) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static u32 is_prime(u32 n)
{
    if (n < 2) return 0;
    if (n <= PRIMES_LIMIT) {
        u32 i = lower_bound(n);
        return i < PRIMES_COUNT && primes[i] == n;
    }
    for (u32 i = 0; i < PRIMES_COUNT; i++) {
        u64 p = primes[i];
        if (p * p > n) return 1;
        if (n % p == 0) return 0;
    }
    return 1;
}

static u32 find_prime(u32 n)
{
    if (n <= 2) return 2;

    if (n <= primes[PRIMES_COUNT - 1])
        return primes[lower_bound(n)];

    for (u32 x = n; x <= 0xFFFFFFFFULL; x++)
        if (is_prime((u32)x)) return (u32)x;

    return 0;
}

void playlist_randomizer_init(PlaylistRandomizer* rand, u32 n)
{
    RORI_ASSERT(rand != NULL && "Dummy dumb dumb");
    rand->n = n;
    rand->p = find_prime(n);
    rand->a = GetRandomValue(0, rand->p - 1);
    rand->b = GetRandomValue(0, rand->p - 1);
}

u32  playlist_randomizer_next(PlaylistRandomizer* rand)
{
    RORI_ASSERT(rand != NULL && "Dummy dumb dumb");
    while ((rand->a*rand->x + rand->b) % rand->p >= rand->n) {
        rand->x = (rand->x + 1) % rand->p;
    }
    u32 result = (rand->a * rand->x + rand->b) % rand->p;
    rand->x = (rand->x + 1) % rand->p;
    return result;
}
