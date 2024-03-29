/***************************************************************************
                          test_playback.c  -  description
                             -------------------
    begin                : Mon Dec 10 2012
    copyright            : (C) 2012-2023 by Giampiero Salvi
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

#include "SoundIO.h"
#include <stdio.h>

#define SMPRATE 48000

int TestCallback(void *dummy, short *speech, int num_samples) {
  return 0;
}

void waitForEnter() {
  printf("Press [Enter] for next step...");
  fflush(stdout);
  getchar();

}

int main(int argc, char **argv) {
  int err;
  SoundIO *s = SoundIO_Create();

  fprintf(stderr, " --------- AUDIO PLAYBACK TEST -------------\n");
  fprintf(stderr, " - Run this test with headphones to avoid  -\n");
  fprintf(stderr, " - feedback noise and clipping             -\n");
  fprintf(stderr, " -------------------------------------------\n");

  waitForEnter();
  fprintf(stderr, " --> Setting up audio callback -------------\n");
  // setting sound source callback
  err = SoundIO_SetCallback(s, (SoundIOCallbackProc *) &TestCallback, NULL);
  if(err != 0) {
    fprintf(stderr, "failed setting callback\n");
    return err;
  }

  fprintf(stderr, " --> Opening audio stream with default values\n");
  err = SoundIO_Open(s, NULL, 0); //char *device, SMPRATE);
  if(err != 0) {
    fprintf(stderr, "failed opening device\n");
    return err;
  }

  fprintf(stderr, " --> Starting audio stream -----------------\n");
  err = SoundIO_Start(s);
  if(err != 0) {
    fprintf(stderr, "failed starting stream\n");
    return err;
  }
  fprintf(stderr, " --> You should hear your voice with minimal delay\n");

  waitForEnter();

  printf(" --> Setting playback delay to half a second...\n");
  SoundIO_Stop(s);
  SoundIO_SetPlaybackDelay(s, 0.5);
  SoundIO_Start(s);
  fprintf(stderr, " --> Now, you should hear your voice with some delay\n");

  waitForEnter();

  printf(" --> Setting playback delay to one second...\n");
  SoundIO_Stop(s);
  SoundIO_SetPlaybackDelay(s, 1.0);
  SoundIO_Start(s);
  fprintf(stderr, " --> Now, you should hear your voice with more delay\n");

  waitForEnter();

  printf(" --> Setting playback delay back to zero...\n");
  SoundIO_Stop(s);
  SoundIO_SetPlaybackDelay(s, 0.0);
  SoundIO_Start(s);
  fprintf(stderr, " --> Now, you should hear your voice with minimal delay, again\n");

  waitForEnter();

  fprintf(stderr, " --> Closing down\n");
  SoundIO_Stop(s);
  SoundIO_Close(s);
  // cleaning up
  SoundIO_Free(&s);

  fprintf(stderr, " --------- AUDIO PLAYBACK TEST ENDED --------\n");

  return 0;
}
