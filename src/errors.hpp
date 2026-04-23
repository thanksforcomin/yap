#pragma once

#include <cstdint>
#include <expected>
#include <type_traits>
#include <utility>

namespace errors {

  enum Error {
    ResamplingError,
    EncoderNotFound,
    EncoderNotAllocated,
    EncoderNotOpen,
    DecoderNotAllocated,
    DecoderNotOpen,
    ResamplerNotAllocated,
    FrameAllocationFault,
    AudioArrayError,
    UnknownInputFormat,
    FailedToOpenDevice,
    CannotFindStreamInfo,
    NoAudioStreamFound
  };

  template <typename T>
  using Result = std::expected<T, Error>;
  
}
