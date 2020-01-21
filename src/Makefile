NICOLIB = ../ext/nico-1.1.2/lib
#NICOLIB = ../ext/nico
CC	= gcc
# CC	= clang
OPT	= -O -ffast-math
ASANFLAGS = -fsanitize=address -O0 -fno-omit-frame-pointer
CFLAGSDEBUG	= -g $(OPT) -Wall -I$(NICOLIB) -D DEBUG -fno-stack-protector
CFLAGSNODEBUG	= -g $(OPT) -Wall -I$(NICOLIB) -D DEBUG -fno-stack-protector
CFLAGS	= -g $(OPT) -Wall -I$(NICOLIB) -D DEBUG -fno-stack-protector $(ASANFLAGS)
LDFLAGS = -L/usr/lib/x86_64-linux-gnu/ -L$(NICOLIB)
LDLIBS = -lportaudio -lnico -lm

modules = Audio.o BinaryBuffer.o Common.o Configuration.o FeatureExtraction.o LikelihoodGen.o \
	nrg.o RecMath.o Recognizer.o RingBuffer.o SoundProc.o \
	SoundIO.o ViterbiDecoder.o

tests = test_playback test_portaudio test_recognizer_live test_recognizer_offline

debug: CFLAGS += -DDEBUG -g
debug: test_recognizer_offline test_recognizer_live

all: librec.a $(tests)

librec.a: $(modules) 
	ar rv librec.a $(modules)

test_recognizer_live: test_recognizer_live.o $(modules) 
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_recognizer_offline: test_recognizer_offline.o $(modules)
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS) -lsndfile

test_playback: test_playback.o $(modules)
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_portaudio: test_portaudio.o $(modules)
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o
	rm -f librec.a
	rm -f $(tests)

# this is what to do to get symbols from AddressSanitizer:
# export ASAN_SYMBOLIZER_PATH=/usr/lib/llvm-3.8/bin/llvm-symbolizer
# ASAN_OPTIONS=symbolize=1 ./test_recognizer_live
