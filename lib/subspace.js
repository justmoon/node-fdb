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
var fdbUtil = require('./fdbUtil');
var tuple = require('./tuple');

var Subspace = function(prefixArray, rawPrefix) {
	if(typeof rawPrefix === 'undefined')
		rawPrefix = new Buffer(0);
	if(typeof prefixArray === 'undefined')
		prefixArray = [];

	rawPrefix = fdbUtil.keyToBuffer(rawPrefix);
	var packed = tuple.pack(prefixArray);

	this.rawPrefix = Buffer.concat([rawPrefix, packed], rawPrefix.length + packed.length);
};

Subspace.prototype.key = function() {
	return this.rawPrefix;
};

Subspace.prototype.pack = function(arr) {
	var packed = tuple.pack(arr);
	return Buffer.concat([this.rawPrefix, packed], this.rawPrefix.length + packed.length) ;
};

Subspace.prototype.unpack = function(key) {
	key = fdbUtil.keyToBuffer(key);
	if(!this.contains(key))
		throw new Error('Cannot unpack key that is not in subspace.');

	return tuple.unpack(key.slice(this.rawPrefix.length));
};

Subspace.prototype.range = function(arr) {
	if(typeof arr === 'undefined')
		arr = [];

	var range = tuple.range(arr);
	return { 
		begin: Buffer.concat([this.rawPrefix, range.begin], this.rawPrefix.length + range.begin.length),
		end: Buffer.concat([this.rawPrefix, range.end], this.rawPrefix.length + range.end.length)
	};
};

Subspace.prototype.contains = function(key) {
	key = fdbUtil.keyToBuffer(key);
	return key.length >= this.rawPrefix.length && fdbUtil.buffersEqual(key.slice(0, this.rawPrefix.length), this.rawPrefix);
};

Subspace.prototype.get = function(item) {
	return this.subspace([item]);
};

Subspace.prototype.subspace = function(arr) {
	return new Subspace(arr, this.rawPrefix);
};

Subspace.prototype.asFoundationDBKey = function() {
	return this.key();
};

module.exports = Subspace;
