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

#ifndef FDB_NODE_FDB_OPTIONS_H
#define FDB_NODE_FDB_OPTIONS_H

#define ADD_OPTION(scope, name, value, type) AddOption(scope, name, value, type)

#include "Version.h"

#include <foundationdb/fdb_c.h>
#include <node.h>
#include <v8-util.h>
#include <node_object_wrap.h>
#include <string>
#include <map>

struct Parameter {
	Parameter() : isNull(true), errorCode(0) { }
	Parameter(std::string param) : param(param), isNull(false), errorCode(0) { }
	Parameter(fdb_error_t errorCode) : isNull(false), errorCode(errorCode) { }

	std::string param;
	bool isNull;
	fdb_error_t errorCode;

	uint8_t const* getValue() { return isNull ? NULL : (uint8_t const*)param.c_str(); }
	int getLength() { return isNull ? 0 : (int)param.size(); }
};

struct ScopeInfo {
	std::string templateClassName;
	void (*optionFunction) (const v8::FunctionCallbackInfo<v8::Value>& info);

	ScopeInfo() { }
	ScopeInfo(std::string templateClassName, void (*optionFunction) (const v8::FunctionCallbackInfo<v8::Value>& info)) {
		this->templateClassName = templateClassName;
		this->optionFunction = optionFunction;
	}
};

class FdbOptions : public node::ObjectWrap {
	public:
		static void Init();

		enum ParameterType {
			None,
			Int,
			String,
			Bytes
		};

		enum Scope {
			NetworkOption,
			ClusterOption,
			DatabaseOption,
			TransactionOption,
			StreamingMode,
			MutationType,
			ConflictRangeType
		};

		static v8::Handle<v8::Value> CreateOptions(Scope scope, v8::Handle<v8::Value> source = v8::Null(v8::Isolate::GetCurrent()));
		static v8::Handle<v8::Value> CreateEnum(Scope scope);

		static Parameter GetOptionParameter(const v8::FunctionCallbackInfo<v8::Value>& info, Scope scope, int optionValue, int index = 0);

		v8::Persistent<v8::Value>& GetSource() {
			return source;
		}

	private:
		typedef v8::Persistent<v8::FunctionTemplate> PersistentFnTemplate;
		typedef v8::PersistentValueMap<Scope, v8::FunctionTemplate, v8::DefaultPersistentValueMapTraits<Scope, v8::FunctionTemplate > > PersistentFnTemplateMap;

		static void New(const v8::FunctionCallbackInfo<v8::Value>& info);
		static v8::Handle<v8::Value> NewInstance(v8::Local<v8::FunctionTemplate> optionsTemplate, v8::Handle<v8::Value> source);

		FdbOptions();
		~FdbOptions();

		void Clear();

		static void InitOptionsTemplate(Scope scope, const char *className);
		static void InitOptions();

		static void AddOption(Scope scope, std::string name, int value, ParameterType type);
		static void WeakCallback(const v8::WeakCallbackData<v8::Value, FdbOptions>& data);

		static std::string ToJavaScriptName(std::string optionName, bool isSetter);

		static std::map<Scope, ScopeInfo> scopeInfo;
		static PersistentFnTemplateMap optionTemplates;
		static std::map<Scope, std::map<int, ParameterType>> parameterTypes;

		v8::Persistent<v8::Value> source;
};

#endif
