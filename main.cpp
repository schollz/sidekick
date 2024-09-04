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

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  for (size_t i = 0; i < size; i++) {
    out[0][i] = in[0][i]; /**< Copy the left input to the left output */
    out[1][i] = in[1][i]; /**< Copy the right input to the right output */
  }
}

int main(void) {
  /** Initialize the patch_sm hardware object */
  hw.Init();

  midi.Init();

  hw.SetAudioBlockSize(AUDIO_BLOCK_SIZE);
  hw.StartAudio(AudioCallback);

  while (1) {
    hw.ProcessAllControls();

    /** Get CV_1 Input (0, 1) */
    float value = hw.GetAdcValue(CV_1);
    System::Delay(100);
    midi.sysex_printf_buffer("CV_1: %f\n", value);
    midi.sysex_send_buffer();
  }
}