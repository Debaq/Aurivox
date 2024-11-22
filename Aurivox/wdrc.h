#ifndef WDRC_H
#define WDRC_H

#include "config.h"
#include <math.h>

class WDRC {
private:
    float alpha_attack;
    float alpha_release;
    float envelope;
    float threshold;
    float ratio;
    float knee_width;
    float band_gain;

    // Solo declaramos las funciones, sin implementarlas aquí
    float db_to_linear(float db);
    float linear_to_db(float linear);

public:
    WDRC();
    void setParameters(const BandParams& params);
    float process(float input);
};

#endif