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

#include <string>
#include "node.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <node_version.h>

#include "Database.h"
#include "NodeCallback.h"
#include "Cluster.h"
#include "Version.h"
#include "FdbError.h"
#include "FdbOptions.h"

uv_thread_t fdbThread;

using namespace v8;
using namespace std;

bool networkStarted = false;

void ApiVersion(const FunctionCallbackInfo<Value>& info) {
	int apiVersion = info[0]->Int32Value();
	fdb_error_t errorCode = fdb_select_api_version(apiVersion);

	if(errorCode != 0) {
		if (errorCode == 2203)
			return NanThrowError(FdbError::NewInstance(errorCode, "API version not supported by the installed FoundationDB C library"));
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));
	}

	info.GetReturnValue().SetNull();
}

static void networkThread(void *arg) {
	fdb_error_t errorCode = fdb_run_network();
	if(errorCode != 0)
		fprintf(stderr, "Unhandled error in FoundationDB network thread: %s (%d)\n", fdb_get_error(errorCode), errorCode);
}

static void runNetwork() {
	fdb_error_t errorCode = fdb_setup_network();

	if(errorCode != 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	uv_thread_create(&fdbThread, networkThread, NULL);  // FIXME: Return code?
}

void CreateCluster(const FunctionCallbackInfo<Value>& info) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);

	FDBFuture *f = fdb_create_cluster(*String::Utf8Value(info[0]->ToString()));
	fdb_error_t errorCode = fdb_future_block_until_ready(f);

	FDBCluster *cluster;
	if(errorCode == 0)
		errorCode = fdb_future_get_cluster(f, &cluster);

	if(errorCode != 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	Local<Value> jsValue = Local<Value>::New(isolate, Cluster::NewInstance(cluster));
	info.GetReturnValue().Set(jsValue);
}

void StartNetwork(const FunctionCallbackInfo<Value>& info) {
	info.GetReturnValue().SetNull();

	if(!networkStarted) {
		networkStarted = true;
		runNetwork();
	}
}

void StopNetwork(const FunctionCallbackInfo<Value>& info) {
	fdb_error_t errorCode = fdb_stop_network();

	if(errorCode != 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	uv_thread_join(&fdbThread);

	//This line forces garbage collection.  Useful for doing valgrind tests
	//while(!V8::IdleNotification());

	info.GetReturnValue().SetNull();

	FdbOptions::Clear();
}

void init(Handle<Object> target){
	Isolate *isolate = Isolate::GetCurrent();
	FdbError::Init( target );
	Database::Init();
	Transaction::Init();
	Cluster::Init();
	FdbOptions::Init();
	Watch::Init();

	target->Set(String::NewFromUtf8(isolate, "apiVersion", String::kInternalizedString), FunctionTemplate::New(isolate, ApiVersion)->GetFunction());
	target->Set(String::NewFromUtf8(isolate, "createCluster", String::kInternalizedString), FunctionTemplate::New(isolate, CreateCluster)->GetFunction());
	target->Set(String::NewFromUtf8(isolate, "startNetwork", String::kInternalizedString), FunctionTemplate::New(isolate, StartNetwork)->GetFunction());
	target->Set(String::NewFromUtf8(isolate, "stopNetwork", String::kInternalizedString), FunctionTemplate::New(isolate, StopNetwork)->GetFunction());
	target->Set(String::NewFromUtf8(isolate, "options", String::kInternalizedString), FdbOptions::CreateOptions(FdbOptions::NetworkOption));
	target->Set(String::NewFromUtf8(isolate, "streamingMode", String::kInternalizedString), FdbOptions::CreateEnum(FdbOptions::StreamingMode));
	target->Set(String::NewFromUtf8(isolate, "atomic", String::kInternalizedString), FdbOptions::CreateOptions(FdbOptions::MutationType));
}

#if NODE_VERSION_AT_LEAST(0, 8, 0)
NODE_MODULE(fdblib, init);
#else
#error "Node.js versions before v0.8.0 are not supported"
#endif
