/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band
    An audio time-stretching and pitch-shifting library.
    Copyright 2007 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "FFT.h"


#include <fftw3.h>

#include <cmath>
#include <iostream>
#include <map>
#include <vector>

class FFTImpl
{
public:
    virtual ~FFTImpl() { }

    virtual void initFloat() = 0;
    virtual void initDouble() = 0;

    virtual void forward(double *realIn, double *realOut, double *imagOut) = 0;
    virtual void forwardPolar(double *realIn, double *magOut, double *phaseOut) = 0;
    virtual void forwardMagnitude(double *realIn, double *magOut) = 0;

    virtual void forward(float *realIn, float *realOut, float *imagOut) = 0;
    virtual void forwardPolar(float *realIn, float *magOut, float *phaseOut) = 0;
    virtual void forwardMagnitude(float *realIn, float *magOut) = 0;

    virtual void inverse(double *realIn, double *imagIn, double *realOut) = 0;
    virtual void inversePolar(double *magIn, double *phaseIn, double *realOut) = 0;

    virtual void inverse(float *realIn, float *imagIn, float *realOut) = 0;
    virtual void inversePolar(float *magIn, float *phaseIn, float *realOut) = 0;
};    




class D_FFTW : public FFTImpl
{
public:
    D_FFTW(unsigned int size) : m_size(size), m_fplanf(0), m_dplanf(0) { 
    }

    ~D_FFTW() {
        if (m_fplanf) {
            fftwf_destroy_plan(m_fplanf);
            fftwf_destroy_plan(m_fplani);
            fftwf_free(m_fbuf);
            fftwf_free(m_fpacked);
        }
        if (m_dplanf) {
            fftw_destroy_plan(m_dplanf);
            fftw_destroy_plan(m_dplani);
            fftw_free(m_dbuf);
            fftw_free(m_dpacked);
        }
    }

    //!!! check return values

    void initFloat() {
        if (m_fplanf) return;
        m_fbuf = (float *)fftw_malloc(m_size * sizeof(float));
        m_fpacked = (fftwf_complex *)fftw_malloc
            ((m_size/2 + 1) * sizeof(fftwf_complex));
        m_fplanf = fftwf_plan_dft_r2c_1d
            (m_size, m_fbuf, m_fpacked, FFTW_MEASURE);
        m_fplani = fftwf_plan_dft_c2r_1d
            (m_size, m_fpacked, m_fbuf, FFTW_MEASURE);
    }

    void initDouble() {
        if (m_dplanf) return;
        m_dbuf = (double *)fftw_malloc(m_size * sizeof(double));
        m_dpacked = (fftw_complex *)fftw_malloc
            ((m_size/2 + 1) * sizeof(fftw_complex));
        m_dplanf = fftw_plan_dft_r2c_1d
            (m_size, m_dbuf, m_dpacked, FFTW_MEASURE);
        m_dplani = fftw_plan_dft_c2r_1d
            (m_size, m_dpacked, m_dbuf, FFTW_MEASURE);
    }

