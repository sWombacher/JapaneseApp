%module cppshared

%include "std_vector.i"
%include "std_string.i"
%include "std_wstring.i"
%include "stdint.i"

%template(WStringVector) std::vector<std::wstring>;

%{
#include "sharedlogic.h"
%}

%include "sharedlogic.h"
