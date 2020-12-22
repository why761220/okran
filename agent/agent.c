#include "okran.h"

OkranContext ctx = { NULL };

void JNICALL callbackVMInit(jvmtiEnv* jvmti, JNIEnv* jni, jthread thr) {


}

void JNICALL callbackVMDeath(jvmtiEnv* jvmti, JNIEnv* env) {

}


void JNICALL callbackClassFileLoadHook(jvmtiEnv* jvmti, JNIEnv* jni, jclass class_being_redefined, jobject loader, const char* name,
	jobject protection_domain, jint class_data_len, const unsigned char* class_data, jint* new_class_data_len,
	unsigned char** new_class_data) {
	jvmtiError err;
	if (ctx.className == NULL) {
		return;
	}
	if (strcmp(ctx.className, name) != 0) {
		return;
	}
	(*jvmti)->SetEventNotificationMode(jvmti, JVMTI_DISABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, (jthread)NULL);
	if ((err = (*jvmti)->Allocate(jvmti, class_data_len, &ctx.classBytes)) == JVMTI_ERROR_NONE) {
		memcpy(ctx.classBytes, class_data, class_data_len);
		ctx.classByteCount = class_data_len;
	}
}

void JNICALL callbackThreadStart(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread) {

}
void JNICALL callbackThreadEnd(jvmtiEnv* jvmti_env, JNIEnv* jni_env, jthread thread) {

}
void JNICALL callbackVMStart(jvmtiEnv* jvmti_env, JNIEnv* jni_env) {

}
void JNICALL callbackClassLoad(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jclass klass) {

}
void JNICALL callbackClassPrepare(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jclass klass) {
	unsigned char* name;
	(*jvmti)->GetSourceFileName(jvmti, klass, &name);
	if (strcmp(name, "ABC.java") == 0) {
		jvmtiError error;
		jint method_count_ptr;
		jmethodID* methods_ptr;
		error = (*jvmti)->GetClassMethods(jvmti, klass, &method_count_ptr, &methods_ptr);
		if (error != JNI_OK) {
			fprintf(stderr, "ERROR: Couldn't get JVMTI environment");
			return;
		}
		printf("%d", method_count_ptr);
		jlocation s, e;
		error = (*jvmti)->GetMethodLocation(jvmti, methods_ptr[1], &s, &e);
		if (error != JNI_OK) {
			fprintf(stderr, "ERROR: Couldn't get JVMTI environment");
			return;
		}
		else {

			error = (*jvmti)->SetBreakpoint(jvmti, methods_ptr[1], s);
			if (error != JNI_OK) {
				fprintf(stderr, "ERROR: Couldn't get JVMTI environment");
				return;
			}
		}
	}
	(*jvmti)->Deallocate(jvmti, name);
}
void JNICALL callbackBreakpoint(jvmtiEnv* jvmti, JNIEnv* jni, jthread thread, jmethodID method, jlocation location) {
	unsigned char* name;
	(*jvmti)->GetMethodName(jvmti, method, &name, NULL, NULL);
	(*jvmti)->Deallocate(jvmti, name);
	jint count;
	jvmtiLocalVariableEntry* tbl;
	jvmtiError error = (*jvmti)->GetLocalVariableTable(jvmti, method, &count, &tbl);
	if (error != JNI_OK) {
		fprintf(stderr, "ERROR: Couldn't get JVMTI environment");
		return;
	}
	for (int i = 0; i < count; i++) {
		(*jvmti)->Deallocate(jvmti, tbl[i].name);
		(*jvmti)->Deallocate(jvmti, tbl[i].signature);
		(*jvmti)->Deallocate(jvmti, tbl[i].generic_signature);
	}
	(*jvmti)->Deallocate(jvmti, (unsigned char*)tbl);
	jobject obj;
	error = (*jvmti)->GetLocalObject(jvmti, thread, 0, 1, &obj);
	if (error != JNI_OK) {
		fprintf(stderr, "ERROR: Couldn't get JVMTI environment");
		return;
	}
	jsize len = (*jni)->GetStringLength(jni, obj);
	jboolean isCopy;
	const char* v = (*jni)->GetStringUTFChars(jni, obj, &isCopy);
	(*jni)->ReleaseStringUTFChars(jni, obj, v);

}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM* vm, char* options, void* reserved) {

	jvmtiEnv* jvmti = NULL;
	jvmtiError error;
	error = (jvmtiError)(*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_2);
	if (error != JNI_OK) {
		fprintf(stderr, "ERROR: Couldn't get JVMTI environment");
		return JNI_ERR;
	}

	jvmtiCapabilities capabilities;
	memset(&capabilities, 0, sizeof(jvmtiCapabilities));
	error = (*jvmti)->GetPotentialCapabilities(jvmti, &capabilities);
	if (error != JVMTI_ERROR_NONE) {
		return error;
	}
	error = (*jvmti)->AddCapabilities(jvmti, &capabilities);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, "ERROR: Unable to AddCapabilities JVMTI");
		return  error;
	}

	jvmtiEventCallbacks callbacks;
	memset((void*)&callbacks, 0, sizeof(jvmtiEventCallbacks));
	callbacks.ClassFileLoadHook = &callbackClassFileLoadHook;
	callbacks.ThreadStart = &callbackThreadStart;
	callbacks.ThreadEnd = &callbackThreadEnd;
	callbacks.VMInit = &callbackVMInit;
	callbacks.VMDeath = &callbackVMDeath;
	callbacks.VMStart = &callbackVMStart;
	callbacks.ClassLoad = &callbackClassLoad;
	callbacks.ClassPrepare = &callbackClassPrepare;
	callbacks.Breakpoint = &callbackBreakpoint;
	error = (jvmtiError)(*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks));
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, "ERROR: Unable to SetEventCallbacks JVMTI!");
		return error;
	}

	//error = (*jvmti)->SetEventNotificationMode(jvmti,JVMTI_ENABLE, JVMTI_EVENT_NATIVE_METHOD_BIND, (jthread)NULL);
	//if (error != JVMTI_ERROR_NONE) {
	//	fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
	//	return  error;
	//}
	/*
	//设置事件通知


	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_START, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}
	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_THREAD_END, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}
	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}

	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}

	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}

	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}
	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}
	error = jvmti->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY, (jthread)NULL);
	if (error != JVMTI_ERROR_NONE) {
		fprintf(stderr, " ERROR: Unable to SetEventNotificationMode JVMTI!");
		return  error;
	}
	*/
	//OkranInit(options);
	return JNI_OK;
}




