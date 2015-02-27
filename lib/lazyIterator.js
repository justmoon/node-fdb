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

var fdbUtil = require('./fdbUtil');
var future = require('./future');

function fetch(state, cb) {
	if(cb)
		state.fetchCallbacks.push(cb);
	if(!state.fetching) {
		state.fetching = true;
		state.fetcher.fetch(function(err, res) {
			var cbs = state.fetchCallbacks;
			state.fetching = false;
			state.fetchCallbacks = [];
			state.results = res;
			state.index = -1;
			state.finished = !res || res.length === 0;

			for(var i = 0; i < cbs.length; ++i)
				cbs[i](err);
		});
	}
}

function iterState(fetcher) {
	return {
		index: -1,
		results: undefined,

		fetching: false,
		fetchCallbacks: [],
		fetcher: fetcher
	};
}

var LazyIterator = function(Fetcher) {
	this.Fetcher = Fetcher;
	this.stateForNext = undefined;

	this.startState = iterState(new Fetcher());

	var startState = this.startState;
	fetch(this.startState);
};

function copyState(state, wantAll) {
	var newState = iterState();
	newState.index = state.index;
	newState.results = state.results;
	newState.fetching = state.fetching;

	if(state.fetching) {
		state.fetchCallbacks.push(function(err) {
			var cbs = newState.fetchCallbacks;
			newState.index = state.index;
			newState.results = state.results;
			newState.fetching = false;
			newState.fetchCallbacks = [];
			newState.finished = state.finished;
			newState.fetcher = state.fetcher.clone(wantAll);
			for(var i = 0; i < cbs.length; ++i)
				cbs[i](err);
		});
	}
	else {
		newState.fetcher = state.fetcher.clone(wantAll);
	}

	return newState;
}

function nextImpl(state, cb) {
	if(state.finished)
		cb();
	else if(state.results && (state.index + 1) < state.results.length)
		cb(null, state.results[++state.index]);
	else {
		fetch(state, function(err) {
			if(err)
				cb(err);
			else if(state.finished)
				cb();
			else
				nextImpl(state, cb);	
		});
	}
}

LazyIterator.prototype.next = function(cb) {
	var itr = this;
	return future.create(function(futureCb) {
		if(!itr.stateForNext)
			itr.stateForNext = copyState(itr.startState);

		nextImpl(itr.stateForNext, futureCb);
	}, cb);
};

LazyIterator.prototype.forEach = function(func, cb) {
	var itr = this;
	return future.create(function(futureCb) {
		var state = copyState(itr.startState);

		fdbUtil.whileLoop(function(loopCb) {
			nextImpl(state, function(err, res) {
				if(err || !res)
					loopCb(err, null);
				else
					func(res, loopCb);
			});
		}, futureCb);

	}, cb);
};

function forEachBatchImpl(state, func, cb) {
	function loopBody(loopCb) {
		function processBatch(err) {
			if(err || state.finished)
				loopCb(err, null);
			else {
				state.index = state.results.length;
				func(state.results, loopCb);
			}
		}

		if(!state.results || state.index === state.results.length)
			fetch(state, processBatch);
		else
			processBatch();
	}

	fdbUtil.whileLoop(loopBody, cb);
}

LazyIterator.prototype.forEachBatch = function(func, cb) {
	var itr = this;
	return future.create(function(futureCb) {
		forEachBatchImpl(copyState(itr.startState), func, futureCb);
	}, cb);
};

LazyIterator.prototype.toArray = function(cb) {
	var itr = this;
	return future.create(function(futureCb) {
		var state = copyState(itr.startState, true);
		var result = [];

		forEachBatchImpl(state, function(arr, itrCb) {
			result = result.concat(arr);
			itrCb();
		}, function(err, res) {
			if(err)
				futureCb(err);
			else
				futureCb(null, result);
		});
	}, cb);
};

module.exports = LazyIterator;
