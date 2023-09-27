#ifndef PILINK_HPP
#define PILINK_HPP

#include <system_error>
#include <memory>
#include <vector>

namespace pilink {

/**
 * @brief The pilink class
 * Abstract class representing bidirectional link with in/out pipe. Have packetized transfers and
 * ordered delivery (but can be unreliable). Any transfer size less than packet_size will be
 * treated as SHORT TRANSFER with indication.
 */
class pilink
{
public:
  struct pipe_info_s {
    size_t packet_size;
    size_t baud_rate;
  };

  struct info_s {
    struct pipe_info_s  in;
    struct pipe_info_s  out;
  };

  virtual ~pilink() {}

  [[nodiscard]]
  virtual std::error_code connect(const char *uri) noexcept = 0;

  [[nodiscard]]
  virtual std::error_code disconnect() noexcept = 0;
  virtual bool is_connected() const noexcept = 0;

  [[nodiscard]]
  virtual std::error_code get_link_info(struct info_s& link_info) noexcept = 0;

  [[nodiscard]]
  virtual std::error_code reset() noexcept = 0;

  [[nodiscard]]
  virtual std::error_code write_some(const unsigned char *data, size_t size, size_t& transferred, unsigned int timeout) noexcept = 0;

  [[nodiscard]]
  virtual std::error_code read_some(unsigned char *data, size_t size, size_t& transferred, unsigned int timeout) noexcept = 0;

  //virtual std::error_code write(const unsigned char *data, size_t size) noexcept = 0;
  //virtual std::error_code read(unsigned char *data, size_t size) noexcept = 0;

  /*
  struct transfer_impl;
  using  transfer = std::unique_ptr<transfer>;
  virtual transfer async_write_some(const unsigned char *data, size_t size, std::error_code& ec) noexcept = 0;
  virtual transfer async_read_some(unsigned char *data, size_t size, std::error_code& ec) noexcept = 0;
  virtual bool async_is_completed(transfer& t) noexcept = 0;
  virtual void async_transfer_status(transfer& t) noexcept = 0;
  virtual void async_transfer_transferred(transfer& t) noexcept = 0;
  */
};

std::unique_ptr<pilink> make_pilink(const char *uri);

std::error_code enumerate(const char *filter, std::vector<std::string>& paths);



} // namespace pilink


#endif // PILINK_HPP
