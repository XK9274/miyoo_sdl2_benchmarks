#ifndef COMMON_FORMAT_H
#define COMMON_FORMAT_H

#include <stddef.h>

#include "common/types.h"

const char *bench_audio_channel_label(int channels);
void bench_format_bytes_human(size_t bytes,
                              char *buffer,
                              size_t buffer_size);

#endif /* COMMON_FORMAT_H */
