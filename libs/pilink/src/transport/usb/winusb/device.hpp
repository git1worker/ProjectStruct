#ifndef TRANSPORT_USB_WINUSB_HPP
#define TRANSPORT_USB_WINUSB_HPP

#include <windows.h>
#include <winusb.h>
#include <usbspec.h>
#include <cassert>

#include "transport_usb_base.hpp"
#include "transport_usb_winusb_error.hpp"

namespace transport {
namespace usb {
namespace winusb {

using error_code_t = std::error_code;
using winusb_code_t = DWORD;

class device;

class transfer : public OVERLAPPED
{
private:
  device* owner_;
public:
  int pending_;

  unsigned char* buffer_;
  size_t size_;


  transfer(transfer&) = delete;
  transfer& operator=(const transfer&) = delete;
  transfer(transfer&&) = delete;
  transfer& operator=(transfer&&) = delete;

public:
  inline
  void completion_status_via_get_overlapped() noexcept;

  void complete(ULONG_PTR key) noexcept
  {
    if (key == 0) // NTSTATUS to OSError mapping
      completion_status_via_get_overlapped();

    pending_ = 0;
  }

  OVERLAPPED* to_overlapped() noexcept
  {
    return static_cast<OVERLAPPED*>(this);
  }

  static transfer* from_overlapped(OVERLAPPED* o) noexcept
  {
    return static_cast<transfer*>(o);
  }


  void prepare(device* owner) noexcept
  {
    assert(is_completed());

    Internal = 0;
    InternalHigh = 0;
    hEvent = 0;
    Offset = 0;
    OffsetHigh = 0;

    owner_ = owner;
    pending_ = 1;
  }

public:

  transfer() noexcept
  {
    // overlapped base
    Internal      = 0;
    InternalHigh  = 0;
    hEvent        = 0;
    Offset        = 0;
    OffsetHigh    = 0;

    owner_        = nullptr;
    pending_      = 0;

    buffer_       = nullptr;
    size_         = 0;
  }

  transfer(unsigned char *ptr, size_t size) noexcept
    : transfer()
  {
    buffer_ = ptr;
    size_ = size;
  }

  ~transfer()
  {
    assert(is_completed());
  }

  void buffer(unsigned char *ptr) noexcept {
    buffer_ = ptr;
  }

  unsigned char *buffer() const noexcept {
    return buffer_;
  }

  void size(size_t size) noexcept {
    size_ = size;
  }

  size_t size() const noexcept {
    return size_;
  }

  error_code_t status() const noexcept
  {
    if (pending_)
      return std::make_error_code(std::errc::operation_would_block);

    return make_winusb_error(static_cast<winusb_code_t>(Internal));
  }

  size_t transferred() const noexcept
  {
    assert(is_completed());
    return static_cast<size_t>(InternalHigh);
  }

  bool is_completed() const noexcept {
    return (!pending_);
  }

  inline
  error_code_t wait(unsigned int ms) noexcept;

  inline
  error_code_t wait_or_cancel(unsigned int ms) noexcept;

  inline
  error_code_t cancel() noexcept;
};

class device
{
private:
public:
  HANDLE                  h_;
  WINUSB_INTERFACE_HANDLE wih_;
  HANDLE                  iocp_;

  interface_info  ii_;

  winusb_code_t create_fds(const char* path) noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    do {
      h_ = ::CreateFileA(
        path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

      if (h_ == INVALID_HANDLE_VALUE) {
        winerr = ::GetLastError();
        break;
      }

      BOOL success = ::WinUsb_Initialize(h_, &wih_);
      if (!success) {
        winerr = ::GetLastError();

        ::CloseHandle(h_);
        h_ = INVALID_HANDLE_VALUE;

        break;
      }

      iocp_ = ::CreateIoCompletionPort(h_, NULL, 0, 0);
      if (iocp_ == NULL) {
        ::WinUsb_Free(wih_);
        wih_ = NULL;

        ::CloseHandle(h_);
        h_ = INVALID_HANDLE_VALUE;

        break;
      }

    } while (false);

    return winerr;
  }

  winusb_code_t set_device_autosuspend(unsigned int enable) noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    BOOL success;

    UCHAR value = static_cast<UCHAR>(enable);
    const ULONG value_length = sizeof(value);

    success = ::WinUsb_SetPowerPolicy(wih_, AUTO_SUSPEND, value_length, &value);
    if (!success)
      winerr = ::GetLastError();

