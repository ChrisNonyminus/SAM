#pragma once

#include <vector>

class SampleConvertor {
public:
	SampleConvertor()  = default;
	~SampleConvertor() = default;

	[[nodiscard]] std::vector<float>
		convert(const uint8_t *inputBuffer, size_t inputSize, unsigned int targetSampleRate);

private:
	[[nodiscard]] std::vector<float> convertUint8ToFloat(const uint8_t *inputBuffer, size_t inputSize);
	[[nodiscard]] std::vector<float>
		resample(const std::vector<float> &input, unsigned int originalSampleRate, unsigned int targetSampleRate);
};
