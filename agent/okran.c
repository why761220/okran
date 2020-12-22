
#include "okran.h"


void freeArgs(OkranArgs* args);
int parseArgs(jvmtiEnv* jvmti, char* s, OkranArgs* args);

int InitOkranContext(jvmtiEnv* jvmti, char* cmd_args, OkranContext* ctx) {
	int ret;
	jvmtiError err;
	jrawMonitorID rawMonitor, sRawMonitor, fRawMonitor;
	memset(ctx, 0, sizeof(OkranContext));
	if ((ret = (*jvmti)->RawMonitorEnter(jvmti, ctx->rawMonitor)) == JVMTI_ERROR_NONE) {
		(*jvmti)->RawMonitorExit(jvmti, ctx->rawMonitor);
		return 0;
	}
	if ((ret = parseArgs(jvmti, cmd_args, &(ctx->args))) != 0) {
		return ret;
	}
	if ((err = (*jvmti)->CreateRawMonitor(jvmti, "okranMonitor", &rawMonitor)) != JVMTI_ERROR_NONE) {
		return -1;
	}
	if ((err = (*jvmti)->CreateRawMonitor(jvmti, "sendMonitor", &sRawMonitor)) != JVMTI_ERROR_NONE) {
		(*jvmti)->DestroyRawMonitor(jvmti, rawMonitor);
		return -1;
	}
	if ((err = (*jvmti)->CreateRawMonitor(jvmti, "freeMonitor", &fRawMonitor)) != JVMTI_ERROR_NONE) {
		(*jvmti)->DestroyRawMonitor(jvmti, rawMonitor);
		(*jvmti)->DestroyRawMonitor(jvmti, sRawMonitor);
		return -1;
	}

	ctx->rawMonitor = rawMonitor;
	ctx->sRawMonitor = sRawMonitor;
	ctx->fRawMonitor = fRawMonitor;
	if (ctx->args.log != NULL && ctx->args.pid != NULL) {
		char fileName[FILENAME_MAX];
		snprintf(fileName, FILENAME_MAX, "%s/%s.log", ctx->args.log, ctx->args.pid);
#ifdef _WINDOWS
		for (char* s = fileName; *s != 0; s++) {
			if (*s == '/') *s = '\\';
		}
#endif
		ctx->logFile = fopen(fileName, "a+");
	}
	else {
		ctx->logFile = NULL;
	}

#ifdef _WINDOWS
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0) {
		LOG(ctx,"socket init error code:%d!", GetLastError());
		return -1;
	}
