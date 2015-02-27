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
#include <vector>
#include <node_buffer.h>
#include <node_version.h>

#include "Transaction.h"
#include "NodeCallback.h"
#include "FdbError.h"
#include "FdbOptions.h"

using namespace v8;
using namespace std;
using namespace node;

// Transaction Implementation
Transaction::Transaction() { };

Transaction::~Transaction() {
	fdb_transaction_destroy(tr);
};

Persistent<Function> Transaction::constructor;

struct NodeValueCallback : NodeCallback {

	NodeValueCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : NodeCallback(future, cbFunc) { }

	virtual Handle<Value> extractValue(FDBFuture* future, fdb_error_t& outErr) {
		Isolate *isolate = Isolate::GetCurrent();
		EscapableHandleScope scope(isolate);

		const char *value;
		int valueLength;
		int valuePresent;

		outErr = fdb_future_get_value(future, &valuePresent, (const uint8_t**)&value, &valueLength);
		if (outErr) return Undefined(isolate);

		Local<Value> jsValue;

		if(!valuePresent)
			jsValue = Local<Value>::New(isolate, Null(isolate));
		else
			jsValue = Local<Value>::New(isolate, makeBuffer(value, valueLength));

		return scope.Escape(jsValue);
	}
};

struct NodeKeyCallback : NodeCallback {

	NodeKeyCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : NodeCallback(future, cbFunc) { }

	virtual Handle<Value> extractValue(FDBFuture* future, fdb_error_t& outErr) {
		Isolate *isolate = Isolate::GetCurrent();
		EscapableHandleScope scope(isolate);

		const char *key;
		int keyLength;

		outErr = fdb_future_get_key(future, (const uint8_t**)&key, &keyLength);
		if (outErr) return Undefined(isolate);

		Local<Value> jsValue = Local<Value>::New(isolate, makeBuffer(key, keyLength));

		return scope.Escape(jsValue);
	}
};

struct NodeVoidCallback : NodeCallback {

	NodeVoidCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : NodeCallback(future, cbFunc) { }

	virtual Handle<Value> extractValue(FDBFuture* future, fdb_error_t& outErr) {
		Isolate *isolate = Isolate::GetCurrent();
		outErr = fdb_future_get_error(future);
		return Undefined(isolate);
	}
};

struct NodeKeyValueCallback : NodeCallback {

	NodeKeyValueCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : NodeCallback(future, cbFunc) { }

	virtual Handle<Value> extractValue(FDBFuture* future, fdb_error_t& outErr) {
		Isolate *isolate = Isolate::GetCurrent();
		EscapableHandleScope scope(isolate);

		const FDBKeyValue *kv;
		int len;
		fdb_bool_t more;

		outErr = fdb_future_get_keyvalue_array(future, &kv, &len, &more);
		if (outErr) return Undefined(isolate);

		/*
		 * Constructing a JavaScript array of KeyValue objects:
		 *  {
		 *  	key: "some key",
		 *  	value: "some value"
		 *  }
		 *
		 */

		Local<Object> returnObj = Local<Object>::New(isolate, Object::New(isolate));
		Handle<Array> jsValueArray = Array::New(isolate, len);

		Handle<String> keySymbol = String::NewFromUtf8(isolate, "key", String::kInternalizedString);
		Handle<String> valueSymbol = String::NewFromUtf8(isolate, "value", String::kInternalizedString);

		for(int i = 0; i < len; i++) {
			Local<Object> jsKeyValue = Object::New(isolate);

			Handle<Value> jsKeyBuffer = makeBuffer((const char*)kv[i].key, kv[i].key_length);
			Handle<Value> jsValueBuffer = makeBuffer((const char*)kv[i].value, kv[i].value_length);

			jsKeyValue->Set(keySymbol, jsKeyBuffer);
			jsKeyValue->Set(valueSymbol, jsValueBuffer);
			jsValueArray->Set(Number::New(isolate, i), jsKeyValue);
		}

		returnObj->Set(String::NewFromUtf8(isolate, "array", String::kInternalizedString), jsValueArray);
		if(more)
			returnObj->Set(String::NewFromUtf8(isolate, "more", String::kInternalizedString), Number::New(isolate, 1));

		return scope.Escape(returnObj);
	}
};

struct NodeVersionCallback : NodeCallback {

	NodeVersionCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : NodeCallback(future, cbFunc) { }

