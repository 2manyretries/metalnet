

#ifndef _IO_H
#define _IO_H

#include <stdbool.h>

/// Initialise - must be called before any IO (ASAP).
bool io_init();

/// Is there pending input to be read.
/// This non-standard API call allows for easy checking.
bool io_input_available(int fd);

#endif

