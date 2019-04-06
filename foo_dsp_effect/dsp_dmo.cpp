#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "dmodsp.h"
#include "resource.h"

static void RunConfigPopupChorus(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);

static const GUID guid_dmo_chorus =
{ 0xb904dac3, 0x5dae, 0x4e9b,{ 0x96, 0xb5, 0x3d, 0x92, 0xbe, 0x33, 0xbf, 0x38 } };
static const GUID guid_dmo_chorus_placement =
{ 0x70fbae8f, 0x53c4, 0x4572,{ 0xbb, 0x3, 0x96, 0x91, 0xdb, 0x1b, 0x58, 0x22 } };
// {4663F983-BD2A-4C3D-B651-1C17786FB9F6}
static const GUID guid_dmo_compressor =
{ 0x4663f983, 0xbd2a, 0x4c3d, { 0xb6, 0x51, 0x1c, 0x17, 0x78, 0x6f, 0xb9, 0xf6 } };


class dsp_dmo_chorus : public dsp_impl_base
{
	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};
	float m_param[kChorusNumParameters];
	int m_rate, m_ch, m_ch_mask;
	bool enabled;
	DMOFilter dmo_filt;
public:
	static GUID g_get_guid()
	{
		// {B904DAC3-5DAE-4E9B-96B5-3D92BE33BF38}
		return guid_dmo_chorus;
	}

	dsp_dmo_chorus(dsp_preset const & in) :m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 )
	{
		parse_preset(m_param, enabled, in);
		m_param[kChorusFeedback] = (m_param[kChorusFeedback] + 99.0f) / 198.0f;
		enabled = true;
	}
	~dsp_dmo_chorus()
	{
		dmo_filt.deinit();
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "Chorus (DirectX)"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			dmo_filt.deinit();
			if (m_ch_mask <= audio_chunk::channel_config_stereo)
			{
				dmo_filt.init(m_rate, DMO_Type::CHORUS, m_ch);
				for (int i = 0; i < kChorusNumParameters; i++)
				dmo_filt.setparameter(i, m_param[i]);
			}
			
		}
		if (m_ch_mask <= audio_chunk::channel_config_stereo)
		{
			audio_sample * data = chunk->get_data();
			unsigned count = chunk->get_data_size();
			dmo_filt.Process(data, count);
		}
	

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		dmo_filt.deinit();
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

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		float params[kChorusNumParameters] =
		{
		0.5f,
		0.2f,
		0.11f,
		1.0f,
		0.75f,
		(40.0f + 99.0f) / 198.0f,
		0.8f
		};
		

		make_preset(params, true, p_out);
		return true;
	}
	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		::RunConfigPopupChorus(p_data, p_parent, p_callback);
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset(float params[kChorusNumParameters],bool enabled, dsp_preset & out)
	{
		dsp_preset_builder builder;
		for (int i = 0; i < kChorusNumParameters; i++)
		builder << params[i];
		builder << enabled;
		builder.finish(g_get_guid(), out);
	}

	static void parse_preset(float *params, bool & enabled, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			for (int i = 0; i < kChorusNumParameters; i++)
			{
				parser >> params[i];
			}
			parser >> enabled;
		}
		catch (exception_io_data) {
			params[kChorusWetDryMix] = 0.5f;
		    params[kChorusDepth] = 0.1f;
			params[kChorusFrequency] = 0.11f;
			params[kChorusWaveShape] = 1.0f;
			params[kChorusPhase] = 0.75f;
			params[kChorusFeedback] = (40.0f + 99.0f) / 198.0f;
			params[kChorusDelay] = 0.8f;
			enabled = true;
		}
	}
};

class CMyDSPPopupChorus : public CDialogImpl<CMyDSPPopupChorus>
{
public:
	CMyDSPPopupChorus(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
	enum { IDD = IDD_DMOCHORUS };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};