    return winerr;
  }

  winusb_code_t set_pipe_timeout(unsigned char endpoint, unsigned long milliseconds) noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    ULONG value = static_cast<ULONG>(milliseconds);
    const ULONG value_length = sizeof(value);

    BOOL success;
    success = ::WinUsb_SetPipePolicy(wih_, endpoint, PIPE_TRANSFER_TIMEOUT, value_length, &value);
    if (!success)
      winerr = ::GetLastError();

    return winerr;
  }

  winusb_code_t set_pipe_rawio(unsigned char endpoint, bool enable) noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    UCHAR policy = enable ? 1 : 0;

    BOOL success;
    success = ::WinUsb_SetPipePolicy(wih_, endpoint, RAW_IO, sizeof(policy), &policy);
    if (!success)
      winerr = ::GetLastError();

    return winerr;
  }

  winusb_code_t get_pipe_maximum_transfer_size(unsigned char endpoint, unsigned long& maxtransfer) noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    ULONG value = 0;
    ULONG value_length = sizeof(value);

    BOOL success;
    success = ::WinUsb_GetPipePolicy(wih_, endpoint, MAXIMUM_TRANSFER_SIZE, &value_length, &value);
    if (!success)
      winerr = ::GetLastError();

    maxtransfer = static_cast<unsigned long>(value);

    return winerr;
  }

  winusb_code_t fill_interface_info() noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    USB_INTERFACE_DESCRIPTOR id;
    ULONG transferred = 0;
    BOOL success = ::WinUsb_QueryInterfaceSettings(wih_,
      0,
      &id
    );
    if (!success) {
      winerr = ::GetLastError();
      return winerr;
    }

    ii_.bInterfaceNumber    = id.bInterfaceNumber;
    ii_.bAlternateSetting   = id.bAlternateSetting;
    ii_.bNumEndpoints       = id.bNumEndpoints;
    ii_.bInterfaceClass     = id.bInterfaceClass;
    ii_.bInterfaceSubClass  = id.bInterfaceSubClass;
    ii_.bInterfaceProtocol  = id.bInterfaceProtocol;

    for (UCHAR i = 0; i < ii_.bNumEndpoints; ++i) {
      WINUSB_PIPE_INFORMATION pi;
      BOOL success = ::WinUsb_QueryPipe(wih_, 0, i, &pi);
      if (!success) {
        winerr = ::GetLastError();
        return winerr;
      }

      auto& e = ii_.endpoints[i];

      e.address = pi.PipeId;
      switch (pi.PipeType)
      {
      case UsbdPipeTypeControl:
        e.type = endpoint_type::control;
        break;

      case UsbdPipeTypeIsochronous:
        e.type = endpoint_type::isochronous;
        break;

      case UsbdPipeTypeBulk:
        e.type = endpoint_type::bulk;
        break;

      case UsbdPipeTypeInterrupt:
        e.type = endpoint_type::interrupt;
        break;

      default:
        assert(false);
        break;
      }

      e.maximum_packet_size = pi.MaximumPacketSize;

      unsigned long maximum_transfer_size = 0;

      if (e.type == endpoint_type::bulk || e.type == endpoint_type::interrupt) {
        if (e.address & 0x80) {
          winerr = set_pipe_rawio(e.address, true);
          if (winerr)
            return winerr;
        }

        winerr = get_pipe_maximum_transfer_size(e.address, maximum_transfer_size);
        if (winerr)
          return winerr;
      }

      e.maximum_transfer_size = maximum_transfer_size;
    }

    return winerr;
  }

  winusb_code_t configure() noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    do {

      // TODO:
      winerr = set_device_autosuspend(0);
      if (winerr != ERROR_SUCCESS)
        break;

      winerr = set_pipe_timeout(0x00, 1000);
      if (winerr != ERROR_SUCCESS)
        break;

      winerr = fill_interface_info();
      if (winerr != ERROR_SUCCESS)
        break;

    } while (false);

    return winerr;
  }

  winusb_code_t close_fds() noexcept
  {
    DWORD winerr = 0;
    if (iocp_ != NULL) {
      BOOL success = ::CloseHandle(iocp_);
      if (!success)
        winerr = ::GetLastError();
      iocp_ = NULL;
    }

    if (wih_ != NULL) {
      BOOL success = ::WinUsb_Free(wih_);
      if (!success)
        winerr = ::GetLastError();
      wih_ = NULL;
    }

    if (h_ != INVALID_HANDLE_VALUE) {
      BOOL success = ::CloseHandle(h_);
      if (!success)
        winerr = ::GetLastError();
      h_ = NULL;
    }

    return winerr;
  }

