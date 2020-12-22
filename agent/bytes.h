#pragma once
#include <jvmti.h>
#include<memory.h>
typedef struct bytes bytes;
typedef struct bytes {
	bytes* next;
	int len;
	int size;
	char bs[1];
}bytes;
typedef struct BytesPool {
	bytes* head;
	bytes* tail;
}BytesPool;
bytes* PopAllBytes(jvmtiEnv* jvmti, jrawMonitorID raw, BytesPool* pool);
int PushBytes(jvmtiEnv* jvmti, jrawMonitorID raw, BytesPool* pool, bytes* bs);
bytes* AllocBytes(jvmtiEnv* jvmti, jrawMonitorID raw, BytesPool* pool, int* count_ptr, int max_count, int size);
