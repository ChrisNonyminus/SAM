#pragma once

#include <stdio.h>
#include <string>

class WavWriter {
public:
	void write(const std::string &filename, const char *buffer, int bufferLength);

private:
	FILE *open(const std::string &filename);
	void close(FILE *file);
	void writeRiffHeader(FILE *file, int bufferLength);
	void writeFormat(FILE *file);
	void writeData(FILE *file, const char *buffer, int bufferLength);
};
