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

#include "FdbOptions.h"
#include "Cluster.h"
#include "Database.h"
#include "Transaction.h"
#include "FdbError.h"

#include <algorithm>
#include <node_buffer.h>

#define INVALID_OPTION_VALUE_ERROR_CODE (fdb_error_t)2006

using namespace v8;
using namespace node;

FdbOptions::PersistentFnTemplateMap FdbOptions::optionTemplates(Isolate::GetCurrent());
std::map<FdbOptions::Scope, ScopeInfo> FdbOptions::scopeInfo;
std::map<FdbOptions::Scope, std::map<int, FdbOptions::ParameterType>> FdbOptions::parameterTypes;

FdbOptions::FdbOptions() { }
FdbOptions::~FdbOptions() {
	Clear();
}

void FdbOptions::InitOptionsTemplate(Scope scope, const char *className) {
	Isolate *isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> tpl = Local<FunctionTemplate>::New(isolate, FunctionTemplate::New(isolate, New));
	optionTemplates.Set(scope, tpl);
	tpl->SetClassName(String::NewFromUtf8(isolate, className, String::kInternalizedString));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
}

void FdbOptions::AddOption(Scope scope, std::string name, int value, ParameterType type) {
	Isolate *isolate = Isolate::GetCurrent();

	Local<FunctionTemplate> tpl;
	if(scope == NetworkOption || scope == ClusterOption || scope == DatabaseOption || scope == TransactionOption || scope == MutationType) {
		bool isSetter = scope != MutationType;
		tpl = optionTemplates.Get(scope);
		tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, ToJavaScriptName(name, isSetter).c_str(), String::kInternalizedString),
			FunctionTemplate::New(isolate, scopeInfo[scope].optionFunction, Integer::New(isolate, value))->GetFunction());
		parameterTypes[scope][value] = type;
	}
	else if(scope == StreamingMode) {
		tpl = optionTemplates.Get(scope);
		tpl->PrototypeTemplate()->Set(String::NewFromUtf8(isolate, ToJavaScriptName(name, false).c_str(), String::kInternalizedString), Integer::New(isolate, value));
	}
	else if(scope == ConflictRangeType) {
		//Conflict range type enum is not exposed to JS code
	}
}

void FdbOptions::New(const FunctionCallbackInfo<Value>& info) {
	FdbOptions *options = new FdbOptions();
	options->Wrap(info.Holder());
}

void FdbOptions::Clear() {
	optionTemplates.Clear();
}

void FdbOptions::WeakCallback(const WeakCallbackData<Value, FdbOptions>& data) { }

Handle<Value> FdbOptions::NewInstance(Local<FunctionTemplate> optionsTemplate, Handle<Value> source) {
	Isolate *isolate = Isolate::GetCurrent();
	EscapableHandleScope scope(isolate);

	Local<FunctionTemplate> funcTpl = Local<FunctionTemplate>::New(isolate, optionsTemplate);
	Local<Object> instance = funcTpl->GetFunction()->NewInstance();

	FdbOptions *optionsObj = ObjectWrap::Unwrap<FdbOptions>(instance);
	optionsObj->source.Reset(isolate, source);
	optionsObj->source.SetWeak(optionsObj, WeakCallback);

	return scope.Escape(instance);
}

Handle<Value> FdbOptions::CreateOptions(Scope scope, Handle<Value> source) {
	return NewInstance(optionTemplates.Get(scope), source);
}

Handle<Value> FdbOptions::CreateEnum(Scope scope) {
	Local<FunctionTemplate> funcTpl = optionTemplates.Get(scope);
	return funcTpl->GetFunction()->NewInstance();
}

Parameter GetStringParameter(const FunctionCallbackInfo<Value>& info, int index) {
	if(info.Length() <= index || (!Buffer::HasInstance(info[index]) && !info[index]->IsString()))
		return INVALID_OPTION_VALUE_ERROR_CODE;
	else if(info[index]->IsString()) {
		String::Utf8Value val(info[index]);
		return std::string(*val, val.length());
	}
	else
		return std::string(Buffer::Data(info[index]->ToObject()), Buffer::Length(info[index]->ToObject()));
};

Parameter FdbOptions::GetOptionParameter(const FunctionCallbackInfo<Value>& info, Scope scope, int optionValue, int index) {
	if(info.Length() > index) {
		int64_t val;
		switch(parameterTypes[scope][optionValue]) {
			case FdbOptions::String:
				return GetStringParameter(info, index);

			case FdbOptions::Bytes:
				if(!Buffer::HasInstance(info[index]))
					return INVALID_OPTION_VALUE_ERROR_CODE;

				return std::string(Buffer::Data(info[index]->ToObject()), Buffer::Length(info[index]->ToObject()));

			case FdbOptions::Int:
				if(!info[index]->IsNumber())
					return INVALID_OPTION_VALUE_ERROR_CODE;
				val = info[index]->IntegerValue();
				return std::string((const char*)&val, 8);


			case FdbOptions::None:
				return Parameter();
		}
	}

	return Parameter();
}

