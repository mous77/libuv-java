/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

#include <assert.h>
#include <time.h>
#include <string.h>

#include "uv.h"
#include "throw.h"
#include "net_java_libuv_handles_Files.h"

#ifdef __MACOS__
#include <sys/fcntl.h>
#endif

#ifdef _WIN32
#include <io.h>
#include <Shlwapi.h>
#include <tchar.h>
#endif

class FileRequest {
private:
  jlong _callback_ptr;
  jint _callback_id;
  jlong _read_buffer_offset;
  jobject _read_buffer;
  jbyte* _byte_array;
  JNIEnv* _env;

public:
  FileRequest(JNIEnv *env, jlong ptr, jint id, jobject read_buffer, jlong byte_array_size, jlong offset);
  ~FileRequest();

  jlong get_callback_ptr() { return _callback_ptr; }

  jint get_callback_id() { return _callback_id; }

  jobject get_read_buffer() { return _read_buffer; }

  jlong get_read_buffer_offset() { return _read_buffer_offset; }

  jbyte* get_byte_array() { return _byte_array; }

  jobject get_array_region_from_bytes(jsize bytes);

  jbyte* get_bytes_from_array_region(jbyteArray data, jsize length, jsize offset);

};

FileRequest::FileRequest(JNIEnv *env, jlong ptr, jint id, jobject read_buffer, jlong byte_array_size, jlong offset) {
  _env = env;
  _callback_ptr = ptr;
  _callback_id = id;
  _read_buffer_offset = offset;
  _read_buffer = NULL;
  _byte_array = NULL;

  if (byte_array_size > 0) {
    _byte_array = new jbyte[byte_array_size];
  }

  if (read_buffer) {
    _read_buffer = read_buffer;
    _read_buffer = env->NewGlobalRef(_read_buffer);
  }
}

FileRequest::~FileRequest() {
  if (_byte_array) {
    delete _byte_array;
  }

  if (_read_buffer) {
    _env->DeleteGlobalRef(_read_buffer);
  }
}

jobject FileRequest::get_array_region_from_bytes(jsize len) {
  _env->SetByteArrayRegion(static_cast<jbyteArray>(_read_buffer), static_cast<jsize>(_read_buffer_offset),  len, _byte_array);
  return _read_buffer;
}

jbyte* FileRequest::get_bytes_from_array_region(jbyteArray data, jsize length, jsize offset) {
  _env->GetByteArrayRegion(data, offset, length, _byte_array);
  return _byte_array;
}

class FileCallbacks {
private:
  static jclass _int_cid;
  static jclass _long_cid;
  static jclass _file_handle_cid;
  static jclass _stats_cid;

  static jmethodID _int_valueof_mid;
  static jmethodID _long_valueof_mid;
  static jmethodID _callback_1arg_mid;
  static jmethodID _callback_narg_mid;
  static jmethodID _stats_init_mid;

  static JNIEnv* _env;

  jobject _instance;

public:

  static jclass _object_cid;

  static void static_initialize(JNIEnv *env, jclass cls);
  static jobject build_stats(uv_statbuf_t *ptr);

  FileCallbacks();
  ~FileCallbacks();

  void initialize(jobject instance);
  void fs_cb(FileRequest *request, uv_fs_type fs_type, ssize_t result, void *ptr);
  void fs_cb(FileRequest *request, uv_fs_type fs_type, int errorno, const char *path);
};

jclass FileCallbacks::_int_cid = NULL;
jclass FileCallbacks::_long_cid = NULL;
jclass FileCallbacks::_file_handle_cid = NULL;
jclass FileCallbacks::_object_cid = NULL;
jclass FileCallbacks::_stats_cid = NULL;

jmethodID FileCallbacks::_int_valueof_mid = NULL;
jmethodID FileCallbacks::_long_valueof_mid = NULL;
jmethodID FileCallbacks::_callback_1arg_mid = NULL;
jmethodID FileCallbacks::_callback_narg_mid = NULL;
jmethodID FileCallbacks::_stats_init_mid = NULL;

