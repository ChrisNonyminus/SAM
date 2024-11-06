#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <algorithm>

#include "reciter.h"
#include "Sam.h"
#include "WavWriter.h"
#include "SDLOutput.h"

#if defined __GNU_LIBRARY__ || defined __GLIBC__ || defined __APPLE__
static void strcat_s(char *dest, int size, char *str) {
	unsigned int dlen = strlen(dest);
	if (dlen >= size - 1) return;
	strncat(dest + dlen, str, size - dlen - 1);
}

#endif

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
				//				EnableSingmode();
				sam.setMode(Sam::Mode::Singing);
			} else if (strcmp(&argv[i][1], "phonetic") == 0) {
				phonetic = 1;
			} else if (strcmp(&argv[i][1], "pitch") == 0) {
				auto p = static_cast<uint8_t>(std::min(atoi(argv[i + 1]), 255));
				//				SetPitch(p);
				sam.setPitch(p);
				i++;
			} else if (strcmp(&argv[i][1], "speed") == 0) {
				auto s = static_cast<uint8_t>(std::min(atoi(argv[i + 1]), 255));
				//				SetSpeed(s);
				sam.setSpeed(s);
				i++;
			} else if (strcmp(&argv[i][1], "mouth") == 0) {
				auto m = static_cast<uint8_t>(std::min(atoi(argv[i + 1]), 255));
				//				SetMouth(m);
				sam.setMouth(m);
				i++;
			} else if (strcmp(&argv[i][1], "throat") == 0) {
				auto t = static_cast<uint8_t>(std::min(atoi(argv[i + 1]), 255));
				//				SetThroat(t);
				sam.setThroat(t);
				i++;
			} else {
				printUsage();
				return 1;
			}
		}

		i++;
	} //while

	std::string inputStr {reinterpret_cast<const char *>(input), inputLength};
	std::transform(inputStr.begin(), inputStr.end(), inputStr.begin(), ::toupper);

	for (i = 0; input[i] != 0; i++)
		input[i] = (unsigned char) toupper((int) input[i]);

	if (!phonetic) {
		strcat_s((char *) input, inputLength, "[");

		if (!Reciter {}.textToPhonemes(input)) {
			return 1;
		}
	} else {
		strcat_s((char *) input, inputLength, "\x9b");
	}

	if (!sam.process({reinterpret_cast<const char *>(input), inputLength})) {
		printUsage();
		return 1;
	}

	if (wavfilename != nullptr) {
		WavWriter {}.write(wavfilename, sam.getBuffer().data(), sam.getBufferSize());
	} else {
		SDLOutput {}.output(sam.getBuffer().data(), sam.getBufferSize());
	}

	return 0;
}
