
#include "bytes.h"


int PushBytes(jvmtiEnv* jvmti, jrawMonitorID raw, BytesPool* pool, bytes* bs) {
	jvmtiError err;
	bytes* tail;

	if (jvmti == NULL || raw == NULL || pool == NULL || bs == NULL) return 0;

	for (tail = bs; tail->next != NULL; tail = tail->next);

	if ((err = (*jvmti)->RawMonitorEnter(jvmti, raw)) == JVMTI_ERROR_NONE) {
		if (pool->head == NULL) {
			pool->head = bs;
			pool->tail = tail;
		}
		else {
			pool->tail->next = bs;
			pool->tail = tail;
		}
		tail->next = NULL;
		(*jvmti)->RawMonitorNotify(jvmti, raw);
		(*jvmti)->RawMonitorExit(jvmti, raw);
		return 0;
	}
	else {
		return -1;
	}
}
bytes* AllocBytes(jvmtiEnv* jvmti, jrawMonitorID raw, BytesPool* pool,int*count_ptr,int max_count,int size) {
	jvmtiError err;
	bytes* prior,*ret = NULL;
	if ((err = (*jvmti)->RawMonitorEnter(jvmti, raw)) == JVMTI_ERROR_NONE) {
		for (prior = NULL,ret = pool->head; ret != NULL; prior = ret,ret = ret->next) {
			if (ret->size >= size) {
				//1.在头同时在尾，2.在头不在尾，3.在尾不在头，4.即不在头也不在尾，在中间
				if (pool->head == ret) pool->head = ret->next;
				if (pool->tail == ret) pool->tail = prior;
				if (prior != NULL) prior->next = ret->next;
				ret->next = NULL;
				break;
			}
		}
		if (ret == NULL && *count_ptr < max_count) {
			if ((err = (*jvmti)->Allocate(jvmti, (size_t)size + sizeof(bytes), (unsigned char**)&ret)) == JVMTI_ERROR_NONE) {
				ret->next = NULL;
				ret->len = 0;
				ret->size = size;
				(*count_ptr)++;
			}
			else {
				ret = NULL;
			}
		}
		(*jvmti)->RawMonitorExit(jvmti, raw);
	}
	return ret;
}
bytes* PopAllBytes(jvmtiEnv* jvmti, jrawMonitorID raw, BytesPool* pool) {
	jvmtiError err;
	bytes* ret = NULL;
	if ((err = (*jvmti)->RawMonitorEnter(jvmti, raw)) == JVMTI_ERROR_NONE) {
		if ((err = (*jvmti)->RawMonitorWait(jvmti, raw, 10000)) == JVMTI_ERROR_NONE) {
			ret = pool->head;
			pool->head = pool->tail = NULL;
		}
		(*jvmti)->RawMonitorExit(jvmti, raw);
	}
	return ret;
}

bytes* NewBytesJ(jvmtiEnv* jvmti, JNIEnv* jni, jrawMonitorID raw, BytesPool*pool,int*pcount,int pmax,jstring js) {

	const char* s;
	jsize len;
	bytes* bs = NULL;
	if ((len = (*jni)->GetStringUTFLength(jni, js)) <= 0) {
		return NULL;
	}
	if ((s = (*jni)->GetStringUTFChars(jni, js, NULL)) == NULL) {
		return NULL;
	}
	if ((bs = AllocBytes(jvmti, raw, pool, pcount, pmax, len)) != NULL) {
		memcpy(bs->bs, s, len);
		bs->bs[len] = 0;
		bs->len = bs->size = len;
	}
	(*jni)->ReleaseStringUTFChars(jni, js, s);
	return bs;
}
