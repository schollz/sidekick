# sidekick

This is a stereo side-chain kick drum module for the daisy patch.Init().

## Controls

### General properties

| Control     | Function    | Comment                               |
| ----------- | ----------- | ------------------------------------- |
| button (b7) | Toggle kick | Triggers a kick manually              |
| toggle (b8) | Toggle mode | Switches between kick and CV out mode |
| gate_in_1   | Trigger     | Triggers the kick through CV          |
| cv_out_1    | CV Out      | Outputs envelope CV on trigger        |

### Kick drum properties (B8 Toggle DOWN)

| Control | Function           | Comment                               |
| ------- | ------------------ | ------------------------------------- |
| cv_1    | Kick Volume        | Controls the kick drum volume         |
| cv_2    | Kick Decay/Self FM | Controls decay and self FM amount     |
| cv_3    | Kick Accent        | Controls the accent for the kick drum |
| cv_4    | Kick Tone          | Controls the tone of the kick drum    |

### CV Out properties (B8 Toggle UP)

When the B8 toggle is up, you can control out the CV output is affected by the triggered envelope.

| Control | Function           | Comment                                                     |
| ------- | ------------------ | ----------------------------------------------------------- |
| cv_1    | Envelope Max Level | Sets the maximum level of the envelope (scaled by 1.5 to 5) |
| cv_2    | Attack Time        | Controls the attack time of the envelope                    |
| cv_3    | Decay Time         | Controls the decay time of the envelope                     |
| cv_4    | (Unused)           | n/a                                                         |

