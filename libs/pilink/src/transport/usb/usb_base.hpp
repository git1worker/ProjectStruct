#ifndef PILINK_TRANSPORT_USB_BASE_HPP
#define PILINK_TRANSPORT_USB_BASE_HPP

namespace pilink {
namespace transport {
namespace usb {

  enum class transfer_type : unsigned char
  {
    control,
    isochronous,
    bulk,
    interrupt
  };

  enum class endpoint_type : unsigned char
  {
    control,
    isochronous,
    bulk,
    interrupt
  };

  struct device_info
  {
    unsigned char  bDeviceClass;
    unsigned char  bDeviceSubClass;
    unsigned char  bDeviceProtocol;
    unsigned char  bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char  iManufacturer;
    unsigned char  iProduct;
    unsigned char  iSerialNumber;
    unsigned char  bNumConfigurations;
  };

  struct endpoint_info
  {
    unsigned char   address;
    endpoint_type   type;
    unsigned short  maximum_packet_size;
    unsigned int    maximum_transfer_size;
  };


  struct interface_info
  {
    unsigned char  bInterfaceNumber;
    unsigned char  bAlternateSetting;
    unsigned char  bNumEndpoints;
    unsigned char  bInterfaceClass;
    unsigned char  bInterfaceSubClass;
    unsigned char  bInterfaceProtocol;

    endpoint_info  endpoints[32];
  };

} // namespace usb
} // namespace transport
} // namespace pilink

#endif // PILINK_TRANSPORT_USB_BASE_HPP
