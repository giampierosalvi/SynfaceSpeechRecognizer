/***************************************************************************
                          test_audio.c  -  description
                             -------------------
    begin                : Mon Dec 10 2012
    copyright            : (C) 2012-2017 by Giampiero Salvi
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

#include "SoundSource.h"
#include <stdio.h>

#define SMPRATE 48000

int TestCallback(void *dummy, short *speech, int num_samples) {
  return 0;
}

void mypause() {
  printf("Press [Enter] to continue . . .");
  fflush(stdout);
  getchar();

}

int main(int argc, char **argv) {
  int err;
  SoundSource *s = SoundSource_Create();

  // setting sound source callback
  err = SoundSource_SetCallback(s, (SoundSourceCallbackProc *) &TestCallback, NULL);
  if(err != 0) {
    fprintf(stderr, "failed setting callback\n");
    return err;
  }

  err = SoundSource_Open(s, NULL, 0); //char *device, SMPRATE);
  if(err != 0) {
    fprintf(stderr, "failed opening device\n");
    return err;
  }

  err = SoundSource_Start(s);
  if(err != 0) {
    fprintf(stderr, "failed starting stream\n");
    return err;
  }

  mypause();

  printf("Setting playback delay to half a second...\n");
  SoundSource_Stop(s);
  SoundSource_SetFBDelay(s, 0.5);
  SoundSource_Start(s);

  mypause();

  printf("Setting playback delay to one second...\n");
  SoundSource_Stop(s);
  SoundSource_SetFBDelay(s, 1.0);
  SoundSource_Start(s);

  mypause();

  printf("Setting playback delay back to zero...\n");
  SoundSource_Stop(s);
  SoundSource_SetFBDelay(s, 0.0);
  SoundSource_Start(s);

  mypause();

  SoundSource_Stop(s);
  SoundSource_Close(s);
  // cleaning up
  SoundSource_Free(&s);

  return 0;
}
