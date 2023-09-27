#ifndef PILINK_TRANSPORT_USB_WINUSB_ERROR_HPP
#define PILINK_TRANSPORT_USB_WINUSB_ERROR_HPP

#include <system_error>

namespace pilink {
namespace transport {
namespace usb {
namespace winusb {

static inline
std::error_code make_winusb_error(int e) noexcept
{
  return std::error_code{ e, std::system_category() };
}

} // namespace winusb
} // namespace usb
} // namespace transport
} // namespace pilink

#endif // PILINK_TRANSPORT_USB_WINUSB_ERROR_HPP
