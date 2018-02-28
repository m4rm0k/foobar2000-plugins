#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "dmodsp.h"
#include "resource.h"

static void RunConfigPopupChorus(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);
static void RunChorusConfigWindow();

DECLARE_COMPONENT_VERSION("MS DMO DSP", "0.1", "Copyright(C) 2018 mudlord\n"
"License: https://github.com/mudlord/foobar2000/blob/master/LICENSE.md \n"
"Thanks to OpenMPT authors.\n"
);
// This will prevent users from renaming the component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_dsp_dmo.dll");

static const GUID guid_dmo_chorus =
{ 0xb904dac3, 0x5dae, 0x4e9b,{ 0x96, 0xb5, 0x3d, 0x92, 0xbe, 0x33, 0xbf, 0x38 } };
static const GUID guid_dmo_chorus_placement =
{ 0x2f476458, 0x9fab, 0x4088,{ 0x90, 0x22, 0x27, 0xa0, 0x1d, 0xa7, 0x29, 0x4 } };

class my_settings : public mainmenu_commands
{
public:
	enum
	{
		cmd_stretch_settings = 0,
		cmd_total
	};

	t_uint32 get_command_count() {
		return cmd_total;
	}

	GUID get_command(t_uint32 p_index) {
		static GUID my_settings_guid = { 0xe1a3b87f, 0x61a7, 0x4989,{ 0x9c, 0xa6, 0x5e, 0x89, 0xcf, 0x8, 0x86, 0xfc } };
		switch (p_index)
		{
		case cmd_stretch_settings: return my_settings_guid;
		default: uBugCheck();
		}
	}

	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_stretch_settings: p_out = "Chorus (DirectX)"; break;
		default: uBugCheck();
		}
	}

	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_stretch_settings: p_out = "Set commands for Paulstretch."; return true;
		default: uBugCheck();
		}
	}

	GUID get_parent() {
		return mainmenu_groups::view_dsp;
	}

	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case cmd_stretch_settings:
			RunChorusConfigWindow();
			break;
		default:
			uBugCheck();
		}
	}

};

