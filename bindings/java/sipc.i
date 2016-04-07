/*
 * Copyright (C) 2006 - 2008 Tresys Technology, LLC
 * Developed Under US JFCOM Sponsorship
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

%module sipcwrapper

%{

#include "../../include/sipc/sipc.h"
#include <string.h>
#include <errno.h>

%}

/* get the Java environment so we can throw exceptions */
%{
	static JNIEnv *sipc_global_jenv;
	jint JNI_OnLoad(JavaVM *vm, void *reserved) {
		(*vm)->AttachCurrentThread(vm, (void **)&sipc_global_jenv, NULL);
		return JNI_VERSION_1_2;
	}
	#define BEGIN_EXCEPTION JNIEnv *local_jenv = sipc_global_jenv; {
	#define END_EXCEPTION }
%}

%include stdint.i
%include exception.i

%typemap(jni) size_t "jlong"
%typemap(jtype) size_t "long"
%typemap(jstype) size_t "long"

%pragma(java) jniclasscode=%{
	static {
		System.loadLibrary("sipcwrapper");
	}
%}

/* SIPC roles */
#define SIPC_CREATOR 0
#define SIPC_SENDER 1
#define SIPC_RECEIVER 2

/* IPC types */
#define SIPC_SYSV_SHM 0
#define SIPC_SYSV_MQUEUES 1
#define SIPC_NUM_TYPES 2

/* SIPC behaviors, for sipc_ioctl() */
#define SIPC_BLOCK 0
#define SIPC_NOBLOCK 1

%exception {
	sipc_global_jenv = jenv;
	$action
}
%javaexception("java.lang.Exception") {
	sipc_global_jenv = jenv;
	$action
}

/* pass the new exception macro to C not just SWIG */
#undef SWIG_exception
#define SWIG_exception(code, msg) {SWIG_JavaException(local_jenv, code, msg); goto fail;}
%inline %{
	#undef SWIG_exception
	#define SWIG_exception(code, msg) {SWIG_JavaException(local_jenv, code, msg); goto fail;}
%}

%inline %{
	typedef struct libsipc {
		sipc_t *sipc;
		int ipc_type;
		size_t data_size;
	} libsipc_t;
%}

%extend libsipc_t {
	libsipc_t(const char *key, int role, int ipc_type, size_t size) {
		libsipc_t *l = NULL;
		BEGIN_EXCEPTION
		l = malloc(sizeof(libsipc_t));
		if (!l)
			SWIG_exception(SWIG_MemoryError, "Out of Memory");
		l->sipc = sipc_open(key, role, ipc_type, size);
		if (!(l->sipc)) {
			char *msg = strerror(errno);
			free(l);
			l = NULL;
			SWIG_exception(SWIG_RuntimeError, msg);
		}
		l->ipc_type = ipc_type;
		l->data_size = size;
		END_EXCEPTION
	fail:
		return l;
	};

	void ioctl(int request) {
		BEGIN_EXCEPTION
		if (sipc_ioctl(self->sipc, request) < 0) {
			SWIG_exception(SWIG_RuntimeError, strerror(errno));
		}
		END_EXCEPTION
	fail:
		return;
	};

	void send_data(size_t msg_len) {
		BEGIN_EXCEPTION
		if (sipc_send_data(self->sipc, msg_len) < 0) {
			SWIG_exception(SWIG_RuntimeError, strerror(errno));
		}
		END_EXCEPTION
	fail:
		return;
	};

	void shm_recv_done() {
		BEGIN_EXCEPTION
		if (sipc_shm_recv_done(self->sipc) < 0) {
			SWIG_exception(SWIG_RuntimeError, strerror(errno));
		}
		END_EXCEPTION
	fail:
		return;
	};
};

%extend libsipc_t {
	jbyteArray recv_data() {
		char *data = NULL;
		size_t len = 0;
		jobject jresult = NULL;
		BEGIN_EXCEPTION
		if (sipc_recv_data(self->sipc, &data, &len) < 0) {
			if (errno == EAGAIN) {
				return NULL;
			}
			SWIG_exception(SWIG_RuntimeError, strerror(errno));
		}
		jresult = (*sipc_global_jenv)->NewByteArray(sipc_global_jenv, len);
		(*sipc_global_jenv)->SetByteArrayRegion(sipc_global_jenv, jresult, 0, len, data);
		if (self->ipc_type == SIPC_SYSV_MQUEUES) {
			free(data);
		}
		END_EXCEPTION
	fail:
		return jresult;
	};
}

%apply char * { String };

%extend libsipc_t {
	%typemap(jni) char * "jobject"
	%typemap(jtype) char * "java.nio.ByteBuffer"
	%typemap(jstype) char * "java.nio.ByteBuffer"
	%typemap(out) char * {
		$result = (*jenv)->NewDirectByteBuffer(jenv, result, arg1->data_size);
	}
	%typemap(javaout) char * {
		return $jnicall;
	}
	char * get_data_ptr() {
		char *ptr = NULL;
		BEGIN_EXCEPTION
		ptr = sipc_get_data_ptr(self->sipc);
		if (!ptr) {
			SWIG_exception(SWIG_RuntimeError, strerror(errno));
		}
		END_EXCEPTION
	fail:
		return ptr;
	};
};

%exception;
%nojavaexception;

%apply String {char *};
%typemap(jni) char * "jstring"
%typemap(jtype) char * "String"
%typemap(jstype) char * "String"

void sipc_unlink(const char *key, int ipc_type);

%extend libsipc_t {
	void close() {
		if (self->sipc != NULL) {
			sipc_close(self->sipc);
			self->sipc = NULL;
		}
	}
	~libsipc_t() {
		/* do a self->close(), if this were C++ */
		libsipc_t_close(self);
		free(self);
	};
};
