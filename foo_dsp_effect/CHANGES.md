0.33
* Fix playbackrate controls.

0.32
* Pitch/tempo/rate UI elements now have the edit box focused first.
* Added finer granularity to playback shift DSP.

0.31.x
* Rewrote pitch/tempo/playback rate UI elements.
  Now they have directly editable pitch/tempo/rate

0.30.3
* Fixed crashes on extreme tempo changes
* Fixed accuracy of tempo slider for Rubberband tempo algorithm

0.30.2
* Added finer granularity to tempo shifting operations.

0.30.1:
* Attempt fix of "illegal instruction" crash on ancient CPUs.
- Should now just require SSE1 instead of SSE2 (MSVC2017 default)
* Readd tempo algorithm switcher in the tempo DSP.

0.30:
* Moved to most recent SDK (requires foobar2000 1.4)
  - Removed redundant code, now uses DSP preset appending code in SDK.
  - Ported GUIs to foobar2000 Default UI elements.
* Removed dynamics compressor DSP, moved to seperate DSP component.
* Wrapped all classes in anonymous namespaces.
* Changed to a dynamic MSVC runtime.
* Rewrote pitch/tempo/rate DSPs.
* Updated SoundTouch to latest Subversion version.
* Updated RubberBand to latest Mercurial version.
* Reverb coefficients are now scaled according to samplerate.