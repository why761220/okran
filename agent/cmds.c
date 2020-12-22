#include "okran.h"


char* split(char* src, int size, const char* delims, int* ret_size, char** next, int* next_size) {
	int count = (int)strlen(delims);
	for (int i = 0; (i + count) <= size; i++) {
		if (memcmp(src + i, delims, count) == 0) {
			if (next != NULL) *next = src + i + count;
			if (next_size != NULL)*next_size = size - i - count;
			if (ret_size != NULL) *ret_size = i;
			return src;
		}
	}
	if (next != NULL) *next = NULL;
	if (next_size != NULL)*next_size = 0;
	if (ret_size != NULL) *ret_size = size;
	return src;
}

typedef struct AgentCmd {
	char* cmd;
	char* pid;
	char* id;
	char* name;
	char* jarFile;
	char* agentFile;
	char* app;
	char* className;
	char* body;
	struct {
		char* home;
	}wls;
	int size;
	int result;
	char error[128];
}AgentCmd;
typedef struct CmdContext {
	SOCKET sock;
}CmdContext;
typedef int (*CmdCallback)(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* octx, CmdContext* cctx, AgentCmd* cmd);
typedef void(*LogFunc)(void* ctx, const char* file, int line, const char* format, ...);
typedef void(*ConnectedCallback)(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* octx);
typedef void(*DisConnectedCallback)(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* octx);
int DecodeAgentCmd(AgentCmd* cmd, char* buf, int size) {
	char* key, * value;
	int klen, vlen;
	if (cmd == NULL || buf == NULL || size <= 0) return -1;
	memset(cmd, 0, sizeof(AgentCmd));
	while (buf != NULL) {
		key = split(buf, size, ";", &klen, &buf, &size);
		key = split(key, klen, "=", &klen, &value, &vlen);
		if (memcmp(key, "size", klen) == 0) {
			char t[41];
			int l = vlen > 40 ? 40 : vlen;
			memcpy(t, value, l);
			t[l] = 0;
			cmd->size = atoi(t);
		}
		else if (memcmp(key, "cmd", klen) == 0) {
			cmd->cmd = value;
			cmd->cmd[vlen] = 0;
		}
		else if (memcmp(key, "id", klen) == 0) {
			cmd->id = value;
			cmd->id[vlen] = 0;
		}
		else if (memcmp(key, "pid", klen) == 0) {
			cmd->pid = value;
			cmd->pid[vlen] = 0;
		}
		else if (memcmp(key, "name", klen) == 0) {
			cmd->name = value;
			cmd->name[vlen] = 0;
		}
		else if (memcmp(key, "jarFile", klen) == 0) {
			cmd->jarFile = value;
			cmd->jarFile[vlen] = 0;
		}
		else if (memcmp(key, "agentFile", klen) == 0) {
			cmd->agentFile = value;
			cmd->agentFile[vlen] = 0;
		}
		else if (memcmp(key, "app", klen) == 0) {
			cmd->app = value;
			cmd->app[vlen] = 0;
		}
		else if (memcmp(key, "className", klen) == 0) {
			cmd->className = value;
			cmd->className[vlen] = 0;
		}
	}
	return 0;
}
int EncodeAgentCmd(AgentCmd* cmd, char* buf, int size) {
	int i = 0;
	char* s;
	if ((i += snprintf(buf + i, (size_t)size - i, "{\"result\":%d", cmd->result)) > size) return -1;

	if ((i += snprintf(buf + i, (size_t)size - i, ",\"size\":%d", cmd->size)) > size) return -1;

	if ((s = cmd->cmd) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"cmd\":\"%s\"", s)) > size)return -1;
	}
	if ((s = cmd->id) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"id\":\"%s\"", s)) > size)return -1;
	}
	if ((s = cmd->pid) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"pid\":\"%s\"",s)) > size)return -1;
	}
	if ((s = cmd->name) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"name\":\"%s\"",s)) > size)return -1;
	}
	if ((s = cmd->jarFile) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"jarFile\":\"%s\"", s)) > size)return -1;
	}
	if ((s = cmd->agentFile) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"agentFile\":\"%s\"", s)) > size)return -1;
	}
	if ((s = cmd->app) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"app\":\"%s\"", s)) > size)return -1;
	}
	if ((s = cmd->className) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"className\":\"%s\"",s)) > size)return -1;
	}
	if ((s = cmd->wls.home) != NULL) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"wls.home\":\"%s\"", s)) > size)return -1;
	}
	if (strlen(cmd->error) > 0) {
		if ((i += snprintf(buf + i, (size_t)size - i, ",\"error\":\"%s\"",cmd->error)) > size)return -1;
	}

	if ((i += snprintf(buf + i, (size_t)size - i, "}")) > size)return -1;
	
	return i;
}