	virtual Handle<Value> extractValue(FDBFuture* future, fdb_error_t& outErr) {
		Isolate *isolate = Isolate::GetCurrent();
		EscapableHandleScope scope(isolate);

		int64_t version;

		outErr = fdb_future_get_version(future, &version);
		if (outErr) return Undefined(isolate);

		//SOMEDAY: This limits the version to 53-bits.  Do something different here?
		Local<Value> jsValue = Local<Value>::New(isolate, Number::New(isolate, (double)version));

		return scope.Escape(jsValue);
	}
};

struct NodeStringArrayCallback : NodeCallback {

	NodeStringArrayCallback(FDBFuture *future, Persistent<Function, CopyablePersistentTraits<Function> > cbFunc) : NodeCallback(future, cbFunc) { }

	virtual Handle<Value> extractValue(FDBFuture *future, fdb_error_t& outErr) {
		Isolate *isolate = Isolate::GetCurrent();
		EscapableHandleScope scope(isolate);

		const char **strings;
		int stringCount;

		outErr = fdb_future_get_string_array(future, &strings, &stringCount);
		if (outErr) return Undefined(isolate);

		Local<Array> jsArray = Local<Array>::New(isolate, Array::New(isolate, stringCount));
		for(int i = 0; i < stringCount; i++)
			jsArray->Set(Number::New(isolate, i), makeBuffer(strings[i], (int)strlen(strings[i])));

		return scope.Escape(jsArray);
	}
};

struct StringParams {
	uint8_t *str;
	int len;

	/*
	 *  String arguments always have to be buffers to
	 *  preserve bytes. Otherwise, stuff gets converted
	 *  to UTF-8.
	 */
	StringParams(Handle<Value> keyVal) {
		str = (uint8_t*)(Buffer::Data(keyVal->ToObject()));
		len = (int)Buffer::Length(keyVal->ToObject());
	}
};

FDBTransaction* Transaction::GetTransactionFromArgs(const FunctionCallbackInfo<Value>& info) {
	return node::ObjectWrap::Unwrap<Transaction>(info.Holder())->tr;
}

Persistent<Function, CopyablePersistentTraits<Function> > Transaction::GetCallback(Handle<Value> funcVal) {
	Isolate *isolate = Isolate::GetCurrent();
	Persistent<Function, CopyablePersistentTraits<Function> > callback(isolate, Handle<Function>::Cast(funcVal));
	return callback;
}

void Transaction::Set(const FunctionCallbackInfo<Value>& info){
	StringParams key(info[0]);
	StringParams val(info[1]);
	fdb_transaction_set(GetTransactionFromArgs(info), key.str, key.len, val.str, val.len);

	info.GetReturnValue().SetNull();
}

