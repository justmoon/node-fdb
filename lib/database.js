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

var Transaction = require('./transaction');
var future = require('./future');
var fdb = require('./fdbModule');
var fdbUtil = require('./fdbUtil');
var apiVersion = require('./apiVersion');

var onError = function(tr, err, func, cb) {
	tr.onError(err, function(retryErr, retryRes) {
		if(retryErr)
			cb(retryErr, retryRes);
		else
			retryLoop(tr, func, cb);
	});
};

var retryLoop = function(tr, func, cb) {
	func(tr, function(err, res) {
		if(err) {
			onError(tr, err, func, cb);
		}
		else {
			tr.commit(function(commitErr, commitRes) {
				if(commitErr)
					onError(tr, commitErr, func, cb);
				else
					cb(commitErr, res);
			});
		}
	});
};

var atomic = function(db, op) {
	return function(key, value, cb) {
		return db.doTransaction(function(tr, innerCb) {
			fdb.atomic[op].call(tr.tr, fdbUtil.keyToBuffer(key), fdbUtil.valueToBuffer(value));
			innerCb();
		}, cb);
	};
};

var Database = function(_db) {
	this._db = _db;
	this.options = _db.options;

	for(var op in fdb.atomic)
		this[op] = atomic(this, op);
};

Database.prototype.createTransaction = function() {
	return new Transaction(this, this._db.createTransaction());
};

Database.prototype.doTransaction = function(func, cb) {
	var tr = this.createTransaction();

	return future.create(function(futureCb) {
		retryLoop(tr, func, futureCb);
	}, cb); 
};

Database.prototype.get = function(key, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.get(key, innerCb);
	}, cb);
};

Database.prototype.getKey = function(keySelector, cb) {
	return this.doTransaction(function(tr, innerCb) {	
		tr.getKey(keySelector, innerCb);
	}, cb);
};

Database.prototype.getRange = function(start, end, options, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.getRange(start, end, options).toArray(innerCb);
	}, cb);
};

Database.prototype.getRangeStartsWith = function(prefix, options, cb) {
	return this.doTransaction(function(tr, innerCb) {
		try {
			tr.getRangeStartsWith(prefix, options).toArray(innerCb);
		}
		catch(e) {
			innerCb(e);
		}
	}, cb);
};

Database.prototype.getAndWatch = function(key, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.get(key, function(err, val) {
			if(err)
				innerCb(err);
			else
				innerCb(undefined, { value: val, watch: tr.watch(key) });
		});
	}, cb);
};

Database.prototype.setAndWatch = function(key, value, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.set(key, value);
		var watchObj = tr.watch(key);
		if(apiVersion.value >= 200)
			innerCb(undefined, { watch: watchObj });
		else
			innerCb(undefined, watchObj);
	}, cb);
};

Database.prototype.clearAndWatch = function(key, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.clear(key);
		var watchObj = tr.watch(key);
		if(apiVersion.value >= 200)
			innerCb(undefined, { watch: watchObj });
		else
			innerCb(undefined, watchObj);
	}, cb);
};

Database.prototype.set = function(key, value, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.set(key, value);
		innerCb();
	}, cb);
};

Database.prototype.clear = function(key, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.clear(key);
		innerCb();
	}, cb);
};

Database.prototype.clearRange = function(start, end, cb) {
	return this.doTransaction(function(tr, innerCb) {
		tr.clearRange(start, end);
		innerCb();
	}, cb);
};

Database.prototype.clearRangeStartsWith = function(prefix, cb) {
	return this.doTransaction(function(tr, innerCb) {
		try {
			tr.clearRangeStartsWith(prefix);
			innerCb();
		}
		catch(e) {
			innerCb(e);
		}
	}, cb);
};

module.exports = Database;
