#include "PhonemeConvertor.h"
#include "tables.h"

uint8_t PhonemeConvertor::getCode37055(uint8_t position, uint8_t mask) {
	writePosition = position;
	return sam::phonemes::tab36376[inputTemp[writePosition]] & mask;
}

unsigned int PhonemeConvertor::match(const char *str) {
	while (*str) {
		uint8_t ch = *str;
		A		   = inputTemp[writePosition++];
		if (A != ch) {
			return 0;
		}
		++str;
	}
	return 1;
}

uint8_t PhonemeConvertor::getRuleByte(unsigned short mem62, uint8_t Y) {
	unsigned int address = mem62;
	if (mem62 >= 37541) {
		address -= 37541;
		return sam::phonemes::rules2[address + Y];
	}
	address -= 32000;
	return sam::phonemes::rules[address + Y];
}

int PhonemeConvertor::handleCH2(uint8_t ch, uint8_t mem) {
	uint8_t tmp;
	writePosition = mem;
	tmp			  = sam::phonemes::tab36376[inputTemp[mem]];
	if (ch == ' ') {
		if (tmp & 128) {
			return 1;
		}
	} else if (ch == '#') {
		if (!(tmp & 64)) {
			return 1;
		}
	} else if (ch == '.') {
		if (!(tmp & 8)) {
			return 1;
		}
	} else if (ch == '^') {
		if (!(tmp & 32)) {
			return 1;
		}
	} else {
		return -1;
	}
	return 0;
}

int PhonemeConvertor::handleCH(uint8_t ch, uint8_t mem) {
	uint8_t tmp;
	writePosition = mem;
	tmp			  = sam::phonemes::tab36376[inputTemp[writePosition]];
	if (ch == ' ') {
		if ((tmp & 128) != 0) {
			return 1;
		}
	} else if (ch == '#') {
		if ((tmp & 64) == 0) {
			return 1;
		}
	} else if (ch == '.') {
		if ((tmp & 8) == 0) {
			return 1;
		}
	} else if (ch == '&') {
		if ((tmp & 16) == 0) {
			if (inputTemp[writePosition] != 72) {
				return 1;
			}
			++writePosition;
		}
	} else if (ch == '^') {
		if ((tmp & 32) == 0) {
			return 1;
		}
	} else if (ch == '+') {
		writePosition = mem;
		ch			  = inputTemp[writePosition];
		if ((ch != 69) && (ch != 73) && (ch != 89)) {
			return 1;
		}
	} else {
		return -1;
	}
	return 0;
}

std::string PhonemeConvertor::textToPhonemes(const std::string &input) {
	auto inputString = input;
	if (inputString.empty() || inputString.back() != '[') {
		inputString += '[';
	}

	uint8_t buffer[inputTempBufferSize];
	std::memset(buffer, 0, sizeof(buffer));

	size_t copyLength = std::min(inputString.size(), sizeof(buffer) - 1);
	std::memcpy(buffer, inputString.data(), copyLength);
	buffer[copyLength] = '\0';

	if (!textToPhonemes(buffer)) {
		return {};
	}

	return std::string(reinterpret_cast<char *>(buffer));
}