public:
  device() noexcept
    : h_ { INVALID_HANDLE_VALUE }
    , wih_ { NULL }
    , iocp_ { NULL }
    , ii_{}
  {
  }

  ~device() noexcept
  {
  }

  bool is_open() const noexcept
  {
    return (wih_ != NULL);
  }

  error_code_t open(const char* uri) noexcept
  {
    // TODO: debug
    if (uri == NULL) {
      //uri = "\\\\?\\usb#vid_152a&pid_82c0#lmuv0699sn0888#{4bbbccbe-bc04-4ee9-a8d8-77a0908bd102}";
      //uri = "\\\\?\\USB#VID_152A&PID_82C0#LMUV0606SN0900#{a5dcbf10-6530-11d2-901f-00c04fb951ed}";
      uri = "\\\\?\\USB#VID_152A&PID_82C0#LMUV0604SN0998#{a5dcbf10-6530-11d2-901f-00c04fb951ed}";
    }

    winusb_code_t winerr;

    winerr = create_fds(uri);
    if (winerr != ERROR_SUCCESS)
      return make_winusb_error(winerr);

    winerr = configure();
    if (winerr != ERROR_SUCCESS)
      close_fds();

    return make_winusb_error(winerr);
  }

  error_code_t close() noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    winerr = close_fds();
    return make_winusb_error(winerr);
  }

  error_code_t reset_pipe(unsigned char endpoint) noexcept
  {
    winusb_code_t winerr = 0;

    BOOL success;
    success = ::WinUsb_ResetPipe(wih_, endpoint);
    if (!success)
      winerr = ::GetLastError();

    return make_winusb_error(winerr);
  }

  winusb_code_t make_submit_result(winusb_code_t winerr, ULONG transferred, transfer& transfer) noexcept
  {
    if (winerr != ERROR_IO_PENDING) {
      transfer.to_overlapped()->Internal = winerr;
      transfer.to_overlapped()->InternalHigh = transferred;
      transfer.pending_ = 0;
      //context_->add_completed_transfer(transfer);
    }
    else {
      winerr = 0;
    }

    return winerr;
  }

  error_code_t submit_control(
      unsigned char bmRequestType, unsigned char bRequest, unsigned short wValue, unsigned short wIndex, unsigned short wLength,
      transfer& transfer) noexcept
  {
    WINUSB_SETUP_PACKET setup_packet;

    setup_packet.RequestType  = bmRequestType;
    setup_packet.Request      = bRequest;
    setup_packet.Value        = wValue;
    setup_packet.Index        = wIndex;
    setup_packet.Length       = wLength;

    winusb_code_t winerr = ERROR_SUCCESS;
    ULONG transferred = 0;
    transfer.prepare(this);
    BOOL success = ::WinUsb_ControlTransfer(
          wih_,
          setup_packet,
          static_cast<PUCHAR>(transfer.buffer()),
          static_cast<ULONG>(transfer.size()),
          &transferred,
          transfer.to_overlapped()
      );
    if (!success)
      winerr = ::GetLastError();

    winerr = make_submit_result(winerr, transferred, transfer);
    return make_winusb_error(winerr);
  }

  const interface_info* get_interface_info() const noexcept
  {
    assert(wih_ != NULL);
    return &ii_;
  }

  error_code_t control_transfer(
      unsigned char bmRequestType, unsigned char bRequest, unsigned short wValue, unsigned short wIndex, unsigned short wLength,
      unsigned char *data, size_t size, size_t& transferred, unsigned int timeout) noexcept
  {
    transfer setup_transfer(data, size);
    error_code_t ec;

    do {
      ec = submit_control(bmRequestType, bRequest, wValue, wIndex, wLength, setup_transfer);
      if (ec)
        break;

      ec = setup_transfer.wait(timeout);
      if (!setup_transfer.is_completed()) {
        error_code_t cancel_code;
        cancel_code = setup_transfer.cancel();
        (void)cancel_code; // warning if not success

        cancel_code = setup_transfer.wait(1000);
        (void)cancel_code; // warning if not success

        assert(setup_transfer.is_completed());
      }
    } while (false);

    transferred = setup_transfer.transferred();

    return ec;
  }

  error_code_t submit_bulk(unsigned char endpoint, transfer& transfer) noexcept
  {
    winusb_code_t winerr = ERROR_SUCCESS;
    ULONG transferred = 0;

    BOOL success;

    transfer.prepare(this);
    if (endpoint & 0x80) {
      success = ::WinUsb_ReadPipe(wih_, endpoint,
        reinterpret_cast<PUCHAR>(transfer.buffer_),
        static_cast<ULONG>(transfer.size_),
        &transferred, transfer.to_overlapped()
      );
    }
    else {
      success = ::WinUsb_WritePipe(wih_, endpoint,
        reinterpret_cast<PUCHAR>(transfer.buffer_),
        static_cast<ULONG>(transfer.size_),
        &transferred, transfer.to_overlapped()
      );
    }

    if (!success)
      winerr = ::GetLastError();

    winerr = make_submit_result(winerr, transferred, transfer);
    return make_winusb_error(winerr);
  }

  error_code_t bulk_transfer(unsigned char endpoint, unsigned char *data, size_t size, size_t& transferred, unsigned int timeout)
  {
    transfer bulk_transfer(data, size);
    error_code_t ec;

    do {
      ec = submit_bulk(endpoint, bulk_transfer);
      if (ec)
        break;

      ec = bulk_transfer.wait(timeout);
      if (!bulk_transfer.is_completed()) {
        error_code_t cancel_code;
        cancel_code = bulk_transfer.cancel();
        (void)cancel_code; // warning if not success

        cancel_code = bulk_transfer.wait(1000);
        (void)cancel_code; // warning if not success

        assert(bulk_transfer.is_completed());
      }
    } while (false);

    transferred = bulk_transfer.transferred();

    return ec;
  }


  winusb_code_t run_once(unsigned int ms) noexcept
  {
    winusb_code_t winerr = 0;

    constexpr ULONG oes_count = 32;
    OVERLAPPED_ENTRY oes[oes_count];
    ULONG oe_removed = 0;
    ULONG oe_to_remove = 32;
    BOOL success = ::GetQueuedCompletionStatusEx(
      iocp_, oes, oe_to_remove, &oe_removed, ms, FALSE
    );
    if (!success) {
      winerr = ::GetLastError();
      return winerr;
    }

    for (ULONG i = 0; i < oe_removed; ++i) {
      OVERLAPPED_ENTRY* oe = oes + i;
      if (oe->lpOverlapped) {
        transfer* transfer = transfer::from_overlapped(oe->lpOverlapped);
        transfer->complete(oe->lpCompletionKey);
      }
      else {
        // TODO:
      }
    }

    return winerr;
  }
};

