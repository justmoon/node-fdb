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

var KeySelector = function(key, orEqual, offset) {
	this.key = fdbUtil.keyToBuffer(key);
	this.orEqual = orEqual;
	this.offset = offset;
};

KeySelector.prototype.next = function() {
	return this.add(1);
};

KeySelector.prototype.prev = function() {
	return this.add(-1);
};

KeySelector.prototype.add = function(addOffset) {
	return new KeySelector(this.key, this.orEqual, this.offset + addOffset);
};

KeySelector.isKeySelector = function(sel) {
	return sel instanceof KeySelector;
};

KeySelector.lastLessThan = function(key) {
	return new KeySelector(key, false, 0);
};

KeySelector.lastLessOrEqual = function(key) {
	return new KeySelector(key, true, 0);
};

KeySelector.firstGreaterThan = function(key) {
	return new KeySelector(key, true, 1);
};

KeySelector.firstGreaterOrEqual = function(key) {
	return new KeySelector(key, false, 1);
};

module.exports = KeySelector;

