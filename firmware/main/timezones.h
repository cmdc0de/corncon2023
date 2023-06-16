#include <stdint.h>

#pragma once

uint32_t getTotalTimeZones();

const char *getTZ(uint32_t i);

const char *getESPTZ(uint32_t i);

const char *getESPTZ(const char *tzname);
