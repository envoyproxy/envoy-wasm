#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "envoy/buffer/buffer.h"

namespace Envoy {
namespace Grpc {
// Last bit for an expanded message without compression.
const uint8_t GRPC_FH_DEFAULT = 0b0u;
// Last bit for a compressed message.
const uint8_t GRPC_FH_COMPRESSED = 0b1u;

constexpr uint64_t GRPC_FRAME_HEADER_SIZE = sizeof(uint8_t) + sizeof(uint32_t);

enum class CompressionAlgorithm { None, Gzip };

struct Frame {
  uint8_t flags_;
  uint32_t length_;
  Buffer::InstancePtr data_;
};

class Encoder {
public:
  Encoder();

  // Creates a new GRPC data frame with the given flags and length.
  // @param flags supplies the GRPC data frame flags.
  // @param length supplies the GRPC data frame length.
  // @param output the buffer to store the encoded data. Its size must be 5.
  void newFrame(uint8_t flags, uint64_t length, std::array<uint8_t, 5>& output);
};

// Wire format (http://www.grpc.io/docs/guides/wire.html) of GRPC data frame
// header:
//
// -----------------------------------------------------------------------
// |R|R|R|R|R|R|R|R|C|      L     |      L     |      L     |      L     |
// -----------------------------------------------------------------------
//    Flag (1 byte)                Message Length (4 bytes)
//
// A fixed header consists of five bytes.
// The first byte is the Flag. The last one "C" bit indicates if the message
// is compressed or not (0 is uncompressed, 1 is compressed). The other seven
// "R" bits are reserved for future use.
// The next four "L" bytes represent the message length in BigEndian format.
enum class State {
  // Waiting for decoding the flags (1 byte) of the GRPC data frame.
  FH_FLAG,
  // Waiting for decoding the 1st byte of the length (4 bytes in total) of the
  // GRPC data frame.
  FH_LEN_0,
  // Waiting for decoding the 2nd byte of the length (4 bytes in total) of the
  // GRPC data frame.
  FH_LEN_1,
  // Waiting for decoding the 3rd byte of the length (4 bytes in total) of the
  // GRPC data frame.
  FH_LEN_2,
  // Waiting for decoding the 4th byte of the length (4 bytes in total) of the
  // GRPC data frame.
  FH_LEN_3,
  // Waiting for decoding the data.
  DATA,
};

class FrameInspector {
public:
  // Inspects the given buffer with GRPC data frame and updates the frame count.
  // Invokes visitor callbacks for each frame in the following sequence:
  //   "frameStart frameDataStart frameData* frameDataEnd"
  // If frameStart returns false, then the inspector aborts.
  // Returns the increase in the frame count.
  uint64_t inspect(const Buffer::Instance& input);

  // Returns the current frame count, corresponding to the request/response
  // message count. Counter is incremented on a frame start.
  uint64_t frameCount() const { return count_; }

  // Returns the current state in the frame parsing.
  State state() const { return state_; }

  virtual ~FrameInspector() = default;

protected:
  virtual bool frameStart(uint8_t) { return true; }
  virtual void frameDataStart() {}
  virtual void frameData(uint8_t*, uint64_t) {}
  virtual void frameDataEnd() {}

  State state_{State::FH_FLAG};
  uint32_t length_{0};
  uint64_t count_{0};
};

class Decoder : public FrameInspector {
public:
  // Decodes the given buffer with GRPC data frame. Drains the input buffer when
  // decoding succeeded (returns true). If the input is not sufficient to make a
  // complete GRPC data frame, it will be buffered in the decoder. If a decoding
  // error happened, the input buffer remains unchanged.
  // @param input supplies the binary octets wrapped in a GRPC data frame.
  // @param output supplies the buffer to store the decoded data.
  // @return bool whether the decoding succeeded or not.
  bool decode(Buffer::Instance& input, std::vector<Frame>& output);

  // Determine the length of the current frame being decoded. This is useful when supplying a
  // partial frame to decode() and wanting to know how many more bytes need to be read to complete
  // the frame.
  uint32_t length() const { return frame_.length_; }

  // Indicates whether it has buffered any partial data.
  bool hasBufferedData() const { return state_ != State::FH_FLAG; }

protected:
  bool frameStart(uint8_t) override;
  void frameDataStart() override;
  void frameData(uint8_t*, uint64_t) override;
  void frameDataEnd() override;

private:
  Frame frame_;
  std::vector<Frame>* output_{nullptr};
  bool decoding_error_{false};
};

} // namespace Grpc
} // namespace Envoy
