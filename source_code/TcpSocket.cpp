#include "TcpSocket.h"

uv_tcp_t* TcpSocket::uv_tcp_socket;
uv_loop_t* TcpSocket::uv_loop;

TcpSocket::TcpSocket(uv_loop_t *loop)
{
  uv_loop = loop;
  uv_tcp_socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
  uv_tcp_init(uv_loop, uv_tcp_socket);

}

uv_tcp_t *TcpSocket::getSocket()
{
  return uv_tcp_socket;
}

void TcpSocket::start()
{
  uv_read_start((uv_stream_t*)uv_tcp_socket, allocBuffer, tcpRead);
}

void TcpSocket::close()
{
  uv_close((uv_handle_t*) uv_tcp_socket, NULL);
}

void TcpSocket::tcpRead(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  SocketDescriptor socketDiscriptor = getSocketDescriptor();

  if(nread > 0)
    {
      uv_buf_t buffer = uv_buf_init(buf->base, nread);
//      decodeTcp(QString::fromUtf8(buffer.base, buffer.len), hiveClient);

      return;
    }
  if(nread < 0)
    {
      if (nread != UV_EOF)
        {
          Log::net(Log::Critical, "TcpSocket::tcpRead()", "TCP Read error: " + QString(uv_err_name(nread)));
        }
      Log::net(Log::Normal, "TcpSocket::tcpRead()", "Disconnected from discriptor: " + QString::number(socketDiscriptor));
      uv_close((uv_handle_t*)handle, NULL);
    }

  free(buf->base);
}

void TcpSocket::tcpWrite(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
  req->buf = uv_buf_init(buf->base, nread);
  uv_write((uv_write_t*)req, handle, &req->buf, 1, tcpWriteCb);
}

void TcpSocket::tcpWriteCb(uv_write_t *handle, int status)
{
  if (status)
    {
      fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }

  freeWriteReq(handle);
}

void TcpSocket::allocBuffer(uv_handle_t *handle, size_t suggestedSize, uv_buf_t *buf)
{
  buf->base = (char*) malloc(suggestedSize);
  buf->len = suggestedSize;
}

void TcpSocket::freeWriteReq(uv_write_t *handle)
{
  write_req_t *req = (write_req_t*) handle;
  free(req->buf.base);
  free(req);
}

int TcpSocket::getSocketDescriptor()
{
  int fd;
  uv_fileno((uv_handle_t*) uv_tcp_socket, &fd);
  return fd;
}