    void packFloat(float *re, float *im) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            m_fpacked[i][0] = re[i];
            m_fpacked[i][1] = im[i];
        }
    }

    void packDouble(double *re, double *im) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            m_dpacked[i][0] = re[i];
            m_dpacked[i][1] = im[i];
        }
    }

    void unpackFloat(float *re, float *im) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            re[i] = m_fpacked[i][0];
            im[i] = m_fpacked[i][1];
        }
    }        

    void unpackDouble(double *re, double *im) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            re[i] = m_dpacked[i][0];
            im[i] = m_dpacked[i][1];
        }
    }        

    void forward(double *realIn, double *realOut, double *imagOut) {
        if (!m_dplanf) initDouble();
        for (unsigned int i = 0; i < m_size; ++i) {
            m_dbuf[i] = realIn[i];
        }
        fftw_execute(m_dplanf);
        unpackDouble(realOut, imagOut);
    }

    void forwardPolar(double *realIn, double *magOut, double *phaseOut) {
        if (!m_dplanf) initDouble();
        for (unsigned int i = 0; i < m_size; ++i) {
            m_dbuf[i] = realIn[i];
        }
        fftw_execute(m_dplanf);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrt(m_dpacked[i][0] * m_dpacked[i][0] +
                             m_dpacked[i][1] * m_dpacked[i][1]);
            phaseOut[i] = atan2(m_dpacked[i][1], m_dpacked[i][0]);
        }
    }

    void forwardMagnitude(double *realIn, double *magOut) {
        if (!m_dplanf) initDouble();
        for (unsigned int i = 0; i < m_size; ++i) {
            m_dbuf[i] = realIn[i];
        }
        fftw_execute(m_dplanf);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrt(m_dpacked[i][0] * m_dpacked[i][0] +
                             m_dpacked[i][1] * m_dpacked[i][1]);
        }
    }

    void forward(float *realIn, float *realOut, float *imagOut) {
        if (!m_fplanf) initFloat();
        for (unsigned int i = 0; i < m_size; ++i) {
            m_fbuf[i] = realIn[i];
        }
        fftwf_execute(m_fplanf);
        unpackFloat(realOut, imagOut);
    }

    void forwardPolar(float *realIn, float *magOut, float *phaseOut) {
        if (!m_fplanf) initFloat();
        for (unsigned int i = 0; i < m_size; ++i) {
            m_fbuf[i] = realIn[i];
        }
        fftwf_execute(m_fplanf);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrtf(m_fpacked[i][0] * m_fpacked[i][0] +
                              m_fpacked[i][1] * m_fpacked[i][1]);
          phaseOut[i] = atan2f(m_fpacked[i][1], m_fpacked[i][0]) ;
        }
    }

    void forwardMagnitude(float *realIn, float *magOut) {
        if (!m_fplanf) initFloat();
        for (unsigned int i = 0; i < m_size; ++i) {
            m_fbuf[i] = realIn[i];
        }
        fftwf_execute(m_fplanf);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrtf(m_fpacked[i][0] * m_fpacked[i][0] +
                              m_fpacked[i][1] * m_fpacked[i][1]);
        }
    }

    void inverse(double *realIn, double *imagIn, double *realOut) {
        if (!m_dplanf) initDouble();
        packDouble(realIn, imagIn);
        fftw_execute(m_dplani);
        for (unsigned int i = 0; i < m_size; ++i) {
            realOut[i] = m_dbuf[i];
        }
    }

    void inversePolar(double *magIn, double *phaseIn, double *realOut) {
        if (!m_dplanf) initDouble();
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            m_dpacked[i][0] = magIn[i] * cos(phaseIn[i]);
            m_dpacked[i][1] = magIn[i] * sin(phaseIn[i]);
        }
        fftw_execute(m_dplani);
        for (unsigned int i = 0; i < m_size; ++i) {
            realOut[i] = m_dbuf[i];
        }
    }

    void inverse(float *realIn, float *imagIn, float *realOut) {
        if (!m_fplanf) initFloat();
        packFloat(realIn, imagIn);
        fftwf_execute(m_fplani);
        for (unsigned int i = 0; i < m_size; ++i) {
            realOut[i] = m_fbuf[i];
        }
    }

    void inversePolar(float *magIn, float *phaseIn, float *realOut) {
        if (!m_fplanf) initFloat();
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            m_fpacked[i][0] = magIn[i] * cosf(phaseIn[i]);
            m_fpacked[i][1] = magIn[i] * sinf(phaseIn[i]);
        }
        fftwf_execute(m_fplani);
        for (unsigned int i = 0; i < m_size; ++i) {
            realOut[i] = m_fbuf[i];
        }
    }

private:
    unsigned int m_size;
    fftwf_plan m_fplanf;
    fftwf_plan m_fplani;
    fftw_plan m_dplanf;
    fftw_plan m_dplani;
    float *m_fbuf;
    fftwf_complex *m_fpacked;
    double *m_dbuf;
    fftw_complex *m_dpacked;
};


class D_Cross : public FFTImpl
{
public:
    D_Cross(unsigned int size) : m_size(size), m_table(0) {
        
        m_a = new double[size];
        m_b = new double[size];
        m_c = new double[size];
        m_d = new double[size];

        m_table = new int[m_size];
    
        unsigned int bits;
        unsigned int i, j, k, m;

        for (i = 0; ; ++i) {
            if (m_size & (1 << i)) {
                bits = i;
                break;
            }
        }
        
        for (i = 0; i < m_size; ++i) {
            
            m = i;
            
            for (j = k = 0; j < bits; ++j) {
                k = (k << 1) | (m & 1);
                m >>= 1;
            }
            
            m_table[i] = k;
        }
    }

