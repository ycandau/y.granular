#include "max_util.h"

// ====  PROCEDURE: MESS_SYM_LONG  ====
// Send a message composed of one symbol and one long integer

void mess_sym_long(void* outlet, t_symbol* sym, t_atom_long l, t_atom* atoms) {

  atom_setsym(atoms, sym);
  atom_setlong(atoms + 1, l);
  outlet_list(outlet, NULL, 2, atoms);
}

// ====  PROCEDURE: MESS_SYM_LONGS  ====
// Send a message composed of one symbol and n long integers

void mess_sym_longs(void* outlet, t_symbol* sym, t_uint32 n, t_atom_long* l, t_atom* atoms) {

  atom_setsym(atoms, sym);
  for (t_uint32 i = 0; i < n; i++) { atom_setlong(atoms + i + 1, l[i]); }
  outlet_list(outlet, NULL, n+1, atoms);
}

// ====  PROCEDURE: MESS_SYM_FLOAT  ====
// Sends a message composed of one symbol and one float

void mess_sym_float(void* outlet, t_symbol* sym, t_float f, t_atom* atoms) {

  atom_setsym(atoms, sym);
  atom_setfloat(atoms + 1, f);
  outlet_list(outlet, NULL, 2, atoms);
}

// ====  PROCEDURE: MESS_SYM_FLOATS  ====
// Send a message composed of one symbol and n floats

void mess_sym_floats(void* outlet, t_symbol* sym, t_uint32 n, t_float* f, t_atom* atoms) {

  atom_setsym(atoms, sym);
  for (t_uint32 i = 0; i < n; i++) { atom_setfloat(atoms + i + 1, f[i]); }
  outlet_list(outlet, NULL, n + 1, atoms);
}

// ====  PROCEDURE: MESS_SYM_DOUBLE  ====
// Sends a message composed of one symbol and one double

void mess_sym_double(void* outlet, t_symbol* sym, t_double d, t_atom* atoms) {

  atom_setsym(atoms, sym);
  atom_setfloat(atoms + 1, d);
  outlet_list(outlet, NULL, 2, atoms);
}

// ====  PROCEDURE: MESS_SYM_DOUBLES  ====
// Send a message composed of one symbol and n floats

void mess_sym_doubles(void* outlet, t_symbol* sym, t_uint32 n, t_double* d, t_atom* atoms) {

  atom_setsym(atoms, sym);
  for (t_uint32 i = 0; i < n; i++) { atom_setfloat(atoms + i + 1, d[i]); }
  outlet_list(outlet, NULL, n + 1, atoms);
}

// ====  PROCEDURE: MESS_SYM_SYM  ====
// Sends a message composed of one symbol and one float

void mess_sym_sym(void* outlet, t_symbol* sym, t_symbol* sym2, t_atom* atoms) {

  atom_setsym(atoms, sym);
  atom_setsym(atoms + 1, sym2);
  outlet_list(outlet, NULL, 2, atoms);
}

  // ====  PROCEDURE: MESS_STRING  ====

void mess_string(void* outlet, char* str, t_atom* atoms) {

  atom_setsym(atoms, gensym(str));
  outlet_list(outlet, NULL, 1, atoms);
}

  // ====  PROCEDURE: MESS_SYM_STRING  ====

void mess_sym_string(void* outlet, t_symbol* sym, char* str, t_atom* atoms) {

  atom_setsym(atoms, sym);
  atom_setsym(atoms + 1, gensym(str));
  outlet_list(outlet, NULL, 2, atoms);
}
