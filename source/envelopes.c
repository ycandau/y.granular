#include "envelopes.h"

// ====  XFADE_NONE  ====

t_double xfade_none(t_double x, t_double a)
{
	return ((x < 0.5) ? 0 : 1);
}

// ====  XFADE_NONE_INV  ====

t_double xfade_none_inv(t_double y, t_double a)
{
	return ((y < 0.5) ? 0 : 1);
}

// ====  XFADE_LINEAR  ====

t_double xfade_linear(t_double x, t_double a)
{
	return x;
}

// ====  XFADE_LINEAR_INV  ====

t_double xfade_linear_inv(t_double y, t_double a)
{
	return y;
}

// ====  XFADE_SQRT  ====

t_double xfade_sqrt(t_double x, t_double a)
{
	return sqrt(x);
}

// ====  XFADE_SQRT_INV  ====

t_double xfade_sqrt_inv(t_double y, t_double a)
{
	return (y * y);
}

// ====  XFADE_SINUS  ====

t_double xfade_sinus(t_double x, t_double a)
{
	return (sin(x * PI / 2));
}

// ====  XFADE_SINUS_INV  ====

t_double xfade_sinus_inv(t_double y, t_double a)
{
	return (2 * asin(y) / PI);
}

// ====  PROCEDURE: RAMP_NONE and RAMP_NONE_INV ====
// No ramp from 0 to 1, to 0 to 1

t_double ramp_none(t_double x, t_double a)
{
	return ((x < 0.5) ? 0 : 1);
}

t_double ramp_none_inv(t_double y, t_double a)		// Technically undefined
{
	return ((y < 0.5) ? 0 : 1);
}

// ====  PROCEDURE: RAMP_LINEAR and RAMP_LINEAR_INV ====
// Linear ramp from 0 to 1, to 0 to 1

t_double ramp_linear(t_double x, t_double a)
{
	return x;
}

t_double ramp_linear_inv(t_double y, t_double a)
{
	return y;
}

// ====  PROCEDURE: RAMP_POLY and RAMP_POLY_INV ====
// Polynomial ramp from 0 to 1, to 0 to 1
// a <= 0:    undefined
// 0 < a < 1: negative curvature
// 1 < a < 0: positive curvature

t_double ramp_poly(t_double x, t_double a)
{
	return ((x != 0) ? exp(a * log(x)) : 0);
}

t_double ramp_poly_inv(t_double y, t_double a)
{
	return ((y != 0) ? exp(log(y) / a) : 0);
}

t_double ramp_poly_s(t_double x, t_double a)
{
	return (((a > 0) && (x > 0)) ? exp(a * log(x)) : 0);
}

t_double ramp_poly_inv_s(t_double y, t_double a)
{
	return (((a > 0) && (y > 0)) ? exp(log(y) / a) : 0);
}

// ====  PROCEDURE: RAMP_EXP and RAMP_EXP_INV ====
// Exponential ramp from 0 to 1, to 0 to 1
// a > 0: positive curvature
// a < 0: negative curvature
// a = 0: undefined - set to identity in safe versions

t_double ramp_exp(t_double x, t_double a)
{
	return (exp(a * x) - 1) / (exp(a) - 1);
}

t_double ramp_exp_inv(t_double y, t_double a)
{
	return (log(1 + y * (exp(a) - 1)) / a);
}

t_double ramp_exp_s(t_double x, t_double a)
{
	return ((a != 0) ? (exp(a * x) - 1) / (exp(a) - 1) : x);
}

t_double ramp_exp_inv_s(t_double y, t_double a)
{
	t_double tmp;

	return ((a != 0) ? (((tmp = 1 + y * (exp(a) - 1)) > 0) ? (log(tmp) / a) : 0) : y);
}

// ====  PROCEDURE: RAMP_SIGMOID and RAMP_SIGMOID_INV ====
// Sigmoid ramp from 0 to 1, to 0 to 1
// TO DO XXX

t_double ramp_sigmoid(t_double x, t_double a)
{
	return (x);
}

t_double ramp_sigmoid_inv(t_double y, t_double a)
{
	return (y);
}

// ====  PROCEDURE: RECTANGULAR_UNIT  ====
// Rectangular function from 0 to 1

t_double env_rectangular(t_double x, t_double a, t_double b)
{
	return 1;
}

// ====  PROCEDURE: TRIANGULAR_UNIT  ====
// Triangular function from 0 to 1 with peak at a

