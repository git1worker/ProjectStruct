#ifndef PILINK_TRANSPORT_USB_LIBUSB_HPP
#define PILINK_TRANSPORT_USB_LIBUSB_HPP

#include <libusb-1.0/libusb.h>
#include <memory>
#include <system_error>
#include <assert.h>

#include "transport/usb/usb_base.hpp"
#include "transport/usb/libusb/error.hpp"

namespace pilink {
namespace transport {
namespace usb {
namespace libusb {

using error_code_t = std::error_code;

class device;

class transfer
{
private:
public:
  libusb_transfer* ptransfer_;
  device* owner_;

public:
  unsigned char* buffer_;
  size_t size_;
  size_t transferred_;
  int status_;
  int completed_;

  void release() noexcept
  {
    if (ptransfer_ != nullptr) {
      assert(is_completed());
      libusb_free_transfer(ptransfer_);
      ptransfer_ = nullptr;
    }
  }

public:
  transfer() noexcept
    : ptransfer_ { nullptr }
    , owner_ { nullptr }
    , buffer_ { nullptr }
    , size_ { 0 }
    , transferred_{ 0 }
    , status_ { LIBUSB_TRANSFER_COMPLETED }
    , completed_ { 1 }
  {
  }

  ~transfer() noexcept
  {
    release();
  }

  bool is_completed() const noexcept
  {
    return (completed_ != 0);
  }

  static void transfer_callback_fn(struct libusb_transfer* t) noexcept
  {
    transfer* self = static_cast<transfer*>(t->user_data);
    self->transferred_ = static_cast<size_t>(t->actual_length);
    self->status_ = t->status;
    self->completed_ = 1;
    if (t->flags & LIBUSB_TRANSFER_FREE_TRANSFER)
      self->ptransfer_ = nullptr;
  }

  error_code_t status() const noexcept
  {
    if (!is_completed())
      return std::make_error_code(std::errc::operation_would_block);

    return make_libusb_transfer_error(status_);
  }

  size_t transferred() const noexcept
  {
    return transferred_;
  }

  error_code_t wait(unsigned int ms) noexcept;

  error_code_t prepare(device* owner) noexcept;

  error_code_t cancel() noexcept
  {
    if (is_completed())
      return {};

    assert(ptransfer_ != nullptr);
    assert(owner_ != nullptr);

    int status;
    status = libusb_cancel_transfer(ptransfer_);
    return make_libusb_error(status);
  }

};

class device
{
private:
public:
  libusb_context* context_;
  libusb_device* device_;
  libusb_device_handle* device_handle_;
  interface_info  ii_;

