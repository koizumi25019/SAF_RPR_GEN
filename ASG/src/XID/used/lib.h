#pragma once
#include <limits.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>


#define EXIT_IF_FAIL(cond, err) if(!(cond)) { result = err; goto end;}
enum { EXIT_FAIL = 0, EXIT_FUNC_SUCCESS = 1 };

typedef size_t vsize_t;      
#define VSIZE_INVALID SIZE_MAX // size_tÇÃç≈ëÂílÇñ≥å¯ílÇ∆Ç∑ÇÈ

#define ALLOC_MEM(size)	allocMem(size, __FILE__, __func__, __LINE__)
#define ALLOC_CON(count, size) allocCon(count, size, __FILE__, __func__, __LINE__)

#define PRINTF(fmt, ...)	 printf_s(fmt, __VA_ARGS__)
#define ERROR_PRINTF(fmt, ...) printf("\x1b[31m" fmt "\x1b[0m", __VA_ARGS__) // ê‘

#ifdef _DEBUG
#define DEBUG_PRINTF(fmt, ...) printf("\x1b[32m"fmt"\033[0m", __VA_ARGS__)//óŒ
#else
#define DEBUG_PRINTF(fmt, ...) ((void)0)
#endif

#include <assert.h>
#define DEBUG_ASSERT(condition) assert(condition)


#define STRNCMP(a,R,b) (strncmp(a,b,strlen(b)) R 0)
#define FREE(x)     (free(x),(x)=NULL)
#define STRCMP(a,R,b) (strcmp(a,b) R 0)

typedef struct Queue Queue_t;

extern FILE* safe_fopen(const char* filename, const char* mode);

extern inline void* allocMem(size_t size, const char* file, const char* function, int line);
extern inline void* allocCon(size_t count, size_t size, const char* file, const char* function, int line);

extern Queue_t* createQueue(size_t capacity);
extern void destroyQueue(Queue_t* queue);
extern void enqueue(Queue_t* queue, void* item);
extern void* dequeue(Queue_t* queue);
extern int isQueueEmpty(Queue_t* queue);
