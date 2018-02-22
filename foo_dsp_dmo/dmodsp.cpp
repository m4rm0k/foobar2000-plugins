#define _USE_MATH_DEFINES
#include "../SDK/foobar2000.h"
#include "dmodsp.h"
#include <atlbase.h>

const char* knownDMOs[] =
{
	"745057C7-F353-4F2D-A7EE-58434477730E", // AEC (Acoustic echo cancellation, not usable)
	"EFE6629C-81F7-4281-BD91-C9D604A95AF6", // Chorus
	"EF011F79-4000-406D-87AF-BFFB3FC39D57", // Compressor
	"EF114C90-CD1D-484E-96E5-09CFAF912A21", // Distortion
	"EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D", // Echo
	"EFCA3D92-DFD8-4672-A603-7420894BAD98", // Flanger
	"DAFD8210-5711-4B91-9FE3-F75B7AE279BF", // Gargle
	"EF985E71-D5C7-42D4-BA4D-2D073E2E96F4", // I3DL2Reverb
	"120CED89-3BF4-4173-A132-3CB406CF3231", // ParamEq
	"87FC0268-9A55-4360-95AA-004A1D9DE26C", // WavesReverb
	"F447B69E-1884-4A7E-8055-346F74D6EDB3" // Resampler DMO (not usable)
	};

#ifndef M_PI
#define M_PI		3.1415926535897932384626433832795
#endif
#define sqr(a) ((a) * (a))

DMOFilter::DMOFilter()
{

}

DMOFilter::~DMOFilter()
{ 

}

void DMOFilter::setQuality(float val)
{
	pf_qfact = val;
}

void DMOFilter::setGain(float val)
{
	pf_gain = val;
}

void DMOFilter::deinit()
{
	if (m_pMediaParams)
	{
		m_pMediaParams->Release();
		m_pMediaParams = nullptr;
	}
	if (m_pParamInfo)
	{
		m_pParamInfo->Release();
		m_pParamInfo = nullptr;
	}
	if (m_pMediaProcess)
	{
		m_pMediaProcess->Release();
		m_pMediaProcess = nullptr;
	}
	if (m_pObject)
	{
		m_pObject->Release();
		m_pObject = nullptr;
	}
	CoUninitialize();
}

void DMOFilter::init(int samplerate, int filter_type)
{
	DMO_MEDIA_TYPE mt;
	WAVEFORMATEX wfx;
	COMInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	CLSID clsid1;
	CLSIDFromString(CComBSTR(knownDMOs[1]), &clsid1);
	HRESULT hr = CoCreateInstance(clsid1,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IMediaObject,
		(void **)&m_pObject);

	hr = m_pObject->QueryInterface(IID_IMediaObjectInPlace, (void**)&m_pMediaProcess);
	m_pObject->QueryInterface(IID_IMediaParamInfo, (void **)&m_pParamInfo);
	m_pObject->QueryInterface(IID_IMediaParams, (void **)&m_pMediaParams);

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.formattype = FORMAT_WaveFormatEx;
	mt.pUnk = nullptr;
	mt.pbFormat = (LPBYTE)&wfx;
	mt.cbFormat = sizeof(WAVEFORMATEX);
	mt.lSampleSize = sizeof(float);
	wfx.wFormatTag = 3; // WAVE_FORMAT_IEEE_FLOAT;
	wfx.nChannels = 1;
	wfx.nSamplesPerSec = samplerate;
	wfx.wBitsPerSample = sizeof(float) * 8;
	wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0;
	hr = m_pObject->SetInputType(0,    //Input Stream index
		&mt,
		0);  // No flags specified
	hr = m_pObject->SetOutputType(0,       // Output Stream Index
		&mt,
		0);  // No flags specified
}

void DMOFilter::Process(float *samples, int numsamples)
{
	m_pMediaProcess->Process(numsamples * sizeof(float), reinterpret_cast<BYTE *>(samples), 0, DMO_INPLACE_NORMAL);
}