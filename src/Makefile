NICOLIB = ../ext/nico-1.1.2/lib
#NICOLIB = ../ext/nico
CC	= gcc
OPT	= -O -ffast-math 
CFLAGS	= -g $(OPT) -Wall -I$(NICOLIB) -D DEBUG -fno-stack-protector
CFLAGS	= $(OPT) -Wall -I$(NICOLIB) -fno-stack-protector
LDFLAGS = -L/usr/lib/x86_64-linux-gnu/ -L$(NICOLIB)
LDLIBS = -lportaudio -lnico -lm

modules = Audio.o BinaryBuffer.o Common.o FeatureExtraction.o LikelihoodGen.o \
	nrg.o RecMath.o Recognizer.o RingBuffer.o SoundProc.o \
	SoundIO.o ViterbiDecoder.o

tests = test_playback test_portaudio test_recognizer_live test_recognizer_offline

all: librec.a $(tests)

librec.a: $(modules) 
	ar rv librec.a $(modules)

test_recognizer_live: test_recognizer_live.o $(modules) 
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_recognizer_offline: test_recognizer_offline.o $(modules)
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_playback: test_playback.o $(modules)
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_portaudio: test_portaudio.o $(modules)
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o
	rm -f librec.a
	rm -f $(tests)

