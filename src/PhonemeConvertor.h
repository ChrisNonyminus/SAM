#pragma once

#include <cstdint>
#include <string>

class PhonemeConvertor {
public:
	PhonemeConvertor()	= default;
	~PhonemeConvertor() = default;

	[[nodiscard]] bool textToPhonemes(uint8_t *input);
	[[nodiscard]] std::string textToPhonemes(const std::string &input);

private:
	[[nodiscard]] uint8_t getCode37055(uint8_t position, uint8_t mask);
	[[nodiscard]] unsigned int match(const char *str);
	[[nodiscard]] static uint8_t getRuleByte(unsigned short mem62, uint8_t Y);
	[[nodiscard]] int handleCH(uint8_t ch, uint8_t mem);
	[[nodiscard]] int handleCH2(uint8_t ch, uint8_t mem);

	uint8_t A;
	uint8_t writePosition {0};

	static constexpr auto inputTempBufferSize = 256;
	uint8_t inputTemp[inputTempBufferSize]	  = {0};
};
