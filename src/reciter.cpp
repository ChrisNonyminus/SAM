//#include <cstdio>
//#include <cstring>
#include "reciter.h"
#include "tables.h"

//unsigned char A, X;
//
//static unsigned char inputTemp[256]; // secure copy of input tab36096

/* Retrieve flags for character at mem59-1 */
unsigned char Reciter::getCode37055(unsigned char position, unsigned char mask) {
	X = position;
	return sam::reciter_tables::tab36376[inputTemp[X]] & mask;
}

unsigned int Reciter::match(const char *str) {
	while (*str) {
		unsigned char ch = *str;
		A				 = inputTemp[X++];
		if (A != ch) {
			return 0;
		}
		++str;
	}
	return 1;
}

unsigned char Reciter::getRuleByte(unsigned short mem62, unsigned char Y) {
	unsigned int address = mem62;
	if (mem62 >= 37541) {
		address -= 37541;
		return sam::reciter_tables::rules2[address + Y];
	}
	address -= 32000;
	return sam::reciter_tables::rules[address + Y];
}

int Reciter::handleCH2(unsigned char ch, unsigned char mem) {
	unsigned char tmp;
	X	= mem;
	tmp = sam::reciter_tables::tab36376[inputTemp[mem]];
	if (ch == ' ') {
		if (tmp & 128) return 1;
	} else if (ch == '#') {
		if (!(tmp & 64)) return 1;
	} else if (ch == '.') {
		if (!(tmp & 8)) return 1;
	} else if (ch == '^') {
		if (!(tmp & 32)) return 1;
	} else return -1;
	return 0;
}

int Reciter::handleCH(unsigned char ch, unsigned char mem) {
	unsigned char tmp;
	X	= mem;
	tmp = sam::reciter_tables::tab36376[inputTemp[X]];
	if (ch == ' ') {
		if ((tmp & 128) != 0) return 1;
	} else if (ch == '#') {
		if ((tmp & 64) == 0) return 1;
	} else if (ch == '.') {
		if ((tmp & 8) == 0) return 1;
	} else if (ch == '&') {
		if ((tmp & 16) == 0) {
			if (inputTemp[X] != 72) return 1;
			++X;
		}
	} else if (ch == '^') {
		if ((tmp & 32) == 0) return 1;
	} else if (ch == '+') {
		X  = mem;
		ch = inputTemp[X];
		if ((ch != 69) && (ch != 73) && (ch != 89)) return 1;
	} else return -1;
	return 0;
}

bool Reciter::textToPhonemes(unsigned char *input) {
	unsigned char mem56; //output position for phonemes
	unsigned char mem57;
	unsigned char mem58;
	unsigned char mem59;
	unsigned char mem60;
	unsigned char mem61;
	unsigned short mem62; // memory position of current rule

	unsigned char mem64; // position of '=' or current character
	unsigned char mem65; // position of ')'
	unsigned char mem66; // position of '('

	unsigned char Y;

	int r;

	inputTemp[0] = ' ';

	// secure copy of input
	// because input will be overwritten by phonemes
	X = 0;
	do {
		A = input[X] & 127;
		if (A >= 112) A = A & 95;
		else if (A >= 96) A = A & 79;
		inputTemp[++X] = A;
	} while (X < 255);
	inputTemp[255] = 27;
	mem56 = mem61 = 255;

pos36554:
	while (true) {
		while (true) {
			X	  = ++mem61;
			mem64 = inputTemp[X];
			if (mem64 == '[') {
				X		 = ++mem56;
				input[X] = 155;
				return true;
			}

			if (mem64 != '.') break;
			X++;
			A = sam::reciter_tables::tab36376[inputTemp[X]] & 1;
			if (A != 0) break;
			mem56++;
			X		 = mem56;
			A		 = '.';
			input[X] = '.';
		}
		mem57 = sam::reciter_tables::tab36376[mem64];
		if ((mem57 & 2) != 0) {
			mem62 = 37541;
			goto pos36700;
		}

		if (mem57 != 0) break;
		inputTemp[X] = ' ';
		X			 = ++mem56;
		if (X > 120) {
			input[X] = 155;
			return true;
		}
		input[X] = 32;
	}

	if (!(mem57 & 128)) return false;

	// go to the right rules for this character.
	X	  = mem64 - 'A';
	mem62 = sam::reciter_tables::tab37489[X] | (sam::reciter_tables::tab37515[X] << 8);

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

	mem60 = X = mem61;
	// compare the string within the bracket
	Y = mem66 + 1;

	while (true) {
		if (getRuleByte(mem62, Y) != inputTemp[X]) goto pos36700;
		if (++Y == mem65) break;
		mem60 = ++X;
	}

	// the string in the bracket is correct

	mem59 = mem61;

	while (true) {
		unsigned char ch;
		while (true) {
			mem66--;
			mem57 = getRuleByte(mem62, mem66);
			if ((mem57 & 128) != 0) {
				mem58 = mem60;
				goto pos37184;
			}
			X = mem57 & 127;
			if ((sam::reciter_tables::tab36376[X] & 128) == 0) break;
			if (inputTemp[mem59 - 1] != mem57) goto pos36700;
			--mem59;
		}

		ch = mem57;

		r = handleCH2(ch, mem59 - 1);
		if (r == -1) {
			switch (ch) {
				case '&':
					if (!getCode37055(mem59 - 1, 16)) {
						if (inputTemp[X] != 'H') r = 1;
						else {
							A = inputTemp[--X];
							if ((A != 'C') && (A != 'S')) r = 1;
						}
					}
					break;

				case '@':
					if (!getCode37055(mem59 - 1, 4)) {
						A = inputTemp[X];
						if (A != 72) r = 1;
						if ((A != 84) && (A != 67) && (A != 83)) r = 1;
					}
					break;
				case '+':
					X = mem59;
					A = inputTemp[--X];
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

		mem59 = X;
	}

	do {
		X = mem58 + 1;
		if (inputTemp[X] == 'E') {
			if ((sam::reciter_tables::tab36376[inputTemp[X + 1]] & 128) != 0) {
				A = inputTemp[++X];
				if (A == 'L') {
					if (inputTemp[++X] != 'Y') goto pos36700;
				} else if ((A != 'R') && (A != 'S') && (A != 'D') && !match("FUL")) goto pos36700;
			}
		} else {
			if (!match("ING")) goto pos36700;
			mem58 = X;
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
				if ((sam::reciter_tables::tab36376[mem57] & 128) == 0) break;
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
						A = inputTemp[X];
						if ((A != 82) && (A != 84) && (A != 67) && (A != 83)) r = 1;
					} else {
						r = -2;
					}
				} else if (A == ':') {
					while (getCode37055(mem58 + 1, 32))
						mem58 = X;
					r = -2;
				} else r = handleCH(A, mem58 + 1);
			}

			if (r == 1) goto pos36700;
			if (r == -2) {
				r = 0;
				continue;
			}
			if (r == 0) mem58 = X;
		} while (r == 0);
	} while (A == '%');
	return false;
}
