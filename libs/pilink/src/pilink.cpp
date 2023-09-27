#include <pilink/pilink.hpp>
#include "transport/usb/usb_impl.hpp"
#include "pilink.hpp"
#include <boost/url.hpp>
#include <system_error>
#include "transport/usb/libusb/enumerate.hpp"

namespace pilink {

std::unique_ptr<pilink> make_pilink(const char * /* uri */)
{
  return std::unique_ptr<pilink>(transport::usb::make_pilink_usb_libusb());
}

std::error_code enumerate(const char *filter, std::vector<std::string> &paths)
{
  auto uri = boost::urls::parse_uri(filter);
  auto uriView = uri.value();
  
  if (uriView.scheme() != "LIBUSB")
    return transport::usb::libusb::enumerate_libusb(filter, paths);
  // if (backe)
  //   return std::error_code();

  // TODO: return some error code
  return std::make_error_code(std::errc::invalid_argument);
}

} // namespace pilink
