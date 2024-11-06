#include "SDLOutput.h"

#ifdef USESDL
#	include <SDL.h>
#	include <SDL_audio.h>

int pos = 0;
char *sdlOutBuffer {nullptr};
size sdlOutLenght {0};
void MixAudio(void *unused, Uint8 *stream, int len) {
	auto bufferpos = sdlOutLenght;
	auto buffer	   = sdlOutBuffer;

	if (pos >= bufferpos) {
		return;
	}

	if ((bufferpos - pos) < len) {
		len = (bufferpos - pos);
	}

	for (int i = 0; i < len; i++) {
		stream[i] = buffer[pos];
		pos++;
	}
}

#endif

SDLOutput::SDLOutput() {
#ifdef USESDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		printf("Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);
#endif
}

void SDLOutput::output(const char *buffer, size_t bufferLength) {
#ifdef USESDL
	sdlOutLenght = bufferLength;
	sdlOutBuffer = buffer;

	auto bufferpos = bufferLength;

	SDL_AudioSpec fmt;
	fmt.freq	 = 22050;
	fmt.format	 = AUDIO_U8;
	fmt.channels = 1;
	fmt.samples	 = 2048;
	fmt.callback = MixAudio;
	fmt.userdata = NULL;

	if (SDL_OpenAudio(&fmt, NULL) < 0) {
		printf("Unable to open audio: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_PauseAudio(0);

	while (pos < bufferpos) {
		SDL_Delay(100);
	}

	SDL_CloseAudio();
#endif
}