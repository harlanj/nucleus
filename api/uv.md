# LibUV API

This addon is located at `global.nucleus.uv` in the JS runtime.  These docs
are heavily based on the [upstream libuv docs](http://docs.libuv.org/en/v1.x/),
but copied here for convenience and modified to match the JavaScript interface
to libuv.

## `uv_loop_t` - Event Loop

Libuv `uv_loop_t` instances will not be exposed directly to JavaScript because
it generally complicates things needlessly.  If your VM supports having multiple
threads, then each one generally needs to have it's own event loop.  Pairing
event loops with VM contexts with threads seems to work well.

### `uv.run()`

This function blocks program execution and waits for events in a loop.
Generally this is the last line in your `main.js`.  In node.js, this is
implicitly run after running through the first tick of your first JS file.

### `uv.walk(callback(handle))`

This function iterates through all the handles currently in the loop.

## `uv_handle_t` - Base handle

`Handle` is the base type for all libuv handle types.

All API methods defined here work with any handle type.The handle is the root
class of most the libuv structures.

### `handle.isActive() → boolean`

Returns `true` if the handle is active, `false` if it’s inactive. What “active”
means depends on the type of handle:

- A `Async` handle is always active and cannot be deactivated, except by closing
  it with `async.close()`.
- A `Pipe`, `Tcp`, `Udp`, etc. handle - basically any handle that deals with
  i/o - is active when it is doing something that involves i/o, like reading,
  writing, connecting, accepting new connections, etc.
- A `Check`, `Idle`, `Timer`, etc. handle is active when it has been started
  with a call to `check.start()`, `idle.start()`, etc.

Rule of thumb: if a handle of type `uv_foo_t` has a `foo.start()` function, then
it’s active from the moment that function is called. Likewise, `foo.stop()`
deactivates the handle again.

### `handle.isClosing() → boolean`

Returns true if the handle is closing or closed, false otherwise.

*Note: This function should only be used between the initialization of the
handle and the arrival of the close callback.*

### `handle.close(onClose)`

Request handle to be closed. `onClose` will be called asynchronously after this
call. This MUST be called on each handle before memory is released.

Handles that wrap file descriptors are closed immediately but `onClose` will
still be deferred to the next iteration of the event loop. It gives you a chance
to free up any resources associated with the handle.

In-progress requests, like `uv_connect_t` or `uv_write_t`, are canceled and have
their callbacks called asynchronously with `err=UV_ECANCELED`.

### `handle.ref()`

Reference the given handle. References are idempotent, that is, if a handle is
already referenced calling this function again will have no effect.

### `handle.unref()`

Un-reference the given handle. References are idempotent, that is, if a handle
is not referenced calling this function again will have no effect.

---

The libuv event loop (if run in the default mode) will run until there are no
active and referenced handles left. The user can force the loop to exit early by
unreferencing handles which are active, for example by calling `timer.unref()`
after calling `timer.start()`.

A handle can be referenced or unreferenced, the refcounting scheme doesn’t use a
counter, so both operations are idempotent.

All handles are referenced when active by default, see `handle.isActive` for a
more detailed explanation on what being active involves.

## `uv_timer_t` - Timer handle

Timer handles are used to schedule callbacks to be called in the future.

### `new Timer() → timer`

Create a new timer instance.

### `timer.start(timeout, repeat, onTimeout)`

Start the timer. *timeout* and *repeat* are in milliseconds.

If *timeout* is zero, the callback fires on the next event loop iteration. If
*repeat* is non-zero, the callback fires first after timeout milliseconds and
then repeatedly after *repeat* milliseconds.

### `timer.stop()`

Stop the timer, the callback will not be called anymore.

### `timer.again()`

Stop the timer, and if it is repeating restart it using the repeat value as the
timeout. If the timer has never been started before it throws UV_EINVAL.

### `timer.setRepeat(repeat)`

Set the repeat interval value in milliseconds. The timer will be scheduled to
run on the given interval, regardless of the callback execution duration, and
will follow normal timer semantics in the case of a time-slice overrun.

For example, if a 50ms repeating timer first runs for 17ms, it will be scheduled
to run again 33ms later. If other tasks consume more than the 33ms following the
first timer callback, then the callback will run as soon as possible.

*Note: If the repeat value is set from a timer callback it does not immediately
take effect. If the timer was non-repeating before, it will have been stopped.
If it was repeating, then the old repeat value will have been used to schedule
the next timeout.*

### `timer.getRepeat() → repeat`

Get the timer repeat value.

## `uv_prepare_t` - Prepare handle

Prepare handles will run the given callback once per loop iteration, right
before polling for i/o.

### `new uv.Prepare() → prepare`

Create a new Prepare instance.

### `prepare.start(onPrepare)`

Start the handle with the given callback.

### `prepare.stop()`

Stop the handle, the callback will no longer be called.

## `uv_check_t` - Check handle

Check handles will run the given callback once per loop iteration, right after
polling for i/o.

### `new uv.Check() → check`

Create a new Check instance.

### `check.start(onCheck)`

Start the handle with the given callback.

### `check.stop()`

Stop the handle, the callback will no longer be called.

## `uv_idle_t` - Idle handle

Idle handles will run the given callback once per loop iteration, right before
the uv_prepare_t handles.

*Note: The notable difference with prepare handles is that when there are active
idle handles, the loop will perform a zero timeout poll instead of blocking for
i/o.*

**Warning: Despite the name, idle handles will get their callbacks called on
every loop iteration, not when the loop is actually “idle”.**

### `new uv.Idle() → idle`

Create a new Idle instance.

### `idle.start(onIdle)`

Start the handle with the given callback.

### `idle.stop()`

Stop the handle, the callback will no longer be called.

## `uv_async_t` - Async handle

Async handles allow the user to “wakeup” the event loop and get a callback
called from another thread.

### `new uv.Async(onAsync)`

Create a new Async instance.

### `async.send()`

Wakeup the event loop and call the async handle’s callback.

*Note: It’s safe to call this function from any thread. The callback will be
called on the loop thread.*

**Warning: libuv will coalesce calls to `async.send()`, that is, not every call
to it will yield an execution of the callback. For example: if `async.send()`
is called 5 times in a row before the callback is called, the callback will only
be called once. If `async.send()` is called again after the callback was called,
it will be called again.**

## `uv_poll_t` - Poll handle

Poll handles are used to watch file descriptors for readability, writability and
disconnection similar to the purpose of poll(2).

The purpose of poll handles is to enable integrating external libraries that
rely on the event loop to signal it about the socket status changes, like c-ares
or libssh2. Using `uv_poll_t` for any other purpose is not recommended;
`uv_tcp_t`, `uv_udp_t`, etc. provide an implementation that is faster and more
scalable than what can be achieved with `uv_poll_t`, especially on Windows.

It is possible that poll handles occasionally signal that a file descriptor is
readable or writable even when it isn’t. The user should therefore always be
prepared to handle EAGAIN or equivalent when it attempts to read from or write
to the fd.

It is not okay to have multiple active poll handles for the same socket, this
can cause libuv to busyloop or otherwise malfunction.

The user should not close a file descriptor while it is being polled by an
active poll handle. This can cause the handle to report an error, but it might
also start polling another socket. However the fd can be safely closed
immediately after a call to `poll.stop()` or `poll.close()`.

*Note: On windows only sockets can be polled with poll handles. On Unix any file
descriptor that would be accepted by poll(2) can be used.*

*Note: On AIX, watching for disconnection is not supported.*

## `uv_signal_t`

*TODO: document this module*

## `uv_process_t`

*TODO: document this module*

## Stream

Stream handles provide an abstraction of a duplex communication channel. Stream
is an abstract type, libuv provides 3 stream implementations in the form of Tcp,
Pipe, and Tty.

### `stream.shutdown(onShutdown) → shutdownReq`

Shutdown the outgoing (write) side of a duplex stream. It waits for pending
write requests to complete. The `onShutdown` callback is called after shutdown
is complete.

### `stream.listen(backlog, onConnection)`

Start listening for incoming connections. `backlog` indicates the number of
connections the kernel might queue, same as listen(2). When a new incoming
connection is received the `onConnection` callback is called.

### `stream.accept(socket)`

This call is used in conjunction with `stream.listen()` to accept incoming
connections. Call this function after receiving a `onConnection` call to accept
the connection.

When the `onConnection` callback is called it is guaranteed that this function
will complete successfully the first time. If you attempt to use it more than
once, it may fail. It is suggested to only call this function once per
`onConnection` call.

*Note: server and client must be handles running on the same loop.*

### `stream.readStart(onRead(err, data))`

Read data from an incoming stream. The `onRead` callback will be made several
times until there is no more data to read or `stream.readStop()` is called.

### `stream.readStop()`

Stop reading data from the stream. The `onRead` callback will no longer be
called.

This function is idempotent and may be safely called on a stopped stream.

### `stream.write(data, onWrite) → writeReq`

Write data to stream. `data` can be either a string or a buffer type.

### `stream.isReadable() → bool`

Returns true if the stream is readable, false otherwise.

### `stream.isWritable() → bool`

Returns true if the stream is writable, false otherwise.

## Tcp

TCP handles are used to represent both TCP streams and servers.

`Tcp.prototype` inherits from `Stream.prototype`.

### `new uv.Tcp() → tcp`

Initialize the handle. No socket is created as of yet.

### `tcp.open(fd)`

Open an existing file descriptor or SOCKET as a TCP handle.

The file descriptor is set to non-blocking mode.

### `tcp.nodelay(enable)`

Enable / disable Nagle’s algorithm.

### `tcp.simultaneous_accepts(enable)`

Enable / disable simultaneous asynchronous accept requests that are queued by
the operating system when listening for new TCP connections.

This setting is used to tune a TCP server for the desired performance. Having
simultaneous accepts can significantly improve the rate of accepting connections
(which is why it is enabled by default) but may lead to uneven load distribution
in multi-process setups.

### `tcp.bind(host, port)`

Bind the handle to an address and port. `host` is a string and can be an IPV4 or
IPV6 value.

When the port is already taken, you can expect to see an UV_EADDRINUSE error
from either `tcp.bind()`, `tcp.listen()` or `tcp.connect()`. That is, a
successful call to this function does not guarantee that the call to
`onConnection` or `onConnect` will succeed as well.

### `tcp.getsockname()` → {family, port, ip}

Get the current address to which the handle is bound.

### `tcp.getpeername()` → {family, port, ip}

Get the address of the peer connected to the handle.]