    ~D_Cross() {
        delete[] m_table;
        delete[] m_a;
        delete[] m_b;
        delete[] m_c;
        delete[] m_d;
    }

    void initFloat() { }
    void initDouble() { }

    void forward(double *realIn, double *realOut, double *imagOut) {
        basefft(false, realIn, 0, m_c, m_d);
        for (size_t i = 0; i <= m_size/2; ++i) realOut[i] = m_c[i];
        for (size_t i = 0; i <= m_size/2; ++i) imagOut[i] = m_d[i];
    }

    void forwardPolar(double *realIn, double *magOut, double *phaseOut) {
        basefft(false, realIn, 0, m_c, m_d);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrt(m_c[i] * m_c[i] + m_d[i] * m_d[i]);
            phaseOut[i] = atan2(m_d[i], m_c[i]) ;
        }
    }

    void forwardMagnitude(double *realIn, double *magOut) {
        basefft(false, realIn, 0, m_c, m_d);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrt(m_c[i] * m_c[i] + m_d[i] * m_d[i]);
        }
    }

    void forward(float *realIn, float *realOut, float *imagOut) {
        for (size_t i = 0; i < m_size; ++i) m_a[i] = realIn[i];
        basefft(false, m_a, 0, m_c, m_d);
        for (size_t i = 0; i <= m_size/2; ++i) realOut[i] = m_c[i];
        for (size_t i = 0; i <= m_size/2; ++i) imagOut[i] = m_d[i];
    }

    void forwardPolar(float *realIn, float *magOut, float *phaseOut) {
        for (size_t i = 0; i < m_size; ++i) m_a[i] = realIn[i];
        basefft(false, m_a, 0, m_c, m_d);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrt(m_c[i] * m_c[i] + m_d[i] * m_d[i]);
            phaseOut[i] = atan2(m_d[i], m_c[i]) ;
        }
    }

    void forwardMagnitude(float *realIn, float *magOut) {
        for (size_t i = 0; i < m_size; ++i) m_a[i] = realIn[i];
        basefft(false, m_a, 0, m_c, m_d);
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            magOut[i] = sqrt(m_c[i] * m_c[i] + m_d[i] * m_d[i]);
        }
    }

    void inverse(double *realIn, double *imagIn, double *realOut) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            double real = realIn[i];
            double imag = imagIn[i];
            m_a[i] = real;
            m_b[i] = imag;
            if (i > 0) {
                m_a[m_size-i] = real;
                m_b[m_size-i] = -imag;
            }
        }
        basefft(true, m_a, m_b, realOut, m_d);
    }

    void inversePolar(double *magIn, double *phaseIn, double *realOut) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            double real = magIn[i] * cos(phaseIn[i]);
            double imag = magIn[i] * sin(phaseIn[i]);
            m_a[i] = real;
            m_b[i] = imag;
            if (i > 0) {
                m_a[m_size-i] = real;
                m_b[m_size-i] = -imag;
            }
        }
        basefft(true, m_a, m_b, realOut, m_d);
    }

    void inverse(float *realIn, float *imagIn, float *realOut) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            float real = realIn[i];
            float imag = imagIn[i];
            m_a[i] = real;
            m_b[i] = imag;
            if (i > 0) {
                m_a[m_size-i] = real;
                m_b[m_size-i] = -imag;
            }
        }
        basefft(true, m_a, m_b, m_c, m_d);
        for (unsigned int i = 0; i < m_size; ++i) realOut[i] = m_c[i];
    }

    void inversePolar(float *magIn, float *phaseIn, float *realOut) {
        for (unsigned int i = 0; i <= m_size/2; ++i) {
            float real = magIn[i] * cosf(phaseIn[i]);
            float imag = magIn[i] * sinf(phaseIn[i]);
            m_a[i] = real;
            m_b[i] = imag;
            if (i > 0) {
                m_a[m_size-i] = real;
                m_b[m_size-i] = -imag;
            }
        }
        basefft(true, m_a, m_b, m_c, m_d);
        for (unsigned int i = 0; i < m_size; ++i) realOut[i] = m_c[i];
    }

private:
    unsigned int m_size;
    int *m_table;
    double *m_a;
    double *m_b;
    double *m_c;
    double *m_d;
    void basefft(bool inverse, double *ri, double *ii, double *ro, double *io);
};