JNIEnv* FileCallbacks::_env = NULL;

void FileCallbacks::static_initialize(JNIEnv* env, jclass cls) {
  _env = env;
  assert(_env);

  _int_cid = env->FindClass("java/lang/Integer");
  assert(_int_cid);
  _int_cid = (jclass) env->NewGlobalRef(_int_cid);
  assert(_int_cid);

  _long_cid = env->FindClass("java/lang/Long");
  assert(_long_cid);
  _long_cid = (jclass) env->NewGlobalRef(_long_cid);
  assert(_long_cid);

  _object_cid = env->FindClass("java/lang/Object");
  assert(_object_cid);
  _object_cid = (jclass) env->NewGlobalRef(_object_cid);
  assert(_object_cid);

  _stats_cid = env->FindClass("net/java/libuv/handles/Stats");
  assert(_stats_cid);
  _stats_cid = (jclass) env->NewGlobalRef(_stats_cid);
  assert(_stats_cid);

  _int_valueof_mid = env->GetStaticMethodID(_int_cid, "valueOf", "(I)Ljava/lang/Integer;");
  assert(_int_valueof_mid);

  _long_valueof_mid = env->GetStaticMethodID(_long_cid, "valueOf", "(J)Ljava/lang/Long;");
  assert(_long_valueof_mid);

  _file_handle_cid = (jclass) env->NewGlobalRef(cls);
  assert(_file_handle_cid);

  _callback_1arg_mid = env->GetMethodID(_file_handle_cid, "callback", "(IILjava/lang/Object;)V");
  assert(_callback_1arg_mid);
  _callback_narg_mid = env->GetMethodID(_file_handle_cid, "callback", "(II[Ljava/lang/Object;)V");
  assert(_callback_narg_mid);

  _stats_init_mid = env->GetMethodID(_stats_cid, "<init>", "(IIIIIIIJIJJJJ)V");
  assert(_stats_init_mid);
}

jobject FileCallbacks::build_stats(uv_statbuf_t *ptr) {
  if (ptr) {
    int blksize = 0;
    jlong blocks = 0;
#ifdef __POSIX__
    blksize = ptr->st_blksize;
    blocks = ptr->st_blocks;
#endif

    return _env->NewObject(_stats_cid,
      _stats_init_mid,
      ptr->st_dev,
      ptr->st_ino,
      ptr->st_mode,
      ptr->st_nlink,
      ptr->st_uid,
      ptr->st_gid,
      ptr->st_rdev,
      ptr->st_size,
      blksize,
      blocks,
      ptr->st_atime * 1000,     // Convert seconds to milliseconds
      ptr->st_mtime * 1000,     // Convert seconds to milliseconds
      ptr->st_ctime * 1000);    // Convert seconds to milliseconds
  }
  return NULL;
}

FileCallbacks::FileCallbacks() {
}

FileCallbacks::~FileCallbacks() {
  _env->DeleteGlobalRef(_instance);
}

void FileCallbacks::initialize(jobject instance) {

  assert(_env);
  assert(instance);

  _instance = _env->NewGlobalRef(instance);
}

