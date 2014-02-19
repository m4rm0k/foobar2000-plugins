
#include "bpm_fft_impl_kissfft.h"

critical_section2 bpm_fft_impl_kissfft::g_plan_mutex;

bpm_fft_impl_kissfft::bpm_fft_impl_kissfft():
	m_plan(nullptr)
{
}

bpm_fft_impl_kissfft::~bpm_fft_impl_kissfft()
{
	if (m_plan != nullptr)
	{
		c_insync2 lock(g_plan_mutex);

		kiss_fft_cleanup();
		free(kiss_buffer);
	}
}

void bpm_fft_impl_kissfft::create_plan(int p_size)
{
	c_insync2 lock(g_plan_mutex);

	if (m_plan != nullptr)
	{
		kiss_fft_cleanup();
		m_plan = nullptr;
		free(kiss_buffer);
	}

	m_plan = kiss_fftr_alloc( p_size,0 ,0,0);
	fftlen = p_size;

	m_input_buffer = (double*)KISS_FFT_MALLOC(p_size*sizeof(double));
	m_output_buffer = (double*)KISS_FFT_MALLOC(p_size*sizeof(double));
    kiss_buffer=(kiss_fft_cpx*)KISS_FFT_MALLOC(p_size*sizeof(kiss_fft_cpx));

}

void bpm_fft_impl_kissfft::execute_plan()
{
	PFC_ASSERT(m_plan != nullptr);
	double * buf = m_input_buffer.get_ptr();
	kiss_fftr(m_plan,buf,kiss_buffer);
	double * ptr  = m_output_buffer.get_ptr();
	for (int i=0;i<fftlen;i++)
	{
		ptr[i] = kiss_buffer[i].r*kiss_buffer[i].r +
        kiss_buffer[i].i * kiss_buffer[i].i;
	}

}

void bpm_fft_impl_kissfft::destroy_plan()
{
	c_insync2 lock(g_plan_mutex);

	if (m_plan != nullptr)
	{
		kiss_fft_cleanup();
		m_plan = nullptr;
		free(kiss_buffer);
	}
}

double * bpm_fft_impl_kissfft::get_input_buffer()
{
	return m_input_buffer.get_ptr();
}

double * bpm_fft_impl_kissfft::get_output_buffer()
{
	return m_output_buffer.get_ptr();
}
