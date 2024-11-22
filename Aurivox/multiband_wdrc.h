#ifndef MULTIBAND_WDRC_H
#define MULTIBAND_WDRC_H

#include <arduinoFFT.h>
#include "wdrc.h"
#include "config.h"

class MultibandWDRC {
private:
    ArduinoFFT<double>* FFT;
    WDRC wdrc_bands[NUM_BANDS];
    double real[FFT_SIZE];
    double imag[FFT_SIZE];
    
    int getBandIndex(double frequency);

public:
    MultibandWDRC();
    ~MultibandWDRC();
    void process(float* input, float* output, int size);
};

#endif