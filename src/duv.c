#import "uv.h"
#import <stdlib.h>
#import <assert.h>

typedef void * duv_ptr;

typedef void (*duv_read_cb)(uv_stream_t* stream, void * context, ssize_t nread, char * buff_data, size_t buff_len);

//
// Handle data pointer wrapper
//
typedef struct {
  void * data;
  duv_read_cb read_cb;
  void * read_context;
} duv_handle_context;


duv_handle_context * duv_ensure_handle_context(uv_handle_t * handle) {
  if(!handle->data) {
    handle->data = malloc(sizeof(duv_handle_context));
  }
  return handle->data;
}

UV_EXTERN void duv_set_handle_data(uv_handle_t* handle, void* data) {
  duv_ensure_handle_context(handle)->data = data;
}

UV_EXTERN void* duv_get_handle_data(uv_handle_t* handle) {
  return duv_ensure_handle_context(handle)->data;
}

UV_EXTERN int duv_tcp_bind4(uv_tcp_t* handle, char *ipv4, int port) {
  struct sockaddr_in addr = uv_ip4_addr(ipv4, port);
  return uv_tcp_bind(handle, addr);
}

typedef void (*duv__write_cb)(uv_stream_t* connection, void * context, int status);

typedef struct  {
  uv_write_t req;
  void * context;
  uv_buf_t * bufs;
  duv__write_cb cb;
  uv_stream_t * connection;
} duv_write_request;



void _duv__write_callback(uv_write_t* req, int status) {
 duv_write_request * request = (duv_write_request*)req;
 request->cb(request->connection, request->context, status);
 free(request->bufs); 
 free(request);
}

UV_EXTERN int duv__write(uv_stream_t* handle, void * context, char * data, int data_len,  duv__write_cb cb) {
  uv_buf_t * bufs = malloc(sizeof(uv_buf_t));
  *bufs = uv_buf_init(data, data_len);

  duv_write_request * req = malloc(sizeof(duv_write_request));
  req->connection = handle;
  req->context = context;
  req->bufs = bufs;
  req->cb = cb;
  return uv_write((uv_write_t*)req, handle, req->bufs, 1, _duv__write_callback);
}

uv_buf_t duv__alloc_cb(uv_handle_t* handle, size_t suggested_size) {
  return uv_buf_init(malloc(suggested_size), suggested_size);
}

void duv__read_cb_bridge(uv_stream_t* stream, ssize_t nread, uv_buf_t buf) {
  duv_handle_context * handle_context = duv_ensure_handle_context((uv_handle_t*)stream);
  handle_context->read_cb(stream, handle_context->read_context, nread, buf.base, buf.len);
}

UV_EXTERN int duv__read_start(uv_stream_t * stream, void * context, duv_read_cb read_cb) {
  duv_handle_context * handle_context = duv_ensure_handle_context((uv_handle_t*)stream);
  handle_context->read_cb = read_cb;
  handle_context->read_context = context;
  return uv_read_start(stream, &duv__alloc_cb, &duv__read_cb_bridge);
}
