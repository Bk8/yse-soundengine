#include "stdafx.h"
#include <cmath>
#include "oscillators.hpp"
#include "utils/misc.hpp"

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */
#define HIOFFSET 1
#define LOWOFFSET 0

union tabfudge {
	Dbl d;
	Int i[2];
};

namespace YSE {
  namespace DSP {

    class cosTable {
    public:
      cosTable();
      ~cosTable();
      Flt * operator()();
    private:
      Flt * table;
    };



    saw::saw() : frequency(440), phase(0), conv(0) {}

    SAMPLE saw::operator()(SAMPLE in) {
	    if (in.getLength() != buffer.getLength()) buffer.resize(in.getLength());

	    inPtr = in.getBuffer();
	    calc(false);

	    return buffer;
    }

    SAMPLE saw::operator()(Flt frequency, UInt length) {
	    if (length != buffer.getLength()) buffer.resize(length);

	    this->frequency = frequency;
	    calc(true);

	    return buffer;
    }


    void saw::calc(Bool useFrequency) {
	    Flt * outPtr = buffer.getBuffer();
	    UInt length = buffer.getLength();

	    conv = 1./sampleRate;
	    Dbl dphase = phase + (Dbl)UNITBIT32;
	    union tabfudge tf;
	    Int normhipart;
	
	    tf.d = UNITBIT32;
	    normhipart = tf.i[HIOFFSET];
	    tf.d = dphase;

	    while (length--) {
		    tf.i[HIOFFSET] = normhipart;
		    if (useFrequency) dphase += frequency * conv;
		    else	dphase += *inPtr++ * conv;
		    *outPtr++ = tf.d - UNITBIT32;
		    tf.d = dphase;
	    }

	    tf.i[HIOFFSET] = normhipart;
	    phase = tf.d - UNITBIT32;

    }

    /**********************************************************************************/

    #define LOGCOSTABSIZE 9
    #define COSTABSIZE (1<<LOGCOSTABSIZE)

    cosTable::cosTable() {
	    Flt *fp;
	    Flt phase = 0;
	    Flt phsinc = (2. * YSE::Pi) / COSTABSIZE;
	    union tabfudge tf;
	
	    table = new Flt[COSTABSIZE + 1];
	    fp = table;
	    for (Int i = COSTABSIZE + 1; i--; fp++, phase += phsinc) *fp = ::cos(phase);

	    tf.d = UNITBIT32 + 0.5;
    }

    cosTable::~cosTable() {
      delete[] table;
    }

    Flt * cosTable::operator()() {
      return table;
    }

    Flt * CosTable() {
      static cosTable table;
      return table();
    }

    /**********************************************************************************/

    cosine::cosine() {}

    SAMPLE cosine::operator()(SAMPLE in) {
	    if (in.getLength() != buffer.getLength()) {
        buffer.resize(in.getLength());
      }

	    Flt * inPtr  = in.getBuffer();
	    Flt * outPtr = buffer.getBuffer();
	    UInt  length = in.getLength();

	    Flt * tab = CosTable();
	    Flt * addr, f1, f2, frac;
	    Dbl   dphase;
	    int   normhipart;
	    union tabfudge tf;

	    tf.d = UNITBIT32;
	    normhipart = tf.i[HIOFFSET];

	    dphase = (Dbl)(*inPtr++ * (Flt)(COSTABSIZE)) + UNITBIT32;
	    tf.d = dphase;
	    addr = tab + (tf.i[HIOFFSET] & (COSTABSIZE - 1));
	    tf.i[HIOFFSET] = normhipart;

	    while (length--) {
		    dphase = (Dbl)(*inPtr++ * (Flt)(COSTABSIZE)) + UNITBIT32;
		    frac = tf.d - UNITBIT32;
		    tf.d = dphase;
		    f1   = addr[0];
		    f2   = addr[1];
		    addr = tab + (tf.i[HIOFFSET] & (COSTABSIZE - 1));
		    *outPtr++ = f1 + frac * (f2 - f1);
		    tf.i[HIOFFSET] = normhipart;
	    }

	    return buffer;
    }

    /**********************************************************************************/

    sine::sine() : phase(0), conv(0) {}

    SAMPLE sine::operator()(SAMPLE in) {
	    if (in.getLength() != buffer.getLength()) buffer.resize(in.getLength());

	    inPtr = in.getBuffer();
	    calc(false);

	    return buffer;
    }
    
    SAMPLE sine::operator()(Flt frequency, UInt length) {
	    if (length != buffer.getLength()) buffer.resize(length);

	    this->frequency = frequency;
	    calc(true);

	    return buffer;
    }


