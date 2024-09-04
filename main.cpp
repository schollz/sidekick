#include "core_cm7.h"
#include "daisy_patch_sm.h"
#include "daisysp.h"
//
#include "lib/daisy_midi.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

#define AUDIO_BLOCK_SIZE 32

DaisyPatchSM hw;
DaisyMidi midi;
Switch button;
Switch toggle;
Overdrive overdrive;
Limiter limiter;
Compressor comp[2];
Compressor comp_main[2];
AnalogBassDrum kick;
AdEnv env;
float kick_volume = 4.0f;
float kick_drive = 0.0f;

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
    cv_values[i] = roundf(cv_values[i] * 50) / 50.0f;
    if (cv_values[i] != cv_knobs[i] || startup) {
      midi.sysex_printf_buffer("env: %2.3f, CV_%d: %f\n", env.Process(), i + 1,
                               cv_values[i]);
      cv_knobs[i] = cv_values[i];
      if (!toggle_state) {
        // alter kick drum properties when togle is down
        switch (i) {
          case 0:
            kick_volume = cv_values[i] * 10.0f;
            break;
          case 1:
            kick.SetDecay(cv_values[i] * 2);
            kick.SetSelfFmAmount(cv_values[i] / 2.0f);
            break;
          case 2:
            kick.SetAccent(cv_values[i]);
            break;
          case 3:
            kick.SetTone(cv_values[i] / 2.0f);
            break;
          default:
            break;
        };
      } else {
        // alter the envelope properties when the toggle is up
        switch (i) {
          case 0:
            env.SetMax(1.5f + cv_values[i] * 3.5f);
            break;
          case 1:
            env.SetTime(ADENV_SEG_ATTACK, cv_values[i]);
            break;
          case 2:
            env.SetTime(ADENV_SEG_DECAY, cv_values[i]);
            break;
          case 3:
            break;
          default:
            break;
        }
      }
    }
  }
  if (startup) {
    midi.sysex_send_buffer();
    startup = false;
  }

  if (hw.gate_in_1.State()) {  // gate in
    if (!gate_in) {
      // midi.sysex_printf_buffer("Gate In\n");
      kick.Trig();
      env.Trigger();
      gate_in = true;
    }
  } else {
    gate_in = false;
  }

  if (button.Pressed() && !button_pressed) {
    midi.sysex_printf_buffer("Button Pressed\n");
    kick.Trig();
    env.Trigger();
    button_pressed = true;
  } else if (!button.Pressed()) {
    button_pressed = false;
  }

  float kick_audio[size];
  float kick_total = 0;
  for (size_t i = 0; i < size; i++) {
    float v = kick.Process(false) * kick_volume;
    kick_audio[i] = v;
    kick_total += kick_audio[i];
  }
  if (kick_drive > 0.0f) {
    limiter.ProcessBlock(kick_audio, size, 0.5f);
  }
  hw.WriteCvOut(CV_OUT_2, env.Process());
  hw.WriteCvOut(CV_OUT_1, kick_total);

  float audio_in_l[size];
  float audio_in_r[size];
  for (size_t i = 0; i < size; i++) {
    audio_in_l[i] = in[0][i];
    audio_in_r[i] = in[1][i];
  }

  // compress the audio in sidechain
  float audio_in_l_side[size];
  float audio_in_r_side[size];
  comp[0].ProcessBlock(audio_in_l, audio_in_l_side, kick_audio, size);
  comp[1].ProcessBlock(audio_in_r, audio_in_r_side, kick_audio, size);

  for (size_t i = 0; i < size; i++) {
    audio_in_l_side[i] = audio_in_l_side[i] + kick_audio[i];
    audio_in_r_side[i] = audio_in_r_side[i] + kick_audio[i];
  }
  comp_main[0].ProcessBlock(audio_in_l_side, audio_in_l_side, size);
  comp_main[1].ProcessBlock(audio_in_r_side, audio_in_r_side, size);

  for (size_t i = 0; i < size; i++) {
    out[0][i] = audio_in_l_side[i];
    out[1][i] = audio_in_l_side[i];
  }

  midi.sysex_send_buffer();
}

int main(void) {
  /** Initialize the patch_sm hardware object */
  hw.Init();

  button.Init(hw.B7);
  toggle.Init(hw.B8);

  // initialize audio things
  for (size_t i = 0; i < 2; i++) {
    comp[i].Init(hw.AudioSampleRate());
    comp[i].SetThreshold(-18.0f);
    comp[i].SetAttack(0.002);
    comp[i].SetRelease(0.05);
    comp[i].SetRatio(6.0f);
    comp[i].AutoMakeup(false);

    comp_main[i].Init(hw.AudioSampleRate());
    comp_main[i].SetThreshold(-10.0f);
    comp_main[i].SetAttack(0.02);
    comp_main[i].SetRelease(0.05);
    comp_main[i].SetRatio(4.0f);
    comp_main[i].AutoMakeup(true);
  }

  env.Init(hw.AudioSampleRate() / AUDIO_BLOCK_SIZE);
  env.SetTime(ADENV_SEG_ATTACK, 0.05);
  env.SetTime(ADENV_SEG_DECAY, 0.2);
  env.SetMin(0.0);
  env.SetMax(5.0f);
  env.SetCurve(1);  // linear

  overdrive.Init();
  limiter.Init();

  kick.Init(hw.AudioSampleRate());
  kick_volume = 0.85 * 10.0f;
  kick.SetDecay(0.65 * 2);
  kick.SetSelfFmAmount(0.65 / 2.0f);
  kick.SetAttackFmAmount(0.0f);
  kick.SetAccent(0.5f);
  kick.SetTone(0.42f / 2.0f);
  // kick.SetFreq(fmap(0.33f, 40.f, 80.f, Mapping::LOG));

  midi.Init();

  hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
  hw.StartAudio(AudioCallback);

  while (1) {
  }
}