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

var KeySelector = require('./keySelector');
var Future = require('./future');
var fdb = require('./fdbModule');
var LazyIterator = require('./lazyIterator');

function getStreamingMode(requestedMode, limit, wantAll) {
	if(wantAll && requestedMode === fdb.streamingMode.iterator) {
		if(limit)
			return fdb.streamingMode.exact;
		else
			return fdb.streamingMode.wantAll;
	}

	return requestedMode;
}

module.exports = function(tr, start, end, options, snapshot) {
	if(!options)
		options = {};

	if(!options.limit)
		options.limit = 0;
	if(!options.reverse)
		options.reverse = false;
	if(!options.streamingMode && options.streamingMode !== 0)
		options.streamingMode = fdb.streamingMode.iterator;

	var RangeFetcher = function(wantAll) {
		this.finished = false;
		this.limit = options.limit;
		this.iterStart = start;
		this.iterEnd = end;
		this.iterationCount = 1;
		this.streamingMode = getStreamingMode(options.streamingMode, this.limit, wantAll);
	};

	RangeFetcher.prototype.clone = function(wantAll) {
		var clone = new RangeFetcher(wantAll);

		clone.finished = this.finished;
		clone.limit = this.limit;
		clone.iterStart = this.iterStart;
		clone.iterEnd = this.iterEnd;
		clone.iterationCount = this.iterationCount;

		return clone;
	};

	RangeFetcher.prototype.fetch = function(cb) {
		var fetcher = this;
		if(fetcher.finished) {
			cb();
		}
		else {
			tr.getRange(fetcher.iterStart.key, fetcher.iterStart.orEqual, fetcher.iterStart.offset, fetcher.iterEnd.key, fetcher.iterEnd.orEqual, fetcher.iterEnd.offset, fetcher.limit, fetcher.streamingMode, fetcher.iterationCount++, snapshot, options.reverse, function(err, res) 
			{
				if(!err) {
					var results = res.array;
					if(results.length > 0) {
						if(!options.reverse)
							fetcher.iterStart = KeySelector.firstGreaterThan(results[results.length-1].key);
						else
							fetcher.iterEnd = KeySelector.firstGreaterOrEqual(results[results.length-1].key);
					}

					if(fetcher.limit !== 0) {
						fetcher.limit -= results.length;
						if(fetcher.limit <= 0)
							fetcher.finished = true;
					}
					if(!res.more)
						fetcher.finished = true;
					cb(undefined, results);
				}
				else {
					cb(err);
				}
			});
		}
	};

	return new LazyIterator(RangeFetcher);
};