void SetNetworkOption(const FunctionCallbackInfo<Value>& info) {
	FDBNetworkOption op = (FDBNetworkOption)info.Data()->Uint32Value();

	Parameter param = FdbOptions::GetOptionParameter(info, FdbOptions::NetworkOption, op);
	fdb_error_t errorCode = param.errorCode;
	if(errorCode == 0)
		errorCode = fdb_network_set_option(op, param.getValue(), param.getLength());

	if(errorCode)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	info.GetReturnValue().SetNull();
}

void SetClusterOption(const FunctionCallbackInfo<Value>& info) {
	Isolate *isolate = Isolate::GetCurrent();
	FdbOptions *options = ObjectWrap::Unwrap<FdbOptions>(info.Holder());
	Local<Value> source = Local<Value>::New(isolate, options->GetSource());
	Cluster *cluster = ObjectWrap::Unwrap<Cluster>(source->ToObject());
	FDBClusterOption op = (FDBClusterOption)info.Data()->Uint32Value();

	Parameter param = FdbOptions::GetOptionParameter(info, FdbOptions::ClusterOption, op);
	fdb_error_t errorCode = param.errorCode;
	if(errorCode == 0)
		errorCode = fdb_cluster_set_option(cluster->GetCluster(), op, param.getValue(), param.getLength());

	if(errorCode)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	info.GetReturnValue().SetNull();
}

void SetDatabaseOption(const FunctionCallbackInfo<Value>& info) {
	Isolate *isolate = Isolate::GetCurrent();
	FdbOptions *options = ObjectWrap::Unwrap<FdbOptions>(info.Holder());
	Local<Value> source = Local<Value>::New(isolate, options->GetSource());
	Database *db = ObjectWrap::Unwrap<Database>(source->ToObject());
	FDBDatabaseOption op = (FDBDatabaseOption)info.Data()->Uint32Value();

	Parameter param = FdbOptions::GetOptionParameter(info, FdbOptions::DatabaseOption, op);
	fdb_error_t errorCode = param.errorCode;
	if(errorCode == 0)
		errorCode = fdb_database_set_option(db->GetDatabase(), op, param.getValue(), param.getLength());

	if(errorCode)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	info.GetReturnValue().SetNull();
}

void SetTransactionOption(const FunctionCallbackInfo<Value>& info) {
	Isolate *isolate = Isolate::GetCurrent();
	FdbOptions *options = ObjectWrap::Unwrap<FdbOptions>(info.Holder());
	Local<Value> source = Local<Value>::New(isolate, options->GetSource());
	Transaction *tr = ObjectWrap::Unwrap<Transaction>(source->ToObject());
	FDBTransactionOption op = (FDBTransactionOption)info.Data()->Uint32Value();

	Parameter param = FdbOptions::GetOptionParameter(info, FdbOptions::TransactionOption, op);
	fdb_error_t errorCode = param.errorCode;
	if(errorCode == 0)
		errorCode = fdb_transaction_set_option(tr->GetTransaction(), op, param.getValue(), param.getLength());

	if(errorCode)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	info.GetReturnValue().SetNull();
}

void CallAtomicOperation(const FunctionCallbackInfo<Value>& info) {
	Transaction *tr = ObjectWrap::Unwrap<Transaction>(info.Holder());
	Parameter key = GetStringParameter(info, 0);
	Parameter value = GetStringParameter(info, 1);

	fdb_error_t errorCode = key.errorCode > 0 ? key.errorCode : value.errorCode;
	if(errorCode > 0)
		return NanThrowError(FdbError::NewInstance(errorCode, fdb_get_error(errorCode)));

	fdb_transaction_atomic_op(tr->GetTransaction(), key.getValue(), key.getLength(), value.getValue(), value.getLength(), (FDBMutationType)info.Data()->Uint32Value());

	info.GetReturnValue().SetNull();
}

//Converts names using underscores as word separators to camel case (but preserves existing capitalization, if present). If isSetter, prepends the word 'set' to each name
std::string FdbOptions::ToJavaScriptName(std::string optionName, bool isSetter) {
	if(isSetter)
		optionName = "set_" + optionName;

	size_t start = 0;
	while(start < optionName.size()) {
		if(start != 0)
			optionName[start] = ::toupper(optionName[start]);
		size_t index = optionName.find_first_of('_', start);
		if(index == std::string::npos)
			break;

		optionName.erase(optionName.begin() + index);

		start = index;
	}

	return optionName;
}

void FdbOptions::Init() {
	scopeInfo[NetworkOption] = ScopeInfo("FdbNetworkOptions", SetNetworkOption);
	scopeInfo[ClusterOption] = ScopeInfo("FdbClusterOptions", SetClusterOption);
	scopeInfo[DatabaseOption] = ScopeInfo("FdbDatabaseOptions", SetDatabaseOption);
	scopeInfo[TransactionOption] = ScopeInfo("FdbTransactionOptions", SetTransactionOption);
	scopeInfo[StreamingMode] = ScopeInfo("FdbStreamingMode", NULL);
	scopeInfo[MutationType] = ScopeInfo("AtomicOperations", CallAtomicOperation);
	//scopeInfo[ConflictRangeType] = ScopeInfo("ConflictRangeType", NULL);

	for(auto itr = scopeInfo.begin(); itr != scopeInfo.end(); ++itr)
		InitOptionsTemplate(itr->first, itr->second.templateClassName.c_str());

	InitOptions();
}