  int fill_interface_info() noexcept
  {
    int status;

    struct libusb_config_descriptor* cd = NULL;
    status = libusb_get_active_config_descriptor(device_, &cd);
    if (status != 0)
      return status;

    const auto& id = *cd->interface->altsetting;
    ii_.bInterfaceNumber    = id.bInterfaceNumber;
    ii_.bAlternateSetting   = id.bAlternateSetting;
    ii_.bNumEndpoints       = id.bNumEndpoints;
    ii_.bInterfaceClass     = id.bInterfaceClass;
    ii_.bInterfaceSubClass  = id.bInterfaceSubClass;
    ii_.bInterfaceProtocol  = id.bInterfaceProtocol;

    for (size_t i = 0; i < id.bNumEndpoints; ++ i) {
      auto& ed = ii_.endpoints[i];
      const auto& es = id.endpoint[i];

      ed.address = es.bEndpointAddress;

      switch(es.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
      {
      case LIBUSB_TRANSFER_TYPE_CONTROL:
        ed.type = endpoint_type::control;
        break;

      case LIBUSB_ENDPOINT_TRANSFER_TYPE_ISOCHRONOUS:
        ed.type = endpoint_type::isochronous;
        break;

      case LIBUSB_TRANSFER_TYPE_BULK:
        ed.type = endpoint_type::bulk;
        break;

      case LIBUSB_ENDPOINT_TRANSFER_TYPE_INTERRUPT:
        ed.type = endpoint_type::interrupt;
        break;

      default:
        assert(false);
        break;
      }

      ed.maximum_packet_size = es.wMaxPacketSize;
      ed.maximum_transfer_size = 2 * 1024 * 1024; // TODO: get maximum transfer size from underlaying system
    }

    libusb_free_config_descriptor(cd);

    return LIBUSB_SUCCESS;
  }


  error_code_t configure_device() noexcept
  {
    int status;

    status = libusb_set_configuration(device_handle_, 0);
    if (status != LIBUSB_SUCCESS && status != LIBUSB_ERROR_NOT_SUPPORTED)
      goto cleanup0;

    status = libusb_claim_interface(device_handle_, 0);
    if (status != 0)
      goto cleanup0;

    status = fill_interface_info();
    if (status != 0)
      goto cleanup0;

  cleanup0:
    return make_libusb_error(status);
  }

  error_code_t create_fds(const char * /* uri */) noexcept
  {
    assert(context_ == NULL);
    assert(device_ == NULL);
    assert(device_handle_ == NULL);

    error_code_t ec{};

    int status;

    constexpr unsigned int KSD_MPL1_VID = 0x152A;
    constexpr unsigned int KSD_MPL1_PID = 0x82C0;

    libusb_context* context = NULL;
    libusb_device** list = NULL;
    libusb_device_handle* device_handle = NULL;

    int index = 0;
    int matched_index = 0;
    ssize_t ndev;

    status = libusb_init(&context);
    if (status != 0)
      goto cleanup0;

    // TODO: allow to set log level from user code
    //status = libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    //if (status != LIBUSB_SUCCESS)
    //  goto cleanup1;

    ndev = libusb_get_device_list(context, &list);
    if (ndev < 0)
      goto cleanup1;

    for (ssize_t i = 0; i < ndev; ++i) {
      libusb_device* device = list[i];

      struct libusb_device_descriptor device_descriptor;
      status = libusb_get_device_descriptor(device, &device_descriptor);
      if (status != 0)
        goto cleanup2;

      if (device_descriptor.idVendor == KSD_MPL1_VID && device_descriptor.idProduct == KSD_MPL1_PID) {
        if (matched_index == index) {
          status = libusb_open(device, &device_handle);
          if (status != 0)
            goto cleanup2;

          context_ = context;
          device_ = device;
          device_handle_ = device_handle;

          libusb_free_device_list(list, 1);

          return {};
        }
        ++matched_index;
      }
    }

    status = LIBUSB_ERROR_NO_DEVICE;

  cleanup2:
    libusb_free_device_list(list, 1);

  cleanup1:
    libusb_exit(context);

  cleanup0:
    return make_libusb_error(status);
  }

public:
  device() noexcept
    : context_{nullptr}
    , device_{nullptr}
    , device_handle_{nullptr}
  {
  }

  ~device()
  {
    if (is_open())
      close();
  }

  bool is_open() const noexcept
  {
    return (device_handle_ != nullptr);
  }

  error_code_t close() noexcept
  {
    if (device_handle_ != nullptr)
    {
      assert(device_ != nullptr);
      assert(context_ != nullptr);

      libusb_release_interface(device_handle_, 0);

      libusb_close(device_handle_);
      device_handle_ = nullptr;

      libusb_exit(context_);
      context_ = nullptr;

      device_ = nullptr;
    }

    return {};
  }

  error_code_t open(const char* uri) noexcept
  {
    error_code_t ec;

    if (is_open())
      return std::make_error_code(std::errc::already_connected);

    ec = create_fds(uri);
    if (ec)
      return ec;

    ec = configure_device();
    if (ec) {
      close();
    }

    return ec;
  }

  const interface_info* get_interface_info() const noexcept
  {
    assert(is_open());
    return &ii_;
  }

  error_code_t reset_pipe(unsigned char endpoint) noexcept
  {
    int result;
    result = libusb_clear_halt(device_handle_, endpoint);
    return make_libusb_error(result);
  }

  error_code_t submit_bulk(unsigned char endpoint, transfer& transfer) noexcept
  {
    assert(is_open());

    error_code_t ec = transfer.prepare(this);
    if (ec)
      return ec;

    transfer.ptransfer_->endpoint = endpoint;
    transfer.ptransfer_->type = LIBUSB_TRANSFER_TYPE_BULK;

    int status;
    transfer.completed_ = 0;
    status = libusb_submit_transfer(transfer.ptransfer_);
    if (status != LIBUSB_SUCCESS) {
      transfer.completed_ = 1;
    }

    return make_libusb_error(status);
  }

  error_code_t control_transfer(
      unsigned char bmRequestType, unsigned char bRequest, unsigned short wValue, unsigned short wIndex, unsigned short wLength,
      unsigned char *data, size_t size, size_t& transferred, unsigned int timeout) noexcept
  {
    (void)size;
    int status_or_transferred;
    transferred = 0;
    status_or_transferred = libusb_control_transfer(device_handle_, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);
    if (status_or_transferred < 0)
      return make_libusb_error(status_or_transferred);

    transferred = static_cast<size_t>(status_or_transferred);
    return make_libusb_error(LIBUSB_SUCCESS);
  }

//  error_code_t control_transfer(
//    unsigned char bmRequestType, unsigned char bRequest, unsigned short wValue, unsigned short wIndex, unsigned short wLength,
//    unsigned char *data, unsigned int timeout) noexcept
//  {
//    int status;
//    status = libusb_control_transfer(device_handle_, bmRequestType, bRequest, wValue, wIndex, data, wLength, timeout);
//    return make_libusb_error(status);
//  }

  error_code_t bulk_transfer(unsigned char endpoint,
    unsigned char* data, size_t length, size_t& transferred, unsigned int timeout) noexcept
  {
    int status;
    int actual_length = 0;
    status = libusb_bulk_transfer(device_handle_, endpoint, data, static_cast<int>(length), &actual_length, timeout);
    transferred = static_cast<size_t>(actual_length);
    return make_libusb_error(status);
  }
};

inline
error_code_t transfer::prepare(device* owner) noexcept
{
  assert(is_completed());

  if (ptransfer_ == nullptr) {
    ptransfer_ = libusb_alloc_transfer(0);
    if (ptransfer_ == nullptr)
      return error::no_mem;

    ptransfer_->dev_handle = owner->device_handle_;
    ptransfer_->flags = LIBUSB_TRANSFER_FREE_TRANSFER;
    ptransfer_->timeout = 0;
    ptransfer_->callback = &transfer_callback_fn;
    ptransfer_->user_data = this;
  }

  owner_ = owner;
  transferred_ = 0;
  ptransfer_->actual_length = 0;
  ptransfer_->buffer = buffer_;
  ptransfer_->length = static_cast<int>(size_);

  return {};
}

inline
error_code_t transfer::wait(unsigned int ms) noexcept
{
  if (is_completed())
    return {};

  assert(ptransfer_ != nullptr);
  assert(owner_ != nullptr);

  timeval tv;
  tv.tv_sec   = static_cast<long int>(ms / 1000);
  tv.tv_usec  = static_cast<long int>((ms % 1000) * 1000);

  int result;
  result = libusb_handle_events_timeout_completed(owner_->context_, &tv, &completed_);

  if (result != LIBUSB_SUCCESS)
    return make_libusb_error(result);

  if (!completed_)
    return error::timeout;

  return status();
}


} // namespace libusb
} // namespace usb
} // namespace transport
} // namespace pilink

#endif // #ifndef PILINK_TRANSPORT_USB_LIBUSB_HPP
