#ifndef CONTROLCONSTANTS_H
#define CONTROLCONSTANTS_H
#include <Arduino.h>

struct controlSignals{
    float tRef;     // Temperatura de referencia
    float temp;     // Temperatura medida
    float error;    // Error
    bool u;        // Acción de control
    float emax;    // error máximo que fija el valor de histéresis

};

const float TREF_DEFAULT = 25;
const float EMAX_DEFAULT = 2;

const int PIN_RELAY = GPIO_NUM_4;

#endif