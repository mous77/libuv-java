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

#ifndef _libuv_java_stream_h_
#define _libuv_java_stream_h_

#include <jni.h>

#include "uv.h"

class StreamCallbacks {
private:
  static jstring _IPV4;
  static jstring _IPV6;

  static jclass _address_cid;
  static jclass _stream_handle_cid;

  static jmethodID _address_init_mid;
  static jmethodID _call_read_callback_mid;
  static jmethodID _call_read2_callback_mid;
  static jmethodID _call_write_callback_mid;
  static jmethodID _call_connect_callback_mid;
  static jmethodID _call_connection_callback_mid;
  static jmethodID _call_close_callback_mid;
  static jmethodID _call_shutdown_callback_mid;

  JNIEnv* _env;
  jobject _instance;

public:
  static void static_initialize(JNIEnv *env, jclass cls);
  static void static_initialize_address(JNIEnv* env);
  static jobject _address_to_js(JNIEnv* env, const sockaddr* addr);

  StreamCallbacks();
  ~StreamCallbacks();

  void initialize(JNIEnv *env, jobject instance);
  void throw_exception(int code, const char* message);

  void on_read(const uv_buf_t* buf, jsize nread);
  void on_read2(const uv_buf_t* buf, jsize nread, jlong ptr, uv_handle_type pending);
  void on_write(int status, jobject buffer, jobject callback, jobject context);
  void on_shutdown(int status, jobject callback, jobject context);
  void on_connect(int status, jobject callback, jobject context);
  void on_connection(int status);
  void on_close();

  inline void on_oom(const char* buf) { OOM(_env, buf); }

};

inline bool is_named_pipe(const uv_stream_t* stream) {
  assert(stream);
  return stream->type == UV_NAMED_PIPE;
}

inline bool is_named_pipe_ipc(const uv_stream_t* stream) {
  return is_named_pipe(stream) && reinterpret_cast<const uv_pipe_t*>(stream)->ipc != 0;
}

inline bool is_tcp(const uv_stream_t* stream) {
  assert(stream);
  return stream->type == UV_TCP;
}

#endif // _libuv_java_stream_h_
