#ifndef YC_ENVELOPES_H_
#define YC_ENVELOPES_H_

// ========  HEADER FILE FOR MISCELLANEOUS MAX UTILITIES  ========

#include "ext.h"			// Header file for all objects, should always be first
#include "z_dsp.h"		// Header file for MSP objects

// ====  OUTPUTTING INFORMATION  ====


// ========  DEFINES  ========

// ====  GLOBAL VARIABLES  ====

// ====  FUNCTION DECLARATIONS  ====

// ==  CROSSFADES FUNCTIONS  ==
//     F: [0,1] --> [0,1]
//          0   -->   1

typedef enum _xfade_type
{
	XFADE_UNDEF,
	XFADE_NONE,
	XFADE_LINEAR,
	XFADE_SQRT,
	XFADE_SINUSOIDAL,
	XFADE_LAST

} t_xfade_type;

t_double xfade_none				(t_double x, t_double a);
t_double xfade_none_inv		(t_double y, t_double a);
t_double xfade_linear			(t_double x, t_double a);
t_double xfade_linear_inv	(t_double y, t_double a);
t_double xfade_sqrt				(t_double x, t_double a);
t_double xfade_sqrt_inv		(t_double y, t_double a);
t_double xfade_sinus			(t_double x, t_double a);
t_double xfade_sinus_inv	(t_double y, t_double a);

typedef t_double(*t_xfade)(t_double, t_double);

// ==  RAMP FUNCTIONS  ==
//     F: [0,1] --> [0,1]
//          0   -->   1

typedef enum _ramp_type
{
	RAMP_UNDEF,
	RAMP_NONE,
	RAMP_LINEAR,
	RAMP_POLY,
	RAMP_EXP,
	RAMP_SIGMOID,
	RAMP_LAST

} t_ramp_type;

t_double ramp_none				(t_double x, t_double a);
t_double ramp_none_inv		(t_double y, t_double a);
t_double ramp_linear			(t_double x, t_double a);
t_double ramp_linear_inv	(t_double y, t_double a);
t_double ramp_poly				(t_double x, t_double a);
t_double ramp_poly_inv		(t_double y, t_double a);
t_double ramp_poly_s			(t_double x, t_double a);
t_double ramp_poly_inv_s	(t_double y, t_double a);
t_double ramp_exp					(t_double x, t_double a);
t_double ramp_exp_inv			(t_double y, t_double a);
t_double ramp_exp_s				(t_double x, t_double a);
t_double ramp_exp_inv_s		(t_double y, t_double a);
t_double ramp_sigmoid			(t_double x, t_double a);
t_double ramp_sigmoid_inv	(t_double y, t_double a);

typedef t_double(*t_ramp)(t_double, t_double);

// ==  ENVELOPE FUNCTIONS  ==
//     F: [0,1] --> [0,1]    max(F) = 1
//          0   -->   0      (or close to it)

typedef enum _env_type
{
	ENV_UNDEF,
	ENV_NONE,
	ENV_RECTANGULAR,
	ENV_TRIANGULAR,
	ENV_TRAPEZOIDAL,
	ENV_WELCH,
	ENV_SINE,
	ENV_HANN,
	ENV_HAMMING,
	ENV_BLACKMAN,
	ENV_NUTTAL,
	ENV_BLACKMAN_NUTTAL,
	ENV_BLACKMAN_HARRIS,
	ENV_FLAT_TOP,
	ENV_TUKEY,
	ENV_EXPODEC,
	ENV_REXPODEC,
	ENV_LAST

} t_env_type;

t_double env_rectangular		 (t_double x, t_double a, t_double b);
t_double env_triangular			 (t_double x, t_double a, t_double b);
t_double env_trapezoidal		 (t_double x, t_double a, t_double b);
t_double env_welch					 (t_double x, t_double a, t_double b);
t_double env_sine						 (t_double x, t_double a, t_double b);
t_double env_hann						 (t_double x, t_double a, t_double b);
t_double env_hamming				 (t_double x, t_double a, t_double b);
t_double env_blackman				 (t_double x, t_double a, t_double b);
t_double env_nuttal					 (t_double x, t_double a, t_double b);
t_double env_blackman_nuttal (t_double x, t_double a, t_double b);
t_double env_blackman_harris (t_double x, t_double a, t_double b);
t_double env_flat_top				 (t_double x, t_double a, t_double b);
t_double env_tukey					 (t_double x, t_double a, t_double b);
t_double env_expodec				 (t_double x, t_double a, t_double b);
t_double env_rexpodec				 (t_double x, t_double a, t_double b);

// ========  END OF HEADER FILE  ========

#endif