void FileCallbacks::fs_cb(FileRequest *request, uv_fs_type fs_type, ssize_t result, void *ptr) {

  assert(_env);
  jobject arg;

  int callback_id = request->get_callback_id();

  switch (fs_type) {
    case UV_FS_CLOSE:
    case UV_FS_RENAME:
    case UV_FS_UNLINK:
    case UV_FS_RMDIR:
    case UV_FS_MKDIR:
    case UV_FS_FTRUNCATE:
    case UV_FS_FSYNC:
    case UV_FS_FDATASYNC:
    case UV_FS_LINK:
    case UV_FS_SYMLINK:
    case UV_FS_CHMOD:
    case UV_FS_FCHMOD:
    case UV_FS_CHOWN:
    case UV_FS_FCHOWN:
      arg = NULL;
      break;

    case UV_FS_OPEN:
      arg = _env->CallStaticObjectMethod(_int_cid, _int_valueof_mid, result);
      break;

    case UV_FS_UTIME:
    case UV_FS_FUTIME:
    case UV_FS_WRITE:
      arg = _env->CallStaticObjectMethod(_long_cid, _long_valueof_mid, result);
      break;

    case UV_FS_READ: {
      jobjectArray args = _env->NewObjectArray(2, _object_cid, 0);
      assert(args);
      jobject bytesRead = _env->CallStaticObjectMethod(_long_cid, _long_valueof_mid, result);
      _env->SetObjectArrayElement(args, 0, bytesRead);
      _env->SetObjectArrayElement(args, 1, request->get_array_region_from_bytes(static_cast<jsize>(result)));
      _env->CallVoidMethod(
          _instance,
          _callback_narg_mid,
          fs_type,
          callback_id,
          args);
      return;
    }

    case UV_FS_STAT:
    case UV_FS_LSTAT:
    case UV_FS_FSTAT:
      arg = build_stats(static_cast<uv_statbuf_t*>(ptr));
      break;

    case UV_FS_READLINK:
      arg = _env->NewStringUTF(static_cast<char*>(ptr));
      break;

    case UV_FS_READDIR: {
      char *namebuf = static_cast<char*>(ptr);
      int nnames = static_cast<int>(result);

      jobjectArray names = _env->NewObjectArray(nnames, _object_cid, 0);

      for (int i = 0; i < nnames; i++) {
        jstring name = _env->NewStringUTF(namebuf);
        _env->SetObjectArrayElement(names, i, name);
#ifndef NDEBUG
        namebuf += strlen(namebuf);
        assert(*namebuf == '\0');
        namebuf += 1;
#else
        namebuf += strlen(namebuf) + 1;
#endif
      }
      _env->CallVoidMethod(
          _instance,
          _callback_narg_mid,
          fs_type,
          callback_id,
          names);
      return;
    }

    default:
      assert(0 && "Unhandled eio response");
  }

  _env->CallVoidMethod(
      _instance,
      _callback_1arg_mid,
      fs_type,
      callback_id,
      arg);
}

void FileCallbacks::fs_cb(FileRequest *request, uv_fs_type fs_type, int errorno, const char *path) {

  assert(_env);

  int callback_id = request->get_callback_id();

  jobject error = _env->CallStaticObjectMethod(_int_cid, _int_valueof_mid, -1);
  jthrowable exception = NewException(_env, errorno, NULL, NULL, path);
  jobjectArray args = _env->NewObjectArray(2, _object_cid, 0);
  assert(args);
  _env->SetObjectArrayElement(args, 0, error);
  _env->SetObjectArrayElement(args, 1, exception);
  _env->CallVoidMethod(
      _instance,
      _callback_narg_mid,
      fs_type,
      callback_id,
      args);
}

static void _fs_cb(uv_fs_t* req) {

  assert(req);
  assert(req->data);

  FileRequest* request = reinterpret_cast<FileRequest*>(req->data);
  assert(request->get_callback_ptr());

  FileCallbacks* cb = reinterpret_cast<FileCallbacks*>(request->get_callback_ptr());

  if (req->result == -1) {
    cb->fs_cb(request, req->fs_type, req->errorno, req->path);
  } else {
    cb->fs_cb(request, req->fs_type, req->result, req->ptr);
  }
  uv_fs_req_cleanup(req);
  delete(req);
  delete(request);
}

static FileRequest* new_request(JNIEnv *env, jlong callback_ptr, int id, jobject read_buffer, jlong byte_array_size, jlong offset) {

  FileRequest* req = new FileRequest(env, callback_ptr, id, read_buffer, byte_array_size, offset);
  return req;
}