	BEGIN_MSG_MAP(CMyDSPPopupChorus)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_DMOCHORUSPHASE, CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_DMOCHORUSWAVETYPE, CBN_SELCHANGE, OnChange)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{

		CWindow w = GetDlgItem(IDC_DMOCHORUSWAVETYPE);
		CWindow w2 = GetDlgItem(IDC_DMOCHORUSPHASE);
		uSendMessageText(w, CB_ADDSTRING, 0, "Triangle");
		uSendMessageText(w, CB_ADDSTRING, 0, "Sine");
		
		uSendMessageText(w2, CB_ADDSTRING, 0, "-180");
		uSendMessageText(w2, CB_ADDSTRING, 0, "-90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "0");
		uSendMessageText(w2, CB_ADDSTRING, 0, "90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "180");


		slider_delayms = GetDlgItem(IDC_DMOCHORUSDELAYMS);
		slider_delayms.SetRange(FreqMin, FreqMax);
		slider_depthms = GetDlgItem(IDC_DMOCHORUSDEPTHMS);
		slider_depthms.SetRange(10, 100);
		slider_freq = GetDlgItem(IDC_DMOCHORUSLFOFREQ);
		slider_freq.SetRange(0, 1000);
		slider_drywet = GetDlgItem(IDC_DMOCHORUSDRYWET);
		slider_drywet.SetRange(0, 100);
		slider_feedback = GetDlgItem(IDC_DMOCHORUSFEEDBACK);
		slider_feedback.SetRange(0, 100);
		{
			bool enabled;
			dsp_dmo_chorus::parse_preset(params, enabled, m_initData);
			slider_delayms.SetPos((double)(100 * params[kChorusDelay]));
			slider_depthms.SetPos((double)(100 * params[kChorusDepth]));
			slider_freq.SetPos((double)(100 * params[kChorusFrequency]));
			slider_drywet.SetPos((double)(100 * params[kChorusWetDryMix]));
			slider_feedback.SetPos((double)(100 * params[kChorusFeedback]));
			RefreshLabel(params[kChorusDelay], params[kChorusDepth], 
			params[kChorusFrequency], params[kChorusWetDryMix],params[kChorusFeedback]);
		}
		int shape = (int)params[kChorusWaveShape];
		::SendMessage(w, CB_SETCURSEL, shape, 0);
		int phase = ((int)round(params[kChorusPhase]));
		::SendMessage(w2, CB_SETCURSEL,phase , 0);

		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnChange(UINT, int id, CWindow)
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
		params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
		params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape] = SendDlgItemMessage(IDC_DMOCHORUSWAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_DMOCHORUSPHASE, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;


		dsp_preset_impl preset;
		dsp_dmo_chorus::make_preset(params, true, preset);
		m_callback.on_preset_changed(preset);
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);

	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
	    params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
    	params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape]  = SendDlgItemMessage(IDC_DMOCHORUSWAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_DMOCHORUSPHASE, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;

		if (LOWORD(nSBCode) != SB_THUMBTRACK)
		{
			dsp_preset_impl preset;
			dsp_dmo_chorus::make_preset(params, true, preset);
			m_callback.on_preset_changed(preset);
		}
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);

	}

	void RefreshLabel(float delay_ms, float depth_ms, float lfo_freq, float drywet,float feedback)
	{
		pfc::string_formatter msg;
		msg << "Delay: ";
		msg << pfc::format_float(delay_ms, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDELAYLAB, msg);
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int(100 * depth_ms) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDEPTHMSLAB, msg);
		msg.reset();
		msg << "LFO Frequency: ";
		msg << pfc::format_float(lfo_freq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DMOCHORUSLFOFREQLAB, msg);
		msg.reset();
		msg << "Wet/Dry Mix : ";
		msg << pfc::format_int(100 * drywet) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDRYWETLAB, msg);
		msg.reset();
		msg << "Feedback : ";
		msg << pfc::format_int(100 * feedback) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSFEEDBACKLAB, msg);

	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	float params[kChorusNumParameters];
	CTrackBarCtrl slider_drywet,slider_depthms, slider_freq,slider_feedback,slider_delayms ;
	CButton m_buttonEchoEnabled;
};


// {31438FD7-07D0-43DE-A588-90B67D5D5AF1}
static const GUID guid_choruselem =
{ 0x31438fd7, 0x7d0, 0x43de, { 0xa5, 0x88, 0x90, 0xb6, 0x7d, 0x5d, 0x5a, 0xf1 } };


