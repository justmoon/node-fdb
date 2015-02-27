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

#include <node.h>
#include <iostream>
#include <string>
#include <cstring>
#include <node_version.h>

#include "Cluster.h"
#include "Database.h"
#include "FdbOptions.h"
#include "NodeCallback.h"

using namespace v8;
using namespace std;

Cluster::Cluster() { }
Cluster::~Cluster() {
	fdb_cluster_destroy(cluster);
}

Persistent<Function> Cluster::constructor;

void Cluster::OpenDatabase(const FunctionCallbackInfo<Value>& info) {
	Cluster *clusterPtr = ObjectWrap::Unwrap<Cluster>(info.Holder());

	std::string dbName = *String::Utf8Value(info[0]->ToString());
	FDBFuture *f = fdb_cluster_create_database(clusterPtr->cluster, (uint8_t*)dbName.c_str(), (int)strlen(dbName.c_str()));

	fdb_error_t errorCode = fdb_future_block_until_ready(f);

	FDBDatabase *database;
	if(errorCode == 0)
		errorCode = fdb_future_get_database(f, &database);

	if(errorCode != 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	Handle<Value> jsValue = Database::NewInstance(database);

	info.GetReturnValue().Set(jsValue);
}

void Cluster::Init() {
	Isolate *isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	tpl->SetClassName(String::NewFromUtf8(isolate, "Cluster", String::kInternalizedString));

	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "openDatabase", String::kInternalizedString), FunctionTemplate::New(isolate, OpenDatabase)->GetFunction());

	constructor.Reset(isolate, tpl->GetFunction());
}

void Cluster::New(const FunctionCallbackInfo<Value>& info) {
	Cluster *c = new Cluster();
	c->Wrap(info.Holder());
}

Handle<Value> Cluster::NewInstance(FDBCluster *ptr) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);

	Local<Function> clusterConstructor = Local<Function>::New(isolate, constructor);
	Local<Object> instance = clusterConstructor->NewInstance(0, NULL);

	Cluster *clusterObj = ObjectWrap::Unwrap<Cluster>(instance);
	clusterObj->cluster = ptr;

	instance->Set(String::NewFromUtf8(isolate, "options", String::kInternalizedString), FdbOptions::CreateOptions(FdbOptions::ClusterOption, instance));

	return scope.Escape(instance);
}