t_double env_triangular(t_double x, t_double a, t_double b)
{
	if (x < a) { return (x / a); }
	else			 { return ((x - 1) / (a - 1)); }
}

// ====  PROCEDURE: TRAPEZOIDAL_UNIT  ====
// Trapezoidal function from 0 to 1 with plateau from a to b

t_double env_trapezoidal(t_double x, t_double a, t_double b)
{
	if (x < a)			{ return (x / a); }
	else if (x < b) { return 1; }
	else						{ return ((x - 1) / (b - 1)); }
}

// ====  PROCEDURE: WELCH_UNIT  ====
// Welch (quadratic) function from 0 to 1

t_double env_welch(t_double x, t_double a, t_double b)
{
	return (4 * x * (1 - x));
}

// ====  PROCEDURE: SINE_UNIT  ====
// Sine function from 0 to 1

t_double env_sine(t_double x, t_double a, t_double b)
{
	return sin(PI * x);
}

// ====  PROCEDURE: HANN_UNIT  ====
// Hann function from 0 to 1

t_double env_hann(t_double x, t_double a, t_double b)
{
	return (0.5 * ( 1 - cos(TWOPI * x)));
}

// ====  PROCEDURE: HAMMING_UNIT  ====
// Hamming function from 0 to 1

t_double env_hamming(t_double x, t_double a, t_double b)
{
	return (25.0/46.0 - 21.0/46.0 * cos(TWOPI * x));
}

// ====  PROCEDURE: BLACKMAN_UNIT  ====
// Blackman function from 0 to 1

t_double env_blackman(t_double x, t_double a, t_double b)
{
	return (7938.0/18608.0 - 9240.0/18608.0 * cos(TWOPI * x) + 1430.0/18608.0 * cos(2 * TWOPI * x));
}

// ====  PROCEDURE: NUTTAL_UNIT  ====
// Nuttal function from 0 to 1

t_double env_nuttal(t_double x, t_double a, t_double b)
{
	return (0.355768 - 0.487396 * cos(TWOPI * x) + 0.144232 * cos(2 * TWOPI * x) - 0.012604 * cos(3 * TWOPI * x));
}

// ====  PROCEDURE: BLACKMAN_NUTTAL_UNIT  ====
// Blackman-Nuttal function from 0 to 1

t_double env_blackman_nuttal(t_double x, t_double a, t_double b)
{
	return (0.3635819 - 0.4891775 * cos(TWOPI * x) + 0.1365995 * cos(2 * TWOPI * x) - 0.0106411 * cos(3 * TWOPI * x));
}

// ====  PROCEDURE: BLACKMAN_HARRIS_UNIT  ====
// Blackman-Harris function from 0 to 1

t_double env_blackman_harris(t_double x, t_double a, t_double b)
{
	return (0.35875 - 0.48829 * cos(TWOPI * x) + 0.14128 * cos(2 * TWOPI * x) - 0.01168 * cos(3 * TWOPI * x));
}

// ====  PROCEDURE: FLAT_TOP_UNIT  ====
// Flat-top function from 0 to 1

t_double env_flat_top(t_double x, t_double a, t_double b)
{
	return ((1 - 1.93 * cos(TWOPI * x) + 1.29 * cos(2 * TWOPI * x) - 0.388 * cos(3 * TWOPI * x) + 0.028 * cos(4 * TWOPI * x)) * 0.215703);
}

// ====  PROCEDURE: TUKEY_UNIT  ====
// Tukey function from 0 to 1

t_double env_tukey(t_double x, t_double a, t_double b)
{
	if (x < a)						{ return (0.5 * (1 - cos(PI * x / a))); }
	else if (x < (1 - a))	{ return 1; }
	else									{ return (0.5 * (1 - cos(PI * (1 - x) / a))); }
}

// ====  PROCEDURE: EXPODEC_UNIT  ====
// Expodec function from 0 to 1

t_double env_expodec(t_double x, t_double a, t_double b)
{
	if (x < a)	{ return (b * x * exp(log(1 / (a * b)) * x / a)); }
	else				{ return (0.5 * (1 + cos(PI * (x - a) / (1 - a)))); }
}

// ====  PROCEDURE: REXPODEC_UNIT  ====
// Rexpodec function from 0 to 1

t_double env_rexpodec(t_double x, t_double a, t_double b)
{
	if (x < a)	{ return (0.5 * (1 - cos(PI * x / a))); }
	else				{ return (b * (1 - x) * exp(log(1 / ((1 - a) * b)) * (1 - x) / (1 - a))); }
}
