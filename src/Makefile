CC	= gcc
OPT	= -O -ffast-math 
#CFLAGS	= $(OPT) -Wall -I../ext/nico-1.1.2/lib/
CFLAGS	= $(OPT) -Wall -I../ext/nico/
LDFLAGS = -L/usr/lib/x86_64-linux-gnu/ -L../ext/nico/
LDLIBS = -lportaudio -lnico -lm

modules = Audio.o Common.o FeatureExtraction.o Feedback.o LikelihoodGen.o \
	nrg.o RecMath.o Recognizer.o RingBuffer.o SoundProc.o \
	SoundSource_pa.o ViterbiDecoder.o

tests = test_audio.o test_portaudio.o test_recognizer2.o test_recognizer.o

all: librec.a

librec.a: $(modules) 
	ar rv librec.a $(modules)

test_recognizer_live: test_recognizer_live.o
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_recognizer_offline: test_recognizer_offline.o
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_audio: test_audio.o
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

test_portaudio: test_portaudio.o
	$(CC) $(OPT) $(CFLAGS) $(modules) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o
	rm -f librec.a