### `tcp.connect(host, port, onConnection)`

Establish an IPv4 or IPv6 TCP connection.

The callback is made when the connection has been established or when a
connection error happened.

## `uv_pipe_t` - Pipe handle

Pipe handles provide an abstraction over local domain sockets on Unix and named
pipes on Windows.

`uv_pipe_t` is a ‘subclass’ of `uv_stream_t`.

### `new uv.Pipe(ipc) → pipe`

Initialize a pipe handle. The *ipc* argument is a boolean to indicate if this
pipe will be used for handle passing between processes.

### `pipe.open(file)`

Open an existing file descriptor or HANDLE as a pipe.

Changed in version 1.2.1: the file descriptor is set to non-blocking mode.

*Note: The passed file descriptor or HANDLE is not checked for its type, but
it’s required that it represents a valid pipe.*

### `pipe.bind(name)`

Bind the pipe to a file path (Unix) or a name (Windows).

*Note: Paths on Unix get truncated to `sizeof(sockaddr_un.sun_path)` bytes,
typically between 92 and 108 bytes.*

### `pipe.connect(name, onConnect)`

Connect to the Unix domain socket or the named pipe.

*Note: Paths on Unix get truncated to `sizeof(sockaddr_un.sun_path)` bytes,
typically between 92 and 108 bytes.*