void AgentCmdHandler(jvmtiEnv* jvmti, JNIEnv* jni, void* ctx, const char* host, unsigned short port, CmdCallback cb, LogFunc log, ConnectedCallback connectEvent, DisConnectedCallback disConnectEvent) {
	int ret;
	char* buf = NULL;
	char* head,*body;
	int hsize, bsize;
	int len, size = 4096 * 1024;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in server;
	AgentCmd cmd;
	CmdContext cmdCtx;
	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_addr.s_addr = inet_addr(host);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
agent_to_proxy_malloc:
	if ((buf = (char*)malloc(size)) == NULL) {
		if (log != NULL)log(ctx, __FILE__, __LINE__, "malloc cmd buff failed!");
		sleep(10000);
		goto agent_to_proxy_malloc;
	}

agent_to_proxy_socket:
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		sleep(5000);
		if (log != NULL)log(ctx, __FILE__, __LINE__, "socket bind failed!");
		goto agent_to_proxy_socket;
	}
agent_to_proxy_connect:
	if ((ret = connect(sock, (struct sockaddr*)&server, sizeof(server))) != 0) {
		if (disConnectEvent != NULL) {
			disConnectEvent(jvmti, jni, ctx);
		}
		ret = GetLastError();
		if (log != NULL)log(ctx, __FILE__, __LINE__, "connect server failed for %s:%d code is %d!", host, port, ret);
		if (ret == 10061) {
			//目标机器没有开启，无法建立socket连接
			sleep(5000);
			goto agent_to_proxy_connect;
		}
		else {
			closesocket(sock);
			sock = INVALID_SOCKET;
			goto agent_to_proxy_socket;
		}
	}
	if (connectEvent != NULL) {
		connectEvent(jvmti, jni, ctx);
	}
	len = 0;
agent_to_proxy_recv_head:
	if ((ret = recv(sock, buf + len, size, 0)) <= 0) {
		closesocket(sock);
		sock = INVALID_SOCKET;
		goto agent_to_proxy_connect;
	}
	len += ret;
	head = split(buf, len, "\r\n\r\n", &hsize, &body, &bsize);
	if (body == NULL) {
		goto agent_to_proxy_recv_head;
	}
	else if (DecodeAgentCmd(&cmd, head, hsize) != 0) {
		len = 0;
		goto agent_to_proxy_recv_body;
	}
agent_to_proxy_recv_body:
	if (len >= size) {
		closesocket(sock);
		sock = INVALID_SOCKET;
		goto agent_to_proxy_socket;
	}
	else if (bsize < cmd.size) {
		if ((ret = recv(sock, buf + len, size, 0)) < 0) {
			closesocket(sock);
			sock = INVALID_SOCKET;
			goto agent_to_proxy_socket;
		}
		len += ret;
		bsize += ret;
		goto agent_to_proxy_recv_body;
	}
	cmdCtx.sock = sock;
	cmd.body = body;
	if ((ret = cb(jvmti, jni, ctx, &cmdCtx, &cmd)) != 0) {
		closesocket(sock);
		sock = INVALID_SOCKET;
		goto agent_to_proxy_socket;
	}
	len = 0;
	goto agent_to_proxy_recv_head;

}
int SendToProxy(CmdContext* ctx, AgentCmd* cmd) {
	int ret;
	int len = 0;
	int size = 0;
	char buf[1024];
	if ((size = EncodeAgentCmd(cmd, buf, sizeof(buf) - 1)) < 0) {
		return -1;
	}
	while (len < size) {
		if ((ret = send(ctx->sock, buf + len, size - len, 0)) < 0) {
			return ret;
		}
		else {
			len += ret;
		}
	}
	if (cmd->body == NULL || cmd->size < 0) return 0;
	len = 0;
	while (len < cmd->size) {
		if ((ret = send(ctx->sock, cmd->body + len, cmd->size - len, 0)) < 0) {
			return ret;
		}
		else {
			len += ret;
		}
	}
	return 0;
}

