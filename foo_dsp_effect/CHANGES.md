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