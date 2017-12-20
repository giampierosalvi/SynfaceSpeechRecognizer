/***************************************************************************
                         Recognizer.h  -  description
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
#include "SoundSource.h"
#include "SoundProc.h"
#include "FeatureExtraction.h"
#include "LikelihoodGen.h"
#include "ViterbiDecoder.h"

int Configuration_ApplyConfigFile(char *filename, Recognizer *r);


#endif /* _CONFIGURATION_H_ */
