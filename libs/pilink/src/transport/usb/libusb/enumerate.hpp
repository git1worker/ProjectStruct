#pragma once
#include <system_error>
#include <vector>
#include <string>

namespace pilink {
namespace transport {
namespace usb {
namespace libusb {

[[nodiscard]]
std::error_code enumerate_libusb(const char* format, std::vector<std::string> &v);

} // namespace libusb
} // namespace usb
} // namespace transport
} // namespace pilink
