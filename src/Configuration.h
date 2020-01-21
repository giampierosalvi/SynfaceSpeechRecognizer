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

/* standard model file names: each configuration directory is
   expected to contain the following files */
#define PARAMETERS_FN   "parameters.conf"
#define RNN_FN          "rnn.rtd"
#define PHONE_PRIOR_FN  "phone_prior.txt"
#define HMM_MAP_FN      "hmm_map.txt"
#define HMM_PRIOR_FN    "hmm_prior.txt"
#define HMM_TRANSMAT_FN "hmm_transmat.txt"

/* definitions that allow to run a switch on strings using a hash function.
 See generate_config_hash.c for more details */
#define CONF_LG_ANN        229392308683868     /* -lg:ann */
#define CONF_LG_PHPRIOR   -4660004979818057853 /* -lg:phprior */
#define CONF_DECODER       7569935825667912    /* -decoder */
#define CONF_FE_INPUTRATE -2197128137810322605 /* -fe:inputrate */
#define CONF_FE_NUMFILTERS 1281989501715611648 /* -fe:numfilters */
#define CONF_FE_NUMCEPS   -4660286263619983022 /* -fe:numceps */
#define CONF_FE_LOWFREQ   -4660286266425750505 /* -fe:lowfreq */
#define CONF_FE_HIFREQ     8243662873751427158 /* -fe:hifreq */
#define CONF_FE_FRAMESTEP -2197132206265240770 /* -fe:framestep */
#define CONF_FE_FRAMELEN  -6215494455062744735 /* -fe:framelen */
#define CONF_FE_PREEMPH   -4660286261163860056 /* -fe:preemph */
#define CONF_FE_LIFTER     8243662873907970909 /* -fe:lifter */
#define CONF_VT_HMM        229392715462360     /* -vt:hmm */
#define CONF_VT_LOOKAHEAD -1365476488673139138 /* -vt:lookahead */
#define CONF_VT_GRAMFACT  -6190292976414017381 /* -vt:gramfact */
#define CONF_VT_INSWEIGHT -1365480744877158200 /* -vt:insweight */


int Configuration_ApplyConfigFromFilename(Recognizer *r, char *filename);


#endif /* _CONFIGURATION_H_ */
