
#include "okran.h"

/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class weblogic_servlet_internal_ServletStubImpl */
extern OkranContext* ctx;
#ifdef __cplusplus
extern "C" {
#endif
//examples for weblogic http servlet trace
#undef weblogic_servlet_internal_ServletStubImpl_serialVersionUID
#define weblogic_servlet_internal_ServletStubImpl_serialVersionUID -8933260145314767552i64
	void TraceForServletStubImpl(OkranContext* ctx, jvmtiEnv* jvmti, JNIEnv* jni, jlong t1, jlong t2,jobject ex, jobject req) {
		jclass clazz, sclazz, uclazz;
		jmethodID mid;
		const char* s;
		int size = 4096;
		jobject userInfo, session, ret;
		jlong timestamp = t2;
		jlong duration = t2 - t1;
		duration = duration <= 0 || duration == t2 ? 1 : duration;
		bytes* bs = AllocBytes(jvmti, ctx->fRawMonitor, &ctx->freePool, &ctx->bytesPoolCount, ctx->args.poolMaxSize, size);
		if (bs == NULL) return;
		bs = BytesPrintf(jvmti, bs, "{\"timestamp\":%ld,\"duration\":%ld,\"isError\":%d", timestamp, duration,(ex==NULL?0:1));
		if (bs == NULL) return;
		clazz = (*jni)->GetObjectClass(jni, req);
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getSession", "()Ljavax/servlet/http/HttpSession;")) != NULL) {
			if ((session = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((sclazz = (*jni)->GetObjectClass(jni, session)) != NULL) {
					if ((mid = (*jni)->GetMethodID(jni, sclazz, "getAttribute", "(Ljava/lang/String;)Ljava/lang/Object;")) != NULL) {
						if ((userInfo = (*jni)->CallObjectMethod(jni, session, mid, (*jni)->NewStringUTF(jni, "UserView"))) != NULL) {
							if ((uclazz = (*jni)->GetObjectClass(jni, userInfo)) != NULL) {
								if ((mid = (*jni)->GetMethodID(jni, uclazz, "getDlzh", "()Ljava/lang/String;")) != NULL) {
									if ((ret = (*jni)->CallObjectMethod(jni, userInfo, mid)) != NULL) {
										if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
											bs = BytesPrintf(jvmti, bs, ",\"userId\":\"%s\"", s);
											(*jni)->ReleaseStringUTFChars(jni, ret, s);
											if (bs == NULL) return;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getRequestURI", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"uri\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}

		if ((mid = (*jni)->GetMethodID(jni, clazz, "getMethod", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"method\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getServerName", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"serverName\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getServerPort", "()I")) != NULL) {
			bs = BytesPrintf(jvmti, bs, ",\"serverPort\":%d", (*jni)->CallIntMethod(jni, req, mid));
			if (bs == NULL) return;
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getLocalAddr", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"localAddr\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}

		if ((mid = (*jni)->GetMethodID(jni, clazz, "getLocalName", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"localName\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getLocalPort", "()I")) != NULL) {
			bs = BytesPrintf(jvmti, bs, ",\"localPort\":%d", (*jni)->CallIntMethod(jni, req, mid));
			if (bs == NULL) return;
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getRemoteAddr", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"remoteAddr\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getRemoteHost", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"remoteHost\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getRemotePort", "()I")) != NULL) {
			bs = BytesPrintf(jvmti, bs, ",\"remotePort\":%d", (*jni)->CallIntMethod(jni, req, mid));
			if (bs == NULL) return;
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getContextPath", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"contextPath\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getRequestedSessionId", "()Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid)) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"sessionId\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((mid = (*jni)->GetMethodID(jni, clazz, "getHeader", "(Ljava/lang/String;)Ljava/lang/String;")) != NULL) {
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid, (*jni)->NewStringUTF(jni, "Referer"))) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"referer\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
			if ((ret = (*jni)->CallObjectMethod(jni, req, mid, (*jni)->NewStringUTF(jni, "User-Agent"))) != NULL) {
				if ((s = (*jni)->GetStringUTFChars(jni, ret, NULL)) != NULL) {
					bs = BytesPrintf(jvmti, bs, ",\"userAgent\":\"%s\"", s);
					(*jni)->ReleaseStringUTFChars(jni, ret, s);
					if (bs == NULL) return;
				}
			}
		}
		if ((bs = BytesPrintf(jvmti, bs, "}")) == NULL) return;
		PushBytes(jvmti, ctx->sRawMonitor, &ctx->sendPool, bs);
	}

#ifdef __cplusplus
}
#endif