void Transaction::Commit(const FunctionCallbackInfo<Value>& info) {
	FDBFuture *f = fdb_transaction_commit(GetTransactionFromArgs(info));
	(new NodeVoidCallback(f, GetCallback(info[0])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::Clear(const FunctionCallbackInfo<Value>& info) {
	StringParams key(info[0]);
	fdb_transaction_clear(GetTransactionFromArgs(info), key.str, key.len);

	info.GetReturnValue().SetNull();
}

/*
 * ClearRange takes two key strings.
 */
void Transaction::ClearRange(const FunctionCallbackInfo<Value>& info) {
	StringParams begin(info[0]);
	StringParams end(info[1]);
	fdb_transaction_clear_range(GetTransactionFromArgs(info), begin.str, begin.len, end.str, end.len);

	info.GetReturnValue().SetNull();
}

/*
 * This function takes a KeySelector and returns a future.
 */
void Transaction::GetKey(const FunctionCallbackInfo<Value>& info) {
	StringParams key(info[0]);
	int selectorOrEqual = info[1]->Int32Value();
	int selectorOffset = info[2]->Int32Value();
	bool snapshot = info[3]->BooleanValue();

	FDBFuture *f = fdb_transaction_get_key(GetTransactionFromArgs(info), key.str, key.len, (fdb_bool_t)selectorOrEqual, selectorOffset, snapshot);
	(new NodeKeyCallback(f, GetCallback(info[4])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::Get(const FunctionCallbackInfo<Value>& info) {
	StringParams key(info[0]);
	bool snapshot = info[1]->BooleanValue();

	FDBFuture *f = fdb_transaction_get(GetTransactionFromArgs(info), key.str, key.len, snapshot);
	(new NodeValueCallback(f, GetCallback(info[2])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::GetRange(const FunctionCallbackInfo<Value>& info) {
	StringParams start(info[0]);
	int startOrEqual = info[1]->Int32Value();
	int startOffset = info[2]->Int32Value();

	StringParams end(info[3]);
	int endOrEqual = info[4]->Int32Value();
	int endOffset = info[5]->Int32Value();

	int limit = info[6]->Int32Value();
	FDBStreamingMode mode = (FDBStreamingMode)info[7]->Int32Value();
	int iteration = info[8]->Int32Value();
	bool snapshot = info[9]->BooleanValue();
	bool reverse = info[10]->BooleanValue();

	FDBFuture *f = fdb_transaction_get_range(GetTransactionFromArgs(info), start.str, start.len, (fdb_bool_t)startOrEqual, startOffset,
												end.str, end.len, (fdb_bool_t)endOrEqual, endOffset, limit, 0, mode, iteration, snapshot, reverse);

	(new NodeKeyValueCallback(f, GetCallback(info[11])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::Watch(const FunctionCallbackInfo<Value>& info) {
	Isolate *isolate = Isolate::GetCurrent();
	Transaction *trPtr = node::ObjectWrap::Unwrap<Transaction>(info.Holder());

	uint8_t *keyStr = (uint8_t*)(Buffer::Data(info[0]->ToObject()));
	int keyLen = (int)Buffer::Length(info[0]->ToObject());

	Persistent<Function> cb(isolate, Handle<Function>::Cast(info[1]));

	FDBFuture *f = fdb_transaction_watch(trPtr->tr, keyStr, keyLen);
	NodeVoidCallback *callback = new NodeVoidCallback(f, cb);
	Handle<Value> watch = Watch::NewInstance(callback);

	callback->start();
	info.GetReturnValue().Set(watch);
}

void Transaction::AddConflictRange(const FunctionCallbackInfo<Value>& info, FDBConflictRangeType type) {
	StringParams start(info[0]);
	StringParams end(info[1]);

	fdb_error_t errorCode = fdb_transaction_add_conflict_range(GetTransactionFromArgs(info), start.str, start.len, end.str, end.len, type);

	if(errorCode != 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	info.GetReturnValue().SetNull();
}

void Transaction::AddReadConflictRange(const FunctionCallbackInfo<Value>& info) {
	return AddConflictRange(info, FDB_CONFLICT_RANGE_TYPE_READ);
}

void Transaction::AddWriteConflictRange(const FunctionCallbackInfo<Value>& info) {
	return AddConflictRange(info, FDB_CONFLICT_RANGE_TYPE_WRITE);
}

void Transaction::OnError(const FunctionCallbackInfo<Value>& info) {
	fdb_error_t errorCode = info[0]->Int32Value();
	FDBFuture *f = fdb_transaction_on_error(GetTransactionFromArgs(info), errorCode);
	(new NodeVoidCallback(f, GetCallback(info[1])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::Reset(const FunctionCallbackInfo<Value>& info) {
	fdb_transaction_reset(GetTransactionFromArgs(info));

	info.GetReturnValue().SetNull();
}

void Transaction::SetReadVersion(const FunctionCallbackInfo<Value>& info) {
	int64_t version = info[0]->IntegerValue();
	fdb_transaction_set_read_version(GetTransactionFromArgs(info), version);

	info.GetReturnValue().SetNull();
}

void Transaction::GetReadVersion(const FunctionCallbackInfo<Value>& info) {
	FDBFuture *f = fdb_transaction_get_read_version(GetTransactionFromArgs(info));
	(new NodeVersionCallback(f, GetCallback(info[0])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::GetCommittedVersion(const FunctionCallbackInfo<Value>& info) {
	int64_t version;
	fdb_error_t errorCode = fdb_transaction_get_committed_version(GetTransactionFromArgs(info), &version);

	if(errorCode != 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	info.GetReturnValue().Set((double)version);
}

void Transaction::Cancel(const FunctionCallbackInfo<Value>& info) {
	fdb_transaction_cancel(GetTransactionFromArgs(info));

	info.GetReturnValue().SetNull();
}

void Transaction::GetAddressesForKey(const FunctionCallbackInfo<Value>& info) {
	StringParams key(info[0]);

	FDBFuture *f = fdb_transaction_get_addresses_for_key(GetTransactionFromArgs(info), key.str, key.len);
	(new NodeStringArrayCallback(f, GetCallback(info[1])))->start();

	info.GetReturnValue().SetNull();
}

void Transaction::New(const FunctionCallbackInfo<Value>& info) {
	Transaction *tr = new Transaction();
	tr->Wrap(info.Holder());
}

Handle<Value> Transaction::NewInstance(FDBTransaction *ptr) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);

	Local<Function> transactionConstructor = Local<Function>::New(isolate, constructor);
	Local<Object> instance = transactionConstructor->NewInstance();

	Transaction *trObj = ObjectWrap::Unwrap<Transaction>(instance);
	trObj->tr = ptr;

	instance->Set(String::NewFromUtf8(isolate, "options", String::kInternalizedString), FdbOptions::CreateOptions(FdbOptions::TransactionOption, instance));

	return scope.Escape(instance);
}

void Transaction::Init() {
	Isolate *isolate = Isolate::GetCurrent();
	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);

	tpl->SetClassName(String::NewFromUtf8(isolate, "Transaction", String::kInternalizedString));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "get", String::kInternalizedString), FunctionTemplate::New(isolate, Get)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "getRange", String::kInternalizedString), FunctionTemplate::New(isolate, GetRange)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "getKey", String::kInternalizedString), FunctionTemplate::New(isolate, GetKey)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "watch", String::kInternalizedString), FunctionTemplate::New(isolate, Watch)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "set", String::kInternalizedString), FunctionTemplate::New(isolate, Set)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "commit", String::kInternalizedString), FunctionTemplate::New(isolate, Commit)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "clear", String::kInternalizedString), FunctionTemplate::New(isolate, Clear)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "clearRange", String::kInternalizedString), FunctionTemplate::New(isolate, ClearRange)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "addReadConflictRange", String::kInternalizedString), FunctionTemplate::New(isolate, AddReadConflictRange)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "addWriteConflictRange", String::kInternalizedString), FunctionTemplate::New(isolate, AddWriteConflictRange)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "onError", String::kInternalizedString), FunctionTemplate::New(isolate, OnError)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "reset", String::kInternalizedString), FunctionTemplate::New(isolate, Reset)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "getReadVersion", String::kInternalizedString), FunctionTemplate::New(isolate, GetReadVersion)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "setReadVersion", String::kInternalizedString), FunctionTemplate::New(isolate, SetReadVersion)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "getCommittedVersion", String::kInternalizedString), FunctionTemplate::New(isolate, GetCommittedVersion)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "cancel", String::kInternalizedString), FunctionTemplate::New(isolate, Cancel)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "getAddressesForKey", String::kInternalizedString), FunctionTemplate::New(isolate, GetAddressesForKey)->GetFunction());

	constructor.Reset(isolate, tpl->GetFunction());
}

