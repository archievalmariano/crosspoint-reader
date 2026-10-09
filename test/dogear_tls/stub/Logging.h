#pragma once
// Host stand-in for lib/Logging (the device logger needs Arduino).
#define LOG_ERR(tag, ...) ((void)0)
#define LOG_INF(tag, ...) ((void)0)
#define LOG_DBG(tag, ...) ((void)0)
