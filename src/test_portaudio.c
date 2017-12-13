/***************************************************************************
                          test_portaudio.c  -  description
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <portaudio.h>
//#include <mcheck.h>

#define NCHANNELS 1

int paCallback( const void *inputBuffer, void *outputBuffer,
		unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo,
		PaStreamCallbackFlags statusFlags,
		void *userData ) {

  if( inputBuffer == NULL ) return 0;

  memcpy(outputBuffer, inputBuffer, framesPerBuffer*sizeof(short));

  return 0;
}

int open_stream(PaStream **streamptr) {
  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;
  const PaStreamInfo *stream_info;
  const PaDeviceInfo *dev_info;
  PaStream *stream;
  PaError err;
  int dev;

  printf("setting up input device\n");
  dev = Pa_GetDefaultInputDevice();
  dev_info = Pa_GetDeviceInfo(dev);
  inputParameters.device = dev;
  if(dev_info != NULL)
    inputParameters.suggestedLatency = dev_info->defaultLowInputLatency;
  else {
    inputParameters.suggestedLatency = 0.0;
    printf("no device info, setting latency to 0.0\n");
  }
  inputParameters.sampleFormat = paInt16;
  inputParameters.channelCount = NCHANNELS;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  printf("setting up output device\n");
  dev = Pa_GetDefaultOutputDevice();
  dev_info = Pa_GetDeviceInfo(dev);
  outputParameters.device = dev;
  if(dev_info != NULL)
    outputParameters.suggestedLatency = dev_info->defaultLowOutputLatency;
  else {
    outputParameters.suggestedLatency = 0.0;
    printf("no device info, setting latency to 0.0\n");
  }
  outputParameters.sampleFormat = paInt16;
  outputParameters.channelCount = NCHANNELS;
  outputParameters.hostApiSpecificStreamInfo = NULL;

  if ((err = Pa_IsFormatSupported(&inputParameters, &outputParameters, 48000.0))) {
    printf("Device cannot be opened with current parameters because:\n");
    printf("%s\n", Pa_GetErrorText(err));
    return err;
  }

  err = Pa_OpenStream(
		      streamptr,
		      &inputParameters,
		      &outputParameters,
		      (double) 48000,
		      (int) 960,//paFramesPerBufferUnspecified,
		      paClipOff, /* we won't output out of range samples so don't bother clipping them */
		      paCallback,
		      NULL );
  if(err != paNoError) {
    printf("could not open portaudio stream because:\n");
    printf("%s\n", Pa_GetErrorText(err));
    return err;
  }
  stream = *streamptr;
  err = Pa_IsStreamActive(stream);
  if(err != paNoError) {
    printf("something went wrong:\n");
    printf("%s\n", Pa_GetErrorText(err));
    return err;
  }

  stream_info = Pa_GetStreamInfo(stream);
  if(stream_info != NULL) {
    printf("streamInfo.structVersion: %d\n", stream_info->structVersion);
    printf("streamInfo.inputLatency: %f\n", stream_info->inputLatency);
    printf("streamInfo.outputLatency: %f\n", stream_info->outputLatency);
    printf("streamInfo.sampleRate: %f\n", stream_info->sampleRate);
  } else {
    printf("streamInfo is null!!\n");
  }

  return 0;
}

int close_stream(PaStream *stream) {
  PaError err;

  if(Pa_IsStreamActive(stream) == paNoError) { /* valid stream */
    err = Pa_CloseStream(stream);
    if(err != paNoError) {
      printf("could not close portaudio stream because:\n");
      printf("%s\n", Pa_GetErrorText(err));
      return err;
    }
  }

  return 0;
}

int start_stream(PaStream *stream) {
  PaError err;

  err = Pa_StartStream(stream);
  if(err != paNoError) {
    printf("could not start stream because:\n");
    printf("%s\n", Pa_GetErrorText(err));
    return err;
  }

  return 0;
}

int stop_stream(PaStream *stream) {
  PaError err;

  err = Pa_StopStream(stream);
  if(err != paNoError) {
    printf("could not stop portaudio stream\n");
    return err;
  }

  return 0;
}

int main(int argc, char **argv) {
  PaStream *stream;
  int i;

  //mtrace();

  printf("initializing PortAudio...\n");
  Pa_Initialize();

  for(i=0;i<10;i++) {
    printf("opening stream...\n");
    open_stream(&stream);
    printf("closing stream...\n");
    close_stream(stream);
  }

  for(i=0;i<5;i++) {
    printf("opening stream...\n");
    open_stream(&stream);
    printf("starting stream...\n");
    start_stream(stream);
    printf("waiting 5 seconds...\n");
    //sleep(5);
    printf("stopping stream...\n");
    stop_stream(stream);
    printf("closing stream...\n");
    close_stream(stream);
  }

  printf("terminating PortAudio...\n");
  Pa_Terminate();

  //muntrace();

  return 0;
}
