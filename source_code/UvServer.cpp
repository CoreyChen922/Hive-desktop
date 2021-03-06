#include "UvServer.h"


uv_loop_t* UvServer::loop;
uv_tcp_t* UvServer::tcp_server;
uv_udp_t* UvServer::udp_server;
QHash<UvServer::SocketDescriptor, HiveProtocol::HiveClientBuffer*> UvServer::buffer_hash;
QHash<QString, UvServer::SocketDescriptor> UvServer::key_sd_hash;


UvServer::UvServer(QObject *parent)
  : QThread(parent)
{
}

UvServer::~UvServer()
{

}

void UvServer::quit()
{
  uv_stop(loop);
//  uv_loop_close(loop);
  Log::net(Log::Normal, "UvServer::closeUvLoop()", "Successfully closed uv event loop.");
}

void UvServer::sendTextMessage(const QJsonObject &msg, const BaseProtocol &protocol)
{
  switch (protocol) {
    case BaseProtocol::Udp:
      {
        uv_udp_send_t *req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
        QByteArray dat = encodeTextMessage(msg);
        uv_buf_t msg = uv_buf_init(dat.data(), dat.count());
        struct sockaddr_in addr;
        uv_ip4_addr("255.255.255.255", 23232, &addr);
        uv_udp_send(req, udp_server, &msg, 1, (const struct sockaddr *)&addr, udpWriteCb);

        Log::net(Log::Normal, "UvServer::sendTextMessage()", "message sent");
        break;
      }
    case BaseProtocol::Tcp:
      {

        break;
      }
    default:
      {

        break;
      }
    }
}

void
UvServer::run()
{
  qDebug()<<"uv thread id: "<<this->currentThreadId();
  Log::net(Log::Normal, "UvServer::run()", "Thread Started");

  loop = uv_default_loop();

  struct sockaddr_in *udpAddr = (sockaddr_in*)malloc(sizeof(sockaddr_in));
  uv_ip4_addr("0.0.0.0", UDP_PORT, udpAddr);
  udp_server = (uv_udp_t*) malloc(sizeof(uv_udp_t));
  uv_udp_init(loop, udp_server);
  uv_udp_bind(udp_server, (const struct sockaddr*) udpAddr, UV_UDP_REUSEADDR);
  uv_udp_recv_start(udp_server, allocBuffer, udpReadCb);
  uv_udp_set_broadcast(udp_server, 1);


  struct sockaddr_in *tcpAddr = (sockaddr_in*)malloc(sizeof(sockaddr_in));
  uv_ip4_addr("0.0.0.0", TCP_PORT, tcpAddr);
  tcp_server = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, tcp_server);
  uv_tcp_bind(tcp_server, (const struct sockaddr*) tcpAddr, 0);
  int r = uv_listen((uv_stream_t*) tcp_server, TCP_BACKLOG, tcpNewConnectionCb);
  if(r)
    {
      Log::net(Log::Error, "UvServer::run()", QString("Listen error: " + QString(uv_strerror(r))));
      fprintf(stderr, "Listen error %s\n", uv_strerror(r));
    }

  uv_timer_t *heartBeatTimer = (uv_timer_t*)malloc(sizeof(uv_timer_t));
  uv_timer_init(loop, heartBeatTimer);
  uv_timer_start(heartBeatTimer, udpHeartBeatCb, 1000, 3000);


  uv_run(loop, UV_RUN_DEFAULT);

  Log::net(Log::Normal, "UvServer::run()", "Quit Thread");
}

void
UvServer::tcpNewConnectionCb(uv_stream_t *handle, int status)
{
  if(status < 0)
    {
      fprintf(stderr, "New connection error %s\n", uv_strerror(status));
      return;
    }

  uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
  uv_tcp_init(loop, client);


  if(uv_accept(handle, (uv_stream_t*)client) == 0)
    {
      uv_read_start((uv_stream_t*)client, allocBuffer, tcpReadCb);
    }
  else
    {
      uv_close((uv_handle_t*) client, NULL);
    }
}