### `pipe.getsockname() → name`

Get the name of the Unix domain socket or the named pipe.

### `pipe.getpeername() → name`

Get the name of the Unix domain socket or the named pipe to which the handle is
connected.

### `pipe.pendingInstances(count)`

Set the number of pending pipe instance handles when the pipe server is waiting
for connections.

*Note: This setting applies to Windows only.*

### `pipe.pendingCount() → count`

Get the pending count.

### `pipe.pendingType() → type`

Used to receive handles over IPC pipes.

First - call `pipe.pendingCount()`, if it’s > 0 then initialize a handle of
the given type, returned by `pipe.pendingType()` and call `pipe.accept(handle)`.

## `uv_tty_t` - Tty handle

TTY handles represent a stream for the console.

### `new uv.Tty(fd, readable) → tty`

Initialize a new TTY stream with the given file descriptor. Usually the file
descriptor will be:

- 0 = stdin
- 1 = stdout
- 2 = stderr

*readable*, specifies if you plan on calling `tty.readStart()` with this stream.
stdin is readable, stdout is not.

On Unix this function will determine the path of the fd of the terminal using
ttyname_r(3), open it, and use it if the passed file descriptor refers to a TTY.
This lets libuv put the tty in non-blocking mode without affecting other
processes that share the tty.

This function is not thread safe on systems that don’t support ioctl TIOCGPTN or
TIOCPTYGNAME, for instance OpenBSD and Solaris.

*Note: If reopening the TTY fails, libuv falls back to blocking writes for
non-readable TTY streams.*

### `tty.setMode(mode)`

Set the TTY using the specified terminal mode.

- 0 = UV_TTY_MODE_NORMAL - Initial/normal terminal mode
- 1 = UV_TTY_MODE_RAW - Raw input mode (On Windows, ENABLE_WINDOW_INPUT is also
  enabled)
- 2 = UV_TTY_MODE_IO - Binary-safe I/O mode for IPC (Unix-only)

### `uv.ttyResetMode()`

To be called when the program exits. Resets TTY settings to default values for
the next process to take over.

This function is async signal-safe on Unix platforms but can fail with error
code UV_EBUSY if you call it when execution is inside `tty.setMode()`

### `tty.getWinsize() → [width, height]`

Gets the current Window size.

## `uv_udp_t`

*TODO: document this module*

## `uv_fs_event_t`

*TODO: document this module*

## `uv_fs_poll_t`

*TODO: document this module*

## Filesystem

*TODO: document this module*

## DNS

*TODO: document this module*

## Miscellaneous

*TODO: document this module*
