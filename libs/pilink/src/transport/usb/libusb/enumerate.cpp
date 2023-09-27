#include "enumerate.hpp"
#include <iostream>
#include <libusb-1.0/libusb.h>
#include <boost/url.hpp>
#include "error.hpp"

namespace pilink {
namespace transport {
namespace usb {
namespace libusb {

[[nodiscard]]
std::error_code enumerate_libusb(const char *format, std::vector<std::string> &v)
{
    using std::cout;
    using std::endl;

    auto uri = boost::urls::parse_uri(format);
    auto uriView = uri.value();
    
    if (uriView.scheme() != "LIBUSB")
        return std::error_code(LIBUSB_ERROR_INVALID_PARAM, error_category_inst);
    
    //  LIBUSB
    if (int code = libusb_init(NULL); code < 0)
        return std::error_code(code, error_category_inst);
    
    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0){
        libusb_exit(NULL);
        return std::error_code(static_cast<int>(cnt), error_category_inst);
    }
    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device *device = list[i];
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(device, &desc) != 0) continue; 
        char vidStr [5];
        char pidStr [5];
        int bus = static_cast<int>(libusb_get_bus_number(device));
        int port = static_cast<int>(libusb_get_port_number(device));
        int addr = static_cast<int>(libusb_get_device_address(device));
        
        int nv = snprintf ( vidStr, 5, "%x", desc.idVendor );
        int np = snprintf ( pidStr, 5, "%x", desc.idProduct );
        // cout << vidStr << ' ' << pidStr << ' ' << bus << ' ' << port << ' ' << addr << ' ' << nv << ' ' << np << endl;
        if (nv <= 0 && np <= 0) {
            libusb_free_device_list(list, 1);
            libusb_exit(NULL);
            return std::error_code(LIBUSB_ERROR_INVALID_PARAM, error_category_inst);
        }
        std::string vidString(vidStr);
        std::string pidString(pidStr);

        bool isCorrect = true;
        for (const auto queryParam : uriView.params()){
            if (queryParam.key == "VID"){
                if (queryParam.value != vidString) 
                    isCorrect = false;
            } else if (queryParam.key == "PID"){
                if (queryParam.value != pidString) 
                    isCorrect = false;
            } else if (queryParam.key == "BUS"){
                if (atoi(queryParam.value.c_str()) != static_cast<int>(bus))
                    isCorrect = false;
            } else if (queryParam.key == "PORT"){
                if (atoi(queryParam.value.c_str()) != static_cast<int>(port))
                    isCorrect = false;
            } else if (queryParam.key == "ADDR"){
                if (atoi(queryParam.value.c_str()) != static_cast<int>(addr))
                    isCorrect = false;
            } else {
                libusb_free_device_list(list, 1);
                libusb_exit(NULL);
                return std::error_code(LIBUSB_ERROR_INVALID_PARAM, error_category_inst);
            }
        }
        if (isCorrect){
            std::string ss("LIBUSB://?");
            ss += "VID=" + vidString + "&PID=" + pidString + 
                "&BUS=" + std::to_string(bus) + "&PORT=" + 
                std::to_string(port) + "&ADDR=" + std::to_string(addr);
            v.push_back(std::move(ss));
        }

    }
    libusb_free_device_list(list, 1);
    libusb_exit(NULL);
    return {};
}


} // namespace libusb
} // namespace usb
} // namespace transport
} // namespace pilink