    void sine::calc(Bool useFrequency) {
	    Flt * outPtr = buffer.getBuffer();
	    UInt length = buffer.getLength();

	    Flt *tab = CosTable(), *addr, f1, f2, frac;
	    Dbl dphase = phase + UNITBIT32;
	    Int normhipart;
	    union tabfudge tf;

	    conv = COSTABSIZE / (Flt)sampleRate;
	
	    tf.d = UNITBIT32;
	    normhipart = tf.i[HIOFFSET];

	    tf.d = dphase;
	    if (useFrequency) dphase += frequency * conv;
	    else	{
        dphase += *inPtr++ * conv;
      }
	    addr = tab + (tf.i[HIOFFSET] & (COSTABSIZE-1));
	    tf.i[HIOFFSET] = normhipart;
	    frac = tf.d - UNITBIT32;

	    while (--length) {
		    tf.d = dphase;
		    f1 = addr[0];
		    f2 = addr[1];
		    if (useFrequency) dphase += frequency * conv;
		    else	dphase += *inPtr++ * conv;
		    addr = tab + (tf.i[HIOFFSET] & (COSTABSIZE-1));
		    tf.i[HIOFFSET] = normhipart;
		    *outPtr++ = f1 + frac * (f2 - f1);
		    frac = tf.d - UNITBIT32;
	    }
	    f1 = addr[0];
	    f2 = addr[1];
	    *outPtr++ = f1 + frac * (f2 - f1);

	    tf.d = UNITBIT32 * COSTABSIZE;
	    normhipart = tf.i[HIOFFSET];
	    tf.d = dphase + (UNITBIT32 * COSTABSIZE - UNITBIT32);
	    tf.i[HIOFFSET] = normhipart;
	    phase = tf.d - UNITBIT32 * COSTABSIZE;
    }

    /**********************************************************************************/

    noise::noise() : value(307 * 1319) {}


    SAMPLE noise::operator()(UInt length) {
	    if (length != buffer.getLength()) buffer.resize(length);
	    Flt * outPtr = buffer.getBuffer();

	    while (length--) {
		    *outPtr++ = ((float)((value & 0x7fffffff) - 0x40000000)) * (float)(1.0 / 0x40000000);
		    value = value * 435898247 + 382842987;
	    }

	    return buffer;
    }

    /*******************************************************************************************/


    vcf::vcf() {
	    q = im = re = isr = 0;
    }

    vcf& vcf::sharpness(Flt q) {
	    this->q = (q > 0 ? q : 0.f);
	    return (*this);
    }

    SAMPLE vcf::operator()(SAMPLE in, SAMPLE center, sample& out2) {
	    if (in.getLength() != buffer.getLength()) buffer.resize(in.getLength()); 
	    if (in.getLength() != out2.getLength()) out2.resize(in.getLength()); 

	    Flt * inPtr = in.getBuffer();
	    Flt * centerPtr = center.getBuffer();
	    Flt * out1Ptr = buffer.getBuffer();
	    Flt * out2Ptr = out2.getBuffer();
	    UInt length = in.getLength();

	    isr = Pi2 / (Flt)sampleRate;
	
	    Flt re2;
	    Flt qinv = (q > 0 ? 1.0f/q : 0);
	    Flt ampcorrect = 2.0f - 2.0f/ (q + 2.0f);
	    Flt coefr, coefi;
	    Flt *tab = CosTable(), * addr, f1, f2, frac;
	    Dbl dphase;
	    Int normhipart, tabindex;
	    union tabfudge tf;

	    tf.d = UNITBIT32;
	    normhipart = tf.i[HIOFFSET];
	    #pragma warning ( disable : 4018 )
	    for (Int i = 0; i < length; i++) {
		    float cf, cfindx, r, oneminusr;

		    cf = *centerPtr++ * isr;
		    if (cf < 0) cf = 0;
		    cfindx = cf * (Flt)(COSTABSIZE / Pi2);
		    r = (qinv > 0 ? 1 - cf * qinv : 0);
		    if (r < 0) r = 0;
		    oneminusr = 1.0f - r;
		    dphase = ((Dbl)(cfindx)) + UNITBIT32;
		    tf.d = dphase;
		    tabindex = tf.i[HIOFFSET] & (COSTABSIZE-1);
		    addr = tab + tabindex;
		    tf.i[HIOFFSET] = normhipart;
		    frac = tf.d - UNITBIT32;
		    f1 = addr[0];
		    f2 = addr[1];
		    coefr = r * (f1 + frac * (f2 - f1));

		    addr = tab + ((tabindex - (COSTABSIZE/4)) & (COSTABSIZE-1));
		    f1 = addr[0];
		    f2 = addr[1];
		    coefi = r * (f1 + frac * (f2 - f1));

		    f1 = *inPtr++;
		    re2 = re;
		    *out1Ptr++ = re = ampcorrect * oneminusr * f1 + coefr * re2 - coefi * im;
		    *out2Ptr++ = im = coefi * re2 + coefr * im;
	    }
		
	    return buffer;
    }

  } // end DSP
}   // end YSE