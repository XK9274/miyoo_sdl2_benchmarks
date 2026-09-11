#include "common/bench_stress.h"

#include <math.h>
#include <string.h>

#define BENCH_TWO_PI 6.28318530717958647692f

void bench_sin_table_init(BenchSinTable *table)
{
    if (!table) {
        return;
    }
    memset(table, 0, sizeof(*table));
    table->sin_table_size = BENCH_SIN_TABLE_SIZE;
    table->sin_table_mask = BENCH_SIN_TABLE_SIZE - 1;
    for (int i = 0; i < table->sin_table_size; ++i) {
        const float angle = (BENCH_TWO_PI * (float)i) / (float)table->sin_table_size;
        table->sin_table[i] = sinf(angle);
    }
}

float bench_sin_table_sin(const BenchSinTable *table, float units)
{
    if (!table || table->sin_table_size <= 0) {
        return 0.0f;
    }
    const int idx = ((int)units) & table->sin_table_mask;
    return table->sin_table[idx];
}

float bench_sin_table_cos(const BenchSinTable *table, float units)
{
    if (!table || table->sin_table_size <= 0) {
        return 0.0f;
    }
    const float quarter_turn = (float)(table->sin_table_size >> 2);
    return bench_sin_table_sin(table, units + quarter_turn);
}

float bench_sin_table_sin_rad(const BenchSinTable *table, float radians)
{
    if (!table || table->sin_table_size <= 0) {
        return 0.0f;
    }
    const float units = radians * (float)table->sin_table_size / BENCH_TWO_PI;
    return bench_sin_table_sin(table, units);
}

float bench_sin_table_cos_rad(const BenchSinTable *table, float radians)
{
    if (!table || table->sin_table_size <= 0) {
        return 0.0f;
    }
    const float units = radians * (float)table->sin_table_size / BENCH_TWO_PI;
    return bench_sin_table_cos(table, units);
}

float bench_stress_factor(int stress_level)
{
    int level = stress_level;
    if (level < 1) {
        level = 1;
    } else if (level > 10) {
        level = 10;
    }
    const float min_factor = 0.5f;
    const float max_factor = 7.0f;
    if (level <= 1) {
        return min_factor;
    }
    const float t = (float)(level - 1) / 9.0f;
    return min_factor + (max_factor - min_factor) * t;
}
