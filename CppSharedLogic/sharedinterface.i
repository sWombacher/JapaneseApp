%module cppshared

%include "std_vector.i"
%include "std_string.i"

%{
#include "sharedlogic.h"
%}

%include "sharedlogic.h"