bool PhonemeConvertor::textToPhonemes(uint8_t *input) {
	uint8_t mem56; //output position for phonemes
	uint8_t mem57;
	uint8_t mem58;
	uint8_t mem59;
	uint8_t mem60;
	uint8_t mem61;
	unsigned short mem62; // memory position of current rule

	uint8_t mem64; // position of '=' or current character
	uint8_t mem65; // position of ')'
	uint8_t mem66; // position of '('

	uint8_t Y;

	int r;

	inputTemp[0] = ' ';

	// secure copy of input
	// because input will be overwritten by phonemes
	writePosition = 0;
	do {
		A = input[writePosition] & 127;
		if (A >= 112) A = A & 95;
		else if (A >= 96) A = A & 79;
		inputTemp[++writePosition] = A;
	} while (writePosition < 255);
	inputTemp[255] = 27;
	mem56 = mem61 = 255;

pos36554:
	while (true) {
		while (true) {
			writePosition = ++mem61;
			mem64		  = inputTemp[writePosition];
			if (mem64 == '[') {
				writePosition		 = ++mem56;
				input[writePosition] = 155;
				return true;
			}

			if (mem64 != '.') break;
			writePosition++;
			A = sam::phonemes::tab36376[inputTemp[writePosition]] & 1;
			if (A != 0) break;
			mem56++;
			writePosition		 = mem56;
			A					 = '.';
			input[writePosition] = '.';
		}
		mem57 = sam::phonemes::tab36376[mem64];
		if ((mem57 & 2) != 0) {
			mem62 = 37541;
			goto pos36700;
		}

		if (mem57 != 0) break;
		inputTemp[writePosition] = ' ';
		writePosition			 = ++mem56;
		if (writePosition > 120) {
			input[writePosition] = 155;
			return true;
		}
		input[writePosition] = 32;
	}

	if (!(mem57 & 128)) return false;

	// go to the right rules for this character.
	writePosition = mem64 - 'A';
	mem62		  = sam::phonemes::tab37489[writePosition] | (sam::phonemes::tab37515[writePosition] << 8);

pos36700:
	// find next rule
	while ((getRuleByte(++mem62, 0) & 128) == 0)
		;
	Y = 0;
	while (getRuleByte(mem62, ++Y) != '(')
		;
	mem66 = Y;
	while (getRuleByte(mem62, ++Y) != ')')
		;
	mem65 = Y;
	while ((getRuleByte(mem62, ++Y) & 127) != '=')
		;
	mem64 = Y;

	mem60 = writePosition = mem61;
	// compare the string within the bracket
	Y = mem66 + 1;

	while (true) {
		if (getRuleByte(mem62, Y) != inputTemp[writePosition]) goto pos36700;
		if (++Y == mem65) break;
		mem60 = ++writePosition;
	}

	// the string in the bracket is correct

	mem59 = mem61;

	while (true) {
		uint8_t ch;
		while (true) {
			mem66--;
			mem57 = getRuleByte(mem62, mem66);
			if ((mem57 & 128) != 0) {
				mem58 = mem60;
				goto pos37184;
			}
			writePosition = mem57 & 127;
			if ((sam::phonemes::tab36376[writePosition] & 128) == 0) break;
			if (inputTemp[mem59 - 1] != mem57) goto pos36700;
			--mem59;
		}

		ch = mem57;

		r = handleCH2(ch, mem59 - 1);
		if (r == -1) {
			switch (ch) {
				case '&':
					if (!getCode37055(mem59 - 1, 16)) {
						if (inputTemp[writePosition] != 'H') r = 1;
						else {
							A = inputTemp[--writePosition];
							if ((A != 'C') && (A != 'S')) r = 1;
						}
					}
					break;

				case '@':
					if (!getCode37055(mem59 - 1, 4)) {
						A = inputTemp[writePosition];
						if (A != 72) r = 1;
						if ((A != 84) && (A != 67) && (A != 83)) r = 1;
					}
					break;
				case '+':
					writePosition = mem59;
					A			  = inputTemp[--writePosition];
					if ((A != 'E') && (A != 'I') && (A != 'Y')) r = 1;
					break;
				case ':':
					while (getCode37055(mem59 - 1, 32))
						--mem59;
					continue;
				default: return false;
			}
		}

		if (r == 1) goto pos36700;

		mem59 = writePosition;
	}

	do {
		writePosition = mem58 + 1;
		if (inputTemp[writePosition] == 'E') {
			if ((sam::phonemes::tab36376[inputTemp[writePosition + 1]] & 128) != 0) {
				A = inputTemp[++writePosition];
				if (A == 'L') {
					if (inputTemp[++writePosition] != 'Y') goto pos36700;
				} else if ((A != 'R') && (A != 'S') && (A != 'D') && !match("FUL")) goto pos36700;
			}
		} else {
			if (!match("ING")) goto pos36700;
			mem58 = writePosition;
		}

	pos37184:
		r = 0;
		do {
			while (true) {
				Y = mem65 + 1;
				if (Y == mem64) {
					mem61 = mem60;

					while (true) {
						mem57 = A = getRuleByte(mem62, Y);
						A		  = A & 127;
						if (A != '=') input[++mem56] = A;
						if ((mem57 & 128) != 0) goto pos36554;
						Y++;
					}
				}
				mem65 = Y;
				mem57 = getRuleByte(mem62, Y);
				if ((sam::phonemes::tab36376[mem57] & 128) == 0) break;
				if (inputTemp[mem58 + 1] != mem57) {
					r = 1;
					break;
				}
				++mem58;
			}

			if (r == 0) {
				A = mem57;
				if (A == '@') {
					if (getCode37055(mem58 + 1, 4) == 0) {
						A = inputTemp[writePosition];
						if ((A != 82) && (A != 84) && (A != 67) && (A != 83)) r = 1;
					} else {
						r = -2;
					}
				} else if (A == ':') {
					while (getCode37055(mem58 + 1, 32))
						mem58 = writePosition;
					r = -2;
				} else r = handleCH(A, mem58 + 1);
			}

			if (r == 1) goto pos36700;
			if (r == -2) {
				r = 0;
				continue;
			}
			if (r == 0) mem58 = writePosition;
		} while (r == 0);
	} while (A == '%');
	return false;
}
