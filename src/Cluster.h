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

#ifndef FDB_NODE_CLUSTER_H
#define FDB_NODE_CLUSTER_H

#include "Version.h"

#include <foundationdb/fdb_c.h>
#include <node.h>
#include <node_object_wrap.h>

class Cluster : public node::ObjectWrap {
	public:
		static void Init();
		static v8::Handle<v8::Value> NewInstance(FDBCluster *ptr);
		static void New(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void OpenDatabase(const v8::FunctionCallbackInfo<v8::Value>& info);
		static void Destroy(const v8::FunctionCallbackInfo<v8::Value>& info);

		FDBCluster* GetCluster() { return cluster; }

	private:
		Cluster();
		~Cluster();
		static v8::Persistent<v8::Function> constructor;
		FDBCluster *cluster;
};

#endif