class uielem_chorus : public CDialogImpl<uielem_chorus>, public ui_element_instance {
public:
	uielem_chorus(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
		echo_enabled = true;
	}
	enum { IDD = IDD_DMOCHORUS1 };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};
private:

	BEGIN_MSG_MAP_EX(uielem_chorus)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_CHORUSENABLED, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
		END_MSG_MAP()



	void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
	HWND get_wnd() { return m_hWnd; }
	void set_configuration(ui_element_config::ptr config) {
		shit = parseConfig(config);
		if (m_hWnd != NULL) {
			ApplySettings();
		}
		m_callback->on_min_max_info_change();
	}
	ui_element_config::ptr get_configuration() { return makeConfig(); }
	static GUID g_get_guid() {
		return guid_choruselem;
	}
	static void g_get_name(pfc::string_base & out) { out = "Chorus (DirectX)"; }
	static ui_element_config::ptr g_get_default_configuration() {
		return makeConfig(true);
	}
	static const char* g_get_description() { return "Modifies the 'Chorus' DSP effect."; }
	static GUID g_get_subclass() {
		return ui_element_subclass_dsp;
	}

	ui_element_min_max_info get_min_max_info() {
		ui_element_min_max_info ret;

		// Note that we play nicely with separate horizontal & vertical DPI.
		// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
		CSize DPI = QueryScreenDPIEx(*this);

		if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
			DPI = CSize(96, 96);
		}


		ret.m_min_width = MulDiv(440, DPI.cx, 96);
		ret.m_min_height = MulDiv(220, DPI.cy, 96);
		ret.m_max_width = MulDiv(440, DPI.cx, 96);
		ret.m_max_height = MulDiv(220, DPI.cy, 96);

		// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
		ret.adjustForWindow(*this);

		return ret;
	}

