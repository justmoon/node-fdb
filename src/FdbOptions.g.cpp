/*
 * FoundationDB Node.js API
 * Copyright (c) 2013 FoundationDB, LLC
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

#include "FdbOptions.h"

void FdbOptions::InitOptions() {
	// Deprecated
	// Parameter: (String) IP:PORT
	ADD_OPTION(NetworkOption, "local_address", 10, String);

	// Deprecated
	// Parameter: (String) path to cluster file
	ADD_OPTION(NetworkOption, "cluster_file", 20, String);

	// Enables trace output to a file in a directory of the clients choosing
	// Parameter: (String) path to output directory (or NULL for current working directory)
	ADD_OPTION(NetworkOption, "trace_enable", 30, String);

	// Sets the maximum size in bytes of a single trace output file. This value should be in the range ``[0, INT64_MAX]``. If the value is set to 0, there is no limit on individual file size. The default is a maximum size of 10,485,760 bytes.
	// Parameter: (Int) max size of a single trace output file
	ADD_OPTION(NetworkOption, "trace_roll_size", 31, Int);

	// Sets the maximum size of a all the trace output files put together. This value should be in the range ``[0, INT64_MAX]``. If the value is set to 0, there is no limit on the total size of the files. The default is a maximum size of 104,857,600 bytes. If the default roll size is used, this means that a maximum of 10 trace files will be written at a time.
	// Parameter: (Int) max total size of trace files
	ADD_OPTION(NetworkOption, "trace_max_logs_size", 32, Int);

	// Set internal tuning or debugging knobs
	// Parameter: (String) knob_name=knob_value
	ADD_OPTION(NetworkOption, "knob", 40, String);

	// Set the TLS plugin to load. This option, if used, must be set before any other TLS options
	// Parameter: (String) file path or linker-resolved name
	ADD_OPTION(NetworkOption, "TLS_plugin", 41, String);

	// Set the certificate chain
	// Parameter: (Bytes) certificates
	ADD_OPTION(NetworkOption, "TLS_cert_bytes", 42, Bytes);

	// Set the file from which to load the certificate chain
	// Parameter: (String) file path
	ADD_OPTION(NetworkOption, "TLS_cert_path", 43, String);

	// Set the private key corresponding to your own certificate
	// Parameter: (Bytes) key
	ADD_OPTION(NetworkOption, "TLS_key_bytes", 45, Bytes);

	// Set the file from which to load the private key corresponding to your own certificate
	// Parameter: (String) file path
	ADD_OPTION(NetworkOption, "TLS_key_path", 46, String);

	// Set the peer certificate field verification criteria
	// Parameter: (Bytes) verification pattern
	ADD_OPTION(NetworkOption, "TLS_verify_peers", 47, Bytes);

	// Set the size of the client location cache. Raising this value can boost performance in very large databases where clients access data in a near-random pattern. Defaults to 100000.
	// Parameter: (Int) Max location cache entries
	ADD_OPTION(DatabaseOption, "location_cache_size", 10, Int);

	// Set the maximum number of watches allowed to be outstanding on a database connection. Increasing this number could result in increased resource usage. Reducing this number will not cancel any outstanding watches. Defaults to 10000 and cannot be larger than 1000000.
	// Parameter: (Int) Max outstanding watches
	ADD_OPTION(DatabaseOption, "max_watches", 20, Int);

	// Specify the machine ID that was passed to fdbserver processes running on the same machine as this client, for better location-aware load balancing.
	// Parameter: (String) Hexadecimal ID
	ADD_OPTION(DatabaseOption, "machine_id", 21, String);

	// Specify the datacenter ID that was passed to fdbserver processes running in the same datacenter as this client, for better location-aware load balancing.
	// Parameter: (String) Hexadecimal ID
	ADD_OPTION(DatabaseOption, "datacenter_id", 22, String);

	// The transaction, if not self-conflicting, may be committed a second time after commit succeeds, in the event of a fault
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "causal_write_risky", 10, None);

	// The read version will be committed, and usually will be the latest committed, but might not be the latest committed in the event of a fault or partition
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "causal_read_risky", 20, None);

	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "causal_read_disable", 21, None);

	// The next write performed on this transaction will not generate a write conflict range. As a result, other transactions which read the key(s) being modified by the next write will not conflict with this transaction. Care needs to be taken when using this option on a transaction that is shared between multiple threads. When setting this option, write conflict ranges will be disabled on the next write operation, regardless of what thread it is on.
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "next_write_no_write_conflict_range", 30, None);

	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "check_writes_enable", 50, None);

	// Reads performed by a transaction will not see any prior mutations that occured in that transaction, instead seeing the value which was in the database at the transaction's read version. This option may provide a small performance benefit for the client, but also disables a number of client-side optimizations which are beneficial for transactions which tend to read and write the same keys within a single transaction.
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "read_your_writes_disable", 51, None);

	// Disables read-ahead caching for range reads. Under normal operation, a transaction will read extra rows from the database into cache if range reads are used to page through a series of data one row at a time (i.e. if a range read with a one row limit is followed by another one row range read starting immediately after the result of the first).
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "read_ahead_disable", 52, None);

	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "durability_datacenter", 110, None);

	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "durability_risky", 120, None);

	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "durability_dev_null_is_web_scale", 130, None);

	// Specifies that this transaction should be treated as highest priority and that lower priority transactions should block behind this one. Use is discouraged outside of low-level tools
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "priority_system_immediate", 200, None);

	// Specifies that this transaction should be treated as low priority and that default priority transactions should be processed first. Useful for doing batch work simultaneously with latency-sensitive work
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "priority_batch", 201, None);

	// This is a write-only transaction which sets the initial configuration. This option is designed for use by database system tools only.
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "initialize_new_database", 300, None);

	// Allows this transaction to read and modify system keys (those that start with the byte 0xFF)
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "access_system_keys", 301, None);

	// Allows this transaction to read system keys (those that start with the byte 0xFF)
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "read_system_keys", 302, None);

	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "debug_dump", 400, None);

	// Parameter: (String) Optional transaction name
	ADD_OPTION(TransactionOption, "debug_retry_logging", 401, String);

	// Set a timeout in milliseconds which, when elapsed, will cause the transaction automatically to be cancelled. Valid parameter values are ``[0, INT_MAX]``. If set to 0, will disable all timeouts. All pending and any future uses of the transaction will throw an exception. The transaction can be used again after it is reset. Like all transaction options, a timeout must be reset after a call to onError. This behavior allows the user to make the timeout dynamic.
	// Parameter: (Int) value in milliseconds of timeout
	ADD_OPTION(TransactionOption, "timeout", 500, Int);

	// Set a maximum number of retries after which additional calls to onError will throw the most recently seen error code. Valid parameter values are ``[-1, INT_MAX]``. If set to -1, will disable the retry limit. Like all transaction options, the retry limit must be reset after a call to onError. This behavior allows the user to make the retry limit dynamic.
	// Parameter: (Int) number of times to retry
	ADD_OPTION(TransactionOption, "retry_limit", 501, Int);

	// Set the maximum amount of backoff delay incurred in the call to onError if the error is retryable. Defaults to 1000 ms. Valid parameter values are ``[0, INT_MAX]``. Like all transaction options, the maximum retry delay must be reset after a call to onError. If the maximum retry delay is less than the current retry delay of the transaction, then the current retry delay will be clamped to the maximum retry delay.
	// Parameter: (Int) value in milliseconds of maximum delay
	ADD_OPTION(TransactionOption, "max_retry_delay", 502, Int);

	// Snapshot read operations will see the results of writes done in the same transaction.
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "snapshot_ryw_enable", 600, None);

	// Snapshot read operations will not see the results of writes done in the same transaction.
	// Parameter: Option takes no parameter
	ADD_OPTION(TransactionOption, "snapshot_ryw_disable", 601, None);

	// Client intends to consume the entire range and would like it all transferred as early as possible.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "want_all", -2, None);

	// The default. The client doesn't know how much of the range it is likely to used and wants different performance concerns to be balanced. Only a small portion of data is transferred to the client initially (in order to minimize costs if the client doesn't read the entire range), and as the caller iterates over more items in the range larger batches will be transferred in order to minimize latency.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "iterator", -1, None);

	// Infrequently used. The client has passed a specific row limit and wants that many rows delivered in a single batch. Because of iterator operation in client drivers make request batches transparent to the user, consider ``WANT_ALL`` StreamingMode instead. A row limit must be specified if this mode is used.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "exact", 0, None);

	// Infrequently used. Transfer data in batches small enough to not be much more expensive than reading individual rows, to minimize cost if iteration stops early.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "small", 1, None);

	// Infrequently used. Transfer data in batches sized in between small and large.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "medium", 2, None);

	// Infrequently used. Transfer data in batches large enough to be, in a high-concurrency environment, nearly as efficient as possible. If the client stops iteration early, some disk and network bandwidth may be wasted. The batch size may still be too small to allow a single client to get high throughput from the database, so if that is what you need consider the SERIAL StreamingMode.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "large", 3, None);

	// Transfer data in batches large enough that an individual client can get reasonable read bandwidth from the database. If the client stops iteration early, considerable disk and network bandwidth may be wasted.
	// Parameter: Option takes no parameter
	ADD_OPTION(StreamingMode, "serial", 4, None);

	// Performs an addition of little-endian integers. If the existing value in the database is not present or shorter than ``param``, it is first extended to the length of ``param`` with zero bytes.  If ``param`` is shorter than the existing value in the database, the existing value is truncated to match the length of ``param``. The integers to be added must be stored in a little-endian representation.  They can be signed in two's complement representation or unsigned. You can add to an integer at a known offset in the value by prepending the appropriate number of zero bytes to ``param`` and padding with zero bytes to match the length of the value. However, this offset technique requires that you know the addition will not cause the integer field within the value to overflow.
	// Parameter: (Bytes) addend
	ADD_OPTION(MutationType, "add", 2, Bytes);

	// Deprecated
	// Parameter: (Bytes) value with which to perform bitwise and
	ADD_OPTION(MutationType, "and", 6, Bytes);

	// Performs a bitwise ``and`` operation.  If the existing value in the database is not present or shorter than ``param``, it is first extended to the length of ``param`` with zero bytes.  If ``param`` is shorter than the existing value in the database, the existing value is truncated to match the length of ``param``.
	// Parameter: (Bytes) value with which to perform bitwise and
	ADD_OPTION(MutationType, "bit_and", 6, Bytes);

	// Deprecated
	// Parameter: (Bytes) value with which to perform bitwise or
	ADD_OPTION(MutationType, "or", 7, Bytes);

	// Performs a bitwise ``or`` operation.  If the existing value in the database is not present or shorter than ``param``, it is first extended to the length of ``param`` with zero bytes.  If ``param`` is shorter than the existing value in the database, the existing value is truncated to match the length of ``param``.
	// Parameter: (Bytes) value with which to perform bitwise or
	ADD_OPTION(MutationType, "bit_or", 7, Bytes);

	// Deprecated
	// Parameter: (Bytes) value with which to perform bitwise xor
	ADD_OPTION(MutationType, "xor", 8, Bytes);

	// Performs a bitwise ``xor`` operation.  If the existing value in the database is not present or shorter than ``param``, it is first extended to the length of ``param`` with zero bytes.  If ``param`` is shorter than the existing value in the database, the existing value is truncated to match the length of ``param``.
	// Parameter: (Bytes) value with which to perform bitwise xor
	ADD_OPTION(MutationType, "bit_xor", 8, Bytes);

	// Performs a little-endian comparison of byte strings. If the existing value in the database is not present or shorter than ``param``, it is first extended to the length of ``param`` with zero bytes.  If ``param`` is shorter than the existing value in the database, the existing value is truncated to match the length of ``param``. The larger of the two values is then stored in the database.
	// Parameter: (Bytes) value to check against database value
	ADD_OPTION(MutationType, "max", 12, Bytes);

	// Performs a little-endian comparison of byte strings. If the existing value in the database is not present or shorter than ``param``, it is first extended to the length of ``param`` with zero bytes.  If ``param`` is shorter than the existing value in the database, the existing value is truncated to match the length of ``param``. The smaller of the two values is then stored in the database.
	// Parameter: (Bytes) value to check against database value
	ADD_OPTION(MutationType, "min", 13, Bytes);

	// Used to add a read conflict range
	// Parameter: Option takes no parameter
	ADD_OPTION(ConflictRangeType, "read", 0, None);

	// Used to add a write conflict range
	// Parameter: Option takes no parameter
	ADD_OPTION(ConflictRangeType, "write", 1, None);

}
