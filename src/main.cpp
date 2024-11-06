#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "reciter.h"
#include "sam.h"

#ifdef USESDL
#	include <SDL.h>
#	include <SDL_audio.h>
#endif

#if defined __GNU_LIBRARY__ || defined __GLIBC__ || defined __APPLE__
static int min(int l, int r) {
	return l < r ? l : r;
}
static void strcat_s(char *dest, int size, char *str) {
	unsigned int dlen = strlen(dest);
	if (dlen >= size - 1) return;
	strncat(dest + dlen, str, size - dlen - 1);
}
void fopen_s(FILE **f, const char *filename, const char *mode) {
	*f = fopen(filename, mode);
}
#endif

void WriteWav(char *filename, const char *buffer, int bufferlength) {
	unsigned int filesize;
	unsigned int fmtlength			 = 16;
	unsigned short int format		 = 1;
	unsigned short int channels		 = 1;
	unsigned int samplerate			 = 22050;
	unsigned short int blockalign	 = 1;
	unsigned short int bitspersample = 8;

	FILE *file;
	fopen_s(&file, filename, "wb");
	if (file == NULL) return;
	//RIFF header
	fwrite("RIFF", 4, 1, file);
	filesize = bufferlength + 12 + 16 + 8 - 8;
	fwrite(&filesize, 4, 1, file);
	fwrite("WAVE", 4, 1, file);

	//format chunk
	fwrite("fmt ", 4, 1, file);
	fwrite(&fmtlength, 4, 1, file);
	fwrite(&format, 2, 1, file);
	fwrite(&channels, 2, 1, file);
	fwrite(&samplerate, 4, 1, file);
	fwrite(&samplerate, 4, 1, file); // bytes/second
	fwrite(&blockalign, 2, 1, file);
	fwrite(&bitspersample, 2, 1, file);

	//data chunk
	fwrite("data", 4, 1, file);
	fwrite(&bufferlength, 4, 1, file);
	fwrite(buffer, bufferlength, 1, file);

	fclose(file);
}

void printUsage() {
	printf("usage: sam [options] Word1 Word2 ....\n");
	printf("options\n");
	printf("	-phonetic 		enters phonetic mode. (see below)\n");
	printf("	-pitch number		set pitch value (default=64)\n");
	printf("	-speed number		set speed value (default=72)\n");
	printf("	-throat number		set throat value (default=128)\n");
	printf("	-mouth number		set mouth value (default=128)\n");
	printf("	-wav filename		output to wav instead of libsdl\n");
	printf("	-sing			special treatment of pitch\n");
	printf("\n");

	printf("     VOWELS                            VOICED CONSONANTS	\n");
	printf("IY           f(ee)t                    R        red		\n");
	printf("IH           p(i)n                     L        allow		\n");
	printf("EH           beg                       W        away		\n");
	printf("AE           Sam                       W        whale		\n");
	printf("AA           pot                       Y        you		\n");
	printf("AH           b(u)dget                  M        Sam		\n");
	printf("AO           t(al)k                    N        man		\n");
	printf("OH           cone                      NX       so(ng)		\n");
	printf("UH           book                      B        bad		\n");
	printf("UX           l(oo)t                    D        dog		\n");
	printf("ER           bird                      G        again		\n");
	printf("AX           gall(o)n                  J        judge		\n");
	printf("IX           dig(i)t                   Z        zoo		\n");
	printf("				       ZH       plea(s)ure	\n");
	printf("   DIPHTHONGS                          V        seven		\n");
	printf("EY           m(a)de                    DH       (th)en		\n");
	printf("AY           h(igh)						\n");
	printf("OY           boy						\n");
	printf("AW           h(ow)                     UNVOICED CONSONANTS	\n");
	printf("OW           slow                      S         Sam		\n");
	printf("UW           crew                      Sh        fish		\n");
	printf("                                       F         fish		\n");
	printf("                                       TH        thin		\n");
	printf(" SPECIAL PHONEMES                      P         poke		\n");
	printf("UL           sett(le) (=AXL)           T         talk		\n");
	printf("UM           astron(omy) (=AXM)        K         cake		\n");
	printf("UN           functi(on) (=AXN)         CH        speech		\n");
	printf("Q            kitt-en (glottal stop)    /H        a(h)ead	\n");
}