inline
void transfer::completion_status_via_get_overlapped() noexcept
{
  DWORD transfer_status = ERROR_SUCCESS;

  // I hope GetOvelappedResult will not switch context when bWait is FALSE... Or will?
  DWORD transferred = 0;
  BOOL success = ::GetOverlappedResult(owner_->h_, to_overlapped(), &transferred, FALSE);
  if (!success)
    transfer_status = ::GetLastError();

  to_overlapped()->InternalHigh = transferred;
  to_overlapped()->Internal = transfer_status;
}

inline
error_code_t transfer::cancel() noexcept
{
  DWORD winerr = ERROR_SUCCESS;

  if (!is_completed()) {
    BOOL success = ::CancelIoEx(owner_->h_, to_overlapped());
    if (!success)
      winerr = ::GetLastError();
  }

  return make_winusb_error(winerr);
}

inline
error_code_t transfer::wait(unsigned int ms) noexcept
{
  DWORD now = ::GetTickCount();

  winusb_code_t winerr;

  for (;;) {
    DWORD elapsed = ::GetTickCount() - now;
    if (is_completed())
      return status();

    if (elapsed >= ms)
      break;

    winerr = owner_->run_once(ms - elapsed);
    if (winerr != ERROR_SUCCESS)
      return make_winusb_error(winerr);
  }

  return std::make_error_code(std::errc::timed_out);
}

inline
error_code_t transfer::wait_or_cancel(unsigned int ms) noexcept
{
  error_code_t ec = wait(ms);
  if (!is_completed()) {
    ec = cancel();
    ec = wait(1000);
  }

  return ec;
}


} // namespace winusb
} // namespace usb
} // namespace transport

#endif // TRANSPORT_USB_WINUSB_HPP
