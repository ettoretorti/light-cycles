@0x8fe36605f60e7770;

struct Packet {
  magic @0 :UInt32;
  # Used to identify the sender
  seqNo @1 :UInt32;

  union {
    ping @2 :Ping;
    pong @3 :Pong;
    keepAlive @4 :Void;
  }
}

struct Ping {
  msg @0 :UInt16;
}

struct Pong {
  msg @0 :UInt16;
}