int GetErrorName(jvmtiEnv* jvmti, jvmtiError err, char* msg) {
	char* errName;
	if ((*jvmti)->GetErrorName(jvmti, err, &errName) == JVMTI_ERROR_NONE) {
		strcpy(msg, errName);
		(*jvmti)->Deallocate(jvmti, errName);
	}
	else {
		strcpy(msg, "unkown jvmti error!");
	}
	return 500;
}
int DefineClass(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* ctx, CmdContext* cmdCtx, AgentCmd* cmd) {
	jclass klass;
	jvmtiError err;
	if (cmd->className == NULL || cmd->body == NULL || cmd->size <= 0) {
		strcpy(cmd->error, "args for className is empty!");
		cmd->result = 500;
		cmd->size = 0;
	}
	else if ((klass=(*jni)->FindClass(jni,cmd->className)) == NULL) {
		if ((*jni)->DefineClass(jni, cmd->className, NULL, cmd->body, cmd->size) == NULL) {
			strcpy(cmd->error, "call DefineClass is error!");
			cmd->result = 500;
			cmd->size = 0;
		}
		else {
			cmd->result = 200;
			cmd->size = 0;
		}
	}
	else {
		jvmtiClassDefinition classes[1] ;
		classes[0].klass = klass;
		classes[0].class_bytes = cmd->body;
		classes[0].class_byte_count = cmd->size;
		if ((err = (*jvmti)->RedefineClasses(jvmti, 1, classes)) == JVMTI_ERROR_NONE) {
			cmd->result = 200;
			cmd->size = 0;
		}
		else {
			cmd->result = GetErrorName(jvmti, err, cmd->error);
			cmd->size = 0;
		}
	}
	 
	return SendToProxy(cmdCtx, cmd);
}
JNIEXPORT void JNICALL Java_TraceHelper_send(JNIEnv*, jclass, jstring);
int RedefineClass(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* ctx, CmdContext* cmdCtx, AgentCmd* cmd) {
	jvmtiError err;
	jclass klass;
	if (cmd->className == NULL) {
		strcpy(cmd->error, "args for className is empty!");
		cmd->result = 500;
		return SendToProxy(cmdCtx, cmd);
	}
	if ((klass = (*jni)->FindClass(jni, cmd->className)) == NULL) {
		strcpy(cmd->error, "class not found!");
		cmd->result = 404;
		return SendToProxy(cmdCtx, cmd);
	}
	jvmtiClassDefinition classes[1];
	classes[0].class_bytes = cmd->body;
	classes[0].class_byte_count = cmd->size;
	classes[0].klass = klass;
	if ((err = (*jvmti)->RedefineClasses(jvmti, 1, classes)) != JVMTI_ERROR_NONE) {
		cmd->result = GetErrorName(jvmti, err, cmd->error);
		return SendToProxy(cmdCtx, cmd);
	}
	if ((klass = (*jni)->FindClass(jni, cmd->className)) != NULL) {
		jmethodID jm = (*jni)->GetStaticMethodID(jni,klass, "$send", "(Ljava/lang/String;)V");
		if (jm != NULL) {
			JNINativeMethod methods[1];
			methods[0].fnPtr = Java_TraceHelper_send;
			methods[0].name = "$send";
			methods[0].signature = "(Ljava/lang/String;)V";
			if ((*jni)->RegisterNatives(jni, klass, methods, 1) == JNI_ERR) {
				LOG(ctx, "RegisterNatives for $send failed!");
			}
		}
	}
	cmd->result = 200;
	return SendToProxy(cmdCtx, cmd);
}
int RetransformClass(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* ctx, CmdContext* cmdCtx, AgentCmd* cmd) {
	jvmtiError err;
	int ret;
	jclass classes[1];

	if (cmd->className == NULL) {
		strcpy(cmd->error, "args for className is empty!");
		cmd->result = 500;
		return SendToProxy(cmdCtx, cmd);
	}

	if ((classes[0] = (*jni)->FindClass(jni, cmd->className)) == NULL) {
		strcpy(cmd->error, "class not found!");
		cmd->result = 404;
		return SendToProxy(cmdCtx, cmd);
	}
	ctx->className = cmd->className;
	ctx->classByteCount = 0;
	ctx->classBytes = NULL;
	if ((err = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, (jthread)NULL)) != JVMTI_ERROR_NONE) {
		cmd->result = GetErrorName(jvmti, err, cmd->error);
		return SendToProxy(cmdCtx, cmd);
	}

	if ((err = (*jvmti)->RetransformClasses(jvmti, 1, classes)) != JVMTI_ERROR_NONE) {
		cmd->result = GetErrorName(jvmti, err, cmd->error);
		return SendToProxy(cmdCtx, cmd);
	}
	if (ctx->classBytes != NULL) {
		cmd->size = ctx->classByteCount;
		cmd->body = ctx->classBytes;
		cmd->result = 200;
		ret = SendToProxy(cmdCtx, cmd);
		(*jvmti)->Deallocate(jvmti, ctx->classBytes);
		ctx->classBytes = NULL;
		ctx->classByteCount = 0;
		return ret;
	}
	else {
		strcpy(cmd->error, "unkown error!");
		cmd->result = 500;
		return SendToProxy(cmdCtx, cmd);
	}
}
int Heartbeat(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* ctx, CmdContext* cctx, AgentCmd* cmd) {
	cmd->result = 200;
	cmd->pid = ctx->args.pid;
	cmd->name = ctx->args.name;
	cmd->app = ctx->args.app;
	cmd->jarFile = ctx->args.jarFile;
	cmd->agentFile = ctx->args.agentFile;
	cmd->wls.home = ctx->args.wls.home;
	return SendToProxy(cctx, cmd);
}
int doCmd(jvmtiEnv* jvmti, JNIEnv* jni, OkranContext* ctx, CmdContext* cctx, AgentCmd* cmd) {
	if (cmd == NULL)return -1;
	if (cmd->cmd == NULL) {
		cmd->result = 500;
		sprintf(cmd->error, "unkown cmd!");
		return SendToProxy(cctx, cmd);
	}
	if (strcmp(cmd->cmd, "Heartbeat") == 0) {
		return Heartbeat(jvmti, jni, ctx, cctx, cmd);
	}
	else if (strcmp(cmd->cmd, "DefineClass") == 0) {
		return DefineClass(jvmti, jni, ctx, cctx, cmd);
	}
	else if (strcmp(cmd->cmd, "RedefineClass") == 0) {
		return RedefineClass(jvmti, jni, ctx, cctx, cmd);
	}
	else if (strcmp(cmd->cmd, "RetransformClass") == 0) {
		return RetransformClass(jvmti, jni, ctx, cctx, cmd);
	}
	else {
		cmd->result = 500;
		sprintf(cmd->error, "unkown cmd!");
		return SendToProxy(cctx, cmd);
	}
}
void CmdRecvThread(jvmtiEnv* jvmti, JNIEnv* jni, void* arg) {
	OkranContext* ctx = (OkranContext*)arg;
	if (jvmti == NULL || jni == NULL || ctx == NULL || ctx->args.host == NULL)return;
	AgentCmdHandler(jvmti, jni, ctx, ctx->args.host, ctx->args.port, doCmd, NULL, NULL, NULL);
}
