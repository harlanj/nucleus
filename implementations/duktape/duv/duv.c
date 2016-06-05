#include "duv.h"

#include "utils.h"
#include "loop.h"
#include "handle.h"
#include "timer.h"

static duk_ret_t duv_tostring(duk_context *ctx) {
  duk_push_this(ctx);
  duk_get_prop_string(ctx, -1, "\xffuv-type");
  duk_get_prop_string(ctx, -2, "\xffuv-data");
  const char* type = duv_type_to_string(duk_get_int(ctx, -2));
  void* data = duk_get_buffer(ctx, -1, 0);
  duk_pop_3(ctx);
  duk_push_sprintf(ctx, "[%s %"PRIXPTR"]", type, data);
  return 1;
}


static const duk_function_list_entry duv_handle_methods[] = {
  {"inspect", duv_tostring, 0},
  {"toString", duv_tostring, 0},
  {"close", duv_close, 1},
  {0,0,0}
};

static const duk_function_list_entry duv_timer_methods[] = {
  {"inspect", duv_tostring, 0},
  {"toString", duv_tostring, 0},
  {"start", duv_timer_start, 3},
  {"stop", duv_timer_stop, 0},
  {"again", duv_timer_again, 0},
  {"setRepeat", duv_timer_set_repeat, 1},
  {"getRepeat", duv_timer_get_repeat, 0},
  {0,0,0}
};

// // req.c
// {"cancel", duv_cancel, 1},

// // stream.c
// {"shutdown", duv_shutdown, 2},
// {"listen", duv_listen, 3},
// {"accept", duv_accept, 2},
// {"read_start", duv_read_start, 2},
// {"read_stop", duv_read_stop, 1},
// {"write", duv_write, 3},
// {"is_readable", duv_is_readable, 1},
// {"is_writable", duv_is_writable, 1},
// {"stream_set_blocking", duv_stream_set_blocking, 2},


static const duk_function_list_entry duv_funcs[] = {
  // loop.c
  {"run", duv_run, 0},
  {"walk", duv_walk, 1},

  {"Timer", duv_timer, 0},

  // // tcp.c
  // {"new_tcp", duv_new_tcp, 0},
  // {"tcp_open", duv_tcp_open, 2},
  // {"tcp_nodelay", duv_tcp_nodelay, 2},
  // {"tcp_keepalive", duv_tcp_keepalive, 3},
  // {"tcp_simultaneous_accepts", duv_tcp_simultaneous_accepts, 2},
  // {"tcp_bind", duv_tcp_bind, 3},
  // {"tcp_getpeername", duv_tcp_getpeername, 1},
  // {"tcp_getsockname", duv_tcp_getsockname, 1},
  // {"tcp_connect", duv_tcp_connect, 4},
  //
  // // pipe.c
  // {"new_pipe", duv_new_pipe, 1},
  // {"pipe_open", duv_pipe_open, 2},
  // {"pipe_bind", duv_pipe_bind, 2},
  // {"pipe_connect", duv_pipe_connect, 3},
  // {"pipe_getsockname", duv_pipe_getsockname, 1},
  // {"pipe_pending_instances", duv_pipe_pending_instances, 2},
  // {"pipe_pending_count", duv_pipe_pending_count, 1},
  // {"pipe_pending_type", duv_pipe_pending_type, 1},
  //
  // // tty.c
  // {"new_tty", duv_new_tty, 2},
  // {"tty_set_mode", duv_tty_set_mode, 2},
  // {"tty_reset_mode", duv_tty_reset_mode, 0},
  // {"tty_get_winsize", duv_tty_get_winsize, 1},
  //
  // // fs.c
  // {"fs_close", duv_fs_close, 2},
  // {"fs_open", duv_fs_open, 4},
  // {"fs_read", duv_fs_read, 4},
  // {"fs_unlink", duv_fs_unlink, 2},
  // {"fs_write", duv_fs_write, 4},
  // {"fs_mkdir", duv_fs_mkdir, 3},
  // {"fs_mkdtemp", duv_fs_mkdtemp, 2},
  // {"fs_rmdir", duv_fs_rmdir, 2},
  // {"fs_scandir", duv_fs_scandir, 2},
  // {"fs_scandir_next", duv_fs_scandir_next, 1},
  // {"fs_stat", duv_fs_stat, 2},
  // {"fs_fstat", duv_fs_fstat, 2},
  // {"fs_lstat", duv_fs_lstat, 2},
  // {"fs_rename", duv_fs_rename, 3},
  // {"fs_fsync", duv_fs_fsync, 2},
  // {"fs_fdatasync", duv_fs_fdatasync, 2},
  // {"fs_ftruncate", duv_fs_ftruncate, 3},
  // {"fs_sendfile", duv_fs_sendfile, 5},
  // {"fs_access", duv_fs_access, 3},
  // {"fs_chmod", duv_fs_chmod, 3},
  // {"fs_fchmod", duv_fs_fchmod, 3},
  // {"fs_utime", duv_fs_utime, 4},
  // {"fs_futime", duv_fs_futime, 4},
  // {"fs_link", duv_fs_link, 3},
  // {"fs_symlink", duv_fs_symlink, 4},
  // {"fs_readlink", duv_fs_readlink, 2},
  // {"fs_chown", duv_fs_chown, 4},
  // {"fs_fchown", duv_fs_fchown, 4},
  //
  // // misc.c
  // {"guess_handle", duv_guess_handle, 1},
  // {"version", duv_version, 0},
  // {"version_string", duv_version_string, 0},
  // {"get_process_title", duv_get_process_title, 0},
  // {"set_process_title", duv_set_process_title, 1},
  // {"resident_set_memory", duv_resident_set_memory, 0},
  // {"uptime", duv_uptime, 0},
  // {"getrusage", duv_getrusage, 0},
  // {"cpu_info", duv_cpu_info, 0},
  // {"interface_addresses", duv_interface_addresses, 0},
  // {"loadavg", duv_loadavg, 0},
  // {"exepath", duv_exepath, 0},
  // {"cwd", duv_cwd, 0},
  // {"os_homedir", duv_os_homedir, 0},
  // {"chdir", duv_chdir, 1},
  // {"get_total_memory", duv_get_total_memory, 0},
  // {"hrtime", duv_hrtime, 0},
  // {"update_time", duv_update_time, 0},
  // {"now", duv_now, 0},
  // {"argv", duv_argv, 0},
  //
  // // miniz.c
  // {"inflate", duv_tinfl, 2},
  // {"deflate", duv_tdefl, 2},

  {NULL, NULL, 0},
};

duk_ret_t duv_push_module(duk_context *ctx) {
  // duv
  duk_push_object(ctx);
  duk_put_function_list(ctx, -1, duv_funcs);

  // duv.Handle.prototype
  duk_push_object(ctx);
  duk_put_function_list(ctx, -1, duv_handle_methods);
  duk_get_prop_string(ctx, -2, "Timer");

  // duv.Timer.prototype
  duk_push_object(ctx);
  duk_put_function_list(ctx, -1, duv_timer_methods);
  duk_dup(ctx, -3);
  duk_set_prototype(ctx, -2);
  duk_put_prop_string(ctx, -2, "prototype");
  duk_pop(ctx);

  // pop Handle.prototype
  duk_pop(ctx);

  return 1;
}