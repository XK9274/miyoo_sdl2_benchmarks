#ifndef COMMON_BENCH_STRESS_H
#define COMMON_BENCH_STRESS_H

#define BENCH_SIN_TABLE_SIZE 512

typedef struct {
    float sin_table[BENCH_SIN_TABLE_SIZE];
    int sin_table_size;
    int sin_table_mask;
} BenchSinTable;

void bench_sin_table_init(BenchSinTable *table);

float bench_sin_table_sin(const BenchSinTable *table, float units);
float bench_sin_table_cos(const BenchSinTable *table, float units);
float bench_sin_table_sin_rad(const BenchSinTable *table, float radians);
float bench_sin_table_cos_rad(const BenchSinTable *table, float radians);

/* Maps stress_level (clamped 1-10) to a 0.5x-7x workload multiplier. */
float bench_stress_factor(int stress_level);

#endif /* COMMON_BENCH_STRESS_H */
