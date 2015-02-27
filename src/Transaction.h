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

#ifndef FDB_NODE_TRANSACTION_H
#define FDB_NODE_TRANSACTION_H

#include "Version.h"

#include <foundationdb/fdb_c.h>
#include <node.h>

#include "NodeCallback.h"

class Transaction: public node::ObjectWrap {
	public:
		static void Init();
		static v8::Handle<v8::Value> NewInstance(FDBTransaction *ptr);
		static void New(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void Get(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void GetKey(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void Set(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void Commit(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void Clear(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void ClearRange(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void GetRange(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void Watch(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void AddConflictRange(const v8::FunctionCallbackInfo<v8::Value>& info, FDBConflictRangeType type);
		static void AddReadConflictRange(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void AddWriteConflictRange(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void OnError(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void Reset(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void SetReadVersion(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void GetReadVersion(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void GetCommittedVersion(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void Cancel(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void GetAddressesForKey(const v8::FunctionCallbackInfo<v8::Value>& info);

		FDBTransaction* GetTransaction() { return tr; }
	private:
		Transaction();
		~Transaction();

		static v8::Persistent<v8::Function> constructor;
		FDBTransaction *tr;

		static FDBTransaction* GetTransactionFromArgs(const v8::FunctionCallbackInfo<v8::Value>& info);
		static v8::Handle<v8::Function> GetCallback(const v8::Handle<v8::Value> funcVal);
};

class Watch : public node::ObjectWrap {
	public:
		static void Init();

		static v8::Handle<v8::Value> NewInstance(NodeCallback *callback);
		static void New(const v8::FunctionCallbackInfo<v8::Value>& info);

		static void Cancel(const v8::FunctionCallbackInfo<v8::Value>& info);

	private:
		Watch();
		~Watch();

		static v8::Persistent<v8::Function> constructor;
		NodeCallback *callback;
};

#endif
