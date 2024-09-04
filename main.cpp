#include "core_cm7.h"
#include "daisy_patch_sm.h"
#include "daisysp.h"
//
#include "lib/daisy_midi.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

#define AUDIO_BLOCK_SIZE 128
#define AUDIO_SAMPLE_RATE 48000

DaisyPatchSM hw;
DaisyMidi midi;
Switch button;
Switch toggle;
Compressor comp;
AnalogBassDrum kick;
float kick_volume = 4.0f;

bool startup = true;
bool button_pressed = false;
bool gate_in = false;
float cv_knobs[4];
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  button.Debounce();
  toggle.Debounce();
  hw.ProcessAllControls();

  bool toggle_state = toggle.Pressed();

  float cv_values[4] = {hw.GetAdcValue(CV_1), hw.GetAdcValue(CV_2),
                        hw.GetAdcValue(CV_3), hw.GetAdcValue(CV_4)};
  for (size_t i = 0; i < 4; i++) {
    cv_values[i] = roundf(cv_values[i] * 100) / 100.0f;
    if (cv_values[i] != cv_knobs[i] || startup) {
      midi.sysex_printf_buffer("CV_%d: %f\n", i + 1, cv_values[i]);
      cv_knobs[i] = cv_values[i];
      if (!toggle_state) {
        // alter kick drum properties when togle is down
        switch (i) {
          case 0:
            kick_volume = cv_values[i] * 8;
            break;
          case 1:
            kick.SetDecay(cv_values[i]);
            kick.SetSelfFmAmount(cv_values[i] / 2.0f);
            break;
          case 2:
            kick.SetAccent(cv_values[i]);
            kick.SetTone(cv_values[i] / 4.0f);
            break;
          case 3:
            kick.SetFreq(fmap(cv_values[i], 40.f, 80.f, Mapping::LOG));
            break;
          default:
            break;
        };
      }
    }
  }
  if (startup) {
    midi.sysex_send_buffer();
    startup = false;
  }

  if (hw.gate_in_1.State()) {  // gate in
    if (!gate_in) {
      midi.sysex_printf_buffer("Gate In\n");
      midi.sysex_send_buffer();
      kick.Trig();
      gate_in = true;
    }
  } else {
    gate_in = false;
  }

  if (button.Pressed() && !button_pressed) {
    kick.Trig();
    button_pressed = true;
  } else if (!button.Pressed()) {
    button_pressed = false;
  }

  float kick_audio[AUDIO_BLOCK_SIZE];
  for (size_t i = 0; i < size; i++) {
    kick_audio[i] = kick_volume * kick.Process(false);
  }

  float audio_in_l[AUDIO_BLOCK_SIZE];
  float audio_in_r[AUDIO_BLOCK_SIZE];
  for (size_t i = 0; i < size; i++) {
    audio_in_l[i] = in[0][i] * 0.5f;
    audio_in_r[i] = in[1][i] * 0.5f;
  }

  for (size_t i = 0; i < size; i++) {
    out[0][i] = audio_in_l[i] +
                kick_audio[i]; /**< Copy the left input to the left output */
    out[1][i] = audio_in_r[i] +
                kick_audio[i]; /**< Copy the right input to the right output */
  }
}

int main(void) {
  /** Initialize the patch_sm hardware object */
  hw.Init();

  button.Init(hw.B7);
  toggle.Init(hw.B8);

  // initialize audio things
  comp.Init(AUDIO_SAMPLE_RATE);
  kick.Init(AUDIO_SAMPLE_RATE);
  kick.SetAccent(0.9f);
  kick.SetDecay(0.5f);
  kick.SetSelfFmAmount(0.3f);
  kick.SetAttackFmAmount(0.0f);
  kick.SetDecay(0.75f);
  kick.SetSelfFmAmount(0.75f);
  kick.SetAccent(0.6f);
  kick.SetTone(0.6f / 4.0f);
  kick.SetFreq(fmap(0.33f, 40.f, 80.f, Mapping::LOG));

  midi.Init();

  hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
  hw.StartAudio(AudioCallback);

  while (1) {
  }
}