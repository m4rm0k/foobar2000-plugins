#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "dmodsp.h"


DECLARE_COMPONENT_VERSION("DMO DSP", "0.1", "Copyright(C) 2018 mudlord\n"
"License: https://github.com/mudlord/foobar2000/blob/master/LICENSE.md \n"
);
// This will prevent users from renaming the component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_dsp_dmo.dll");

class dsp_iir_cddemph : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	int p_freq; //40.0, 13000.0 (Frequency: Hz)
	int p_gain; //gain
	int p_type; //filter type
	bool iir_enabled;
	pfc::array_t<DMOFilter> m_buffers;
public:
	static GUID g_get_guid()
	{
		// {FEA092A6-EA54-4f62-B180-4C88B9EB2B67}
		// {943393FF-A077-4D76-9832-51CA19A75FDA}
		static const GUID guid =
		{ 0x943393ff, 0xa077, 0x4d76,{ 0x98, 0x32, 0x51, 0xca, 0x19, 0xa7, 0x5f, 0xda } };

		return guid;
	}

	dsp_iir_cddemph() :m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), p_freq(400), p_gain(10), p_type(0)
	{
		iir_enabled = true;
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "RIAA CD deemphasis filter"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			m_buffers.set_count( 0 );
			m_buffers.set_count( m_ch );
			for ( unsigned i = 0; i < m_ch; i++ )
			{
				DMOFilter & e = m_buffers[ i ];
				e.setFrequency(p_freq);
				e.setQuality(0.707);
				e.setGain(p_gain);
				e.init(m_rate,RIAA_CD);
			}
		}

		for ( unsigned i = 0; i < m_ch; i++ )
		{
			DMOFilter & e = m_buffers[ i ];
			audio_sample * data = chunk->get_data() + i;
			for ( unsigned j = 0, k = chunk->get_sample_count(); j < k; j++ )
			{
				*data = e.Process(*data);
				data += m_ch;
			}
		}

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		m_buffers.set_count( 0 );
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
	}
	double get_latency()
	{
		return 0;
	}
	bool need_track_change_mark()
	{
		return false;
	}
};

static dsp_factory_nopreset_t<dsp_iir_cddemph> foo_dsp_tutorial_nopreset;