#endif
	return 0;
}
void freeArgs(OkranArgs* args) {
	if (args->id != NULL) {
		free(args->id);
		args->id = NULL;
	}
	if (args->pid != NULL) {
		free(args->pid);
		args->pid = NULL;
	}
	if (args->name != NULL) {
		free(args->name);
		args->name = NULL;
	}
	if (args->host != NULL) {
		free(args->host);
		args->host = NULL;
	}
	if (args->auth != NULL) {
		free(args->auth);
		args->auth = NULL;
	}
	if (args->log != NULL) {
		free(args->log);
		args->log = NULL;
	}
	if (args->app != NULL) {
		free(args->app);
		args->app = NULL;
	}
	if (args->jarFile != NULL) {
		free(args->jarFile);
		args->jarFile = NULL;
	}
	if (args->agentFile != NULL) {
		free(args->agentFile);
		args->agentFile = NULL;
	}
	if (args->url.host != NULL) {
		free(args->url.host);
		args->url.host = NULL;
	}
	if (args->url.uri != NULL) {
		free(args->url.uri);
		args->url.uri = NULL;
	}
	if (args->wls.home != NULL) {
		free(args->wls.home);
		args->wls.home = NULL;
	}
}
int parseArgs(jvmtiEnv* jvmti, char* s, OkranArgs* args) {
	int len, ks, vs;
	char* k, * v;
	char tmp[40];
	memset(args, 0, sizeof(OkranArgs));
	len = (int)strlen(s);
	while (s != NULL) {
		k = split(s, len, ";", &ks, &s, &len);
		k = split(k, ks, "=", &ks, &v, &vs);
		if (k != NULL && v != NULL) {
			if (memcmp(k, "id", ks) == 0) {
				if ((args->id = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->id, v, vs);
					args->id[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "pid", ks) == 0) {
				if ((args->pid = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->pid, v, vs);
					args->pid[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "name", ks) == 0) {
				if ((args->name = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->name, v, vs);
					args->name[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "app", ks) == 0) {
				if ((args->app = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->app, v, vs);
					args->app[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "log", ks) == 0) {
				if (vs >= FILENAME_MAX) {
					freeArgs(args);
					return -1;
				}
				else if ((args->log = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->log, v, vs);
					args->log[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "port", ks) == 0) {
				if (vs >= sizeof(tmp)) {
					freeArgs(args);
					return -1;
				}
				memcpy(tmp, v, vs);
				tmp[vs] = 0;
				args->port = atoi(tmp);
			}
			else if (memcmp(k, "host", ks) == 0) {
				if ((args->host = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->host, v, vs);
					args->host[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "auth", ks) == 0) {
				if ((args->auth = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->auth, v, vs);
					args->auth[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "pool.maxsize", ks) == 0) {
				if (vs >= sizeof(tmp)) {
					freeArgs(args);
					return -1;
				}
				memcpy(tmp, v, vs);
				tmp[vs] = 0;
				args->poolMaxSize = atoi(tmp);
			}
			else if (memcmp(k, "url.host", ks) == 0) {
				if ((args->url.host = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->url.host, v, vs);
					args->url.host[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "url.uri", ks) == 0) {
				if ((args->url.uri = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->url.uri, v, vs);
					args->url.uri[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "url.port", ks) == 0) {
				if (vs >= sizeof(tmp)) {
					freeArgs(args);
					return -1;
				}
				memcpy(tmp, v, vs);
				tmp[vs] = 0;
				args->url.port = atoi(tmp);
			}
			else if (memcmp(k, "wls.home", ks) == 0) {
				if ((args->wls.home = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->wls.home, v, vs);
					args->wls.home[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "wls.home", ks) == 0) {
				if ((args->wls.home = malloc((size_t)vs + 1)) != NULL) {
					memcpy(args->wls.home, v, vs);
					args->wls.home[vs] = 0;
				}
				else {
					freeArgs(args);
					return -1;
				}
			}
			else if (memcmp(k, "jarFile", ks) == 0) {
			if ((args->jarFile = malloc((size_t)vs + 1)) != NULL) {
				memcpy(args->jarFile, v, vs);
				args->jarFile[vs] = 0;
			}
			else {
				freeArgs(args);
				return -1;
			}
			}
			else if (memcmp(k, "agentFile", ks) == 0) {
			if ((args->agentFile = malloc((size_t)vs + 1)) != NULL) {
				memcpy(args->agentFile, v, vs);
				args->agentFile[vs] = 0;
			}
			else {
				freeArgs(args);
				return -1;
			}
			}
		}
	}
	if (args->id == 0 || args->host == 0 || args->name == 0 || args->pid == 0 || args->port == 0) {
		freeArgs(args);
		return -1;
	}
	for (char* s = args->jarFile; s != 0 && *s != 0; s++) {
		if (*s == '\\')*s = '/';
	}
	for (char* s = args->agentFile; s != 0 && *s != 0; s++) {
		if (*s == '\\')*s = '/';
	}
	for (char* s = args->log; s != 0 && *s != 0; s++) {
		if (*s == '\\')*s = '/';
	}
	for (char* s = args->wls.home; s != 0 && *s != 0; s++) {
		if (*s == '\\')*s = '/';
	}
	return 0;
}


void log(OkranContext* ctx, const char* file, int line,  const char* format,...) {
#ifdef _WINDOWS
#define FILENAME(x) strrchr(x, '\\') ? strrchr(x, '\\') + 1 : x
#else
#define FILENAME(x) strrchr(x, '/') ? strrchr(x, '/') + 1 : x
#endif
	if (ctx == NULL || ctx->logFile == NULL) {
		return;
	}
	va_list ap;
	va_start(ap, format);
	fprintf(ctx->logFile, "%sT%s[%s:%d]", __DATE__, __TIME__, FILENAME(file), line);
	vfprintf(ctx->logFile, format, ap);
	fprintf(ctx->logFile, "\r\n");
	fflush(ctx->logFile);
	va_end(ap);
}

void jni_sleep(JNIEnv* jni, jlong millis) {
	jclass c;
	jmethodID m;
	if ((c = (*jni)->FindClass(jni, "Ljava/lang/Thread;")) != NULL) {
		if ((m = (*jni)->GetStaticMethodID(jni, c, "sleep", "(J)V")) != NULL) {
			(*jni)->CallStaticVoidMethod(jni, c, m, millis);
		}
	}
}



bytes* BytesPrintf(jvmtiEnv*jvmti, bytes* bs, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	while (1) {
		int len = snprintf(bs->bs + bs->len, (size_t)bs->size - bs->len, fmt, *args);
		if ((len = len + bs->len) > bs->size) {
			bytes* t;
			jvmtiError err;
			if ((err = (*jvmti)->Allocate(jvmti, len + sizeof(bytes), (unsigned char**)&t)) != JVMTI_ERROR_NONE) {
				(*jvmti)->Deallocate(jvmti, (unsigned char*)bs);
				bs = NULL;
				break;
			}
			memcpy(t->bs, bs->bs, bs->len);
			t->len = bs->len;
			t->size = len;
			(*jvmti)->Deallocate(jvmti, (unsigned char*)bs);
		}
		else {
			break;
		}
	}
	va_end(args);
	return bs;
}

void jvmtiLog(OkranContext*ctx,jvmtiEnv* jvmti, jvmtiError err, const char* file, int line) {
	char* errName;
	if ((*jvmti)->GetErrorName(jvmti, err, &errName) == JVMTI_ERROR_NONE) {
		log(ctx, file, line, errName);
		(*jvmti)->Deallocate(jvmti, errName);
	}
	else {
		log(ctx, file, line, "unkown jvmti error!");
	}
}


