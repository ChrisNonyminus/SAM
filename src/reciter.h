#pragma once

#include <cstdint>
#include <string>

class Reciter {
public:
	Reciter()  = default;
	~Reciter() = default;

	[[nodiscard]] bool textToPhonemes(uint8_t *input);

private:
	[[nodiscard]] uint8_t getCode37055(uint8_t position, uint8_t mask);
	[[nodiscard]] unsigned int match(const char *str);
	[[nodiscard]] static uint8_t getRuleByte(unsigned short mem62, uint8_t Y);
	[[nodiscard]] int handleCH(uint8_t ch, uint8_t mem);
	[[nodiscard]] int handleCH2(uint8_t ch, uint8_t mem);

	uint8_t A;
	uint8_t X;

	static constexpr auto inputTempBufferSize = 256;
	uint8_t inputTemp[inputTempBufferSize]	  = {0};
};
