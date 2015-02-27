/*
 * FoundationDB Node.js API
 * Copyright (c) 2012 FoundationDB, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef FDB_NODE_NODE_CALLBACK_H
#define FDB_NODE_NODE_CALLBACK_H

#include "FdbError.h"

#include <v8.h>
#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <node.h>
#include <nan.h>
#include <node_buffer.h>
#include <node_version.h>
#include <foundationdb/fdb_c.h>

#if NODE_VERSION_AT_LEAST(0, 7, 9)
#else
#error Node version too old
#endif

using namespace std;
using namespace v8;
using namespace node;

struct NodeCallback {

public:
	NodeCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : future(future), cbFunc(cbFunc), refCount(1) {
		uv_async_init(uv_default_loop(), &handle, &NodeCallback::nodeThreadCallback);
		uv_ref((uv_handle_t*)&handle);
		handle.data = this;
	}

	void start() {
		if (fdb_future_set_callback(future, &NodeCallback::futureReadyCallback, this)) {
			fprintf(stderr, "fdb_future_set_callback failed.\n");
			abort();
		}
	}

	virtual ~NodeCallback() {
		cbFunc.Reset();
		fdb_future_destroy(future);
	}

	void addRef() {
		++refCount;
	}

	void delRef() {
		if(--refCount == 0) {
			delete this;
		}
	}

	FDBFuture* getFuture() {
		return future;
	}

private:
	void close() {
		uv_close((uv_handle_t*)&handle, &NodeCallback::closeCallback);
	}

	static void closeCallback(uv_handle_s *handle) {
		NodeCallback *nc = (NodeCallback*)((uv_async_t*)handle)->data;
		nc->delRef();
	}

	static void futureReadyCallback(FDBFuture *f, void *ptr) {
		NodeCallback *nc = (NodeCallback*)ptr;
		uv_async_send(&nc->handle);
	}

	static void nodeThreadCallback(uv_async_t *handle) {
    Isolate *isolate = Isolate::GetCurrent();
		NodeCallback *nc = (NodeCallback*)handle->data;
		FDBFuture *future = nc->future;

		uv_unref((uv_handle_t*)handle);

		Handle<Value> jsError;
		Handle<Value> jsValue;

		fdb_error_t errorCode;
		jsValue = nc->extractValue(future, errorCode);
		if (errorCode == 0)
			jsError = NanNull();
		else
			jsError = FdbError::NewInstance(errorCode, fdb_get_error(errorCode));

		Handle<Value> args[2] = { jsError, jsValue };

		Local<Function> callback = Local<Function>::New(isolate, nc->cbFunc);

		v8::TryCatch ex;
		callback->Call(isolate->GetCurrentContext()->Global(), 2, args);

		if(ex.HasCaught())
			fprintf(stderr, "\n%s\n", *String::Utf8Value(ex.StackTrace()->ToString()));

		nc->close();
	}

	FDBFuture* future;
	uv_async_t handle;
	Persistent<Function, CopyablePersistentTraits<Function> > cbFunc;
	int refCount;

protected:
	virtual Handle<Value> extractValue(FDBFuture* future, fdb_error_t& outErr) = 0;

	static Handle<Value> makeBuffer(const char *arr, int length) {
		Isolate *isolate = Isolate::GetCurrent();
		EscapableHandleScope scope(isolate);
		Local<Object> buf = Buffer::New(isolate, length);
		memcpy(Buffer::Data(buf), (const char*)arr, length);

		return scope.Escape(buf);
	}
};

#endif
