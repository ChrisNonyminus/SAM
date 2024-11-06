#include "WavWriter.h"

void WavWriter::write(const std::string &filename, const char *buffer, int bufferLength) {
	FILE *file = open(filename);
	writeRiffHeader(file, bufferLength);
	writeFormat(file);
	writeData(file, buffer, bufferLength);
	close(file);
}

FILE *WavWriter::open(const std::string &filename) {
	auto file = fopen(filename.c_str(), "wb");
	if (file == nullptr) {
		throw std::invalid_argument {"Cant create file at " + filename};
	}
	return file;
}

void WavWriter::close(FILE *file) {
	if (file != nullptr) {
		fclose(file);
	}
}

void WavWriter::writeRiffHeader(FILE *file, int bufferLength) {
	fwrite("RIFF", 4, 1, file);
	auto filesize = bufferLength + 12 + 16 + 8 - 8;
	fwrite(&filesize, 4, 1, file);
	fwrite("WAVE", 4, 1, file);
}

void WavWriter::writeFormat(FILE *file) {
	static constexpr unsigned int formatLength		  = 16;
	static constexpr unsigned short int format		  = 1;
	static constexpr unsigned short int channels	  = 1;
	static constexpr unsigned int sampleRate		  = 22050;
	static constexpr unsigned int bytesPerSecond	  = 22050;
	static constexpr unsigned short int blockAlign	  = 1;
	static constexpr unsigned short int bitsPerSample = 8;

	fwrite("fmt ", 4, 1, file);
	fwrite(&formatLength, 4, 1, file);
	fwrite(&format, 2, 1, file);
	fwrite(&channels, 2, 1, file);
	fwrite(&sampleRate, 4, 1, file);
	fwrite(&bytesPerSecond, 4, 1, file);
	fwrite(&blockAlign, 2, 1, file);
	fwrite(&bitsPerSample, 2, 1, file);
}

void WavWriter::writeData(FILE *file, const char *buffer, int bufferLength) {
	fwrite("data", 4, 1, file);
	fwrite(&bufferLength, 4, 1, file);
	fwrite(buffer, bufferLength, 1, file);
}
