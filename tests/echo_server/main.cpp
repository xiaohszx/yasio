#include "yasio/yasio.hpp"

using namespace yasio::inet;

// 6: TCP server, 10: UDP server, 26: KCP server
void run_echo_server(const char* ip, u_short port, int channel_kind)
{
  io_hostent endpoints[] = {{ip, port}};
  io_service server(endpoints, 1);
  server.set_option(YOPT_S_NO_NEW_THREAD, 1);
  server.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 0);

  // !important, because we set YOPT_S_NO_NEW_THREAD, so need a timer to start server
  deadline_timer timer(server);
  timer.expires_from_now(std::chrono::seconds(1));
  timer.async_wait_once([&, channel_kind]() {
    server.set_option(YOPT_C_LFBFD_PARAMS, 0, 65535, -1, 0, 0);
    server.open(0, channel_kind);
  });
  server.start([&, channel_kind](event_ptr ev) {
    switch (ev->kind())
    {
      case YEK_PACKET:
        server.write(ev->transport(), std::move(ev->packet()));

        if (channel_kind & YCM_UDP)
        { // udp & kcp, close after 500ms
          timer.expires_from_now(std::chrono::milliseconds(500));
          auto transport = ev->transport();
          timer.async_wait_once([&, transport]() { server.close(transport); });
        }
        break;
      case YEK_CONNECT_RESPONSE:
        printf("A client is income, status=%d, %lld, combine 2 packet and send to client!\n",
               ev->status(), ev->timestamp());
        break;
    }
  });
}

int main(int argc, char** argv)
{
  if (argc > 3)
    run_echo_server(argv[1], atoi(argv[2]), atoi(argv[3]));
  return 0;
}