static mainmenu_commands_factory_t<my_settings> g_mainmenu_paulstretch_settings;



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
	enum { IDD = IDD_CHORUS };
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
		COMMAND_HANDLER_EX(IDC_PHASE, CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_WAVETYPE, CBN_SELCHANGE, OnChange)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{

		CWindow w = GetDlgItem(IDC_WAVETYPE);
		CWindow w2 = GetDlgItem(IDC_PHASE);
		uSendMessageText(w, CB_ADDSTRING, 0, "Triangle");
		uSendMessageText(w, CB_ADDSTRING, 0, "Sine");
		
		uSendMessageText(w2, CB_ADDSTRING, 0, "-180");
		uSendMessageText(w2, CB_ADDSTRING, 0, "-90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "0");
		uSendMessageText(w2, CB_ADDSTRING, 0, "90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "180");

		m_buttonEchoEnabled = GetDlgItem(IDC_CHORUSENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_HIDE);


		slider_delayms = GetDlgItem(IDC_CHORUSDELAYMS);
		slider_delayms.SetRange(FreqMin, FreqMax);
		slider_depthms = GetDlgItem(IDC_CHORUSDEPTHMS);
		slider_depthms.SetRange(10, 100);
		slider_freq = GetDlgItem(IDC_CHORUSLFOFREQ);
		slider_freq.SetRange(0, 1000);
		slider_drywet = GetDlgItem(IDC_CHORUSDRYWET);
		slider_drywet.SetRange(0, 100);
		slider_feedback = GetDlgItem(IDC_CHORUSFEEDBACK);
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

		params[kChorusWaveShape] = SendDlgItemMessage(IDC_WAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_PHASE, CB_GETCURSEL);
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

		params[kChorusWaveShape]  = SendDlgItemMessage(IDC_WAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_PHASE, CB_GETCURSEL);
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
		::uSetDlgItemText(*this, IDC_CHORUSDELAYLAB, msg);
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int(100 * depth_ms) << " %";
		::uSetDlgItemText(*this, IDC_CHORUSDEPTHMSLAB, msg);
		msg.reset();
		msg << "LFO Frequency: ";
		msg << pfc::format_float(lfo_freq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_CHORUSLFOFREQLAB, msg);
		msg.reset();
		msg << "Wet/Dry Mix : ";
		msg << pfc::format_int(100 * drywet) << " %";
		::uSetDlgItemText(*this, IDC_CHORUSDRYWETLAB, msg);
		msg.reset();
		msg << "Feedback : ";
		msg << pfc::format_int(100 * feedback) << " %";
		::uSetDlgItemText(*this, IDC_CHORUSFEEBACKLAB, msg);

	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	float params[kChorusNumParameters];
	CTrackBarCtrl slider_drywet,slider_depthms, slider_freq,slider_feedback,slider_delayms ;
	CButton m_buttonEchoEnabled;
};

static cfg_window_placement cfg_placement_chorus(guid_dmo_chorus_placement);

class CMyDSPWindowChorus : public CDialogImpl<CMyDSPWindowChorus>
{
public:
	CMyDSPWindowChorus(){ }
	enum { IDD = IDD_CHORUS };
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

	BEGIN_MSG_MAP(CMyDSPWindowChorus)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_PHASE, CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_WAVETYPE, CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_CHORUSENABLE, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
		MSG_WM_DESTROY(OnDestroy)
	END_MSG_MAP()

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


	void GetConfig()
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
		params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
		params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape] = SendDlgItemMessage(IDC_WAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_PHASE, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;
		echo_enabled = IsEchoEnabled();
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);
	}

	void SetConfig()
	{

		CWindow w = GetDlgItem(IDC_WAVETYPE);
		CWindow w2 = GetDlgItem(IDC_PHASE);
		slider_delayms.SetPos((double)(100 * params[kChorusDelay]));
		slider_depthms.SetPos((double)(100 * params[kChorusDepth]));
		slider_freq.SetPos((double)(100 * params[kChorusFrequency]));
		slider_drywet.SetPos((double)(100 * params[kChorusWetDryMix]));
		slider_feedback.SetPos((double)(100 * params[kChorusFeedback]));
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);
	int shape = (int)params[kChorusWaveShape];
	::SendMessage(w, CB_SETCURSEL, shape, 0);
	int phase = ((int)round(params[kChorusPhase]));
	::SendMessage(w2, CB_SETCURSEL, phase, 0);
	RefreshLabel(params[kChorusDelay], params[kChorusDepth],
		params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);
	}


	BOOL OnInitDialog(CWindow, LPARAM)
	{

		modeless_dialog_manager::g_add(m_hWnd);
		cfg_placement_chorus.on_window_creation(m_hWnd);
		CWindow w = GetDlgItem(IDC_WAVETYPE);
		CWindow w2 = GetDlgItem(IDC_PHASE);
		uSendMessageText(w, CB_ADDSTRING, 0, "Triangle");
		uSendMessageText(w, CB_ADDSTRING, 0, "Sine");

		uSendMessageText(w2, CB_ADDSTRING, 0, "-180");
		uSendMessageText(w2, CB_ADDSTRING, 0, "-90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "0");
		uSendMessageText(w2, CB_ADDSTRING, 0, "90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "180");

		m_buttonEchoEnabled = GetDlgItem(IDC_CHORUSENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);


		slider_delayms = GetDlgItem(IDC_CHORUSDELAYMS);
		slider_delayms.SetRange(FreqMin, FreqMax);
		slider_depthms = GetDlgItem(IDC_CHORUSDEPTHMS);
		slider_depthms.SetRange(10, 100);
		slider_freq = GetDlgItem(IDC_CHORUSLFOFREQ);
		slider_freq.SetRange(0, 1000);
		slider_drywet = GetDlgItem(IDC_CHORUSDRYWET);
		slider_drywet.SetRange(0, 100);
		slider_feedback = GetDlgItem(IDC_CHORUSFEEDBACK);
		slider_feedback.SetRange(0, 100);

		ApplySettings();
		return TRUE;
	}

	void OnDestroy()
	{
		modeless_dialog_manager::g_remove(m_hWnd);
		cfg_placement_chorus.on_window_destruction(m_hWnd);
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}

	void OnButton(UINT, int id, CWindow)
	{
		DestroyWindow();
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

	void RefreshLabel(float delay_ms, float depth_ms, float lfo_freq, float drywet, float feedback)
	{
		pfc::string_formatter msg;
		msg << "Delay: ";
		msg << pfc::format_float(delay_ms, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_CHORUSDELAYLAB, msg);
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int(100 * depth_ms) << " %";
		::uSetDlgItemText(*this, IDC_CHORUSDEPTHMSLAB, msg);
		msg.reset();
		msg << "LFO Frequency: ";
		msg << pfc::format_float(lfo_freq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_CHORUSLFOFREQLAB, msg);
		msg.reset();
		msg << "Wet/Dry Mix : ";
		msg << pfc::format_int(100 * drywet) << " %";
		::uSetDlgItemText(*this, IDC_CHORUSDRYWETLAB, msg);
		msg.reset();
		msg << "Feedback : ";
		msg << pfc::format_int(100 * feedback) << " %";
		::uSetDlgItemText(*this, IDC_CHORUSFEEBACKLAB, msg);

	}
	bool m_ownEchoUpdate;
	float params[kChorusNumParameters];
	CTrackBarCtrl slider_drywet, slider_depthms, slider_freq, slider_feedback, slider_delayms;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
};

static CWindow g_pitchdlg;
static void RunChorusConfigWindow()
{
	if (!core_api::assert_main_thread()) return;

	if (!g_pitchdlg.IsWindow())
	{
		CMyDSPWindowChorus  * dlg = new  CMyDSPWindowChorus();
		g_pitchdlg = dlg->Create(core_api::get_main_window());

	}
	if (g_pitchdlg.IsWindow())
	{
		g_pitchdlg.ShowWindow(SW_SHOW);
		::SetForegroundWindow(g_pitchdlg);
	}
}

static void RunConfigPopupChorus(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
{
	CMyDSPPopupChorus popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

static dsp_factory_t<dsp_dmo_chorus> g_dsp_chorus_factory;