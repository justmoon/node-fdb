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
var future = require('./future');

var strinc = function(str) {
	var buf = buffer(str);

	var lastNonFFByte;
	for(lastNonFFByte = buf.length-1; lastNonFFByte >= 0; --lastNonFFByte)
		if(buf[lastNonFFByte] != 0xFF)
			break;

	if(lastNonFFByte < 0)
		throw new Error('invalid argument \'' + str + '\': prefix must have at least one byte not equal to 0xFF');

	var copy = new Buffer(lastNonFFByte + 1);
	str.copy(copy, 0, 0, copy.length);
	++copy[lastNonFFByte];

	return copy;
};

var whileLoop = function(func, cb) {
	var calledCallback = true;
	function outer(err, res) {
		if(err || typeof(res) !== 'undefined') {
			cb(err, res);
		}
		else if(!calledCallback) {
			calledCallback = true;
		}
		else {
			while(calledCallback) {
				calledCallback = false;
				func(outer);
			}
			
			calledCallback = true;
		}
	}

	outer();
};

var keyToBuffer = function(key) {
	if(typeof(key.asFoundationDBKey) == 'function')
		return buffer(key.asFoundationDBKey());

	return buffer(key);
};

var valueToBuffer = function(val) {
	if(typeof(val.asFoundationDBValue) == 'function')
		return buffer(val.asFoundationDBValue());

	return buffer(val);
};

var buffersEqual = function(buf1, buf2) {
	if(!buf1 || !buf2)
		return buf1 === buf2;

	if(buf1.length !== buf2.length)
		return false;

	for(var i = 0; i < buf1.length; ++i)
		if(buf1[i] !== buf2[i])
			return false;

	return true;
};

module.exports = { 
	strinc: strinc, 
	whileLoop: whileLoop, 
	keyToBuffer: keyToBuffer, 
	valueToBuffer: valueToBuffer,
	buffersEqual: buffersEqual
};

