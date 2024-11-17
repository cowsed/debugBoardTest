#include "vdb/protocol.h"

namespace VDP {
constexpr ChannelID control_channel_id = 0;
static const Channel ControlChannel{control_channel_id, nullptr};

class Registry {
public:
  int num_bad = 0;
  int num_small = 0;
  enum Side {
    Controller,
    Listener,
  };
  using CallbackFn = std::function<void(const VDP::Channel &)>;
  Registry(AbstractDevice *device, Side reg_type);
  /// @brief Call this if you are a device who has a packet for the protocol to
  /// decode
  /// @param pac the packet to take.
  void take_packet(const Packet &pac);

  PartPtr get_remote_schema(ChannelID id);

  void install_broadcast_callback(CallbackFn on_broadcast);
  void install_data_callback(CallbackFn on_data);

  // creates a new channel for the given VDP object, broadcasts it to the device
  // on the other end of the line this channel is open to be written too
  // immediately. however, it is not garaunteed to be sent to the other side
  // until broadcasting has been completed
  Channel open_channel(PartPtr for_data);

  bool send_data(const Channel &data);

private:
  ChannelID new_channel_id() {
    next_channel_id++;
    return next_channel_id;
  }

  Side reg_type;
  vex::timer waiting_on_ack_timer;
  bool needs_ack = false;
  static constexpr size_t ack_ms = 100;

  AbstractDevice *device;
  // Our channels (us -> them)
  std::vector<Channel> my_channels = {ControlChannel};
  ChannelID next_channel_id = 0;
  ChannelID highest_acked_channel = 0;

  // The channels we know about from the other side
  // (them -> us)
  std::vector<Channel> remote_channels = {ControlChannel};

  CallbackFn on_broadcast = [&](VDP::Channel chan) {
    std::string schema_str = chan.data->pretty_print();
    printf("VDB-%s: No Broadcast Callback installed: Received broadcast "
           "for channel id "
           "%d:\n%s\n",
           (reg_type == Side::Controller ? "Controller" : "Listener"),
           int(chan.id), schema_str.c_str());
  };
  CallbackFn on_data = [](VDP::Channel chan) {
    printf("VDB-%s: No Data Callback installed: Received data for channel "
           "%d:\n%s\n",
           int(chan.id), chan.data->pretty_print_data().c_str());
  };
};
} // namespace VDP