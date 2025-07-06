#pragma once

#include "AudioTools/AudioCodecs/HeaderParserAAC.h"
#include "AudioTools/AudioCodecs/HeaderParserMP3.h"
#include "AudioTools/CoreAudio/AudioBasic/StrView.h"

namespace audio_tools {

/**
 * @brief  Logic to detemine the mime type from the content.
 * By default the following mime types are supported (audio/aac, audio/mpeg,
 * audio/vnd.wave, audio/ogg). You can register your own custom detection logic
 * to cover additional file types.
 *
 * Please not that the distinction between mp3 and aac is difficult and might
 * fail is some cases
 * @ingroup codecs
 * @ingroup decoder
 * @author Phil Schatzmann
 * @copyright GPLv3
 */

class MimeDetector {
 public:
  MimeDetector() {
    setCheck("audio/vnd.wave", checkWAV);
    setCheck("audio/ogg", checkOGG);
    setCheck("video/MP2T", checkMP2T);
    setCheck("audio/prs.sid", checkSID);
    setCheck("audio/m4a", checkM4A);
    setCheck("audio/mpeg", checkMP3Ext);
    setCheck("audio/aac", checkAACExt);
  }

  bool begin() {
    is_first = true;
    return true;
  }

  /// write the header to determine the mime
  size_t write(uint8_t* data, size_t len) {
    actual_mime = default_mime;
    determineMime(data, len);
    return len;
  }

  /// adds/updates the checking logic for the indicated mime
  void setCheck(const char* mime, bool (*check)(uint8_t* start, size_t len)) {
    StrView mime_str{mime};
    for (int j = 0; j < checks.size(); j++) {
      Check l_check = checks[j];
      if (mime_str.equals(l_check.mime)) {
        l_check.check = check;
        return;
      }
    }
    Check check_to_add{mime, check};
    checks.push_back(check_to_add);
  }

  // /// Define the callback that will notify about mime changes
  void setMimeCallback(void (*callback)(const char*)) {
    TRACED();
    this->notifyMimeCallback = callback;
  }

  /// Provides the actual mime type, that was determined from the first
  /// available data
  const char* mime() { return actual_mime; }

  static bool checkAAC(uint8_t* start, size_t len) {
    return start[0] == 0xFF &&
           (start[1] == 0xF0 || start[1] == 0xF1 || start[1] == 0xF9);
  }

  static bool checkAACExt(uint8_t* start, size_t len) {
    // checking logic for files
    if (memcmp(start + 4, "ftypM4A", 7) == 0) {
      return true;
    }
    // check for streaming
    HeaderParserAAC aac;
    // it should start with a synch word
    int pos = aac.findSyncWord((const uint8_t*)start, len);
    if (pos == -1) {
      return false;
    }
    // make sure that it is not an mp3
    if (aac.isValid(start + pos, len - pos)) {
      return false;
    }
    return true;
  }

  static bool checkMP3(uint8_t* start, size_t len) {
    return memcmp(start, "ID3", 3) == 0 ||
           (start[0] == 0xFF && ((start[1] & 0xE0) == 0xE0));
  }

  static bool checkMP3Ext(uint8_t* start, size_t len) {
    HeaderParserMP3 mp3;
    return mp3.isValid(start, len);
  }

  static bool checkWAV(uint8_t* start, size_t len) {
    return memcmp(start, "RIFF", 4) == 0;
  }

  static bool checkOGG(uint8_t* start, size_t len) {
    return memcmp(start, "OggS", 4) == 0;
  }

  /// MPEG-2 TS Byte Stream Format
  static bool checkMP2T(uint8_t* start, size_t len) {
    if (len < 189) return start[0] == 0x47;

    return start[0] == 0x47 && start[188] == 0x47;
  }

  /// Commodore 64 SID File
  static bool checkSID(uint8_t* start, size_t len) {
    return memcmp(start, "PSID", 4) == 0 || memcmp(start, "RSID", 4) == 0;
  }

  static bool checkM4A(uint8_t* header, size_t len) {
    if (len < 12) return false;

    // prevent false detecton by mp3 files
    if (memcmp(header, "ID3", 3) == 0) return false;
  
     // Special hack when we position to start of mdat box
    if (memcmp(header + 4, "mdat", 4) != 0) return true;
   
    // Check for "ftyp" at offset 4
    if (memcmp(header + 4, "ftyp", 4) != 0) return false;

    // Check for "M4A " or similar major brand
    if (memcmp(header + 8, "M4A ", 4) == 0 ||
        memcmp(header + 8, "mp42", 4) == 0 ||
        memcmp(header + 8, "isom", 4) == 0)
      return true;

    return false;
  }

  /// Provides the default mime type if no mime could be determined
  void setDefaultMime(const char* mime) { default_mime = mime; }

 protected:
  struct Check {
    const char* mime = nullptr;
    bool (*check)(uint8_t* data, size_t len) = nullptr;
    Check(const char* mime, bool (*check)(uint8_t* data, size_t len)) {
      this->mime = mime;
      this->check = check;
    }
    Check() = default;
  };
  Vector<Check> checks{0};
  bool is_first = false;
  const char* actual_mime = nullptr;
  const char* default_mime = nullptr;
  void (*notifyMimeCallback)(const char* mime) = nullptr;

  /// Update the mime type
  void determineMime(void* data, size_t len) {
    if (is_first) {
      actual_mime = lookupMime((uint8_t*)data, len);
      if (notifyMimeCallback != nullptr && actual_mime != nullptr) {
        notifyMimeCallback(actual_mime);
      }
      is_first = false;
    }
  }

  /// Default logic which supports aac, mp3, wav and ogg
  const char* lookupMime(uint8_t* data, size_t len) {
    for (int j = 0; j < checks.size(); j++) {
      Check l_check = checks[j];
      if (l_check.check(data, len)) {
        return l_check.mime;
      }
    }
    return default_mime;
  }
};

}  // namespace audio_tools
