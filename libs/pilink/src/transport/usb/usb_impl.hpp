#ifndef PILINK_TRANSPORT_USB_USB_IMPL_HPP
#define PILINK_TRANSPORT_USB_USB_IMPL_HPP

#include <cstring>
#include <pilink/pilink.hpp>
#include "transport/usb/usb_base.hpp"
#include "transport/usb/libusb/device.hpp"

namespace pilink {
namespace transport {
namespace usb {


template<typename device>
class pilink_usb : public pilink
{
private:

  device device_;
  unsigned int timeout_;

  transport::usb::endpoint_info in_;
  transport::usb::endpoint_info out_;

public:
  pilink_usb() noexcept;
  ~pilink_usb();

  virtual std::error_code connect(const char *uri) noexcept override;
  virtual std::error_code disconnect() noexcept override;
  virtual bool is_connected() const noexcept override;

  virtual std::error_code get_link_info(struct info_s& link_info) noexcept override;

  virtual std::error_code reset() noexcept override;
  virtual std::error_code write_some(const unsigned char *data, size_t size, size_t& transferred, unsigned int timeout) noexcept override;
  virtual std::error_code read_some(unsigned char *data, size_t size, size_t& transferred, unsigned int timeout) noexcept override;
};

template<typename device>
pilink_usb<device>::pilink_usb() noexcept
  : device_{}
  , timeout_{1000}
  , in_{}
  , out_{}
{
}

template<typename device>
pilink_usb<device>::~pilink_usb()
{
  if (device_.is_open())
    device_.close();
}

static inline
bool is_in_endpoint(unsigned char address) noexcept
{
  return ((address & 0x80) != 0x00);
}

static inline
bool is_out_endpoint(unsigned char address) noexcept
{
  return !is_in_endpoint(address);
}

template<typename device>
std::error_code  pilink_usb<device>::connect(const char *uri) noexcept
{
  std::error_code ec;

  if (device_.is_open()) {
    device_.close();
  }

  ec = device_.open(uri);
  if (ec)
    return ec;

  auto ii = device_.get_interface_info();
  bool in_pipe_found = false;
  bool out_pipe_found = false;

  for (size_t i = 0; i < ii->bNumEndpoints; ++ i) {
    auto& e = ii->endpoints[i];
    if (e.type == transport::usb::endpoint_type::bulk) {

      if (!in_pipe_found && is_in_endpoint(e.address)) {
        in_ = e;
        in_pipe_found = true;
      }

      if (!out_pipe_found && is_out_endpoint(e.address)) {
        out_ = e;
        out_pipe_found = true;
      }

      if (in_pipe_found && out_pipe_found)
        break;
    }
  }

  if (!in_pipe_found || !out_pipe_found) {
    device_.close();
    ec = std::make_error_code(std::errc::protocol_not_supported);
  }

  ec = reset();
  if (ec) {
    device_.close();
  }

  return ec;
}

template<typename device>
std::error_code  pilink_usb<device>::disconnect() noexcept
{
  return device_.close();
}

template<typename device>
bool pilink_usb<device>::is_connected() const noexcept
{
  return device_.is_open();
}

template<typename device>
std::error_code  pilink_usb<device>::get_link_info(info_s &link_info) noexcept
{
  // TODO:
  (void)link_info;
  return std::make_error_code(std::errc::operation_not_supported);
}

template<typename device>
std::error_code  pilink_usb<device>::reset() noexcept
{
  if (!is_connected())
    return std::make_error_code(std::errc::not_connected);

  std::error_code ec{};

  constexpr unsigned char host_to_device   = 0x00;
  constexpr unsigned char type_vendor      = 0x40;
  constexpr unsigned char recipient_device = 0x00;

  do {
    size_t transferred = 0;
    ec = device_.control_transfer(
      host_to_device | type_vendor | recipient_device,
      0x00,
      0x0000,
      0x0000,
      0x0000,
      nullptr, 0, transferred,
      timeout_
    );
    if (ec)
      break;

    ec = device_.reset_pipe(in_.address);
    if (ec)
      break;

    ec = device_.reset_pipe(out_.address);
    if (ec)
      break;

  } while (false);

  return ec;
}

template<typename device>
std::error_code  pilink_usb<device>::write_some(const unsigned char *data, size_t size, size_t &transferred, unsigned int timeout) noexcept
{
  if (!is_connected())
    return std::make_error_code(std::errc::not_connected);

  std::error_code ec{};
  unsigned char endpoint = out_.address;
  size_t max_transfer_size = out_.maximum_transfer_size;

  // calculate chunk size from link output baudrate
  size_t really_transferred = 0;
  unsigned char *buffer = const_cast<unsigned char *>(data);
  while (size != 0) {
    size_t current_transfer_size = std::min(size, max_transfer_size);
    size_t current_transferred = 0;
    ec = device_.bulk_transfer(endpoint, buffer, current_transfer_size, current_transferred, timeout);
    if (ec)
      break;

    if (current_transferred != current_transfer_size) {
      ec = std::make_error_code(std::errc::argument_out_of_domain);
      break;
    }

    really_transferred += current_transferred;
    buffer += current_transferred;
    size -= current_transferred;
  }

  transferred = really_transferred;
  return ec;
}

template<typename device>
std::error_code  pilink_usb<device>::read_some(unsigned char *data, size_t size, size_t &transferred, unsigned int timeout) noexcept
{
  if (!is_connected())
    return std::make_error_code(std::errc::not_connected);

  std::error_code ec{};
  unsigned char endpoint = in_.address;
  size_t max_transfer_size = in_.maximum_transfer_size;
  size_t packet_size = in_.maximum_packet_size;


  // calculate chunk size from link output baudrate
  size_t really_transferred = 0;
  unsigned char *buffer = const_cast<unsigned char *>(data);
  while (size >= packet_size) {
    size_t current_transfer_size = std::min(size, max_transfer_size);
    size_t current_transferred = 0;

    ec = device_.bulk_transfer(endpoint, buffer, current_transfer_size, current_transferred, timeout);
    if (ec)
      break;

    if (current_transferred != current_transfer_size) {
      ec = std::make_error_code(std::errc::argument_out_of_domain);
      break;
    }

    really_transferred += current_transferred;
    buffer += current_transferred;
    size -= current_transferred;
  }

  do {
    if (ec || size == 0)
      break;

    constexpr size_t stack_align_buffer_size = 2048;
    unsigned char stack_align_buffer[stack_align_buffer_size];
    unsigned char *align_buffer = stack_align_buffer;

    std::unique_ptr<unsigned char[]> heap_align_buffer;
    if (packet_size > stack_align_buffer_size) {
      align_buffer = ::new (std::nothrow) unsigned char[packet_size];

      if (align_buffer == nullptr)
        return std::make_error_code(std::errc::not_enough_memory);

      heap_align_buffer.reset(align_buffer);
    }

    size_t current_transferred = 0;
    ec = device_.bulk_transfer(endpoint, align_buffer, packet_size, current_transferred, timeout);
    if (ec)
      break;

    if (current_transferred > size) {
      ec = std::make_error_code(std::errc::argument_list_too_long); // TODO:
      current_transferred = size;
    }

    ::memcpy(buffer, align_buffer, current_transferred);
    really_transferred += current_transferred;

  } while (false);


  transferred = really_transferred;
  return ec;
}

pilink *make_pilink_usb_libusb() noexcept
{
  return ::new(std::nothrow) pilink_usb<libusb::device>;
}

} // namespace usb
} // namespace transport
} // namespace pilink

#endif // #ifndef PILINK_TRANSPORT_USB_USB_IMPL_HPP