// Watch implementation
Watch::Watch() : callback(NULL) { };

Watch::~Watch() {
	if(callback) {
		if(callback->getFuture())
			fdb_future_cancel(callback->getFuture());

		callback->delRef();
	}
};

Persistent<Function> Watch::constructor;

Handle<Value> Watch::NewInstance(NodeCallback *callback) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);

	Local<Function> watchConstructor = Local<Function>::New(isolate, constructor);
	Local<Object> instance = watchConstructor->NewInstance();

	Watch *watchObj = ObjectWrap::Unwrap<Watch>(instance);
	watchObj->callback = callback;
	callback->addRef();

	return scope.Escape(instance);
}

void Watch::New(const FunctionCallbackInfo<Value>& info) {
	Watch *c = new Watch();
	c->Wrap(info.Holder());
}

void Watch::Cancel(const FunctionCallbackInfo<Value>& info) {
	NodeCallback *callback = node::ObjectWrap::Unwrap<Watch>(info.Holder())->callback;

	if(callback && callback->getFuture())
		fdb_future_cancel(callback->getFuture());

	info.GetReturnValue().SetNull();
}

void Watch::Init() {
	Isolate *isolate = Isolate::GetCurrent();
	Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
	tpl->SetClassName(String::NewFromUtf8(isolate, "Watch", String::kInternalizedString));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, "cancel", String::kInternalizedString), FunctionTemplate::New(isolate, Cancel)->GetFunction());

	constructor.Reset(isolate, tpl->GetFunction());
}