void
UvServer::tcpReadCb(uv_stream_t *handle, ssize_t nread, const uv_buf_t *buf)
{
  SocketDescriptor socketDiscriptor = getSocketDescriptor((uv_handle_t *)handle);

  if(nread > 0)
    {
      HiveClientBuffer *hiveClient;
      if(buffer_hash.contains(socketDiscriptor))
        {
          Log::net(Log::Normal, "UvServer::tcpRead()", "Reading message form old user: " + QString::number(socketDiscriptor));
          hiveClient = buffer_hash.value(socketDiscriptor);
        }
      else
        {
          Log::net(Log::Normal, "UvServer::tcpRead()", "Reading message form new user: " + QString::number(socketDiscriptor));
          hiveClient = new HiveClientBuffer();
          buffer_hash.insert(socketDiscriptor, hiveClient);
        }

      uv_buf_t buffer = uv_buf_init(buf->base, nread);
      decodeTcp(QString::fromUtf8(buffer.base, buffer.len), hiveClient);

      // << echo server
      write_req_t *req = (write_req_t*)malloc(sizeof(write_req_t));
      req->buf = uv_buf_init(buf->base, nread);
      uv_write((uv_write_t*)req, handle, &req->buf, 1, tcpWriteCb);

      return;
    }
  if(nread < 0)
    {
      if (nread != UV_EOF)
        {
          fprintf(stderr, "TCP Read error %s\n", uv_err_name(nread));
        }
      Log::net(Log::Normal, "UvServer::tcpRead()", "Disconnected from discriptor: " + QString::number(socketDiscriptor));

      uv_close((uv_handle_t*)handle, NULL); // NULL is a close callback
    }

  free(buf->base);
}

void
UvServer::tcpWriteCb(uv_write_t *handle, int status)
{
  if (status)
    {
      fprintf(stderr, "Write error %s\n", uv_strerror(status));
    }

  freeWriteReq(handle);
}

void
UvServer::udpReadCb(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags)
{
  if(nread >= 0 && addr) //addr sometimes is a nullptr
    {
      char senderAddr[17] = { 0 };
      uv_ip4_name((const struct sockaddr_in*)addr, senderAddr, 16);
      fprintf(stderr, "Recv from %s\n", senderAddr);

      uv_buf_t buffer = uv_buf_init(buf->base, nread);
      qDebug()<<"base size: "<< nread<<"base content: "<<QString::fromLatin1(buffer.base);
      qDebug()<<decodeHivePacket(buffer.base, &senderAddr[0]);
    }
  else
    {
      fprintf(stderr, "UDP Read error %s\n", uv_err_name(nread));
//      uv_close((uv_handle_t*) handle, NULL);
    }

  free(buf->base);
  return;
}

void UvServer::udpWriteCb(uv_udp_send_t *handle, int status)
{
  if(status)
    {
      fprintf(stderr, "Send error %s\n", uv_strerror(status));
      return;
    }
  free(handle); //<<not sure
  Log::net(Log::Normal, "UvServer::udpWriten()", "UDP message successfully sent!");
}

void UvServer::udpHeartBeatCb(uv_timer_t *handle)
{
  qDebug()<<"----------"<<UV_EOF;
///successful test code for timer not crashing
//  struct sockaddr_in send2;
//  printf("%d", uv_ip4_addr("255.255.255.255", 23232, &send2));
//  uv_udp_send_t *req2 = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
//  uv_buf_t buffer;
//  char text[] = "hello?? :)\0";
//  buffer = uv_buf_init(&text[0], sizeof(text) - 1);
//  uv_udp_send(req2, udp_server, &buffer, 1, (const struct sockaddr *)&send2, udpWriten);

  uv_udp_send_t *req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
  QByteArray dat = encodeHeartBeat();
  uv_buf_t msg = uv_buf_init(dat.data(), dat.count());

  struct sockaddr_in addr;
  uv_ip4_addr("255.255.255.255", 23232, &addr);
  uv_udp_send(req, udp_server, &msg, 1, (const struct sockaddr *)&addr, udpWriteCb);

  Log::net(Log::Normal, "UvServer::udpHeartBeatCb()", "heart beat sent");
}

void
UvServer::allocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  buf->base = (char*) malloc(suggested_size);
  buf->len = suggested_size;
}

void
UvServer::freeWriteReq(uv_write_t *handle)
{
  write_req_t *wr = (write_req_t*) handle;
  free(wr->buf.base);
  free(wr);
}

int
UvServer::getSocketDescriptor(uv_handle_t *handle)
{
  int fd;
  uv_fileno(handle, &fd);
  return fd;
}







