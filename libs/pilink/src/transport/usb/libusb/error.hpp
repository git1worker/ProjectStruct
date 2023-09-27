#ifndef PILINK_TRANSPORT_USB_LIBUSB_ERROR_HPP
#define PILINK_TRANSPORT_USB_LIBUSB_ERROR_HPP

#include <system_error>
#include <libusb-1.0/libusb.h>

namespace pilink {
namespace transport {
namespace usb {
namespace libusb {

enum class error
{
  success       = LIBUSB_SUCCESS,
  io            = LIBUSB_ERROR_IO,
  invalid_param = LIBUSB_ERROR_INVALID_PARAM,
  access        = LIBUSB_ERROR_ACCESS,
  no_device     = LIBUSB_ERROR_NO_DEVICE,
  not_found     = LIBUSB_ERROR_NOT_FOUND,
  busy          = LIBUSB_ERROR_BUSY,
  timeout       = LIBUSB_ERROR_TIMEOUT,
  overflow      = LIBUSB_ERROR_OVERFLOW,
  pipe          = LIBUSB_ERROR_PIPE,
  interrupted   = LIBUSB_ERROR_INTERRUPTED,
  no_mem        = LIBUSB_ERROR_NO_MEM,
  not_supported = LIBUSB_ERROR_NOT_SUPPORTED,
  other         = LIBUSB_ERROR_OTHER
};

class error_category : public std::error_category
{
public:
  virtual const char* name() const noexcept override;
  virtual std::string message(int e) const override;
  virtual std::error_condition default_error_condition(int e) const noexcept override;
}; // class error_category

inline const error_category error_category_inst;

static inline
std::error_code make_libusb_error(int e) noexcept
{
  return std::error_code{ e, error_category_inst };
}

static inline
std::error_code make_error_code(error e) noexcept
{
  return make_libusb_error(static_cast<int>(e));
}

// LIBUSB_TRANSFER_ERROR

enum class transfer_error
{
  completed     = LIBUSB_TRANSFER_COMPLETED,
  error         = LIBUSB_TRANSFER_ERROR,
  timed_out     = LIBUSB_TRANSFER_TIMED_OUT,
  cancelled     = LIBUSB_TRANSFER_CANCELLED,
  stall         = LIBUSB_TRANSFER_STALL,
  no_device     = LIBUSB_TRANSFER_NO_DEVICE,
  overflow      = LIBUSB_TRANSFER_OVERFLOW
};


class transfer_error_category : public std::error_category
{
public:
  virtual const char* name() const noexcept override;
  virtual std::string message(int e) const override;
  virtual std::error_condition default_error_condition(int e) const noexcept override;
}; // class transfer_error_category

inline const transfer_error_category transfer_error_category_inst;

static inline
std::error_code make_libusb_transfer_error(int e) noexcept
{
  return std::error_code{ e, transfer_error_category_inst };
}

static inline
std::error_code make_error_code(transfer_error e) noexcept
{
  return make_libusb_transfer_error(static_cast<int>(e));
}

} // namespace libusb
} // namespace usb
} // namespace transport
} // namespace pilink

namespace std {

template<>
struct is_error_code_enum<pilink::transport::usb::libusb::error> : public true_type {};

template<>
struct is_error_code_enum<pilink::transport::usb::libusb::transfer_error> : public true_type {};

} // namespace std

#endif // #ifndef PILINK_TRANSPORT_USB_LIBUSB_ERROR_HPP
