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

package net.java.libuv.handles;

import net.java.libuv.cb.AsyncCallback;

public final class AsyncHandle extends Handle {

    private boolean closed;

    private AsyncCallback onSend = null;

    static {
        _static_initialize();
    }

    public void setAsyncCallback(final AsyncCallback callback) {
        onSend = callback;
    }

    public AsyncHandle(final LoopHandle loop) {
        super(_new(loop.pointer()), loop);
        _initialize(pointer);
    }

    public int send() {
        return _send(pointer);
    }

    public void close() {
        if (!closed) {
            _close(pointer);
        }
        closed = true;
    }

    @Override
    protected void finalize() throws Throwable {
        close();
        super.finalize();
    }

    private void callSend(final int status) {
        if (onSend != null) {loop.callbackHandler.handleAsyncCallback(onSend, status);}
    }

    private static native long _new(final long loop);

    private static native void _static_initialize();

    private native void _initialize(final long ptr);

    private native int _send(final long ptr);

    private native void _close(final long ptr);

}