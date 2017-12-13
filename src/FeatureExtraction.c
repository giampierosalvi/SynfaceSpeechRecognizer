/***************************************************************************
                          FeatureExtraction.c  -  description
                             -------------------
    begin                : Sat Dec 21 2002
    copyright            : (C) 2002-2017 by Giampiero Salvi
    email                : giampi@kth.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***************************************************************************/

#include "FeatureExtraction.h"

/* just for debugging */
#include "Common.h"
#include <stdlib.h>

FeatureExtraction *FeatureExtraction_Create() {
  FeatureExtraction *fe;
  fe = (FeatureExtraction *) malloc(sizeof(FeatureExtraction));
  fe->speetures = NULL;

  /* copy defaul values */
  fe->inputrate = INPUTRATE;
  fe->numfilters = NUMFILTERS;
  fe->numceps = NUMCEPS;
  fe->lowfreq = LOWFREQ;
  fe->hifreq = HIFREQ;
  fe->framestep = FRAMESTEP;
  fe->framelen = FRAMEWIDTHSEC;
  fe->preemph = PREEMPH;
  fe->lifter = LIFTER;

  return fe;
}


int FeatureExtraction_Free(FeatureExtraction **feptr) {
  FeatureExtraction *fe = *feptr;

  if(fe == NULL) return 0;

  DBGPRINTF("freing Speetures...\n");
  FeatureExtraction_Deactivate(fe);
  free(fe);
  *feptr = NULL;

  return 0;
}

int FeatureExtraction_PushSpeech(FeatureExtraction *fe, short *speech, int num_samples) {
  PushSpeech(fe->speetures, speech, num_samples);
  return 0;
}

float* FeatureExtraction_PopFeatures(FeatureExtraction *fe) {
  return PopFeatures(fe->speetures);
}

/* these are used for starting and stopping, perhaps not complete */
int FeatureExtraction_Activate(FeatureExtraction *fe) {
  int base_type = MELCEP_FEATURES | ENERGY_FEATURES;

  if(fe->speetures != NULL) FeatureExtraction_Deactivate(fe);

  DBGPRINTF("initialize speetures...\n");
  DBGPRINTF("    base_type = %d\n",base_type);
  DBGPRINTF("    engine->num_filters = %d\n",fe->numfilters);
  DBGPRINTF("    used_size = %d\n",fe->numceps);
  DBGPRINTF("    smpRate = %f\n",(float) fe->inputrate);
  DBGPRINTF("    frame_length_sec = %f\n", fe->framestep);
  DBGPRINTF("    frame_width_sec = %f\n", fe->framelen);
  DBGPRINTF("    engine->low_cut_freq = %f\n", fe->lowfreq);
  DBGPRINTF("    engine->high_cut_freq = %f\n", fe->hifreq);
  fe->speetures = InitSpeetures( /* from Speetures.c */
	       base_type, /* base_type, */
	       fe->numfilters,    /* engine->num_filters, */
	       fe->numceps,    /* used_size, */
	       ADD_ZEROTH_CEP, /* add_flags, */ 
	       (float) fe->inputrate,  /* engine->samp_freq, */
	       fe->framestep,   /* engine->frame_length, (80 samp)*/
	       fe->framelen,    /* engine->frame_width, (200 samps)*/
	       fe->lowfreq,     /* engine->low_cut_freq, */ 
	       fe->hifreq); /* engine->high_cut_freq); */
  /* other speetures parameters... */
  fe->speetures->pre_emp_factor = fe->preemph; /* engine->pre_emp; */
  fe->speetures->cep_lift = fe->lifter; /* engine->cep_lift; */
  DBGPRINTF("number of features: %d\n",
	 fe->speetures->frame_size);
  DBGPRINTF("zeroth coeff. position: %d\n",
	 fe->speetures->energy_offset+1);

  fe->speetures->status = 1;

  return 0;
}

int FeatureExtraction_Deactivate(FeatureExtraction *fe) {
  if(fe->speetures != NULL) {
    SpeechEnd(fe->speetures);
    FreeSpeetures(fe->speetures);
    fe->speetures = NULL;
  }
  return 0;
}
