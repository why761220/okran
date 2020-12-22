#include "okran.h"

void TraceSendThread(jvmtiEnv* jvmti, JNIEnv* jni, void* arg) {
	int ret, len;
	char head[256] = { 0 }, *pid = NULL, *host = NULL, *uri = NULL;
	unsigned short port = 0;
	bytes* list, *bs;
	int changed = 0;
	int retry = 0, total = 0;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in server;
	OkranContext* ctx = (OkranContext*)arg;
	if (ctx == NULL)return;
	
	while (1) {
		{//检查变量是否发生变化
			changed = 0;
			(*jvmti)->RawMonitorEnter(jvmti, ctx->rawMonitor);
			if (host == NULL || uri == NULL || pid == NULL || strcmp(host, ctx->args.url.host) != 0 || strcmp(uri,ctx->args.url.uri) !=0 || port != ctx->args.url.port) {
				changed = 1;
				if (host != NULL) {
					free(host);
					host = NULL;
				}
				if ((host = malloc(strlen(ctx->args.url.host) + 1)) != NULL) {
					strcpy(host, ctx->args.url.host);
				}
				if (uri != NULL) {
					free(uri);
					uri = NULL;
				}
				if ((uri = malloc(strlen(ctx->args.url.uri) + 1)) != NULL) {
					strcpy(uri, ctx->args.url.uri);
				}
				if (pid != NULL) {
					free(pid);
					pid = NULL;
				}
				if ((pid = malloc(strlen(ctx->args.pid) + 1)) != NULL) {
					strcpy(pid, ctx->args.pid);
				}
				port = ctx->args.url.port;
			}
			(*jvmti)->RawMonitorExit(jvmti, ctx->rawMonitor);
			if (host == NULL || uri == NULL || pid == NULL) {
				LOG(ctx, "malloc(pid|host|uri) is failed!");
				sleep(10000);
				continue;
			}
			if (changed) {
				memset(&server, 0, sizeof(struct sockaddr_in));
				server.sin_addr.s_addr = inet_addr(host);
				server.sin_family = AF_INET;
				server.sin_port = htons(port);
				if (sock != INVALID_SOCKET) {
					closesocket(sock);
					sock = INVALID_SOCKET;
				}
			}
		}
		list = PopAllBytes(jvmti, ctx->sRawMonitor, &ctx->sendPool);
		for (bs = list; bs != NULL; bs = bs->next) {
			len = snprintf(head, sizeof(head), "POST %s HTTP/1.1\r\nHost: %s\r\nPID: %s\r\ncontent-length: %d\r\n\r\n", uri, host,pid, bs->len);
			if (len > sizeof(head)) {
				continue;
			}
			retry = 0;
trace_send_thread_retry:
			if (sock == INVALID_SOCKET) {
				if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
					LOG(ctx,"socket create error code:%d!", GetLastError());
					if ((retry++) < 3) {
						goto trace_send_thread_retry;
					}
					else {
						continue;
					}	
				}
				if ((ret = connect(sock, (struct sockaddr*)&server, sizeof(server)))!= 0) {
					closesocket(sock);
					sock = INVALID_SOCKET; 
					if ((retry++) < 3) {
						goto trace_send_thread_retry;
					}
					else {
						continue;
					}
				}
			}
			if ((ret = send(sock, head, len, 0)) != len) {
				LOG(ctx, "send data failed for %s:%d code is %d!", host, port, GetLastError());
				closesocket(sock);
				sock = INVALID_SOCKET;
				if ((retry++) < 3) {
					goto trace_send_thread_retry;
				}
				else {
					continue;
				}
			}
			if ((ret = send(sock, bs->bs, bs->len, 0)) != bs->len) {
				closesocket(sock);
				sock = INVALID_SOCKET;
				if ((retry++) < 3) {
					goto trace_send_thread_retry;
				}
				else {
					continue;
				}
			}
			if (total > 100) {
				total = 0;
				closesocket(sock);
				sock = INVALID_SOCKET;
			}
			else {
				total++;
			}
		}
		PushBytes(jvmti, ctx->fRawMonitor, &ctx->freePool, list);
	}
}