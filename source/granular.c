//==============================================================================
//
//  @file granular~
//  @author Yves Candau <ycandau@gmail.com>
//  
//  @brief A Max external for granular synthesis.
//  
//  This Source Code Form is subject to the terms of the Mozilla Public
//  License, v. 2.0. If a copy of the MPL was not distributed with this
//  file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
//==============================================================================

// ========  HEADER FILES  ========

// Max headers
#include <time.h>

#include "max_util.h"
#include "buffer.h"

#include "linked_list.h"
#include "envelopes.h"

// ========  DEFINES  ========

#define	SEEDERS_MAX		10
#define	GRAINS_MAX		100
#define POLY_MAX			10
#define ENV_N_SMP			1000

// ====  NUMERICAL CONSTANTS:  FOR CALCULATIONS  ====

#define LN2 0.693147180559945309417

// ====  ERROR CODES  ====

#define ERR_ARG				-1

#define	BUFF_NO_LINK	-1		// Buffer has not been linked to
#define BUFF_NO_SYM		-2		// Failed to get a symbol / name for the buffer
#define BUFF_NO_REF		-3		// Failed to get a reference for the buffer
#define BUFF_NO_OBJ		-4		// Failed to get an object for the buffer
#define BUFF_NO_FILE	-5		// Failed to load a file in the buffer
#define BUFF_READY		 1		// Buffer is succesfully linked to and a file has been loaded into it

// ========  STRUCT DEFINITION: SEEDER  ========
// Each seeder can generate a stream of grains at regular intervals
// Seeders are accessed in two ways:
//   - directly by an index in the seeder array, this is used by all interface methods
//   - through a linked list, this is used by the perform64 method, to only loop through
//     the seeders that are actually in use

typedef struct _seeder
{
	t_int16		index;				// Index of the seeder in the seeder array
	t_bool		is_on;				// When inactive the seeder is not processed in the perform64 method

	// Used to set grain parameters
	t_double	ampl;					// Amplitude multiplier
	t_int32		src_begin;		// Beginning in samples in the source buffer
	t_double	src_len_ms;		// Length in ms in the source buffer: used externally
	t_int32		src_len;			// Length in samples in the source buffer: used internally
	t_double	shift;				// Pitch shift: used externally, +1 is one octave up
	t_double	shift_r;			// Pitch shift ratio: used internally
	t_int32		out_len;			// Length in samples for the output

	// Used to determine grain generation
	t_double	period;				// Period ratio between two subsequent grains
	t_int32		period_len;		// Period length in samples between two subsequent grains - output
	t_double	speed;				// Displacement ratio between two subsequent grains

	// Used for randomization
	t_double	ampl_rand;
	t_double	begin_rand;
	t_double	length_rand;
	t_double	shift_rand;
	t_double	period_rand;

	// Source buffer symbol, reference, and object 
	t_symbol		 *buff_sym;			// Name
	t_buffer_ref *buff_ref;			// Buffer reference
	t_buffer_obj *buff_obj;			// Buffer object
	t_int16				buff_n_chn;		// Number of channels
	t_int32				buff_n_frm;		// Length in frames
	t_atom_float	buff_msr;			// Samplerate in ms

	t_int8				buff_state;		// Whether the buffer is linked to and contains a sound file
	t_symbol		 *buff_file;		// Name of the file loaded in the buffer
	t_symbol		 *buff_path;		// Full path of the file loaded in the buffer
	t_bool				buff_is_chg;	// Used when the buffer was just changed to intercept notifications
	
	// Envelope
	t_env_type		env_type;			// Envelope type
	t_symbol		 *env_sym;			// Envelope symbol
	t_double			env_alpha;		// First envelope parameter
	t_double			env_beta;			// Second envelope parameter
	float				 *env_values;		// Array to store the envelope

	t_double(*env_func) (t_double, t_double, t_double);	// Envelope function: not used at this point XXX

	// Countdown to next grain generation for each stream of grains
	t_int16		poly_cnt;
	t_int32		period_cntd[POLY_MAX];

} t_seeder;

// ========  STRUCT DEFINITION: GRAIN  ========

typedef struct _grain
{
	// Index of the seeder that created the grain
	t_int16		index;
	t_bool		is_new;

	// Grain parameters
	t_double	ampl;					// Amplitude multiplier
	t_int32		src_begin;		// Beginning in samples in the source buffer
	t_int32		src_len;			// Length in samples in the source buffer
	t_int32		out_begin;		// Beginning in samples for the output
	t_int32		out_len;			// Length in samples for the output

	// Variables used for calculations
	t_int32		out_cntd;			// Countdown in samples to end of grain
	t_int32		src_I;				// Index for interpolation in the source buffer
	t_int32		src_R;				// Remainder for interpolation in the source buffer
	t_int32		env_I;				// Index for interpolation in the envelope LUT
	t_int32		env_R;				// Remainder for interpolation in the envelope LUT

} t_grain;

// ========  STRUCTURE DECLARATION  ========

typedef struct _granular
{
	t_pxobject		obj;						// Use t_pxobject for MSP objects
																// Outlet 0: Is a signal outlet so no need for a pointer
	void				 *outl_bounds;		// Outlet 1: List outlet to output grain boundaries in ms
	void				 *outl_mess;			// Outlet 2: General message outlet
	void				 *outl_compl;			// Outlet 3: Bang outlet to indicate task completion
	t_atom				mess_arr[20];		// To output messages

	t_double			msamplerate;		// Stores the current samplerate in ms
	t_int16				connected[2];		// Inlet and outlet signal connection status
	
	t_symbol		 *buff_env_sym;		// The buffer's name
	t_buffer_ref *buff_env_ref;		// Buffer reference for grain output
	t_buffer_obj *buff_env_obj;		// Buffer object
	t_int16				env_n_frm;			// Envelope buffer and array length in samples

	t_double	master;					// Amplitude multiplier for whole output
	t_int16		poly_max;				// Maximum number of grain streams per seeder
	
	t_int16		seeders_max;		// Maximum number of seeders
	t_int16		seeders_cnt;		// Current number of seeders
	t_seeder *seeders_arr;		// Array to store the seeders
	t_list	 *seeders_list;		// Linked list to go through the active seeders
	t_int16		seeders_foc;		// Index of the seeder which is in focus, used to output grain boundaries

	t_int16		grains_max;			// Maximum number of grains
	t_int16		grains_cnt;			// Current number of grains
	t_grain	 *grains_arr;			// Array to store the grains
	t_list	 *grains_list;		// List to manage the grain indexes

	t_double (*env_func) (t_double, t_double, t_double);	// Envelope function XXX

} t_granular;

// ========  METHOD PROTOTYPES  ========

// ====  GENERAL MAX METHODS  ====

