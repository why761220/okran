#pragma once
#include<jni.h>
#include<jvmti.h>
#include <time.h>
#include "bytes.h"
#define HOST_SIZE 80
#define URI_SIZE 128
#ifdef _WINDOWS
#include <winsock.h>
#define sleep Sleep
#else
#endif
#ifdef __cplusplus
extern "C" {
#endif
	typedef struct OkranArgs {
		char *id;
		char* name;
		char* pid;
		char *log;
		char *host;
		char* auth;
		char* jarFile;
		char* agentFile;
		char* app;
		int port;
		int poolMaxSize;
		struct {
			char* host;
			char* uri;
			int port;
		}url;
		struct  {
			char* home;
		}wls;
	}OkranArgs;
	typedef struct ClassFile {
		int size;
		char name[FILENAME_MAX];
		char* bytecode;
		int max_size;
	}ClassFile;
	typedef struct OkranContext {
		jrawMonitorID rawMonitor;
		jthread traceThread;
		jthread cmdThread;
		FILE* logFile;
		char* className;
		char* classBytes;
		int classByteCount;
		BytesPool sendPool;
		jrawMonitorID sRawMonitor;
		BytesPool freePool;
		int bytesPoolCount;
		jrawMonitorID fRawMonitor;
		jvmtiCapabilities capabilities;
		OkranArgs args;
	}OkranContext;
	typedef struct cmd_thread_args {
		OkranContext* ctx;
		char req_buf[1204*1024*8];
		char resp_buf[1204 * 1024 * 8];
	}cmd_thread_args;

	int InitOkranContext(jvmtiEnv* jvmti,char*cmd_args, OkranContext* ctx);
	void log(OkranContext* ctx, const char* file, int line,const char* format,...);
	#define LOG(ctx,format,...) log(ctx,__FILE__,__LINE__,format,__VA_ARGS__)
	void jvmtiLog(OkranContext* ctx, jvmtiEnv* jvmti, jvmtiError err, const char* file, int line);
	#define LOG2(ctx,jvmti,err)jvmtiLog(ctx,jvmti,err,__FILE__,__LINE__)
	void jni_sleep(JNIEnv* jni, jlong millis);
	void CmdRecvThread(jvmtiEnv* jvmti, JNIEnv* jni, void* options);
	void TraceSendThread(jvmtiEnv* jvmti, JNIEnv* jni, void* arg);
	char* split(char* src, int size, const char* delims, int* ret_size, char** next, int* next_size);
	#define http_split split
	bytes* BytesPrintf(jvmtiEnv*jvmti, bytes* bs, const char* fmt, ...);
	int GetErrorName(jvmtiEnv* jvmti, jvmtiError err, char* msg);
#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */


