#pragma once

class Reciter {
public:
	Reciter()  = default;
	~Reciter() = default;

	[[nodiscard]] bool textToPhonemes(unsigned char *input);

private:
	[[nodiscard]] unsigned char getCode37055(unsigned char npos, unsigned char mask);
	[[nodiscard]] unsigned int match(const char *str);
	[[nodiscard]] static unsigned char getRuleByte(unsigned short mem62, unsigned char Y);
	[[nodiscard]] int handleCH(unsigned char ch, unsigned char mem);
	[[nodiscard]] int handleCH2(unsigned char ch, unsigned char mem);

	unsigned char A;
	unsigned char X;

	static constexpr auto inputTempBufferSize = 256;
	unsigned char inputTemp[inputTempBufferSize];
};
