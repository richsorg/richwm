#pragma once

#define ERROR(fmt, ...) printf("\x1b[91mERROR\x1b[0m: " fmt, ##__VA_ARGS__)
#define WARN(fmt, ...) printf("\x1b[93mWARN\x1b[0m: " fmt, ##__VA_ARGS__)
#define INFO(fmt, ...) printf("\x1b[92mINFO\x1b[0m: " fmt, ##__VA_ARGS__)

#define DIE(fmt, ...) 										\
	printf("\x1b[91mERROR\x1b[0m: " fmt, ##__VA_ARGS__);		\
	exit(1);
