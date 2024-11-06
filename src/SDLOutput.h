#pragma once

#include <cstdint>
#include <cstdio>

class SDLOutput {
public:
	SDLOutput();
	~SDLOutput() = default;

	void output(const char *buffer, size_t bufferLength);

private:
};
