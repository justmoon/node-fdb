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

"use strict";

var buffer = require('./bufferConversion');
var transactional = require('./retryDecorator');
var Database = require('./database');
var LazyIterator = require('./lazyIterator');
var fdb = require('./fdbModule');
var fdbUtil = require('./fdbUtil');

var KEY_SERVERS_PREFIX = buffer.fromByteLiteral('\xff/keyServers/');
var PAST_VERSION_ERROR_CODE = 1007;

function getBoundaryKeysImpl(tr, begin, end, callback) {
	function BoundaryFetcher(wantAll) {
		this.tr = tr;
		this.begin = begin;
		this.end = end;
		this.lastBegin = undefined;

		var fetcher = this;

		if(wantAll)
			this.streamingMode = fdb.streamingMode.wantAll;
		else
			this.streamingMode = fdb.streamingMode.iterator;

		function iteratorCb(err, res) {
			if(err) {
				if(err.code === PAST_VERSION_ERROR_CODE && fetcher.begin !== fetcher.lastBegin) {
					fetcher.tr = fetcher.tr.db.createTransaction();
					readKeys();
				}
				else {
					fetcher.tr.onError(err, function(e) {
						if(e)
							fetcher.fetchCb(e);
						else
							readKeys();
					});
				}
			}
			else
				fetcher.fetchCb();
		}
	
		function readKeys() {
			fetcher.lastBegin = fetcher.begin;
			fetcher.tr.options.setReadSystemKeys();
			fetcher.tr.snapshot.getRange(fetcher.begin, fetcher.end, {streamingMode: fetcher.streamingMode}).forEachBatch(function(kvs, innerCb) {
				fetcher.forEachCb = innerCb;
				var keys = kvs.map(function(kv) { return kv.key.slice(13); });
				var last = kvs[kvs.length-1].key;
				fetcher.begin = Buffer.concat([last, buffer.fromByteLiteral('\x00')], last.length + 1);
				fetcher.fetchCb(undefined, keys);
			}, iteratorCb);

			fetcher.streamingMode = fdb.streamingMode.wantAll;
		}

		this.fetch = function(cb) {
			this.fetchCb = cb;
			if(this.read)
				this.forEachCb();
			else {
				this.read = true;
				readKeys();
			}
		};

		this.clone = function(wantAll) {
			var clone = new BoundaryFetcher(wantAll);

			clone.tr = this.tr.db.createTransaction();
			clone.begin = this.begin;
			clone.end = this.end;
			clone.lastBegin = this.lastBegin;

			return clone;
		};
	}

	callback(null, new LazyIterator(BoundaryFetcher));
}

function getBoundaryKeys(databaseOrTransaction, begin, end, callback) {
	begin = fdbUtil.keyToBuffer(begin);
	end = fdbUtil.keyToBuffer(end);

	begin = Buffer.concat([KEY_SERVERS_PREFIX, begin], KEY_SERVERS_PREFIX.length + begin.length);
	end = Buffer.concat([KEY_SERVERS_PREFIX, end], KEY_SERVERS_PREFIX.length + end.length);

	if(databaseOrTransaction instanceof Database) {
		getBoundaryKeysImpl(databaseOrTransaction.createTransaction(), begin, end, callback);
	}
	else {
		var tr = databaseOrTransaction.db.createTransaction();
		databaseOrTransaction.getReadVersion(function(err, ver) {
			tr.setReadVersion(ver);
			getBoundaryKeysImpl(tr, begin, end, callback);
		});
	}
}

var getAddressesForKey = transactional(function (tr, key, cb) {
	key = fdbUtil.keyToBuffer(key);
	tr.tr.getAddressesForKey(key, cb);
});

module.exports = {getBoundaryKeys: getBoundaryKeys, getAddressesForKey: getAddressesForKey};
