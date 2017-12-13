%module Rec
%include "std_string.i"
%include "std_vector.i"
%{
#include "rec.h"
#include <sstream>
#include <iomanip>
  %}
namespace std {
  %template(UCharVector) vector<unsigned char>;
  %template(IntVector) vector<int>;
  %template(FloatVector) vector<float>;
  %template(DoubleVector) vector<double>;
  %template(UIntVector) vector<unsigned int>;
  %extend vector<float> {
    int load(string data) {
      istringstream datastream;
      istringstream tmpstream;
      float ftmp;
      string stmp;

      datastream.clear();
      datastream.str(data);
      $self->clear();
      while(datastream >> stmp) {
        if(stmp=="Inf" || stmp=="inf" || stmp=="INF") {
	  $self->push_back(INFINITY);
	} else {
	  if(stmp=="-Inf" || stmp=="-inf" || stmp=="-INF") {
	    $self->push_back(-INFINITY);
          } else {
	    tmpstream.clear();
	    tmpstream.str(stmp);
	    tmpstream >> ftmp;
	    $self->push_back(ftmp);
          }
	}
      }
      return 0;
    }
    int affine(float a, float b=0) {
      unsigned int i;
      for(i=0; i<$self->size(); i++) {
        $self->at(i) *= a;
	$self->at(i) += b;
      }
      return 0;
    }
    float *datap() {
      return &self->front();
    }
  };
}
%include rec.h