static FileRequest* new_request(JNIEnv *env, jlong callback_ptr, int id) {
  return new_request(env, callback_ptr, id, NULL, 0, 0);
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _static_initialize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_net_java_libuv_handles_Files__1static_1initialize
  (JNIEnv *env, jclass cls) {

  FileCallbacks::static_initialize(env, cls);
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _new
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_net_java_libuv_handles_Files__1new
  (JNIEnv *env, jclass cls) {

  FileCallbacks* cb = new FileCallbacks();
  return reinterpret_cast<jlong>(cb);
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _initialize
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_net_java_libuv_handles_Files__1initialize
  (JNIEnv *env, jobject that, jlong ptr) {

  assert(ptr);
  FileCallbacks* cb = reinterpret_cast<FileCallbacks*>(ptr);
  cb->initialize(that);
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _close
* Signature: (JIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1close
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_close(loop, req, fd, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_close(loop, &req, fd, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_close");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _open
 * Signature: (JLjava/lang/String;IIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1open
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint flags, jint mode, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int fd;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    fd = uv_fs_open(loop, req, cpath, flags, mode, _fs_cb);
  } else {
    uv_fs_t req;
    fd = uv_fs_open(loop, &req, cpath, flags, mode, NULL);
    uv_fs_req_cleanup(&req);
    if (fd == -1) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_open", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return fd;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _read
 * Signature: (JI[BJJJIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1read
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jbyteArray buffer, jlong length, jlong offset, jlong position, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    jlong buffer_size = length - offset;
    FileRequest* request = new_request(env, callback_ptr, callback, buffer, buffer_size, offset);
    req->data = request;
    r = uv_fs_read(loop, req, fd, request->get_byte_array(), length, position, _fs_cb);
  } else {
    uv_fs_t req;
    jbyte* base = new jbyte[length - offset];
    r = uv_fs_read(loop, &req, fd, base, length, position, NULL);
    env->SetByteArrayRegion(buffer, (jsize)offset, (jsize)length, base);
    uv_fs_req_cleanup(&req);
    delete base;
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_read");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _unlink
 * Signature: (JLjava/lang/String;IJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1unlink
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_unlink(loop, req, cpath, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_unlink(loop, &req, cpath, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_unlink", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _write
 * Signature: (JI[BJJJIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1write
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jbyteArray data, jlong length, jlong offset, jlong position, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);

  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    jsize buffer_size = env->GetArrayLength(data);
    FileRequest* request = new_request(env, callback_ptr, callback, NULL, (jlong) buffer_size, 0);
    jbyte* base = request->get_bytes_from_array_region(data, (jsize) (buffer_size - offset), (jsize) offset);
    req->data = request;
    r = uv_fs_write(loop, req, fd, base, length, position, _fs_cb);
  } else {
    uv_fs_t req;
    jsize buffer_size = env->GetArrayLength(data);
    jbyte* base = new jbyte[buffer_size];
    env->GetByteArrayRegion(data, (jsize) offset, (jsize) (buffer_size - offset), base);
    r = uv_fs_write(loop, &req, fd, base, length, position, NULL);
    uv_fs_req_cleanup(&req);
    delete base;
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_write");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _mkdir
 * Signature: (JLjava/lang/String;IIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1mkdir
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint mode, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_mkdir(loop, req, cpath, mode, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_mkdir(loop, &req, cpath, mode, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_mkdir", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _rmdir
 * Signature: (JLjava/lang/String;IJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1rmdir
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_rmdir(loop, req, cpath, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_rmdir(loop, &req, cpath, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_rmdir", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _readdir
 * Signature: (JLjava/lang/String;IIJ)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_net_java_libuv_handles_Files__1readdir
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint flags, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  jobjectArray names = NULL;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    uv_fs_readdir(loop, req, cpath, flags, _fs_cb);
  } else {
    uv_fs_t req;
    int r = uv_fs_readdir(loop, &req, cpath, flags, NULL);
    if (r >= 0) {
        char *namebuf = static_cast<char*>(req.ptr);
        int nnames = static_cast<int>(req.result);
        names = env->NewObjectArray(nnames, FileCallbacks::_object_cid, 0);

        for (int i = 0; i < nnames; i++) {
          jstring name = env->NewStringUTF(namebuf);
          env->SetObjectArrayElement(names, i, name);
#ifndef NDEBUG
          namebuf += strlen(namebuf);
          assert(*namebuf == '\0');
          namebuf += 1;
#else
          namebuf += strlen(namebuf) + 1;
#endif
        }
    } else {
        ThrowException(env, uv_last_error(loop).code, "uv_fs_readdir", NULL, cpath);
    }
    uv_fs_req_cleanup(&req);
  }
  env->ReleaseStringUTFChars(path, cpath);
  return names;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _stat
 * Signature: (JLjava/lang/String;IJ)Lnet/java/libuv/handles/Stats;
 */
JNIEXPORT jobject JNICALL Java_net_java_libuv_handles_Files__1stat
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  jobject stats = NULL;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    uv_fs_stat(loop, req, cpath, _fs_cb);
  } else {
    uv_fs_t req;
    int r = uv_fs_stat(loop, &req, cpath, NULL);
    stats = FileCallbacks::build_stats(static_cast<uv_statbuf_t *>(req.ptr));
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_stat", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return stats;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _fstat
 * Signature: (JIIJ)Lnet/java/libuv/handles/Stats;
 */
JNIEXPORT jobject JNICALL Java_net_java_libuv_handles_Files__1fstat
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  jobject stats = NULL;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    uv_fs_fstat(loop, req, fd, _fs_cb);
  } else {
    uv_fs_t req;
    int r = uv_fs_fstat(loop, &req, fd, NULL);
    stats = FileCallbacks::build_stats(static_cast<uv_statbuf_t*>(req.ptr));
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_fstat");
    }
  }
  return stats;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _rename
 * Signature: (JLjava/lang/String;Ljava/lang/String;IJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1rename
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jstring new_path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* src_path = env->GetStringUTFChars(path, 0);
  const char* dst_path = env->GetStringUTFChars(new_path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_rename(loop, req, src_path, dst_path, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_rename(loop, &req, src_path, dst_path, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_rename", NULL, src_path);
    }
  }
  env->ReleaseStringUTFChars(path, src_path);
  env->ReleaseStringUTFChars(new_path, dst_path);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _fsync
 * Signature: (JIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1fsync
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_fsync(loop, req, fd, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_fsync(loop, &req, fd, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_fsync");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _fdatasync
 * Signature: (JIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1fdatasync
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_fdatasync(loop, req, fd, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_fdatasync(loop, &req, fd, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_fdatasync");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _ftruncate
 * Signature: (JIJIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1ftruncate
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jlong offset, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_ftruncate(loop, req, fd, offset, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_ftruncate(loop, &req, fd, offset, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_ftruncate");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _sendfile
 * Signature: (JIIJJIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1sendfile
  (JNIEnv *env, jobject that, jlong loop_ptr, jint out_fd, jint in_fd, jlong offset, jlong length, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_sendfile(loop, req, out_fd, in_fd, offset, length, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_sendfile(loop, &req, out_fd, in_fd, offset, length, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_sendfile");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _chmod
 * Signature: (JLjava/lang/String;IIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1chmod
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint mode, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_chmod(loop, req, cpath, mode, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_chmod(loop, &req, cpath, mode, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_chmod", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _utime
 * Signature: (JLjava/lang/String;DDIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1utime
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jdouble atime, jdouble mtime, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_utime(loop, req, cpath, atime, mtime, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_utime(loop, &req, cpath, atime, mtime, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_utime", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _futime
 * Signature: (JIDDIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1futime
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jdouble atime, jdouble mtime, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_futime(loop, req, fd, atime, mtime, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_futime(loop, &req, fd, atime, mtime, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_futime");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _lstat
 * Signature: (JLjava/lang/String;IJ)Lnet/java/libuv/handles/Stats;
 */
JNIEXPORT jobject JNICALL Java_net_java_libuv_handles_Files__1lstat
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  jobject stats = NULL;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    uv_fs_lstat(loop, req, cpath, _fs_cb);
  } else {
    uv_fs_t req;
    int r = uv_fs_lstat(loop, &req, cpath, NULL);
    stats = FileCallbacks::build_stats(static_cast<uv_statbuf_t*>(req.ptr));
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_lstat", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return stats;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _link
 * Signature: (JLjava/lang/String;Ljava/lang/String;IJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1link
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jstring new_path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* src_path = env->GetStringUTFChars(path, 0);
  const char* dst_path = env->GetStringUTFChars(new_path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_link(loop, req, src_path, dst_path, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_link(loop, &req, src_path, dst_path, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_link", NULL, src_path);
    }
  }
  env->ReleaseStringUTFChars(path, src_path);
  env->ReleaseStringUTFChars(new_path, dst_path);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _symlink
 * Signature: (JLjava/lang/String;Ljava/lang/String;IIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1symlink
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jstring new_path, jint flags, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* src_path = env->GetStringUTFChars(path, 0);
  const char* dst_path = env->GetStringUTFChars(new_path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_symlink(loop, req, src_path, dst_path, flags, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_symlink(loop, &req, src_path, dst_path, flags, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_symlink", NULL, src_path);
    }
  }
  env->ReleaseStringUTFChars(path, src_path);
  env->ReleaseStringUTFChars(new_path, dst_path);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _readlink
 * Signature: (JLjava/lang/String;IJ)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_net_java_libuv_handles_Files__1readlink
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  jstring link = NULL;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    uv_fs_readlink(loop, req, cpath, _fs_cb);
  } else {
    uv_fs_t req;
    int r = uv_fs_readlink(loop, &req, cpath, NULL);
    link = env->NewStringUTF(static_cast<char*>(req.ptr));
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_readklink", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return link;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _fchmod
 * Signature: (JIIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1fchmod
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jint mode, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_fchmod(loop, req, fd, mode, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_fchmod(loop, &req, fd, mode, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_fchmod");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _chown
 * Signature: (JLjava/lang/String;IIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1chown
  (JNIEnv *env, jobject that, jlong loop_ptr, jstring path, jint uid, jint gid, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  const char* cpath = env->GetStringUTFChars(path, 0);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_chown(loop, req, cpath, (uv_uid_t)uid, (uv_gid_t)gid, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_chown(loop, &req, cpath, (uv_uid_t)uid, (uv_gid_t)gid, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_chown", NULL, cpath);
    }
  }
  env->ReleaseStringUTFChars(path, cpath);
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _fchown
 * Signature: (JIIIIJ)I
 */
JNIEXPORT jint JNICALL Java_net_java_libuv_handles_Files__1fchown
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd, jint uid, jint gid, jint callback, jlong callback_ptr) {

  assert(loop_ptr);
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  int r;

  if (callback) {
    uv_fs_t* req = new uv_fs_t();
    req->data = new_request(env, callback_ptr, callback);
    r = uv_fs_fchown(loop, req, fd, (uv_uid_t)uid, (uv_gid_t)gid, _fs_cb);
  } else {
    uv_fs_t req;
    r = uv_fs_fchown(loop, &req, fd, (uv_uid_t)uid, (uv_gid_t)gid, NULL);
    uv_fs_req_cleanup(&req);
    if (r < 0) {
      ThrowException(env, uv_last_error(loop).code, "uv_fs_fchown");
    }
  }
  return r;
}

/*
 * Class:     net_java_libuv_handles_Files
 * Method:    _get_path
 * Signature: (JI)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_net_java_libuv_handles_Files__1get_1path
  (JNIEnv *env, jobject that, jlong loop_ptr, jint fd) {
  assert(loop_ptr);
#ifdef __MACOS__
  char pathbuf[PATH_MAX];
  if (fcntl(fd, F_GETPATH, pathbuf) >= 0) {
    // pathbuf now contains *a* path to the open file descriptor
    jstring path = env->NewStringUTF(static_cast<char*>(pathbuf));
    return path;
  } else {
    uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
    ThrowException(env, uv_last_error(loop).code, "fcntl");
    return NULL;
  }
#elif _WIN32
  HANDLE handle = (HANDLE) _get_osfhandle(fd);
  PFILE_NAME_INFO filename_info = (PFILE_NAME_INFO) HeapAlloc(GetProcessHeap(), 0, sizeof(FILE_NAME_INFO) + MAX_PATH);

  // Get the filename
  if (!GetFileInformationByHandleEx(handle, FileNameInfo, filename_info, MAX_PATH)) {
    HeapFree(GetProcessHeap(), 0, filename_info);
    ThrowException(env, GetLastError(), "GetFileInformationByHandleEx");
    return NULL;
  }

  // Get the volume serial number
  LPBY_HANDLE_FILE_INFORMATION file_info = (LPBY_HANDLE_FILE_INFORMATION) HeapAlloc(GetProcessHeap(), 0, sizeof(BY_HANDLE_FILE_INFORMATION));
  if (!GetFileInformationByHandle(handle, file_info)) {
    HeapFree(GetProcessHeap(), 0, filename_info);
    HeapFree(GetProcessHeap(), 0, file_info);
    ThrowException(env, GetLastError(), "GetFileInformationByHandle");
    return NULL;
  }

  // Get the buffer size needed to hold the drive strings
  DWORD buffer_len = GetLogicalDriveStrings(0, NULL) * sizeof(TCHAR);
  if (buffer_len == 0) {
    HeapFree(GetProcessHeap(), 0, filename_info);
    HeapFree(GetProcessHeap(), 0, file_info);
    ThrowException(env, GetLastError(), "GetLogicalDriveStrings");
    return NULL;
  }
  LPTSTR drives = (LPTSTR) malloc((buffer_len + 1) * sizeof(TCHAR));
  assert(drives);

  // Find all the drives
  DWORD r = GetLogicalDriveStrings(buffer_len, drives);
  if (r == 0) {
    free(drives);
    HeapFree(GetProcessHeap(), 0, filename_info);
    HeapFree(GetProcessHeap(), 0, file_info);
    ThrowException(env, GetLastError(), "GetLogicalDriveStrings");
    return NULL;
  }

  LPTSTR single_drive = drives;
  while (*single_drive) {
    DWORD serial_number;
    if (GetVolumeInformation(single_drive, NULL, NULL, &serial_number, NULL, NULL, NULL, 0)) {
      if (serial_number == file_info->dwVolumeSerialNumber) {
        break;
      }
    } else {
      free(drives);
      HeapFree(GetProcessHeap(), 0, filename_info);
      HeapFree(GetProcessHeap(), 0, file_info);
      ThrowException(env, GetLastError(), "GetVolumeInformation");
      return NULL;
    }
    single_drive += _tcslen(single_drive) + 1;
  }

  size_t drive_len = _tcslen(single_drive);
  size_t filename_len = filename_info->FileNameLength / sizeof(wchar_t); // FileNameLength is in bytes
  size_t total_len = drive_len + filename_len + 1;
  wchar_t* wpath = (wchar_t*) malloc(sizeof(wchar_t) * total_len);
  assert(wpath);

  wchar_t* filename = filename_info->FileName;
  if (filename[0] == '\\') {
    // Since the drive letter ends with a \ we can remove the leading one from the filename.
    filename = filename_info->FileName + 1;
    total_len -= sizeof(wchar_t);
  }

  _snwprintf(wpath, total_len, TEXT("%s%s"), single_drive, filename);
  jstring path = env->NewString((jchar*) wpath, (jsize) total_len);

  free(wpath);
  free(drives);
  HeapFree(GetProcessHeap(), 0, file_info);
  HeapFree(GetProcessHeap(), 0, filename_info);
  return path;
#else /* POSIX */
  uv_loop_t* loop = reinterpret_cast<uv_loop_t*>(loop_ptr);
  char proc_path[sizeof("/proc/self/fd/") + 3 * sizeof(int)];
  snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", fd);
  uv_fs_t req;
  int r = uv_fs_readlink(loop, &req, proc_path, NULL);
  jstring path = env->NewStringUTF(static_cast<char*>(req.ptr));
  uv_fs_req_cleanup(&req);
  if (r < 0) {
    ThrowException(env, uv_last_error(loop).code, "uv_fs_readlink");
    return NULL;
  }
  return path;
#endif
}
