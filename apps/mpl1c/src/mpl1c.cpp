#include <pilink/pilink.hpp>

int main(int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  std::error_code ec;

  auto link = pilink::make_pilink("LIBUSB");
  ec = link->connect("test");


  ec = link->disconnect();

  return 0;
}
