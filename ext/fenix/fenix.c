#include "fenix.h"

VALUE mFenix;

void Init_fenix()
{
	mFenix = rb_define_module("Fenix");

	Init_fenix_file();
}
