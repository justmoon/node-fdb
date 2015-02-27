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
var Cluster = require('./cluster');
var future = require('./future');
var Transactional = require('./retryDecorator');
var tuple = require('./tuple');
var buffer = require('./bufferConversion');
var fdb = require('./fdbModule');
var FDBError = require('./error');
var locality = require('./locality');
var directory = require('./directory');
var Subspace = require('./subspace');
var selectedApiVersion = require('./apiVersion');

var fdbModule = {};

module.exports = {
    FDBError: FDBError,
	apiVersion: function(version) {
		if(selectedApiVersion.value && version !== selectedApiVersion.value)
			throw new Error('Cannot select multiple different FDB API versions');
		if(version < 14)
			throw new RangeError('FDB API versions before 14 are not supported');
		if(version > 300)
			throw new RangeError('Latest known FDB API version is 300');

		if(!selectedApiVersion.value) {
			fdb.apiVersion(version);

			if(version < 23)
				Cluster.prototype.createDatabase = Cluster.prototype.openDatabase;

			fdbModule.FDBError = this.FDBError;
			fdbModule.KeySelector = KeySelector;
			fdbModule.future = future;
			fdbModule.transactional = Transactional;
			fdbModule.tuple = tuple;
			fdbModule.buffer = buffer;
			fdbModule.locality = locality;
			fdbModule.directory = directory.directory;
			fdbModule.DirectoryLayer = directory.DirectoryLayer;
			fdbModule.Subspace = Subspace;

			fdbModule.options = fdb.options;
			fdbModule.streamingMode = fdb.streamingMode;

			var dbCache = {};
			var clusterCache = {};

			var doInit = function() {
				fdb.startNetwork();

				process.on('exit', function() {
					//Clearing out the caches makes memory debugging a little easier
					dbCache = null;
					clusterCache = null;

					fdb.stopNetwork();
				});

				//Subsequent calls do nothing
				doInit = function() { };
			};

			fdbModule.init = function() {
				doInit();
			};
			
			fdbModule.createCluster = function(clusterFile, cb) {
				if(!clusterFile)
					clusterFile = '';

				var cluster = new Cluster(fdb.createCluster(clusterFile));

				if(version < 23)
					return future.resolve(cluster)(cb);
				else
					return cluster;
			};

			fdbModule.open = function(clusterFile, databaseName, cb) {
				if(!databaseName)
					databaseName = 'DB';

				if(clusterFile)
					fdb.options.setClusterFile(clusterFile);

				this.init();

				var finish = function(err, database) {
					if(version >= 23) //err will be undefined if version >= 23
						return database;
					else if(err)
						return future.reject(err)(cb);
					else
						return future.resolve(database)(cb);
				};

				var updateDatabaseCacheAndFinish = function(err, database) {
					if(!err)
						dbCache[[clusterFile, databaseName]] = database;

					return finish(err, database);
				};

				var getDatabase = function(cluster) {
					var database = dbCache[[clusterFile, databaseName]];
					if(database)
						return finish(undefined, database);
					else {
						database = cluster.openDatabase(databaseName);
						if(version >= 23)
							return updateDatabaseCacheAndFinish(undefined, database);
						else
							return database(updateDatabaseCacheAndFinish);
					}	
				};

				var updateClusterCacheAndGetDatabase = function(err, cluster) {
					if(err)
						finish(err);
					else {
						clusterCache[clusterFile] = cluster;
						return getDatabase(cluster);
					}
				};

				var cluster = clusterCache[clusterFile];
				if(cluster) 
					return getDatabase(cluster);
				else {
					cluster = fdbModule.createCluster(clusterFile);
					if(version >= 23)
						return updateClusterCacheAndGetDatabase(undefined, cluster);
					else
						return cluster(updateClusterCacheAndGetDatabase);
				}
			};
		}

		selectedApiVersion.value = version;
		return fdbModule;
	}
};

fdb.FDBError = module.exports.FDBError;
