#ifdef PID_AUTOTUNE
    #ifndef pidtune_h
    #define pidtune_h

    /**
     * Some example PID auto tuning rules:
     * { {  44, 24,   0 } }  ZIEGLER_NICHOLS_PI
     * { {  34, 40, 160 } }  ZIEGLER_NICHOLS_PID
     * { {  64,  9,   0 } }  TYREUS_LUYBEN_PI
     * { {  44,  9, 126 } }  TYREUS_LUYBEN_PID
     * { {  66, 80,   0 } }  CIANCONE_MARLIN_PI
     * { {  66, 88, 162 } }  CIANCONE_MARLIN_PID
     * { {  28, 50, 133 } }  PESSEN_INTEGRAL_PID
     * { {  60, 40,  60 } }  SOME_OVERSHOOT_PID
     * { { 100, 40,  60 } }   NO_OVERSHOOT_PID
    */


    enum Peak {
        MINIMUM = -1,
        NOT_A_PEAK = 0,
        MAXIMUM = 1
    };

    enum AutoTunerState {
        AUTOTUNER_OFF = 0, 
        STEADY_STATE_AT_BASELINE = 1,
        STEADY_STATE_AFTER_STEP_UP = 2,
        RELAY_STEP_UP = 4,
        RELAY_STEP_DOWN = 8,
        CONVERGED = 16,
        FAILED = 128
    }; 

    // Tuning rule divisors
    enum {
        KP_DIVISOR = 0,
        TI_DIVISOR = 1,
        TD_DIVISOR = 2
    };

    static const float CONST_PI          = 3.14159265358979323846;
    static const float CONST_SQRT2_DIV_2 = 0.70710678118654752440;


    // TODO: finish documentation of autotune functions

    /**
     * Initializes the PID tuning process.
     * @param input pointer to input value
     * @param output pointer to output value
    */
    void pidtune_init(float* Input, float* Output);

    /**
     * Cancels the PID tuning process.
    */
    void pidtune_cancel();

    /**
     * Runs one cycle of the PID tuning process.
     * @return false if the calibration is currently running, true otherwise.
    */
    bool pidtune_runtime();

    /**
     * Calculates offset of oscillation in process value as a proportion of the amplitude approximation.
     * Assumes a trapezoidal oscillation that is stationary over the last 2 relay cycles. 
     * Needs constant phase lag, so recent changes to noiseBand are bad
    */
    float pidtune_processValueOffset(float avgStep1, float avgStep2);

    /**
     * Sets the output step size.
    */
    void pidtune_setOutputStep(float step);

    /**
     * Sets the PID tuning rule, Kp divisor.
    */
    void pidtune_setKpDivisor(uint8_t kPdiv);

    /**
     * Sets the PID tuning rule, Ti divisor.
    */
    void pidtune_setTiDivisor(uint8_t tIdiv);

    /**
     * Sets the PID tuning rule, Td divisor.
    */
    void pidtune_setTdDivisor(uint8_t tDdiv);

    /**
     * Sets the PID noise band.
    */
    void pidtune_setNoiseBand(float band);

    /**
     * Sets the PID tuning lookback time in seconds.
    */
    void pidtune_setLookbackSec(int sec);

    /**
     * @return the final PID tuning rule, Kp.
    */
    float pidtune_getKp();

    /**
     * @return the final PID tuning rule, Ti.
    */
    float pidtune_getTi();

    /**
     * @return the final PID tuning rule, Td.
    */
    float pidtune_getTd();

    #endif // pidtune_h
#endif // PID_AUTOTUNE
