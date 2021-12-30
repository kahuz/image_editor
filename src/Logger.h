#ifndef __LOGGER_H__
#define __LOGGER_H__

#define DEBUG 1

#if DEBUG
#ifdef _WIN32
#define Log( fmt, ...) printf("[%s : %d] " fmt "\n", __func__, __LINE__, __VA_ARGS__) 
#define Err( fmt, ...) printf("[Error ! %s : %d ]" fmt "\n", __func__, __LINE__, __VA_ARGS__)
#else
#define Log( fmt, args...) printf("[%s : %d] " fmt "\n", __func__, __LINE__, ##args) 
#define Err( fmt, args...) printf("[Error ! %s : %d ]" fmt "\n", __func__, __LINE__, ##args)
#endif
#else
#define Log( fmt, args)
#define Err( fmt, args)
#endif	// #if DEBUG

#endif // #ifndef __LOGGER_H__
