#include "SampleConvertor.h"

std::vector<float>
	SampleConvertor::convert(const char *inputBuffer, size_t inputSize, unsigned int targetSampleRate) {
	static constexpr auto originalSampleRate = 22050;

	auto floatBuffer	 = convertUint8ToFloat(inputBuffer, inputSize);
	auto resampledBuffer = resample(floatBuffer, originalSampleRate, targetSampleRate);

	return resampledBuffer;
}

std::vector<float> SampleConvertor::convertUint8ToFloat(const char *inputBuffer, size_t inputSize) {
	std::vector<float> floatBuffer;
	floatBuffer.reserve(inputSize);
	for (size_t i = 0; i < inputSize; ++i) {
		float sample = (static_cast<float>(inputBuffer[i]) - 128.0f) / 128.0f;
		floatBuffer.push_back(sample);
	}
	return floatBuffer;
}

std::vector<float> SampleConvertor::resample(const std::vector<float> &input,
											 unsigned int originalSampleRate,
											 unsigned int targetSampleRate) {
	if (originalSampleRate == targetSampleRate) {
		return input;
	}

	double ratio	  = static_cast<double>(targetSampleRate) / originalSampleRate;
	size_t outputSize = static_cast<size_t>(input.size() * ratio);

	std::vector<float> output;
	output.reserve(outputSize);

	for (size_t i = 0; i < outputSize; ++i) {
		double inputIndex = static_cast<double>(i) / ratio;
		size_t index	  = static_cast<size_t>(std::floor(inputIndex));
		double frac		  = inputIndex - index;

		if (index + 1 < input.size()) {
			output.push_back(static_cast<float>((1.0 - frac) * input[index] + frac * input[index + 1]));
		} else {
			output.push_back(input.back());
		}
	}

	return output;
}