void
D_Cross::basefft(bool inverse, double *ri, double *ii, double *ro, double *io)
{
    if (!ri || !ro || !io) return;

    unsigned int i, j, k, m;
    unsigned int blockSize, blockEnd;

    double tr, ti;

    double angle = 2.0 * M_PI;
    if (inverse) angle = -angle;

    const unsigned int n = m_size;

    if (ii) {
	for (i = 0; i < n; ++i) {
	    ro[m_table[i]] = ri[i];
	    io[m_table[i]] = ii[i];
	}
    } else {
	for (i = 0; i < n; ++i) {
	    ro[m_table[i]] = ri[i];
	    io[m_table[i]] = 0.0;
	}
    }

    blockEnd = 1;

    for (blockSize = 2; blockSize <= n; blockSize <<= 1) {

	double delta = angle / (double)blockSize;
	double sm2 = -sin(-2 * delta);
	double sm1 = -sin(-delta);
	double cm2 = cos(-2 * delta);
	double cm1 = cos(-delta);
	double w = 2 * cm1;
	double ar[3], ai[3];

	for (i = 0; i < n; i += blockSize) {

	    ar[2] = cm2;
	    ar[1] = cm1;

	    ai[2] = sm2;
	    ai[1] = sm1;

	    for (j = i, m = 0; m < blockEnd; j++, m++) {

		ar[0] = w * ar[1] - ar[2];
		ar[2] = ar[1];
		ar[1] = ar[0];

		ai[0] = w * ai[1] - ai[2];
		ai[2] = ai[1];
		ai[1] = ai[0];

		k = j + blockEnd;
		tr = ar[0] * ro[k] - ai[0] * io[k];
		ti = ar[0] * io[k] + ai[0] * ro[k];

		ro[k] = ro[j] - tr;
		io[k] = io[j] - ti;

		ro[j] += tr;
		io[j] += ti;
	    }
	}

	blockEnd = blockSize;
    }

/* fftw doesn't rescale, so nor will we

    if (inverse) {

	double denom = (double)n;

	for (i = 0; i < n; i++) {
	    ro[i] /= denom;
	    io[i] /= denom;
	}
    }
*/
}

int
FFT::m_method = -1;

FFT::FFT(unsigned int size)
{
    if (size < 2) throw InvalidSize;
    if (size & (size-1)) throw InvalidSize;

    if (m_method == -1) {
        m_method = 1;
    }

    switch (m_method) {

    case 0:
        d = new D_Cross(size);
        break;

    case 1:
        std::cerr << "FFT::FFT(" << size << "): using FFTW3 implementation"
                  << std::endl;
        d = new D_FFTW(size);
        break;

    default:
        std::cerr << "FFT::FFT(" << size << "): using built-in implementation"
                  << std::endl;
        d = new D_Cross(size);
        break;
    }
}

FFT::~FFT()
{
    delete d;
}

void
FFT::forward(double *realIn, double *realOut, double *imagOut)
{
    d->forward(realIn, realOut, imagOut);
}

void
FFT::forwardPolar(double *realIn, double *magOut, double *phaseOut)
{
    d->forwardPolar(realIn, magOut, phaseOut);
}

void
FFT::forwardMagnitude(double *realIn, double *magOut)
{
    d->forwardMagnitude(realIn, magOut);
}

void
FFT::forward(float *realIn, float *realOut, float *imagOut)
{
    d->forward(realIn, realOut, imagOut);
}

void
FFT::forwardPolar(float *realIn, float *magOut, float *phaseOut)
{
    d->forwardPolar(realIn, magOut, phaseOut);
}

void
FFT::forwardMagnitude(float *realIn, float *magOut)
{
    d->forwardMagnitude(realIn, magOut);
}

void
FFT::inverse(double *realIn, double *imagIn, double *realOut)
{
    d->inverse(realIn, imagIn, realOut);
}

void
FFT::inversePolar(double *magIn, double *phaseIn, double *realOut)
{
    d->inversePolar(magIn, phaseIn, realOut);
}

void
FFT::inverse(float *realIn, float *imagIn, float *realOut)
{
    d->inverse(realIn, imagIn, realOut);
}

void
FFT::inversePolar(float *magIn, float *phaseIn, float *realOut)
{
    d->inversePolar(magIn, phaseIn, realOut);
}

void
FFT::initFloat() 
{
    d->initFloat();
}

void
FFT::initDouble() 
{
    d->initDouble();
}


void
FFT::tune()
{
}

