/***************************************************************************
                          FeatureExtraction.h  -  description
                             -------------------
    begin                : Tue Jan 7 2003
    copyright            : (C) 2003-2017 by Giampiero Salvi
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

#ifndef _FEATUREEXTRACTION_H_
#define _FEATUREEXTRACTION_H_

#include "Speetures.h"

/* these are default values */
#define INPUTRATE     8000
#define NUMFILTERS    24
#define NUMCEPS       12
#define LOWFREQ       0.0
#define HIFREQ        4000.0
#define FRAMESTEP     0.01
#define FRAMEWIDTHSEC 0.025
#define PREEMPH       0.97
#define LIFTER        22.0

typedef struct {
  int debug;
  int         stopped;     /* stop generation */
  int inputrate;
  int numfilters;
  int numceps;
  float lowfreq;
  float hifreq;
  float framestep;
  float framelen;
  float preemph;
  float lifter;
  Speetures *speetures;    /* feature extraction structure a poiter to
			      this is also given to SoundSource */
} FeatureExtraction;

FeatureExtraction *FeatureExtraction_Create();
int FeatureExtraction_Free(FeatureExtraction **feptr);

int FeatureExtraction_PushSpeech(FeatureExtraction *fe, short *speech, int num_samples);
float* FeatureExtraction_PopFeatures(FeatureExtraction *fe);

/* these are used for starting and stopping, perhaps not complete */
int FeatureExtraction_Activate(FeatureExtraction *fe);
int FeatureExtraction_Deactivate(FeatureExtraction *fe);

#endif /* _FEATUREEXTRACTION_H_ */
