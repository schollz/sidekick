#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;
#define MIDI_RT_CLOCK 0xF8
#define MIDI_RT_START 0xFA
#define MIDI_RT_CONTINUE 0xFB
#define MIDI_RT_STOP 0xFC
#define MIDI_RT_ACTIVESENSE 0xFE
#define MIDI_RT_RESET 0xFF
#define MIDI_NOTE_OFF 0x80
#define MIDI_NOTE_ON 0x90
#define MIDI_AFTERTOUCH 0xA0
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM_CHANGE 0xC0
#define MIDI_CHANNEL_PRESSURE 0xD0
#define MIDI_PITCH_BEND 0xE0
#define MIDI_SYSEX_START 0xF0
#define MIDI_SYSEX_END 0xF7
#define MIDI_TIMING 0xF8

class DaisyMidi {
 public:
  DaisyMidi()
      : note_on_callback_(nullptr),
        note_off_callback_(nullptr),
        midi_timing_callback_(nullptr),
        sysex_callback_(nullptr) {}

  void Init() {
    // Initialize MIDI
    MidiUsbTransport::Config midiusb_out_config;
    midiusb_out.Init(midiusb_out_config);
    midiusb_out.StartRx(
        [](uint8_t* data, size_t size, void* context) {
          static_cast<DaisyMidi*>(context)->handlerUSBMidiEvent(data, size);
        },
        this);
  }

  void sysex_printf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    sysex_send(buffer);
  }

  void sysex_printf_buffer(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    // add buffer to sysex_buffer
    for (size_t i = 0; i < strlen(buffer); i++) {
      if (sysex_buffer_i >= 126) {
        return;
      }
      sysex_buffer[sysex_buffer_i] = buffer[i];
      sysex_buffer_i++;
    }
  }

  void sysex_send_buffer() {
    if (sysex_buffer_i == 0) {
      return;
    }
    // send sysex_buffer
    sysex_send(sysex_buffer);
    // reset sysex_buffer
    sysex_buffer_i = 0;
    for (size_t i = 0; i < 126; i++) {
      sysex_buffer[i] = 0;
    }
  }

  void handlerHWMidiEvent(MidiEvent ev) {
    if (in_sysex_message) {
      // add bytes to sysex buffer
      if (ev.type == SystemCommon && ev.sc_type == SystemExclusive) {
        for (size_t i = 0; i < ev.sysex_message_len; i++) {
          midi_buffer[midi_buffer_index++] = ev.sysex_data[i];
        }
      }
    }
    switch (ev.type) {
      case SystemRealTime: {
        switch (ev.srt_type) {
          case TimingClock:
            if (midi_timing_callback_) {
              midi_timing_callback_();
            }
            break;
          default:
            break;
        }
      }
      case SystemCommon: {
        switch (ev.sc_type) {
          case SystemExclusive:
            in_sysex_message = true;
            break;
          case SysExEnd:
            in_sysex_message = false;
            if (sysex_callback_ && midi_buffer_index > 0) {
              sysex_callback_(reinterpret_cast<const uint8_t*>(midi_buffer),
                              midi_buffer_index);
            }
            midi_buffer_index = 0;
            break;
          default:
            break;
        }
      }
      case NoteOff:
        if (note_off_callback_) {
          note_off_callback_(ev.channel, ev.data[0], ev.data[1]);
        }
        break;
      case NoteOn:
        if (note_on_callback_) {
          note_on_callback_(ev.channel, ev.data[0], ev.data[1]);
        }
      default:
        break;
    }
  }

  // Setters for callbacks
  void SetNoteOnCallback(void (*callback)(uint8_t channel, uint8_t note,
                                          uint8_t velocity)) {
    note_on_callback_ = callback;
  }

  void SetNoteOffCallback(void (*callback)(uint8_t channel, uint8_t note,
                                           uint8_t velocity)) {
    note_off_callback_ = callback;
  }

  void SetMidiTimingCallback(void (*callback)()) {
    midi_timing_callback_ = callback;
  }

  void SetSysExCallback(void (*callback)(const uint8_t* data, size_t size)) {
    sysex_callback_ = callback;
  }

 private:
  MidiUsbTransport midiusb_out;
  char midi_buffer[128];
  char sysex_buffer[126];
  size_t sysex_buffer_i = 0;
  size_t midi_buffer_index = 0;
  bool in_sysex_message = false;

  // Function pointers for callbacks
  void (*note_on_callback_)(uint8_t channel, uint8_t note, uint8_t velocity);
  void (*note_off_callback_)(uint8_t channel, uint8_t note, uint8_t velocity);
  void (*midi_timing_callback_)();
  void (*sysex_callback_)(const uint8_t* data, size_t size);

  void sysex_send(char* str) {
    // Build sysex message from str
    uint8_t sysex_message[128];
    sysex_message[0] = 0xF0;  // sysex start
    for (size_t i = 0; i < strlen(str); i++) {
      sysex_message[i + 1] = str[i];
    }
    sysex_message[strlen(str) + 1] = 0xF7;  // sysex end

    midiusb_out.Tx(sysex_message, strlen(str) + 2);
  }

  void handlerUSBMidiEvent(uint8_t* data, size_t size) {
    // Iterate through the incoming MIDI data
    if (!in_sysex_message) {
      switch (data[0]) {
        case MIDI_NOTE_ON:
          if (note_on_callback_) {
            uint8_t channel = data[0] & 0x0F;
            uint8_t note = data[1];
            uint8_t velocity = data[2];
            note_on_callback_(channel, note, velocity);
          }
          break;

        case MIDI_NOTE_OFF:
          if (note_off_callback_) {
            uint8_t channel = data[0] & 0x0F;
            uint8_t note = data[1];
            uint8_t velocity = data[2];
            note_off_callback_(channel, note, velocity);
          }
          break;

        case MIDI_TIMING:
          if (midi_timing_callback_) {
            midi_timing_callback_();
          }
          break;

        default:
          break;
      }
    }

    // SYSEX
    for (size_t i = 0; i < size; ++i) {
      uint8_t byte = data[i];

      if (in_sysex_message) {
        if (byte == MIDI_SYSEX_END ||
            midi_buffer_index >= sizeof(midi_buffer)) {
          in_sysex_message = false;
          if (sysex_callback_ && midi_buffer_index > 0) {
            sysex_callback_(reinterpret_cast<const uint8_t*>(midi_buffer),
                            midi_buffer_index);
          }
          midi_buffer_index = 0;
        } else {
          midi_buffer[midi_buffer_index++] = byte;
        }
      } else if (byte == MIDI_SYSEX_START) {
        in_sysex_message = true;
        midi_buffer_index = 0;
      }
    }
  }
};
