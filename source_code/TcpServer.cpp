#include "TcpServer.h"

uv_tcp_t* TcpServer::uv_tcp_server;
uv_loop_t* TcpServer::uv_loop;


TcpServer::TcpServer(const char *ipAddr, const int &port, const int &backLog, uv_loop_t *loop)
{
  uv_loop = loop;
  struct sockaddr_in *socketAddr = (sockaddr_in*)malloc(sizeof(sockaddr_in));
  uv_ip4_addr(ipAddr, port, socketAddr);

  uv_tcp_server = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
  uv_tcp_init(uv_loop, uv_tcp_server);
  uv_tcp_bind(uv_tcp_server, (const struct sockaddr*) socketAddr, 0);
  int r = uv_listen((uv_stream_t*) uv_tcp_server, backLog, tcpNewConnectionCb);
  if(r)
    {
      Log::net(Log::Error, "UvServer::run()", QString("Listen error: " + QString(uv_strerror(r))));
      fprintf(stderr, "Listen error %s\n", uv_strerror(r));
    }


}

bool TcpServer::accept(uv_stream_t *handle, TcpSocket *client)
{
  return uv_accept(handle, (uv_stream_t*)client->getSocket()) == 0;
}

void TcpServer::tcpNewConnectionCb(uv_stream_t *handle, int status)
{
  if(status < 0)
    {
      Log::net(Log::Critical, "TcpServer::tcpNewConnectionCb()", uv_strerror(status));
      return;
    }

  TcpSocket *client = new TcpSocket(uv_loop);
  if(accept(handle, client))
    {
      client->start();
    }
  else
    {
      client->close();
    }
}