private:
	void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
	bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

	void EchoDisable() {
		static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_chorus);
	}

	void EchoEnable(float params[], bool echo_enabled) {
		dsp_preset_impl preset;
		dsp_dmo_chorus::make_preset(params, echo_enabled, preset);
		static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
	}

	void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		if (IsEchoEnabled()) {
			GetConfig();
			dsp_preset_impl preset;
			dsp_dmo_chorus::make_preset(params, echo_enabled, preset);
			//yes change api;
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}
		else {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_chorus);
		}

	}

	void OnScroll(UINT scrollID, int pos, CWindow window)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			if (LOWORD(scrollID) != SB_THUMBTRACK)
			{
				EchoEnable(params, echo_enabled);
			}
		}

	}

	void OnChange(UINT, int id, CWindow)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			OnConfigChanged();
		}
	}

	void DSPConfigChange(dsp_chain_config const& cfg)
	{
		if (!m_ownEchoUpdate && m_hWnd != NULL) {
			ApplySettings();
		}
	}

	//set settings if from another control
	void ApplySettings()
	{
		dsp_preset_impl preset;
		if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_dmo_chorus, preset)) {
			SetEchoEnabled(true);
			dsp_dmo_chorus::parse_preset(params, echo_enabled, preset);
			SetEchoEnabled(echo_enabled);
			SetConfig();
		}
		else {
			SetEchoEnabled(false);
			SetConfig();
		}
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}


	void GetConfig()
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
		params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
		params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape] = SendDlgItemMessage(IDC_DMOCHORUSWAVETYPE2, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_DMOCHORUSPHASE2, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;
		echo_enabled = IsEchoEnabled();
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);
	}

	void SetConfig()
	{
		CWindow w = GetDlgItem(IDC_DMOCHORUSWAVETYPE2);
		CWindow w2 = GetDlgItem(IDC_DMOCHORUSPHASE2);
		slider_delayms.SetPos((double)(100 * params[kChorusDelay]));
		slider_depthms.SetPos((double)(100 * params[kChorusDepth]));
		slider_freq.SetPos((double)(100 * params[kChorusFrequency]));
		slider_drywet.SetPos((double)(100 * params[kChorusWetDryMix]));
		slider_feedback.SetPos((double)(100 * params[kChorusFeedback]));
		int shape = (int)params[kChorusWaveShape];
		::SendMessage(w, CB_SETCURSEL, shape, 0);
		int phase = ((int)round(params[kChorusPhase]));
		::SendMessage(w2, CB_SETCURSEL, phase, 0);
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);

	}

	static uint32_t parseConfig(ui_element_config::ptr cfg) {
		return 1;

	}
	static ui_element_config::ptr makeConfig(bool init = false) {
		ui_element_config_builder out;

		if (init)
		{
			uint32_t crap = 1;
			out << crap;
		}
		else
		{
			uint32_t crap = 2;
			out << crap;
		}
		return out.finish(g_get_guid());
	}

	BOOL OnInitDialog(CWindow, LPARAM) {
		CWindow w = GetDlgItem(IDC_DMOCHORUSWAVETYPE2);
		CWindow w2 = GetDlgItem(IDC_DMOCHORUSPHASE2);
		uSendMessageText(w, CB_ADDSTRING, 0, "Triangle");
		uSendMessageText(w, CB_ADDSTRING, 0, "Sine");

		uSendMessageText(w2, CB_ADDSTRING, 0, "-180");
		uSendMessageText(w2, CB_ADDSTRING, 0, "-90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "0");
		uSendMessageText(w2, CB_ADDSTRING, 0, "90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "180");

		m_buttonEchoEnabled = GetDlgItem(IDC_DMOCHORUSENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);


		slider_delayms = GetDlgItem(IDC_DMOCHORUSDELAYMS2);
		slider_delayms.SetRange(FreqMin, FreqMax);
		slider_depthms = GetDlgItem(IDC_DMOCHORUSDEPTHMS2);
		slider_depthms.SetRange(10, 100);
		slider_freq = GetDlgItem(IDC_DMOCHORUSLFOFREQ2);
		slider_freq.SetRange(0, 1000);
		slider_drywet = GetDlgItem(IDC_DMOCHORUSDRYWET2);
		slider_drywet.SetRange(0, 100);
		slider_feedback = GetDlgItem(IDC_DMOCHORUSFEEDBACK2);
		slider_feedback.SetRange(0, 100);


		float params2[kChorusNumParameters] =
		{
			0.5f,
			0.2f,
			0.11f,
			1.0f,
			0.75f,
			(40.0f + 99.0f) / 198.0f,
			0.8f
		};
		pfc::copy_array_loop_t(params, params2, kChorusNumParameters);


		m_buttonEchoEnabled = GetDlgItem(IDC_DMOCHORUSENABLE);
		m_ownEchoUpdate = false;

		ApplySettings();
		return FALSE;
	}


	uint32_t shit;

	void RefreshLabel(float delay_ms, float depth_ms, float lfo_freq, float drywet, float feedback)
	{
		pfc::string_formatter msg;
		msg << "Delay: ";
		msg << pfc::format_float(delay_ms, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDELAYLAB2, msg);
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int(100 * depth_ms) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDEPTHMSLAB2, msg);
		msg.reset();
		msg << "LFO Frequency: ";
		msg << pfc::format_float(lfo_freq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DMOCHORUSLFOFREQLAB2, msg);
		msg.reset();
		msg << "Wet/Dry Mix : ";
		msg << pfc::format_int(100 * drywet) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDRYWETLAB2, msg);
		msg.reset();
		msg << "Feedback : ";
		msg << pfc::format_int(100 * feedback) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSFEEDBACKLAB2, msg);

	}

	bool m_ownEchoUpdate;
	float params[kChorusNumParameters];
	CTrackBarCtrl slider_drywet, slider_depthms, slider_freq, slider_feedback, slider_delayms;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
protected:
	const ui_element_instance_callback::ptr m_callback;
};

class myElem_t2 : public  ui_element_impl_withpopup< uielem_chorus > {
	bool get_element_group(pfc::string_base& p_out)
	{
		p_out = "Effect DSP";
		return true;
	}

	bool get_menu_command_description(pfc::string_base& out) {
		out = "Opens a window for chorus effects.";
		return true;
	}

};
static service_factory_single_t<myElem_t2> g_myElemFactory;

static void RunConfigPopupChorus(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
{
	CMyDSPPopupChorus popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

static dsp_factory_t<dsp_dmo_chorus> g_dsp_chorus_factory;