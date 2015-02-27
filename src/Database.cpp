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

#include "Database.h"
#include "FdbOptions.h"
#include "NodeCallback.h"

using namespace v8;
using namespace std;

Database::Database() { };

Database::~Database() {
	fdb_database_destroy(db);
};

Persistent<Function> Database::constructor;

void Database::Init() {
	Isolate *isolate = Isolate::GetCurrent();
	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->SetClassName(String::NewFromUtf8(isolate, "Database", String::kInternalizedString));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "createTransaction", String::kInternalizedString), FunctionTemplate::New(isolate, CreateTransaction)->GetFunction());

	constructor.Reset(isolate, tpl->GetFunction());
}

void Database::CreateTransaction(const v8::FunctionCallbackInfo<v8::Value>& info) {
	Database *dbPtr = node::ObjectWrap::Unwrap<Database>(info.Holder());
	FDBDatabase *db = dbPtr->db;
	FDBTransaction *tr;
	fdb_error_t err = fdb_database_create_transaction(db, &tr);
	if (err) {
		NanThrowError(FdbError::NewInstance(err, fdb_get_error(err)));
		return info.GetReturnValue().SetUndefined();
	}

	info.GetReturnValue().Set(Transaction::NewInstance(tr));
}

void Database::New(const FunctionCallbackInfo<Value>& info) {
	Database *db = new Database();
	db->Wrap(info.Holder());

	info.GetReturnValue().Set(info.Holder());
}

Handle<Value> Database::NewInstance(FDBDatabase *ptr) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);
	Local<Function> databaseConstructor = Local<Function>::New(isolate, constructor);
	Local<Object> instance = databaseConstructor->NewInstance(0, NULL);
	Database *dbObj = ObjectWrap::Unwrap<Database>(instance);
	dbObj->db = ptr;

	instance->Set(String::NewFromUtf8(isolate, "options", String::kInternalizedString), FdbOptions::CreateOptions(FdbOptions::DatabaseOption, instance));

	return scope.Escape(instance);
}