JNIEXPORT jint JNICALL Agent_OnAttach(JavaVM* vm, char* options, void* reserved) {
	jvmtiEnv* jvmti = NULL;
	JNIEnv* jni = NULL;
	jvmtiError err;
	jclass clazz;
	jmethodID jm;

	if ((err = (jvmtiError)(*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_2)) != JNI_OK) {
		return JNI_ERR;
	}
	if ((err = (jvmtiError)(*vm)->GetEnv(vm, (void**)&jni, JNI_VERSION_1_4)) != JNI_OK) {
		return JNI_ERR;
	}
	if (InitOkranContext(jvmti, options, &ctx) != 0) {
		return JNI_ERR;
	}

	memset(&ctx.capabilities, 0, sizeof(jvmtiCapabilities));
	if ((err = (*jvmti)->GetPotentialCapabilities(jvmti, &ctx.capabilities)) != JVMTI_ERROR_NONE) {
		LOG2(&ctx, jvmti, err);
		return err;
	}
	if (((*jvmti)->AddCapabilities(jvmti, &ctx.capabilities)) != JVMTI_ERROR_NONE) {
		LOG2(&ctx, jvmti, err);
		return err;
	}
	jvmtiEventCallbacks callbacks;
	memset((void*)&callbacks, 0, sizeof(jvmtiEventCallbacks));
	callbacks.ClassFileLoadHook = &callbackClassFileLoadHook;
	if ((err = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks))) != JVMTI_ERROR_NONE) {
		LOG2(&ctx, jvmti, err);
		return err;
	}

	if ((clazz = (*jni)->FindClass(jni, "Ljava/lang/Thread;")) != NULL) {
		if ((jm = (*jni)->GetMethodID(jni, clazz, "<init>", "()V")) != NULL) {
			if ((ctx.cmdThread = (*jni)->NewObject(jni, clazz, jm)) != NULL) {
				if ((err = (*jvmti)->RunAgentThread(jvmti, ctx.cmdThread, CmdRecvThread, &ctx, JVMTI_THREAD_MIN_PRIORITY)) != JVMTI_ERROR_NONE) {
					LOG2(&ctx, jvmti, err);
					return JNI_ERR;;
				}
			}
			else {
				LOG(&ctx, "create cmd thread failed!");
				return JNI_ERR;
			}

			if ((ctx.traceThread = (*jni)->NewObject(jni, clazz, jm)) != NULL) {
				if ((err = (*jvmti)->RunAgentThread(jvmti, ctx.traceThread, TraceSendThread, &ctx, JVMTI_THREAD_MIN_PRIORITY)) != JVMTI_ERROR_NONE) {
					LOG2(&ctx, jvmti, err);
					return JNI_ERR;
				}
				else {
					return JNI_OK;
				}
			}
			else {
				LOG(&ctx, "create send thread failed!");
				return JNI_ERR;
			}
		}
		else {
			LOG(&ctx, "GetMethodID for Thread <init> failed!");
			return JNI_ERR;
		}
	}
	else {
		LOG(&ctx, "create send thread failed!");
		return JNI_ERR;
	}
}


JNIEXPORT void JNICALL
Agent_OnUnload(JavaVM* vm) {

}

/*
 * Class:     TraceHelper
 * Method:    trace
 * Signature: (IJJLjava/lang/Object;Ljava/lang/Object;[Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_TraceHelper_trace(JNIEnv* jni, jclass helper, jint kind, jlong t1, jlong t2, jobject dst, jobject ex, jobjectArray args) {
	
}

/*
 * Class:     TraceHelper
 * Method:    send
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_TraceHelper_send(JNIEnv* jni, jclass helper, jstring data) {
	JavaVM* vm;
	jvmtiEnv* jvmti;
	if (data == NULL)return;
	int len;
	if ((*jni)->GetJavaVM(jni, &vm) == JNI_ERR) {
		LOG(&ctx, "GetJavaVM failed!");
		return;
	}
	if ((*vm)->GetEnv(vm, (void**)&jvmti, JVMTI_VERSION_1_2) != JNI_OK) {
		LOG(&ctx, "GetEnv for JVMTI_VERSION_1_2 is failed!");
		return;
	}

	if ((len = (int)(*jni)->GetStringUTFLength(jni, data)) <= 0) {
		return;
	}
	bytes* bs = AllocBytes(jvmti, ctx.fRawMonitor, &ctx.freePool, &ctx.bytesPoolCount, ctx.args.poolMaxSize, len);
	if (bs == NULL) {
		return;
	}
	const char* s = (*jni)->GetStringUTFChars(jni, data, NULL);
	if (s == NULL) {
		return;
	}
	memcpy(bs->bs, s, len);
	bs->bs[len] = 0;
	bs->len = len;
	(*jni)->ReleaseStringUTFChars(jni, data, s);
	PushBytes(jvmti, ctx.sRawMonitor, &ctx.sendPool, bs);
}
