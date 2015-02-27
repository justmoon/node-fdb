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
#include "FdbError.h"

using namespace v8;
using namespace node;

static Persistent<Object> module;

void FdbError::Init(Handle<Object> module) {
	Isolate *isolate = Isolate::GetCurrent();
	::module.Reset(isolate, module);
}

Handle<Value> FdbError::NewInstance(fdb_error_t code, const char *description) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);

	Local<Object> moduleObj = Local<Object>::New(isolate, module);
	Local<Value> constructor = moduleObj->Get( String::NewFromUtf8(isolate, "FDBError", String::kInternalizedString) );
	Local<Object> instance;
	if (!constructor.IsEmpty() && constructor->IsFunction()) {
		Local<Value> constructorArgs[] = { String::NewFromUtf8(isolate, description), Integer::New(isolate, code) };
		instance = Local<Function>::Cast(constructor)->NewInstance(2, constructorArgs);
	} else {
		// We can't find the (javascript) FDBError class, so construct and throw *something*
		instance = Exception::Error(String::NewFromUtf8(isolate, "FDBError class not found.  Unable to deliver error."))->ToObject();
	}

	return scope.Escape(instance);
}