void*		granular_new				(t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_free				(t_granular *x);
t_max_err	granular_notify		(t_granular *x, t_symbol *sender_sym, t_symbol *msg, void *sender_ptr, void *data);

void		granular_dsp64			(t_granular *x, t_object *dsp64, t_int16 *count, t_double samplerate, t_int32 maxvectorsize, t_int32 flags);
void		granular_perform64	(t_granular *x, t_object *dsp64, t_double **ins, t_int16 numins, t_double **outs, t_int16 numouts, t_int32 sampleframes, t_int32 flags, void *userparam);
void		granular_assist			(t_granular *x, void *b, t_int16 type, t_int16 arg, char *str);

// ====  GRANULAR METHODS  ====

void		granular_master				(t_granular *x, t_double ampl);
void		granular_all_on				(t_granular *x);
void		granular_all_off			(t_granular *x);
void		granular_post_seeders	(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_post_grains	(t_granular *x);
void		granular_post_buffers	(t_granular *x);
void		granular_get_active		(t_granular *x);

// ====  SEEDER METHODS  ====

t_int16 granular_check_args		(t_granular *x, const char *method, t_int16 argc, t_atom *argv, t_int16 argc_exp);

void		granular_set_seeder		(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_get_seeder		(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_seeder_on		(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_seeder_off		(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);

void		granular_focus				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_ampl					(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_begin				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_length				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_shift				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_period				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_speed				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_poly					(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_period_rand	(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_buffer				(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_file					(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);

void		granular_envelope			(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void		granular_output_env		(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);

// ====  GRAIN METHODS  ====

t_grain*	granular_add_grain_fs		(t_granular *x, t_seeder *seeder, t_int32 src_offset, t_int32 out_offset);
t_grain*	granular_add_grain			(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv);
void			granular_output_grain		(t_granular *x);

void	granular_bang				(t_granular *x);

// ========  GLOBAL CLASS POINTER AND STATIC VARIABLES  ========

static t_class	 *granular_class = NULL;
static t_max_err	max_err;

static t_symbol	 *sym_empty;
static t_symbol	 *sym_on;
static t_symbol	 *sym_off;
static t_symbol	 *sym_all;
static t_symbol	 *sym_seeder;
static t_symbol  *sym_active;
static t_symbol	 *sym_env;

// ========  INITIALIZATION ROUTINE  ========

int C74_EXPORT main(void)
{
	t_class *c = class_new("granular~", (method)granular_new, (method)granular_free, (long)sizeof(t_granular), 0L, A_GIMME, 0);

	class_addmethod(c, (method)granular_notify,	"notify",	A_CANT, 0);
	class_addmethod(c, (method)granular_dsp64,	"dsp64",	A_CANT, 0);
	class_addmethod(c, (method)granular_assist,	"assist",	A_CANT, 0);
	
	class_addmethod(c, (method)granular_master,				"master",				A_FLOAT, 0);
	class_addmethod(c, (method)granular_all_on,				"all_on",								 0);
	class_addmethod(c, (method)granular_all_off,			"all_off",							 0);
	class_addmethod(c, (method)granular_post_seeders, "post_seeders",	A_GIMME, 0);
	class_addmethod(c, (method)granular_post_grains,	"post_grains",					 0);
	class_addmethod(c, (method)granular_post_buffers, "post_buffers",					 0);
	class_addmethod(c, (method)granular_get_active,		"get_active",						 0);

	class_addmethod(c, (method)granular_set_seeder,		"set_seeder",		A_GIMME, 0);
	class_addmethod(c, (method)granular_get_seeder,		"get_seeder",		A_GIMME, 0);
	class_addmethod(c, (method)granular_seeder_on,		"seeder_on",		A_GIMME, 0);
	class_addmethod(c, (method)granular_seeder_off,		"seeder_off",		A_GIMME, 0);

	class_addmethod(c, (method)granular_focus,				"focus",				A_GIMME, 0);
	class_addmethod(c, (method)granular_ampl,					"ampl",					A_GIMME, 0);
	class_addmethod(c, (method)granular_begin,				"begin",				A_GIMME, 0);
	class_addmethod(c, (method)granular_length,				"length",				A_GIMME, 0);
	class_addmethod(c, (method)granular_shift,				"shift",				A_GIMME, 0);
	class_addmethod(c, (method)granular_period,				"period",				A_GIMME, 0);
	class_addmethod(c, (method)granular_speed,				"speed",				A_GIMME, 0);
	class_addmethod(c, (method)granular_poly,					"poly",					A_GIMME, 0);
	class_addmethod(c, (method)granular_period_rand,	"period_rand",	A_GIMME, 0);
	class_addmethod(c, (method)granular_buffer,				"buffer",				A_GIMME, 0);
	class_addmethod(c, (method)granular_file,					"file",					A_GIMME, 0);

	class_addmethod(c, (method)granular_envelope,			"envelope",			A_GIMME, 0);
	class_addmethod(c, (method)granular_output_env,		"output_env",		A_GIMME, 0);

	class_addmethod(c, (method)granular_add_grain,		"add_grain",		A_GIMME, 0);
	class_addmethod(c, (method)granular_output_grain,	"output_grain",					 0);

	class_addmethod(c, (method)granular_bang,					"bang",									 0);

	class_dspinit(c);
	class_register(CLASS_BOX, c);
	granular_class = c;

	sym_empty				= gensym("");
	sym_on					= gensym("on");
	sym_off					= gensym("off");
	sym_all					= gensym("all");
	sym_seeder			= gensym("seeder");
	sym_active			= gensym("active");
	sym_env					= gensym("env");

	return 0;
}

// ========  NEW INSTANCE ROUTINE: GRANULAR_NEW  ========
// Called when the object is created

void *granular_new(t_symbol *sym, t_int16 argc, t_atom *argv)
{
	t_granular *x = NULL;
	x = (t_granular *)object_alloc(granular_class);
	
	if (x == NULL) {
		MY_ERR("Object allocation failed.");
		return NULL; }

	TRACE("granular_new");

	// Inlets and outlets
	dsp_setup((t_pxobject *)x, 1);											// One MSP inlet

	x->outl_compl		= bangout((t_object*)x);						// Outlet 3: Bang outlet to indicate task completion
	x->outl_mess		= outlet_new((t_object*)x, NULL);		// Outlet 2: General message outlet
	x->outl_bounds	= listout((t_object*)x);						// Outlet 1: List outlet to output grain boundaries in ms
	outlet_new((t_object *)x, "signal");								// Outlet 0: Signal outlet
	
	// Process arguments: The object accepts: no arguments, one integer, or two integers
	
	// If there are no arguments provided
	if (argc == 0) {
		x->seeders_max = SEEDERS_MAX;
		x->grains_max	 = GRAINS_MAX; }
	
	// If there is one argument provided
	else if ((argc == 1) && (atom_gettype(argv) == A_LONG)) {
		x->seeders_max = SEEDERS_MAX;
		x->grains_max  = (t_int16)atom_getlong(argv); }

	// If there are two arguments provided
	else if ((argc == 2) && (atom_gettype(argv) == A_LONG) && (atom_gettype(argv) == A_LONG)) {
		x->seeders_max = (t_int16)atom_getlong(argv);
		x->grains_max	 = (t_int16)atom_getlong(argv + 1); }

	// Otherwise arguments are invalid and default values are used
	else {
		x->seeders_max = SEEDERS_MAX;
		x->grains_max	 = GRAINS_MAX;

		MY_ERR("granular_new:  Invalid arguments");
		MY_ERR2("  The arguments determine the maximum number of seeders and grains.");
		MY_ERR2("  The default values are:  Max seeders: %i - Max grains: %i", SEEDERS_MAX, GRAINS_MAX);
		MY_ERR2("  Possible arguments are:");
		MY_ERR2("    No arguments:  Max seeders: %i (default) - Max grains: %i (default)", SEEDERS_MAX, GRAINS_MAX);
		MY_ERR2("    One Integer:	 Max seeders: %i (default) - Max grains: Arg 0", SEEDERS_MAX);
		MY_ERR2("    Two Integers:  Max seeders: Arg 0 - Max grains: Arg 1"); }

	POST("granular_new:  Granular object created. Maximum of %i seeders and %i grains.", x->seeders_max, x->grains_max);
	POST("  You need to link seeders to buffers and load files before being able to use the granular object.");
	
	// Initialize samplerate
	x->msamplerate  = sys_getsr() * 0.001;

	// Initialize master level and maximum number of grain streams per seeder
	x->master			= 1.;
	x->poly_max		= POLY_MAX;

	// Allocate and initialize grain array and index list
	x->grains_cnt		= 0;
	x->grains_arr		= (t_grain *)sysmem_newptr(sizeof(t_grain)* x->grains_max);
	x->grains_list	= list_new(x->grains_max);

	// Allocate and initialize seeder array and index list
	x->seeders_cnt	= 0;
	x->seeders_list	= list_new(x->seeders_max);
	x->seeders_arr	= (t_seeder *)sysmem_newptr(sizeof(t_seeder) * x->seeders_max);
	x->seeders_foc	= 0;

	// Initialize each seeder
	x->env_n_frm = ENV_N_SMP;

	for (t_int16 index = 0; index < x->seeders_max; index++) {
		
		t_seeder *seeder = x->seeders_arr + index;
		
		seeder->index				= index;
		seeder->is_on				= false;

		seeder->ampl				= 1;
		seeder->src_begin		= 0;
		seeder->src_len_ms	= 100;
		seeder->src_len			= (t_int32)(seeder->src_len_ms * x->msamplerate);
		seeder->shift				= 0;
		seeder->shift_r			= 1;
		seeder->out_len			= (t_int32)(seeder->src_len * seeder->shift_r);

		seeder->period			= 0.37;
		seeder->period_len	= (t_int32)(seeder->out_len * seeder->period);
		seeder->speed				= 1;

		seeder->ampl_rand		= 0.25;
		seeder->begin_rand	= 0.25;
		seeder->length_rand	= 0.25;
		seeder->shift_rand	= 0.25;
		seeder->period_rand	= 0.25;

		seeder->buff_sym		= sym_empty;
		seeder->buff_ref		= NULL;
		seeder->buff_obj		= NULL;
		seeder->buff_n_chn	= 0;
		seeder->buff_n_frm	= 0;
		seeder->buff_msr		= (t_atom_float)x->msamplerate;

		seeder->buff_state	= BUFF_NO_LINK;
		seeder->buff_file		= sym_empty;
		seeder->buff_path		= sym_empty;
		seeder->buff_is_chg = false;
	
		seeder->env_type		= ENV_HANN;
		seeder->env_sym			= gensym("hann");
		seeder->env_alpha		= 0;
		seeder->env_beta		= 0;
		seeder->env_values	= (float *)sysmem_newptr((long)(x->env_n_frm * sizeof(float)));

		t_double f;
		for (t_int16 i = 0; i < x->env_n_frm; i++) {
			f = (t_double)i / (x->env_n_frm - 1);
			seeder->env_values[i] = (float)env_hann(f, seeder->env_alpha, seeder->env_beta); }

		seeder->poly_cnt				= 1;
		seeder->period_cntd[0]	= 0; }

	// Initialize envelope output buffer
	x->buff_env_sym = sym_empty;
	x->buff_env_ref = NULL;
	x->buff_env_obj = NULL;
	
	// Initialize random
	srand((unsigned int)time(NULL));
	
	return (x);
}

// ========  METHOD: GRANULAR_FREE  ========

void granular_free(t_granular *x)
// Called when the object is deleted
{
	TRACE("granular_free");

	// Free grains array and list
	sysmem_freeptr(x->grains_arr);
	list_free(x->grains_list);

	// Free seeders buffer references and envelope arrays
	t_seeder *seeder;
	for (t_int16 index = 0; index < x->seeders_max; index++) {
		seeder = x->seeders_arr + index;
		if (seeder->buff_ref != NULL) { object_free(seeder->buff_ref); }
		sysmem_freeptr(seeder->env_values); }

	// Free seeders array and list
	sysmem_freeptr(x->seeders_arr);
	list_free(x->seeders_list);

	// Free envelope buffer
	if (x->buff_env_ref != NULL) { object_free(x->buff_env_ref); }

	dsp_free((t_pxobject *)x);
}

// ========  METHOD: GRANULAR_NOTIFY  ========

t_max_err granular_notify(t_granular *x, t_symbol *sender_sym, t_symbol *msg, void *sender_ptr, void *data)
{
	TRACE("granular_notify");
	
	t_symbol *class_name = object_classname(data);

	//==== If the object sending the notification is a buffer
	if (class_name == gensym("buffer~")) {

		//== Get the name of the buffer
		t_symbol *buff_name = object_method_direct(t_symbol *, (t_object *), data, gensym("getname"));

		// If it is the envelope output buffer
		if (buff_name == x->buff_env_sym) { return buffer_ref_notify(x->buff_env_ref, sender_sym, msg, sender_ptr, data); }

		// Loop through the source buffers
		for (t_int16 index = 0; index < x->seeders_max; index++) {

			t_seeder *seeder = x->seeders_arr + index;
			t_buffer_obj *buff_obj = buffer_ref_getobject(seeder->buff_ref);

			// If it is one of the source buffers
			if ((buff_name == seeder->buff_sym) && (buff_obj)) {

				seeder->buff_n_frm = (t_int32)buffer_getframecount(buff_obj);
				seeder->buff_n_chn = (t_int16)buffer_getchannelcount(buff_obj);
				seeder->buff_msr	 = buffer_getmillisamplerate(buff_obj);
				seeder->src_len		 = (t_int32)(seeder->src_len_ms * seeder->buff_msr);

				POST("notify - %s:  Buffer %s, Length: %ims, Frames: %i, Channels: %i, Samplerate: %.0f, File: %s",
					msg->s_name, seeder->buff_sym->s_name, (t_int16)(seeder->buff_n_frm / seeder->buff_msr),
					seeder->buff_n_frm, seeder->buff_n_chn, 1000 * seeder->buff_msr, seeder->buff_file->s_name);

				return buffer_ref_notify(seeder->buff_ref, sender_sym, msg, sender_ptr, data); } }

		// If it is any other buffer
		POST("notify:  Buffer \"%s\" - %s", buff_name->s_name, msg->s_name); return 0; }

	// In all other cases
	else { POST("notify:  %s object - %s", class_name->s_name, msg->s_name); return 0; }
}

// ========  METHOD: GRANULAR_DSP64  ========
// Called when the DAC is enabled

void granular_dsp64(t_granular *x, t_object *dsp64, t_int16 *count, t_double samplerate, t_int32 maxvectorsize, t_int32 flags)
{
	TRACE("granular_dsp64");

	object_method(dsp64, gensym("dsp_add64"), x, granular_perform64, 0, NULL);

	// Signal connection status
	x->connected[0] = count[0];		// Number of signal connections coming in
	x->connected[1] = count[1];		// Number of signal connections going out
	POST("Samplerate = %.0f - Maxvectorsize = %i - Count: %i %i", samplerate, maxvectorsize, x->connected[0], x->connected[1]);

	// Recalculate everything that depends on the samplerate
	x->msamplerate = samplerate * 0.001;

	for (t_int16 index = 0; index < x->seeders_max; index++) {
		x->seeders_arr[index].out_len		 = (t_int32)(x->seeders_arr[index].src_len_ms * x->seeders_arr[index].shift_r * x->msamplerate);
		x->seeders_arr[index].period_len = (t_int32)(x->seeders_arr[index].out_len * x->seeders_arr[index].period); }
}

// ========  METHOD: GRANULAR_PERFORM64  ========

void granular_perform64(t_granular *x, t_object *dsp64, t_double **ins, t_int16 numins, t_double **outs, t_int16 numouts,
	t_int32 sampleframes, t_int32 flags, void *userparam)
{
	//TRACE("granular_perform64");

	//====== Seeder variables
	t_seeder *seeder;

	t_int16	 *node = x->seeders_list->first_used;
	t_int32		period;

	//====== BEGIN: SEEDER LOOP
	while (*node != LIST_END) {

		//==== Set the current seeder
		seeder = x->seeders_arr + *node;
		
		//==== If the seeder is active
		if (seeder->is_on) {

			//== Process the main grain stream

			//== Add all the grains that the seeder generates this vector cycle
			while (seeder->period_cntd[0] < sampleframes) {
				
				// Add a grain
				granular_add_grain_fs(x, seeder, 0, seeder->period_cntd[0]);

				// Calculate and add the period for the next grain
				period = (t_int32)(seeder->period_len * (1 + (seeder->period_rand * (2.0 * rand() / RAND_MAX - 1))));
				seeder->period_cntd[0] += period;
				
				// Calculate the beginning for the next grain, using the speed value
				seeder->src_begin += (t_int32)(period * seeder->speed * seeder->buff_msr / x->msamplerate);

				// Test the boundaries and adjust if necessary
				if (seeder->src_begin < 0) { seeder->src_begin = seeder->buff_n_frm - seeder->src_len; }
				if (seeder->src_begin + seeder->src_len > seeder->buff_n_frm) { seeder->src_begin = 0; } }
			
			//== Loop through the poly streams
			for (t_int16 i = 1; i < seeder->poly_cnt; i++) {

				while (seeder->period_cntd[i] < sampleframes) {

					// Add a grain
					granular_add_grain_fs(x, seeder, (t_int32)((seeder->period_cntd[i] - seeder->period_cntd[0]) * seeder->speed
						* seeder->buff_msr / x->msamplerate), seeder->period_cntd[i]);

					// Calculate and add the period for the next grain
					seeder->period_cntd[i] += (t_int32)(seeder->period_len * (1 + (seeder->period_rand * (2.0 * rand() / RAND_MAX - 1)))); }

				//== Set the period countdown for the next vector cycle
				seeder->period_cntd[i] -= sampleframes; }

			//== Set the period countdown for the next vector cycle
			seeder->period_cntd[0] -= sampleframes; }

		//==== Iterate the seeder index list
		node = x->seeders_list->array + *node; }

	//====== END: SEEDER LOOP

	//====== Set the output vector to 0
	t_int32		n = sampleframes;
	t_double *out = outs[0];
	while (n--) { *out++ = 0; }

	//====== Grain and calculation variables
	t_grain  *grain;
	t_int32		ind, src_len, env_len, out_len;
	t_double  mult, inv_out_len;
	float		 *buff_src;

	node = x->grains_list->first_used;

	//====== BEGIN: GRAIN LOOP
	while (*node != LIST_END) {

		//==== Set the current grain and corresponding seeder
		grain = x->grains_arr + *node;
		seeder = x->seeders_arr + grain->index;

		//==== Set local variables
		out			= outs[0] + grain->out_begin;
		n				= sampleframes - grain->out_begin;
		mult		= x->master * grain->ampl;
		src_len = grain->src_len - 1;
		out_len = grain->out_len - 1;
		env_len = x->env_n_frm - 1;
		inv_out_len = 1 / (t_double)out_len;

		//====== Access and lock the source buffer
		buff_src = buffer_locksamples(seeder->buff_obj);
		
		//==== Write the grain to the output
		while (n && grain->out_cntd) {

			//== Calculate interpolated values from buffer and envelope
			ind = (grain->src_begin + grain->src_I) * seeder->buff_n_chn;
			*out++ += mult
				* (seeder->env_values[grain->env_I] + grain->env_R * inv_out_len * (seeder->env_values[grain->env_I + 1] - seeder->env_values[grain->env_I]))
				* (buff_src[ind] + grain->src_R * inv_out_len * (buff_src[ind + seeder->buff_n_chn] - buff_src[ind]));
			
			//== Iterate integer and fractional values
			grain->src_R += src_len;
			while (grain->src_R >= out_len) { grain->src_R -= out_len; grain->src_I++; }

			grain->env_R += env_len;
			while (grain->env_R >= out_len) { grain->env_R -= out_len; grain->env_I++; }

			n--; grain->out_cntd--; }
		
		//====== Unlock the samples
		buffer_unlocksamples(seeder->buff_obj);
			
		//==== Reset the output beginning to zero in case the grain was new
		grain->out_begin = 0;

		//==== If the grain is unfinished iterate the grain index list
		if (grain->out_cntd != 0) {
			node = x->grains_list->array + *node; }

		//==== Otherwise remove the grain and do not increment the index list
		else {
			x->grains_cnt--;
			list_remove_node(x->grains_list, node); } }
	
	//====== END: GRAIN LOOP

	//====== Eliminate values that are out of bounds
	n = sampleframes;
	out = outs[0];
	while (n--) {
		if (*out > 1)	 { *out = 2 - *out; }
		if (*out < -1) { *out = -2 - *out; }
		out++; }

	//====== Send out a message with the grain boundaries of the seeder in focus
	seeder = x->seeders_arr + x->seeders_foc;
	atom_setfloat(x->mess_arr, seeder->src_begin / seeder->buff_msr);
	atom_setfloat(x->mess_arr + 1, (seeder->src_begin + seeder->src_len) / seeder->buff_msr);
	outlet_list(x->outl_bounds, NULL, 2, x->mess_arr);
}

// ========  METHOD: GRANULAR_ASSIST  ========

void granular_assist(t_granular *x, void *b, t_int16 type, t_int16 arg, char *str)
{
	TRACE("granular_assist");

	if (type == ASSIST_INLET) {
		switch (arg) {
		case 0: sprintf(str, "Inlet 0: All purpose (signal, list)"); break;
		default: break; } }

	else if (type == ASSIST_OUTLET) {
		switch (arg) {
		case 0: sprintf(str, "Outlet 0: Signal outlet (signal)"); break;
		case 1: sprintf(str, "Outlet 1: List outlet to output grain boundaries in ms (list)"); break;
		case 2: sprintf(str, "Outlet 2: General message outlet (various)"); break;
		case 3: sprintf(str, "Outlet 3: Bang outlet to indicate task completion (bang)"); break;
		default: break; } }
}

// ========  GENERAL INTERFACE PROCEDURES  ========

// ====  METHOD: GRANULAR_MASTER  ====

void granular_master(t_granular *x, t_double master)
{
	//TRACE("granular_master");

	x->master = master;
}

// ====  METHOD: GRANULAR_ALL_ON  ====

void granular_all_on(t_granular *x)
{
	TRACE("granular_all_on");
	
	t_int16 *node = x->seeders_list->first_used + 1;

	// Go through the inactive seeders link list
	while (*node != LIST_END) {

		t_seeder *seeder = x->seeders_arr + *node;

		if (seeder->buff_state == BUFF_READY) {

			// Update the seeder counter, set the seeder on, and add the node to the active link list
			// Note: No need to iterate the node as that is taken care of by list_insert_first
			x->seeders_cnt++;
			seeder->is_on = true;
			list_insert_first(x->seeders_list); }
	
		else {
			// But in case the seeder is not set on the node needs to be incremented
			node = x->seeders_list->array + *node; } }

	outlet_bang(x->outl_compl);
}

// ====  METHOD: GRANULAR_ALL_OFF  ====

void granular_all_off(t_granular *x)
{
	TRACE("granular_all_off");
	
	t_int16 *node = x->seeders_list->first_used;

	// Go through the active seeders link list
	while (*node != LIST_END) {

		t_seeder *seeder = x->seeders_arr + *node;

		// Update the seeder counter, set the seeder on, and add the node to the active link list
		// Note: No need to iterate the node as that is taken care of by list_remove_node
		x->seeders_cnt--;
		seeder->is_on = false;
		list_remove_node(x->seeders_list, node); }

	outlet_bang(x->outl_compl);
}

// ====  METHOD: GRANULAR_POST_SEEDERS  ====

void granular_post_seeders(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_post_seeders");

	//list_post(x, x->seeders_list);
	t_symbol *symbol = atom_getsym(argv);

	if ((argc != 1) || (atom_gettype(argv) != A_SYM) ||
		((symbol != sym_on) && (symbol != sym_off) && (symbol != sym_all))) {
		MY_ERR("post_seeders:  Invalid arguments. The method expects one symbol: \"all\", \"on\" or \"off\".");
		return; }

	// Local variables
	t_seeder *seeder;
	char			buff_state[20];

	// Post the active seeders
	if ((symbol == sym_on) || (symbol == sym_all)) {
	
		POST("Number of active seeders:  %i", x->seeders_cnt);
	
		for (t_int16 index = 0; index < x->seeders_max; index++) {

			seeder = x->seeders_arr + index;
						
			switch (seeder->buff_state) {
			case BUFF_NO_LINK:	strcpy(buff_state, "NO LINK"); break;
			case BUFF_NO_SYM:		strcpy(buff_state, "NO SYMBOL"); break;
			case BUFF_NO_REF:		strcpy(buff_state, "NO REFERENCE"); break;
			case BUFF_NO_OBJ:		strcpy(buff_state, "NO OBJECT"); break;
			case BUFF_NO_FILE:	strcpy(buff_state, "NO FILE"); break;
			default:						strcpy(buff_state, ""); break; }
			
			if (seeder->is_on) {
		
				POST("  Seeder %i - ON - Ampl: %.2f, Beg Src: %.0fms, Len Src: %.0fms, Len Out: %.0fms, Shift: %.2f",
					index, seeder->ampl, seeder->src_begin / seeder->buff_msr, seeder->src_len_ms,
					seeder->out_len / x->msamplerate, seeder->shift);

				POST("    Period : %.2f, Period Len : %.0fms, Speed : %.2f, Random : %.2f, Poly : %i, Env: %s, Buffer: %s%s%s",
					seeder->period, seeder->period_len / x->msamplerate, seeder->speed, seeder->period_rand,
					seeder->poly_cnt, seeder->env_sym->s_name, seeder->buff_sym->s_name,
					(seeder->buff_sym != sym_empty ? " - " : ""),
					(seeder->buff_state == BUFF_READY ? seeder->buff_file->s_name : buff_state)); } } }

	// Post the inactive seeders
	if ((symbol == sym_off) || (symbol == sym_all)) {

		POST("Number of inactive seeders:  %i", x->seeders_max - x->seeders_cnt);

		for (t_int16 index = 0; index < x->seeders_max; index++) {

			seeder = x->seeders_arr + index;

			switch (seeder->buff_state) {
			case BUFF_NO_LINK:	strcpy(buff_state, "NO LINK"); break;
			case BUFF_NO_SYM:		strcpy(buff_state, "NO SYMBOL"); break;
			case BUFF_NO_REF:		strcpy(buff_state, "NO REFERENCE"); break;
			case BUFF_NO_OBJ:		strcpy(buff_state, "NO OBJECT"); break;
			case BUFF_NO_FILE:	strcpy(buff_state, "NO FILE"); break;
			default:						strcpy(buff_state, ""); break; }
			
			if (!seeder->is_on) {

				POST("  Seeder %i - OFF - Ampl: %.2f, Beg Src: %.0fms, Len Src: %.0fms, Len Out: %.0fms, Shift: %.2f",
					index, seeder->ampl, seeder->src_begin / seeder->buff_msr, seeder->src_len_ms,
					seeder->out_len / x->msamplerate, seeder->shift);

				POST("    Period : %.2f, Period Len : %.0fms, Speed : %.2f, Random : %.2f, Poly : %i, Env: %s, Buffer: %s%s%s",
					seeder->period, seeder->period_len / x->msamplerate, seeder->speed, seeder->period_rand,
					seeder->poly_cnt, seeder->env_sym->s_name, seeder->buff_sym->s_name,
					(seeder->buff_sym != sym_empty ? " - " : ""),
					(seeder->buff_state == BUFF_READY ? seeder->buff_file->s_name : buff_state)); } } }
}

// ====  METHOD: GRANULAR_POST_GRAINS  ====

void granular_post_grains(t_granular *x)
{
	TRACE("granular_post_grains");

	//list_post(x, x->grains_list);
	POST("Number of current grains: %i", x->grains_cnt);

	t_int16	 cnt = 0;
	t_int16	 *node = x->grains_list->first_used;
	t_grain	 *grain;
	t_seeder *seeder;

	while (*node != LIST_END) {
		cnt++;
		grain = x->grains_arr + *node;
		seeder = x->seeders_arr + grain->index;

		POST("  Grain %i - Ampl: %.2f, Beg Src: %.0fms / %i, Len Src: %0.fms / %i, Len Out: %.0fms / %i",
			cnt, grain->ampl, grain->src_begin / seeder->buff_msr, grain->src_begin, grain->src_len / seeder->buff_msr,
			grain->src_len, grain->out_len / x->msamplerate, grain->out_len);

		node = x->grains_list->array + *node; }
}

// ====  METHOD: GRANULAR_POST_BUFFERS  ====

void granular_post_buffers(t_granular *x)
{
	TRACE("granular_post_buffers");

	t_seeder *seeder;

	POST("Buffers:");

	for (t_int16 index = 0; index < x->seeders_max; index++) {

		seeder = x->seeders_arr + index;

		if (seeder->buff_state == BUFF_NO_LINK) {
			POST("  Seeder %i:  No buffer linked. Use \"buffer\" message to link a buffer to a seeder.", index); }
		else if (seeder->buff_state == BUFF_NO_SYM) {
			POST("  Seeder %i:  Buffer has no valid name. Use \"buffer\" message to link a buffer to a seeder.", index); }
		else if (seeder->buff_state == BUFF_NO_REF) {
			POST("  Seeder %i:  Buffer %s has no valid reference. Use \"buffer\" message to link a buffer to a seeder.", index, seeder->buff_sym->s_name); }
		else if (seeder->buff_state == BUFF_NO_OBJ) {
			POST("  Seeder %i:  Buffer %s has no valid object. Use \"buffer\" message to link a buffer to a seeder.", index, seeder->buff_sym->s_name); }
		else if (seeder->buff_state == BUFF_NO_FILE) {
			POST("  Seeder %i:  Buffer %s has no audio file loaded in. Use \"file\" message to load a file.", index, seeder->buff_sym->s_name); }

		else {
			POST("  Seeder %i:  Buffer %s, Length: %ims, Frames: %i, Channels: %i, Samplerate: %.0f, File: %s",
				index, seeder->buff_sym->s_name, (t_int16)(seeder->buff_n_frm / seeder->buff_msr),
				seeder->buff_n_frm, seeder->buff_n_chn, 1000 * seeder->buff_msr, seeder->buff_file->s_name); } }
}

// ====  METHOD: GRANULAR_GET_ACTIVE  ====

void granular_get_active(t_granular *x)
{
	TRACE("granular_get_active");

	t_atom	 *atom = x->mess_arr;

	for (t_int16 index = 0; index < x->seeders_max; index++) {
		atom_setlong(atom++, (x->seeders_arr + index)->is_on);
	}
	
	outlet_anything(x->outl_mess, sym_active, x->seeders_max, x->mess_arr);
}

// ========  INTERNAL PROCEDURES  ========
// The method receives an atom with an integer and checks that this integer is a valid index in the seeder array
// and that the corresponding seeder already exists
// Arguments:
//   t_atom     *atom:      Should contain one integer from 0 to (x->seeders_max - 1)
//   const char *method:    The name of the method calling granular_seeder_index
//   t_int16     argc:      The number of arguments expected
//   t_int16     argc_exp:  The number of arguments expected

t_int16 granular_check_args(t_granular *x, const char *method, t_int16 argc, t_atom *argv, t_int16 argc_exp)
{
	// Check the number of arguments
	if (argc != argc_exp) {
		if (argc_exp == 1) {
			MY_ERR("%s:  Invalid arguments. The method expects one integer as the seeder index.", method); return ERR_ARG; }
		else if (argc_exp == 2) {
			MY_ERR("%s:  Invalid arguments. The method expects two arguments.", method); return ERR_ARG; }
		else {
			MY_ERR("%s:  Invalid arguments. The method expects %i parameters.", method, argc_exp); return ERR_ARG; } }
	
	// The first atom has to contain an integer
	if (atom_gettype(argv) != A_LONG) {
		MY_ERR("%s:  Arg 0 (index of the seeder):  Has to be an integer.", method); return ERR_ARG; }

	t_int16 index = (t_int16)atom_getlong(argv);

	// Check the boundaries
	if (index < 0) {
		MY_ERR("%s:  Arg 0 (index of the seeder):  Has to be 0 or more. Was %i instead.", method, index); return ERR_ARG; }

	if (index >= x->seeders_max) {
		MY_ERR("%s:  Arg 0 (index of the seeder):  Has to be %i at most. Was %i instead.", method, x->seeders_max - 1, index); return ERR_ARG; }

	return index;
}

// ========  SEEDERS  ========

// ====  METHOD: GRANULAR_SET_SEEDER  ====
// Sets all seeder parameters. Called by set_seeder message.
// Arguments:	Int Float Float Float Float Float Float
//   Arg 0:  Int   - Seeder index
//   Arg 1:  Float - Amplitude
//   Arg 2:  Float - Beginning
//   Arg 3:  Float - Length
//   Arg 4:  Float - Shift
//   Arg 5:  Float - Period
//   Arg 6:  Float - Speed
//   Arg 7:  Float - Random
//   Arg 8:  Int   - Number of simultaneous grain streams

void granular_set_seeder(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_set_seeder");
	
	// Check the number of arguments
	if (argc != 9) {
		MY_ERR("add_seeder:  Wrong number of arguments. The method expects:");
		MY_ERR2("  Arg 0:  Int - Seeder index");
		MY_ERR2("  Arg 1:  Float - Amplitude");
		MY_ERR2("  Arg 2:  Float - Beginning");
		MY_ERR2("  Arg 3:  Float - Length");
		MY_ERR2("  Arg 4:  Float - Shift");
		MY_ERR2("  Arg 5:  Float - Period");
		MY_ERR2("  Arg 6:  Float - Speed");
		MY_ERR2("  Arg 7:  Float - Random");
		MY_ERR2("  Arg 8:  Int - Number of simultaneous grain streams");
		return; }

	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "set_seeder", argc, argv, 9);
	if (index == ERR_ARG) { return; }

	// Set the seeder pointer
	t_seeder *seeder		= x->seeders_arr + index;

	seeder->ampl				= (t_double)atom_getfloat(argv + 1);

	seeder->src_begin		= (t_int32)(atom_getfloat(argv + 2) * seeder->buff_n_frm);
	seeder->src_len_ms	= (t_double)atom_getfloat(argv + 3);
	seeder->src_len			= (t_int32)(seeder->src_len_ms * seeder->buff_msr);

	if (seeder->src_begin < 0) { seeder->src_begin = 0; }
	if (seeder->src_begin + seeder->src_len > seeder->buff_n_frm) { seeder->src_begin = seeder->buff_n_frm - seeder->src_len; }

	seeder->shift				= (t_double)atom_getfloat(argv + 4);
	seeder->shift_r			= (t_double)exp(- LN2 * seeder->shift);
	seeder->out_len			= (t_int32)(seeder->src_len_ms * seeder->shift_r * x->msamplerate);

	seeder->period			= (t_double)atom_getfloat(argv + 5);
	seeder->period_len	= (t_int32)(seeder->period * seeder->out_len);
	
	seeder->speed				= (t_double)atom_getfloat(argv + 6);
	seeder->period_rand	= (t_double)atom_getfloat(argv + 7);
	
	seeder->poly_cnt		= (t_int16)atom_getlong(argv + 8);
	
	if ((seeder->poly_cnt < 1) || (seeder->poly_cnt > x->poly_max)) {
		MY_ERR("add_seeder:  Arg 8 (number of grain streams):  Has to be between 1 and %i. Was %i instead. Set to 1.",
			x->poly_max, seeder->poly_cnt);
		seeder->poly_cnt = 1;	}

	for (int i = 0; i < seeder->poly_cnt; i++) {
		seeder->period_cntd[i] = (t_int32)(i * seeder->period_len / seeder->poly_cnt); }
	
	return;
}

// ====  METHOD: GRANULAR_GET_SEEDER  ====
// Gets all seeder parameters. Called by get_seeder message.
// Arguments:	Int Float Float Float Float Float Float
//		Arg 0:  Int - Seeder index
//		Arg 1:	Sym - ON or OFF
//		Arg 2:  Float - Amplitude
//		Arg 3:  Float - Beginning
//		Arg 4:  Float - Length
//		Arg 5:  Float - Shift
//		Arg 6:  Float - Period
//		Arg 7:  Float - Speed
//		Arg 8:  Float - Random
//		Arg 9:  Int - Number of simultaneous grain streams
//    Arg 10:  Symbol - Envelope type
//    Arg 11:  Symbol - Buffer name
//    Arg 12:  Symbol - File name

void granular_get_seeder(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_get_seeder");

	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "get_seeder", argc, argv, 1);
	if (index == ERR_ARG) { return; }

	t_seeder *seeder = x->seeders_arr + index;
	t_atom	 *atom	 = x->mess_arr;

	atom_setlong (atom++, index);
	atom_setsym  (atom++, (seeder->is_on ? sym_on : sym_off));
	atom_setfloat(atom++, seeder->ampl);
	atom_setfloat(atom++, seeder->src_begin);
	atom_setfloat(atom++, seeder->src_len_ms);
	atom_setfloat(atom++, seeder->shift);
	atom_setfloat(atom++, seeder->period);
	atom_setfloat(atom++, seeder->speed);
	atom_setfloat(atom++, seeder->period_rand);
	atom_setfloat(atom++, seeder->poly_cnt);
	atom_setsym  (atom++, seeder->env_sym);
	atom_setsym	 (atom++, seeder->buff_sym);
	atom_setsym	 (atom++, seeder->buff_file);

	outlet_anything(x->outl_mess, sym_seeder, 13, x->mess_arr);
}

// ====  METHOD: GRANULAR_SEEDER_ON  ====

void granular_seeder_on(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_seeder_on");

	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "seeder_on", argc, argv, 1);
	if (index == ERR_ARG) {
		outlet_bang(x->outl_compl);
		return; }

	t_seeder *seeder = x->seeders_arr + index;

	// Check if the seeder is already on
	if (seeder->is_on == true) {
		//POST("seeder_on:  Arg 0 (index of the seeder):  Seeder %i is already on.", index);
		outlet_bang(x->outl_compl);
		return; }

	// Check that a file has been loaded in the buffer
	if (seeder->buff_state == BUFF_NO_FILE) {
		POST("seeder_on:  Source buffer for seeder %i has no file loaded in.", index);
		outlet_bang(x->outl_compl);
		return; }

	// Check that the seeder has a buffer linked to it
	if (seeder->buff_state != BUFF_READY) {
		POST("seeder_on:  Source buffer for seeder %i is not ready to be used.", index);
		outlet_bang(x->outl_compl);
		return; }

	// Update the linked list to add a node and check for an error from list_insert_index
	// This should have been caught by the previous test already
	if (list_insert_index(x->seeders_list, index) == LIST_END) {
		MY_ERR("seeder_off:  Error calling list_insert_index. Could not find the index %i.", index);
		outlet_bang(x->outl_compl);
		return; }

	// Update the seeder counter and set the seeder on
	x->seeders_cnt++;
	seeder->is_on = true;

	outlet_bang(x->outl_compl);
}

// ====  METHOD: GRANULAR_SEEDER_OFF  ====

void granular_seeder_off(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_seeder_off");
	
	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "seeder_off", argc, argv, 1);
	if (index == ERR_ARG) {
		outlet_bang(x->outl_compl);
		return; }

	// Check if the seeder is already off
	if (x->seeders_arr[index].is_on == false) {
		//POST("seeder_off:  Arg 0 (index of the seeder):  Seeder %i is already off.", index);
		outlet_bang(x->outl_compl);
		return; }
	
	// Update the linked list to remove a node and check for an error from list_remove_index
	// This should have been caught by the previous test already
	if (list_remove_index(x->seeders_list, index) == LIST_END) {
		MY_ERR("seeder_off:  Error calling list_remove_index. Could not find the index %i.", index);
		outlet_bang(x->outl_compl);
		return; }
	
	// Update the seeder counter and set the seeder off
	x->seeders_cnt--;
	x->seeders_arr[index].is_on = false;

	outlet_bang(x->outl_compl);
}

// ====  METHOD: GRANULAR_FOCUS  ====

void granular_focus(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_focus");

	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "focus", argc, argv, 1);
	if (index == ERR_ARG) { return; }

	x->seeders_foc = index;

	outlet_bang(x->outl_compl);
}

// ====  METHOD: GRANULAR_AMPL  ====

void granular_ampl(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_ampl");
	
	t_int16 index = (t_int16)atom_getlong(argv);

	x->seeders_arr[index].ampl = (t_double)atom_getfloat(argv + 1);
}

// ====  METHOD: GRANULAR_BEGIN  ====
// Argument comes in as a float between 0 and 1


void granular_begin(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_begin");
	
	t_int16 index = (t_int16)atom_getlong(argv);

	x->seeders_arr[index].src_begin = (t_int32)(atom_getfloat(argv + 1) * x->seeders_arr[index].buff_n_frm);

	if (x->seeders_arr[index].src_begin < 0) {
		x->seeders_arr[index].src_begin = 0; }
	if (x->seeders_arr[index].src_begin + x->seeders_arr[index].src_len > x->seeders_arr[index].buff_n_frm) {
		x->seeders_arr[index].src_begin = x->seeders_arr[index].buff_n_frm - x->seeders_arr[index].src_len; }
}

// ====  METHOD: GRANULAR_LENGTH  ====
// Argument is the source length in ms

void granular_length(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_length");
	
	t_int16 index = (t_int16)atom_getlong(argv);

	x->seeders_arr[index].src_len_ms = (t_double)atom_getfloat(argv + 1);
	x->seeders_arr[index].src_len		 = (t_int32)(x->seeders_arr[index].src_len_ms * (x->seeders_arr + index)->buff_msr);
	x->seeders_arr[index].out_len		 = (t_int32)(x->seeders_arr[index].src_len_ms * x->seeders_arr[index].shift_r * x->msamplerate);
	x->seeders_arr[index].period_len = (t_int32)(x->seeders_arr[index].out_len * x->seeders_arr[index].period);
}

// ====  METHOD: GRANULAR_SHIFT  ====

void granular_shift(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_shift");
	
	t_int16 index = (t_int16)atom_getlong(argv);

	x->seeders_arr[index].shift			 = (t_double)atom_getfloat(argv + 1);
	x->seeders_arr[index].shift_r		 = (t_double)exp(- LN2 * x->seeders_arr[index].shift);
	x->seeders_arr[index].out_len		 = (t_int32)(x->seeders_arr[index].src_len_ms * x->seeders_arr[index].shift_r * x->msamplerate);
	x->seeders_arr[index].period_len = (t_int32)(x->seeders_arr[index].out_len * x->seeders_arr[index].period);
}

// ====  METHOD: GRANULAR_PERIOD  ====

void granular_period(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_period");
	
	t_int16 index = (t_int16)atom_getlong(argv);

	x->seeders_arr[index].period		 = (t_double)atom_getfloat(argv + 1);
	x->seeders_arr[index].period_len = (t_int32)(x->seeders_arr[index].out_len * x->seeders_arr[index].period);
}

// ====  METHOD: GRANULAR_SPEED  ====

void granular_speed(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_speed");
	
	t_int16 index = (t_int16)atom_getlong(argv);
	
	x->seeders_arr[index].speed		 = (t_double)atom_getfloat(argv + 1);
}

// ====  METHOD: GRANULAR_POLY  ====

void granular_poly(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_poly");
	
	t_int16 index		 = (t_int16)atom_getlong(argv);
	t_int16 poly_cnt = (t_int16)atom_getfloat(argv + 1);

	if ((poly_cnt < 1) || (poly_cnt > x->poly_max)) {
		MY_ERR("poly:  Arg 2 (number of grain streams):  Has to be between 1 and %i. Was %i instead.",
			x->poly_max, poly_cnt);
		return; }

	x->seeders_arr[index].poly_cnt = poly_cnt;

	for (int i = 0; i < x->seeders_arr[index].poly_cnt; i++) {
		x->seeders_arr[index].period_cntd[i] =
			(t_int32)(i * x->seeders_arr[index].period_len / x->seeders_arr[index].poly_cnt); }
}

// ====  METHOD: GRANULAR_PERIOD_RAND  ====

void granular_period_rand(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	//TRACE("granular_period_rand");
	
	t_int16 index = (t_int16)atom_getlong(argv);

	x->seeders_arr[index].period_rand = (t_double)atom_getfloat(argv + 1);
}

// ====  METHOD: GRANULAR_BUFFER  ====

void granular_buffer(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_buffer");
	
	//====== Check the number of arguments
	if (argc == 2) {

		//==== If the first atom is a symbol it has to be "env"
		if ((atom_getsym(argv) == sym_env) && (atom_gettype(argv + 1) == A_SYM)) {
			
			//== Create the envelope buffer reference and object
			x->buff_env_sym = atom_getsym(argv + 1);

			if (x->buff_env_ref) {
				buffer_ref_set(x->buff_env_ref, x->buff_env_sym); }
			else {
				x->buff_env_ref = buffer_ref_new((t_object*)x, x->buff_env_sym); }
						
			x->buff_env_obj = buffer_ref_getobject(x->buff_env_ref);

			//== If the envelope buffer is successfully found
			if (x->buff_env_obj) {
				
				// Set the size of the envelope buffer
				t_max_err max_err = object_method_long(x->buff_env_obj, gensym("sizeinsamps"), x->env_n_frm, NULL);
				if (max_err != MAX_ERR_NONE) {
					MY_ERR("buffer:  Unable to set the size of the envelope buffer \"%s\"", x->buff_env_sym->s_name); return; }

				POST("buffer:  Envelope buffer \"%s\" successfully linked to.", x->buff_env_sym->s_name); return; }

			else {
				x->buff_env_ref = NULL;
				MY_ERR("buffer:  Unable to link to envelope buffer \"%s\".", x->buff_env_sym->s_name);
				return; } }

		//==== If the first atom is an integer
		else if ((atom_gettype(argv) == A_LONG) && (atom_gettype(argv + 1) == A_SYM)) {

			//== Check that it is between 0 and (x->seeders_max - 1)
			t_int16 index = (t_int16)atom_getlong(argv);
			if ((index >= 0) && (index < x->seeders_max)) {

				t_seeder *seeder = x->seeders_arr + index;

				// Create the source buffer reference and object
				seeder->buff_sym = atom_getsym(argv + 1);

				// Test the name / symbol
				if (seeder->buff_sym == sym_empty) {
					seeder->buff_state = BUFF_NO_SYM;
					MY_ERR("buffer:  Unable to link seeder %i to source buffer. Could not get a valid name.", index);
					return; }

				// Test the buffer reference
				if (x->buff_env_ref) { buffer_ref_set(seeder->buff_ref, seeder->buff_sym); }
				else { seeder->buff_ref = buffer_ref_new((t_object*)x, seeder->buff_sym); }

				if (seeder->buff_ref == NULL) {
					seeder->buff_state = BUFF_NO_REF;
					MY_ERR("buffer:  Unable to link seeder %i to source buffer \"%s\". Could not get a valid reference.", index, seeder->buff_sym->s_name);
					return; }

				// Test the buffer object
				seeder->buff_obj = buffer_ref_getobject(seeder->buff_ref);

				if (seeder->buff_obj == NULL) {
					seeder->buff_state = BUFF_NO_OBJ;
					MY_ERR("buffer:  Unable to link seeder %i to source buffer \"%s\". Could not get a valid object.", index, seeder->buff_sym->s_name);
					return; }

				// Test if a file is loaded
				seeder->buff_n_frm = (t_int32)buffer_getframecount(seeder->buff_obj);
				seeder->buff_n_chn = (t_int16)buffer_getchannelcount(seeder->buff_obj);
				seeder->buff_msr	 = buffer_getmillisamplerate(seeder->buff_obj);
				seeder->src_len		 = (t_int32)(seeder->src_len_ms * seeder->buff_msr);

				if ((seeder->buff_n_frm == 0) || (seeder->buff_n_chn == 0) || (seeder->buff_msr == 0)) {
					seeder->buff_state = BUFF_NO_FILE;
					POST("buffer:  Seeder %i successfully linked to source buffer \"%s\". No file loaded yet.", index, seeder->buff_sym->s_name);
					return; }

				// Otherwise the buffer is linked and a file is loaded.
				seeder->buff_state = BUFF_READY;
				POST("buffer:  Seeder %i successfully linked to source buffer \"%s\".", index, seeder->buff_sym->s_name);
				return; }
		
			//== Otherwise the index is out of bounds
			else {
				MY_ERR("buffer:  Arg 0 (index of the seeder):  Has to be between 0 and %i, was %i instead.", x->seeders_max - 1, index);
				return; } } }

	MY_ERR("buffer:  Invalid arguments. The method expects:");
	MY_ERR2("  Arg 0:  Int or Symbol - Seeder index to set a source buffer or \"env\" to set the envelope buffer.");
	MY_ERR2("  Arg 1:  Symbol - The name of the buffer");
}

// ====  METHOD: GRANULAR_FILE  ====

void granular_file(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_file");

	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "file", argc, argv, 3);
	if (index == ERR_ARG) { outlet_bang(x->outl_compl); return; }

	t_seeder *seeder = x->seeders_arr + index;

	// If the seeder is on
	if (seeder->is_on == true) {
	
		// Remove all grains linked to the seeder
		t_int16 *node = x->grains_list->first_used;
		t_grain *grain;

		while (*node != LIST_END) {

			grain = x->grains_arr + *node;

			if (grain->index == index) {
				x->grains_cnt--;
				list_remove_node(x->grains_list, node); }
			else {
				node = x->grains_list->array + *node; } }

		// Remove the seeder from the active list
		list_remove_index(x->seeders_list, index);

		// Update the seeder counter and set the seeder off
		x->seeders_cnt--;
		seeder->is_on = false; }

	// Get the file name and full name with path
	
	t_symbol *file	 = atom_getsym(argv + 1);
	t_symbol *path	 = atom_getsym(argv + 2);

	seeder->buff_file		= file;
	seeder->buff_path		= path;
	seeder->buff_state	= BUFF_READY;
	seeder->buff_is_chg = true;

	// Send out a read message to the buffer object
	atom_setsym  (x->mess_arr, seeder->buff_path);
	atom_setfloat(x->mess_arr + 1, 0);
	atom_setlong (x->mess_arr + 2, -1);
	atom_setlong (x->mess_arr + 3, 1);
	
	t_atom ret;
	object_method_typed(seeder->buff_obj, gensym("read"), 4, x->mess_arr, &ret);
	buffer_setdirty(seeder->buff_obj);
	seeder->src_begin = 0;
	
	outlet_bang(x->outl_compl);
}

// ========  ENVELOPES  ========

// ====  METHOD: GRANULAR_ENVELOPE  ====

void granular_envelope(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_envelope");

	//Check the validity of the arguments
	t_int16 index = granular_check_args(x, "envelope", argc, argv, 2);
	if (index == ERR_ARG) { return; }

	t_seeder *seeder = x->seeders_arr + index;
	t_symbol *env_sym = atom_getsym(argv + 1);

	if (env_sym == gensym("none"))									{ seeder->env_func = env_rectangular;			seeder->env_type = ENV_NONE; }
	else if (env_sym == gensym("rectangular"))			{ seeder->env_func = env_rectangular;			seeder->env_type = ENV_RECTANGULAR; }
	else if (env_sym == gensym("welch"))						{ seeder->env_func = env_welch;						seeder->env_type = ENV_WELCH; }
	else if (env_sym == gensym("sine"))							{ seeder->env_func = env_sine;						seeder->env_type = ENV_SINE; }
	else if (env_sym == gensym("hann"))							{ seeder->env_func = env_hann;						seeder->env_type = ENV_HANN; }
	else if (env_sym == gensym("hamming"))					{ seeder->env_func = env_hamming;					seeder->env_type = ENV_HAMMING; }
	else if (env_sym == gensym("blackman"))					{ seeder->env_func = env_blackman;				seeder->env_type = ENV_BLACKMAN; }
	else if (env_sym == gensym("nuttal"))						{ seeder->env_func = env_nuttal;					seeder->env_type = ENV_NUTTAL; }
	else if (env_sym == gensym("blackman-nuttal"))	{ seeder->env_func = env_blackman_nuttal;	seeder->env_type = ENV_BLACKMAN_NUTTAL; }
	else if (env_sym == gensym("blackman-harris"))	{ seeder->env_func = env_blackman_harris;	seeder->env_type = ENV_BLACKMAN_HARRIS; }
	else if (env_sym == gensym("flat top"))					{ seeder->env_func = env_flat_top;				seeder->env_type = ENV_FLAT_TOP; }

	else if (env_sym == gensym("triangular")) {
		seeder->env_func = env_triangular;
		seeder->env_type = ENV_TRIANGULAR;
		seeder->env_alpha = 0.5; }

	else if (env_sym == gensym("trapezoidal")) {
		seeder->env_func = env_trapezoidal;
		seeder->env_type = ENV_TRAPEZOIDAL;
		seeder->env_alpha = 0.1;
		seeder->env_beta = 0.9; }

	else if (env_sym == gensym("tukey")) {
		seeder->env_func = env_tukey;
		seeder->env_type = ENV_TUKEY;
		seeder->env_alpha = 0.2;
		seeder->env_beta = 0.8; }

	else if (env_sym == gensym("expodec")) {
		seeder->env_func = env_expodec;
		seeder->env_type = ENV_EXPODEC;
		seeder->env_alpha = 0.9;
		seeder->env_beta = 0.2; }

	else if (env_sym == gensym("rexpodec")) {
		seeder->env_func = env_rexpodec;
		seeder->env_type = ENV_REXPODEC;
		seeder->env_alpha = 0.1;
		seeder->env_beta = 0.2; }

	else { MY_ERR("The envelope type \"%s\" is not recognized", sym->s_name); return; }

	seeder->env_sym = env_sym;

	// Calculate the envelope values (unless the type is unrecognized)
	t_double f;
	for (t_int16 i = 0; i < x->env_n_frm; i++) {
		f = (t_double)i / (x->env_n_frm - 1);
		seeder->env_values[i] = (float)seeder->env_func(f, seeder->env_alpha, seeder->env_beta); }
}

// ====  METHOD: GRANULAR_OUTPUT_ENV  ====

void granular_output_env(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_output_env");
	
	// Check the validity of the arguments
	t_int16 index = granular_check_args(x, "focus", argc, argv, 1);
	if (index == ERR_ARG) { return; }
	
	t_seeder *seeder = x->seeders_arr + index;

	if (x->buff_env_ref == NULL) {
		MY_ERR("output_env:  The envelope buffer is not set."); return; }

	x->buff_env_obj = buffer_ref_getobject(x->buff_env_ref);
	if (!x->buff_env_obj) {
		MY_ERR("The envelope buffer \"%s\" does not seem to exit.", x->buff_env_sym->s_name); return; }

	float		*buffer = buffer_locksamples(x->buff_env_obj);
	float		*env_values = seeder->env_values;
	t_int32	 cntd = x->env_n_frm;

	while (cntd--) { *buffer++ = *env_values++; }

	buffer_setdirty(x->buff_env_obj);
	buffer_unlocksamples(x->buff_env_obj);
}

// ========  GRAINS  ========

// ====  METHOD: GRANULAR_ADD_GRAIN_FS  ====
// Add a grain from a seeder. Used internally. No access through calls.
// No checking of grain boundaries. Validity is tested in the granular_perform64 method by the seeder.

t_grain* granular_add_grain_fs(t_granular *x, t_seeder *seeder, t_int32 src_offset, t_int32 out_offset)
{
	//TRACE("granular_add_grain_fs");
	
	if (x->grains_cnt == x->grains_max) {
		MY_ERR("Impossible to add grain:  Maximum number already reached.");
		return NULL; }

	x->grains_cnt++;
	t_grain *grain = (x->grains_arr + list_insert_first(x->grains_list));

	grain->index			= seeder->index;
	grain->is_new			= true;

	grain->ampl				= seeder->ampl;
	grain->src_begin	= seeder->src_begin + src_offset;
	grain->src_len		= seeder->src_len;

	if (grain->src_begin < 0) { grain->src_begin = 0; }
	if (grain->src_begin + grain->src_len > seeder->buff_n_frm) { grain->src_begin = seeder->buff_n_frm - grain->src_len; }

	grain->out_begin	= out_offset;
	grain->out_len		= seeder->out_len;

	grain->out_cntd		= grain->out_len;
	
	grain->src_I	= 0;
	grain->src_R	= 0;

	grain->env_I	= 0;
	grain->env_R	= 0;

	return grain;
}

// ====  METHOD: GRANULAR_ADD_GRAIN  ====
// Add a grain directly without using a seeder. Called by add_grain message. Validity is checked.
// Args:	Float Float Float Float
//		Arg 0:  Float - Amplitude
//		Arg 1:  Float - Beginning
//		Arg 2:  Float - Length
//		Arg 3:  Float - Shift

t_grain* granular_add_grain(t_granular *x, t_symbol *sym, t_int16 argc, t_atom *argv)
{
	TRACE("granular_add_grain");
	/*
	if (x->grains_cnt == x->grains_max) {
		MY_ERR("add_grain:  Impossible to add:  Maximum number of grains already reached.");
		return NULL; }

	if (argc != 4) {
		MY_ERR("add_grain:  Invalid arguments. The method expects:");
		MY_ERR2("  Arg 0:  Float - Amplitude");
		MY_ERR2("  Arg 1:  Float - Beginning");
		MY_ERR2("  Arg 2:  Float - Length");
		MY_ERR2("  Arg 3:  Float - Shift"); return NULL; }

	t_int32 src_begin = (t_int32)(atom_getfloat(argv + 1) * x->msamplerate);
	t_int32 src_len		= (t_int32)(atom_getfloat(argv + 2) * x->msamplerate);

	if (src_begin < 0) {
		MY_ERR("Impossible to add grain: beginning is out of bounds.");
		return NULL; }

	if (src_begin + src_len > x->buff_src_n_frm) {
		MY_ERR("Impossible to add grain: ending is out of bounds.");
		return NULL; }

	x->grains_cnt++;
	t_grain *grain		= (x->grains_arr + list_insert_first(x->grains_list));

	grain->index = -1;

	grain->ampl				= (t_double)atom_getfloat(argv);

	grain->src_begin	= src_begin;
	grain->src_len		= src_len;

	grain->out_begin	= 0;
	grain->out_len		= (t_int32)(grain->src_len / exp(LN2 * atom_getfloat(argv + 3)));

	grain->out_cntd		= grain->out_len;

	grain->src_I	= 0;
	grain->src_R	= 0;

	grain->env_I	= 0;
	grain->env_R	= 0;

	return grain;*/
	return NULL;
}

// ====  METHOD: GRANULAR_OUTPUT_GRAIN  ====

void granular_output_grain(t_granular *x)
{
	TRACE("granular_output_grain");
	/*
	// Create the grain to output
	// Note: removal will happen in granular_perform64 method
	t_grain *grain = granular_add_grain(x);
	if (grain == NULL) { return; }

	// Access and lock the source buffer
	x->buff_src_obj = buffer_ref_getobject(x->buff_src_ref);
	if (!x->buff_src_obj) { MY_ERR("HERE The source buffer \"%s\" does not seem to exit.", x->buff_src_sym->s_name); return; }
	float *buff_src = buffer_locksamples(x->buff_src_obj);

	// Access, resize and lock the output buffer
	x->buff_out_obj = buffer_ref_getobject(x->buff_out_ref);
	if (!x->buff_out_obj) { MY_ERR("HERE The output buffer \"%s\" does not seem to exit.", x->buff_out_sym->s_name); return; }
	
	max_err = object_method_long(x->buff_out_obj, gensym("sizeinsamps"), grain->out_len, NULL);
	if (max_err != MAX_ERR_NONE) { MY_ERR("Unable to reset size of output buffer \"%s\": Error %i", x->buff_out_sym->s_name, max_err); return; }	
	
	float *buff_out = buffer_locksamples(x->buff_out_obj);

	// Output the grain to the output buffer
	t_double mult				 = x->master * grain->ampl;
	t_int32	 src_len		 = grain->src_len - 1;
	t_int32	 env_len		 = x->env_n_frm - 1;
	t_int32  out_len		 = grain->out_len - 1;
	t_double inv_out_len = 1 / (t_double)out_len;

	t_int32 n = grain->out_len;
	t_int32 src_I = 0, src_R = 0, env_I = 0, env_R = 0, ind;
	
	// Calculate interpolated values from buffer and envelope
	while (n--) {
		ind = (grain->src_begin + src_I) * x->buff_src_n_chn;
		*buff_out++ += (float)(mult
			* (x->env_values[env_I] + env_R * inv_out_len * (x->env_values[env_I + 1] - x->env_values[env_I]))
			* (buff_src[ind] + src_R * inv_out_len * (buff_src[ind + x->buff_src_n_chn] - buff_src[ind])));

		// Iterate integer and fractional values
		src_R += src_len;
		while (src_R >= out_len) { src_R -= out_len; src_I++; }

		env_R += env_len;
		while (env_R >= out_len) { env_R -= out_len; env_I++; } }

	// Unlock the source and output buffers
	buffer_unlocksamples(x->buff_src_obj);
	buffer_unlocksamples(x->buff_out_obj);
	buffer_setdirty(x->buff_out_obj);*/
}

// ====  METHOD: GRANULAR_BANG  ====

void granular_bang(t_granular *x)
{
	TRACE("granular_bang");
}
