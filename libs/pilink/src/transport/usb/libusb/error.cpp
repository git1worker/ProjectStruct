#include "transport/usb/libusb/error.hpp"

namespace pilink {
namespace transport {
namespace usb {
namespace libusb {

const char *error_category::name() const noexcept
{
  return "libusb";
}

std::string error_category::message(int e) const
{
  switch (static_cast<error>(e))
  {
  case error::success:
    return "Success (no error)";
  case error::io:
    return "Input/output error";
  case error::invalid_param:
    return "Invalid parameter";
  case error::access:
    return "Access denied (insufficient permissions)";
  case error::no_device:
    return "No such device (it may have been disconnected)";
  case error::not_found:
    return "Entity not found";
  case error::busy:
    return "Resource busy";
  case error::timeout:
    return "Operation timed out";
  case error::overflow:
    return "Overflow";
  case error::pipe:
    return "Pipe error";
  case error::interrupted:
    return "System call interrupted (perhaps due to signal)";
  case error::no_mem:
    return "Insufficient memory";
  case error::not_supported:
    return "Operation not supported or unimplemented on this platform";
  case error::other:
    return "Other error";

  default:
    break;
  }

  return "unknown error";
}

std::error_condition error_category::default_error_condition(int e) const noexcept
{
  switch (static_cast<error>(e))
  {
  case error::io:
    return std::errc::io_error;

  case error::invalid_param:
    return std::errc::invalid_argument;

  case error::access:
    return std::errc::permission_denied;

  case error::no_device:
    return std::errc::no_such_device;

  case error::not_found:
    return std::errc::no_such_file_or_directory;

  case error::busy:
    return std::errc::device_or_resource_busy;

  case error::timeout:
    return std::errc::timed_out;

    //case error::overflow:
    //  return *no_equivalence*;

  case error::pipe:
    return std::errc::broken_pipe;

  case error::interrupted:
    return std::errc::interrupted;

  case error::no_mem:
    return std::errc::not_enough_memory;

  case error::not_supported:
    return std::errc::not_supported;

    //case error::other:
    //  return *no_equivalence*;
  default:
    break;
  }
  return std::error_condition(e, *this);
}

const char *transfer_error_category::name() const noexcept
{
  return "libusb::transfer";
}

std::string transfer_error_category::message(int e) const
{
  switch (static_cast<transfer_error>(e))
  {
  case transfer_error::completed:
    return "Transfer completed without error";
  case transfer_error::error:
    return "Transfer failed";
  case transfer_error::timed_out:
    return "Transfer timed out";
  case transfer_error::cancelled:
    return "Transfer was cancelled";
  case transfer_error::stall:
    return "Halt condition detected (endpoint stalled)/Control request not supported";
  case transfer_error::no_device:
    return "Device was disconnected";
  case transfer_error::overflow:
    return "Device sent more data than requested";

  default:
    break;
  }

  return "unknown error";
}

std::error_condition transfer_error_category::default_error_condition(int e) const noexcept
{
  switch (static_cast<transfer_error>(e))
  {
  case transfer_error::error:
    return std::errc::io_error;

  case transfer_error::timed_out:
    return std::errc::timed_out;

  case transfer_error::cancelled:
    return std::errc::operation_canceled;

  case transfer_error::stall:
    return std::errc::broken_pipe;

  case transfer_error::no_device:
    return std::errc::no_such_device;

    //case transfer_error::overflow:
    //  return *no_equivalence*;

  default:
    break;
  }

  return std::error_condition(e, *this);
}



} // namespace libusb
} // namespace usb
} // namespace transport
} // namespace pilink