#ifdef USESDL

int pos = 0;
void MixAudio(void *unused, Uint8 *stream, int len) {
	int bufferpos = GetBufferLength();
	char *buffer  = GetBuffer();
	int i;
	if (pos >= bufferpos) return;
	if ((bufferpos - pos) < len) len = (bufferpos - pos);
	for (i = 0; i < len; i++) {
		stream[i] = buffer[pos];
		pos++;
	}
}

void OutputSound() {
	int bufferpos = GetBufferLength();
	bufferpos /= 50;
	SDL_AudioSpec fmt;

	fmt.freq	 = 22050;
	fmt.format	 = AUDIO_U8;
	fmt.channels = 1;
	fmt.samples	 = 2048;
	fmt.callback = MixAudio;
	fmt.userdata = NULL;

	/* Open the audio device and start playing sound! */
	if (SDL_OpenAudio(&fmt, NULL) < 0) {
		printf("Unable to open audio: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_PauseAudio(0);
	//SDL_Delay((bufferpos)/7);

	while (pos < bufferpos) {
		SDL_Delay(100);
	}

	SDL_CloseAudio();
}

#else

void OutputSound() {
}

#endif

int main(int argc, char **argv) {
	int i;
	int phonetic = 0;

	char *wavfilename = NULL;

	static constexpr auto inputLength = 256;
	unsigned char input[inputLength];

	memset(input, 0, inputLength);

	Sam sam;

	if (argc <= 1) {
		printUsage();
		return 1;
	}

	i = 1;
	while (i < argc) {
		if (argv[i][0] != '-') {
			strcat_s((char *) input, inputLength, argv[i]);
			strcat_s((char *) input, inputLength, " ");
		} else {
			if (strcmp(&argv[i][1], "wav") == 0) {
				wavfilename = argv[i + 1];
				i++;
			} else if (strcmp(&argv[i][1], "sing") == 0) {
				EnableSingmode();
				sam.setMode(Sam::Mode::Singing);
			} else if (strcmp(&argv[i][1], "phonetic") == 0) {
				phonetic = 1;
			} else if (strcmp(&argv[i][1], "pitch") == 0) {
				auto p = static_cast<uint8_t>(min(atoi(argv[i + 1]), 255));
				SetPitch(p);
				sam.setPitch(p);
				i++;
			} else if (strcmp(&argv[i][1], "speed") == 0) {
				auto s = static_cast<uint8_t>(min(atoi(argv[i + 1]), 255));
				SetSpeed(s);
				sam.setSpeed(s);
				i++;
			} else if (strcmp(&argv[i][1], "mouth") == 0) {
				auto m = static_cast<uint8_t>(min(atoi(argv[i + 1]), 255));
				SetMouth(m);
				sam.setMouth(m);
				i++;
			} else if (strcmp(&argv[i][1], "throat") == 0) {
				auto t = static_cast<uint8_t>(min(atoi(argv[i + 1]), 255));
				SetThroat(t);
				sam.setThroat(t);
				i++;
			} else {
				printUsage();
				return 1;
			}
		}

		i++;
	} //while

	for (i = 0; input[i] != 0; i++)
		input[i] = (unsigned char) toupper((int) input[i]);

	if (!phonetic) {
		strcat_s((char *) input, inputLength, "[");

		if (!Reciter {}.textToPhonemes(input)) return 1;
	} else strcat_s((char *) input, inputLength, "\x9b");

#ifdef USESDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
#endif

	SetInput(input);
	sam.setInput({reinterpret_cast<const char *>(input), inputLength});

	if (!SAMMain()) {
		printUsage();
		return 1;
	}

	if (!sam.process()) {
		printUsage();
		return 1;
	}

	if (wavfilename != NULL) {
		WriteWav(wavfilename, sam.getBuffer().data(), GetBufferLength() / 50);
		//		WriteWav(wavfilename, GetBuffer(), GetBufferLength() / 50);
	} else {
		OutputSound();
	}

	return 0;
}
