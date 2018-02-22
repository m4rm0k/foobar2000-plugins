#ifndef		DMODSP_H
#define		DMODSP_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <malloc.h>
#include <mediaobj.h>
#include <dshow.h>
#include <uuids.h>
#include <medparam.h>


/* filter types */
enum DMO_Type {
	CHORUS = 1, 
    COMPRESSOR = 2,
	DISTORTION = 3,
	ECHO =3,
	FLANGER=4, 
	GARGLE=5, 
	REVERB=6, 
	PEQ=7,
	REVERBWAVES=8
};

class DMOFilter
{
private:                           
	float pf_freq, pf_qfact, pf_gain;
	int type, pf_q_is_bandwidth; 
	float xn1,xn2,yn1,yn2;
	float omega, cs, a1pha, beta, b0, b1, b2, a0, a1,a2, A, sn;

	HRESULT COMInit;
	IMediaObject *m_pObject;
	IMediaObjectInPlace *m_pMediaProcess;
	IMediaParamInfo *m_pParamInfo;
	IMediaParams *m_pMediaParams;
public:
	
	DMOFilter();
	~DMOFilter();
	void Process(float *samples, int numsamples)
	void setFrequency(float val);
	void setQuality(float val);
	void setGain(float val);
	void deinit();
	void init(int samplerate,int filter_type);
	void make_poly_from_roots(double const * roots, size_t num_roots, float * poly);
};


#endif		//ECHO_H
