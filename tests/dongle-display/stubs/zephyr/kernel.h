#pragma once
#include <stdbool.h>
struct k_spinlock { int unused; };
typedef int k_spinlock_key_t;
static inline k_spinlock_key_t k_spin_lock(struct k_spinlock *p) { (void)p; return 0; }
static inline void k_spin_unlock(struct k_spinlock *p, k_spinlock_key_t k) { (void)p; (void)k; }
