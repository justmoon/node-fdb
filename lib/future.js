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

function isFunction(f) {
	return typeof(f) == 'function';	
}

function isObject(o) {
	return o === Object(o);
}

var resolvePromise = function(promise, value) {
	var called = false;
	try {
		if(promise === value)
			promise._state.reject(new TypeError('promise.then cannot be fulfilled with itself as the argument.'));

		if(isObject(value)) {
			var then = value.then;
			if(isFunction(then)) {
				then.call(value, function(res) {
					if(!called) {
						called = true;
						resolvePromise(promise, res, promise);
					}
				}, function(err) {
					if(!called) {
						called = true;
						promise._state.reject(err);
					}
				});
			}
			else 
				promise._state.fulfill(value);
		}
		else
			promise._state.fulfill(value);
	}
	catch(error) {
		if(!called)
			promise._state.reject(error);
	}
};	

var FuturePrototype = {
	cancel: function() {
		//cancel is not implemented for most futures
	},

	then: function(onFulfilled, onRejected) {
		var self = this;
		var future = create();
		this._state.addCallback(function(err, res) {
			process.nextTick(function() {
				try {
					if(self._state.rejected) {
						if(isFunction(onRejected))
							res = onRejected(err);
						else {
							future._state.reject(err);
							return;
						}
					}
					else if(isFunction(onFulfilled))
						res = onFulfilled(res);

					resolvePromise(future, res);
				}
				catch(error) {
					future._state.reject(error);
				}
			});
		});

		return future;
	},

	"catch": function(onRejected) {
		this.then(undefined, onRejected);
	}
};

FuturePrototype.__proto__ = Function.__proto__;

var FutureState = function() {
	this.callbacks = [];
	this.fulfilled = false;
	this.rejected = false;
};

FutureState.prototype.triggerCallbacks = function() {
	for(var i = 0; i < this.callbacks.length; ++i)
		this.callbacks[i](this.error, this.value);

	this.callbacks = [];
};

FutureState.prototype.addCallback = function(cb) {
	if(!this.rejected && !this.fulfilled)
		this.callbacks.push(cb);
	else
		cb(this.error, this.value);
};

FutureState.prototype.fulfill = function(value) {
	if(!this.fulfilled && !this.rejected) {
		this.fulfilled = true;
		this.value = value;
		this.triggerCallbacks();
	}
};

FutureState.prototype.reject = function(reason) {
	if(!this.fulfilled && !this.rejected) {
		this.rejected = true;
		this.error = reason;
		this.triggerCallbacks();
	}
};

var getFutureCallback = function(futureState) {
	return function(err, val) {
		if(err)
			futureState.reject(err);
		else
			futureState.fulfill(val);
	};
};

var create = function(func, cb) {
	if(cb)
		func(cb);
	else {
		// This object is used to break a reference cycle with C++ objects
		var futureState = new FutureState();

		var future = function(callback) {
			if(typeof callback === 'undefined')
				return future;

			future.then(function(val) { callback(undefined, val); }, callback);
		};

		future._state = futureState;
		future.__proto__ = FuturePrototype;

		if(func)
			func.call(future, getFutureCallback(futureState));

		return future;
	}
};

var resolve = function(value) {
	var f = create();
	f._state.fulfill(value);
	return f;
};

var reject = function(reason) {
	var f = create();
	f._state.reject(reason);
	return f;
};

var all = function(futures) {
	var future = create(function(futureCb) {
		var count = futures.length;

		if(count === 0)
			futureCb(undefined, []);

		var successCallback = function() {
			if(--count === 0)
				futureCb(undefined, futures.map(function(f) { return f._state.value; }));
		};
		
		for(var i = 0; i < futures.length; ++i) {
			if(futures[i] && isFunction(futures[i].then))
				futures[i].then(successCallback, futureCb);
			else
				successCallback();
		}
	});

	return future;
};

var race = function(futures) {
	var future = create(function(futureCb) {
		var successCallback = function(val) {
			futureCb(undefined, val);
		};

		for(var i = 0; i < futures.length; ++i) {
			if(futures[i] && isFunction(futures[i].then))
				futures[i].then(successCallback, futureCb);
			else {
				futureCb(undefined, futures[i]);
				break;
			}
		}
	});

	return future;
};

module.exports = { 
	FuturePrototype: FuturePrototype, 
	create: create, 
	resolve: resolve, 
	reject: reject, 
	all: all,
	race: race
};

