/***************************************************************************
                         Configuration.h  -  description
                             -------------------
    begin                : Tue May 13 2003
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

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include "Recognizer.h"
#include "SoundIO.h"
#include "SoundProc.h"
#include "FeatureExtraction.h"
#include "LikelihoodGen.h"
#include "ViterbiDecoder.h"

/* standard model file names: each configuration directory is
   expected to contain the following files */
#define PARAMETERS_FN   "parameters.conf"
#define RNN_FN          "rnn.rtd"
#define PHONE_PRIOR_FN  "phone_prior.txt"
#define HMM_MAP_FN      "hmm_map.txt"
#define HMM_PRIOR_FN    "hmm_prior.txt"
#define HMM_TRANSMAT_FN "hmm_transmat.txt"

int Configuration_ApplyConfig(char *dirname, Recognizer *r);


#endif /* _CONFIGURATION_H_ */
