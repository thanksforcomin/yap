#include "src/processing.hpp"
#include "src/utils.hpp"

#include "src/decoder.ipp"
#include "src/encoder.ipp"
#include "src/resampler.ipp"

#include <libavutil/samplefmt.h>

namespace audio {
  FrameWrapper::FrameWrapper() noexcept : frame(av_frame_alloc()) {
    if (!frame)
      utils::report_error("Something went wrong when creating a frame");
  }
  
  FrameWrapper::~FrameWrapper() {
    av_frame_free(&frame);
  }
  
  FrameWrapper::FrameWrapper(FrameWrapper &&other) noexcept
  : frame(other.frame) {
    other.frame = nullptr;
  }
  
  auto FrameWrapper::operator=(FrameWrapper &&other) noexcept -> FrameWrapper & {
    if (this != &other) {
      av_frame_free(&frame);
      frame = other.frame;
      other.frame = nullptr;
    }
    return *this;
  }
}
