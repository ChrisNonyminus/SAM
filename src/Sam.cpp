#include "Sam.h"

void Sam::setSpeed(uint8_t _speed) {
	speed = _speed;
}

void Sam::setPitch(uint8_t _pitch) {
	pitch = _pitch;
}

void Sam::setMouth(uint8_t _mouth) {
	mouth = _mouth;
}

void Sam::setThroat(uint8_t _throat) {
	throat = _throat;
}

void Sam::setMode(Sam::Mode _mode) {
	mode = _mode;
}

const std::array<char, Sam::bufferLength> &Sam::getBuffer() const {
	return outBuffer;
}

size_t Sam::getBufferSize() const {
	return writePosition / 50;
}

void Sam::reset() {
	updateFrequencyTables();
	phonemeIndex[phonemeSize - 1] = 32;
}

uint8_t Sam::translate(uint8_t a, uint8_t b) {
	return (((unsigned int) a * b) >> 8) << 1;
}

void Sam::updateFrequencyTables() {
	static constexpr uint8_t mouthFormants5_29[30] = {0,  0,  0,  0,  0,  10, 14, 19, 24, 27, 23, 21, 16, 20, 14,
													  18, 14, 18, 18, 16, 13, 15, 11, 18, 14, 11, 9,  6,  6,  6};

	static constexpr uint8_t throatFormants5_29[30] = {
		255, 255, 255, 255, 255, 84, 73, 67, 63, 40, 44, 31, 37, 45, 73,
		49,	 36,  30,  51,	37,	 29, 69, 24, 50, 30, 24, 83, 46, 54, 86,
	};

	static constexpr uint8_t mouthFormants48_53[6]	= {19, 27, 21, 27, 18, 13};
	static constexpr uint8_t throatFormants48_53[6] = {72, 39, 31, 43, 30, 34};

	for (int i = 0; i < sam::render_tables::freq1DataSize; ++i) {
		freq1Data[i] = sam::render_tables::freq1data[i];
	}

	for (int i = 0; i < sam::render_tables::freq2DataSize; ++i) {
		freq2Data[i] = sam::render_tables::freq2data[i];
	}

	uint8_t newFrequency = 0;
	uint8_t pos			 = 5;

	while (pos < 30) {
		auto initialFrequency = mouthFormants5_29[pos];
		if (initialFrequency != 0) {
			newFrequency = translate(mouth, initialFrequency);
		}
		freq1Data[pos] = newFrequency;

		initialFrequency = throatFormants5_29[pos];
		if (initialFrequency != 0) {
			newFrequency = translate(throat, initialFrequency);
		}
		freq2Data[pos] = newFrequency;
		pos++;
	}

	pos = 0;
	while (pos < 6) {
		freq1Data[pos + 48] = translate(mouth, mouthFormants48_53[pos]);
		freq2Data[pos + 48] = translate(throat, throatFormants48_53[pos]);
		pos++;
	}
}

void Sam::checkForPhonemeEnd() {
	for (uint8_t &i: phonemeIndex) {
		if (i > 80) {
			i = END;
			break;
		}
	}
}

bool Sam::process(const std::string &_input) {
	reset();
	input = _input;

	if (!parser1()) {
		return false;
	}
	parser2();
	copyStress();
	setPhonemeLength();
	adjustLengths();
	code41240();
	checkForPhonemeEnd();
	insertBreath();
	prepareOutput();
	return true;
}

void Sam::rescaleAmplitude() {
	for (int i = phonemeEndMarker; i >= 0; i--) {
		amplitude1[i] = sam::render_tables::amplitudeRescale[amplitude1[i]];
		amplitude2[i] = sam::render_tables::amplitudeRescale[amplitude2[i]];
		amplitude3[i] = sam::render_tables::amplitudeRescale[amplitude3[i]];
	}
}

void Sam::assignPitchContour() {
	for (int i = 0; i < phonemeSize; i++) {
		pitches[i] -= (frequency1[i] >> 1);
	}
}

void Sam::addInflection(uint8_t inflection, uint8_t pos) {
	if (pos < 30) {
		pos = 0;
	} else {
		pos -= 30;
	}

	auto pitchPos = pitches[pos];
	auto end	  = pos;

	while ((pitchPos = pitches[pos]) == 127) {
		++pos;
	}

	while (pos != end) {
		pitchPos += inflection;
		pitches[pos] = pitchPos;
		while ((++pos != end) && pitches[pos] == phonemeEndMarker)
			;
	}
}

void Sam::createFrames() {
	uint8_t tableWritePosition = 0;
	for (int i = 0; i < phonemeSize; ++i) {
		if (phonemeIndexOutput[i] == phonemeEndMarker) {
			break;
		}

		if (phonemeIndexOutput[i] == phonemePeriod) {
			addInflection(risingInflection, tableWritePosition);
		} else if (phonemeIndexOutput[i] == phonemeQuestion) {
			addInflection(fallingInflection, tableWritePosition);
		}

		auto phase1 = sam::render_tables::tab47492[stressOutput[i] + 1];
		auto phase2 = phonemeLengthOutput[i];

		do {
			frequency1[tableWritePosition] = freq1Data[phonemeIndexOutput[i]];
			frequency2[tableWritePosition] = freq2Data[phonemeIndexOutput[i]];
			frequency3[tableWritePosition] = sam::render_tables::freq3data[phonemeIndexOutput[i]];
			amplitude1[tableWritePosition] = sam::render_tables::ampl1data[phonemeIndexOutput[i]];
			amplitude2[tableWritePosition] = sam::render_tables::ampl2data[phonemeIndexOutput[i]];
			amplitude3[tableWritePosition] = sam::render_tables::ampl3data[phonemeIndexOutput[i]];
			sampledConsonantFlag[tableWritePosition] =
				sam::render_tables::sampledConsonantFlags[phonemeIndexOutput[i]];
			pitches[tableWritePosition] = pitch + phase1;
			++tableWritePosition;
		} while (--phase2 != 0);
	}
}

uint8_t Sam::readTable(uint8_t table, uint8_t index) {
	switch (table) {
		case 168: return pitches[index];
		case 169: return frequency1[index];
		case 170: return frequency2[index];
		case 171: return frequency3[index];
		case 172: return amplitude1[index];
		case 173: return amplitude2[index];
		case 174: return amplitude3[index];
		default: return 0;
	}
}

void Sam::writeTable(uint8_t table, uint8_t index, uint8_t value) {
	switch (table) {
		case 168: pitches[index] = value; return;
		case 169: frequency1[index] = value; return;
		case 170: frequency2[index] = value; return;
		case 171: frequency3[index] = value; return;
		case 172: amplitude1[index] = value; return;
		case 173: amplitude2[index] = value; return;
		case 174: amplitude3[index] = value; return;
		default: return;
	}
}

void Sam::interpolate(uint8_t width, uint8_t table, uint8_t frame, char mem53) {
	auto sign	   = (mem53 < 0);
	auto remainder = abs(mem53) % width;
	auto div	   = mem53 / width;
	auto error	   = 0;
	auto pos	   = width;
	auto val	   = readTable(table, frame) + div;

	while (--pos) {
		error += remainder;
		if (error >= width) {
			error -= width;
			if (sign) {
				val--;
			} else if (val) {
				val++;
			}
			writeTable(table, ++frame, val);
			val += div;
		}
	}
}

void Sam::interpolatePitch(uint8_t pos, uint8_t mem49, uint8_t phase3) {
	auto currentWidth  = phonemeLengthOutput[pos] / 2;
	auto nextWidth	   = phonemeLengthOutput[pos + 1] / 2;
	auto width		   = currentWidth + nextWidth;
	char computedPitch = static_cast<char>(pitches[nextWidth + mem49] - pitches[mem49 - currentWidth]);
	interpolate(width, 168, phase3, computedPitch);
}

uint8_t Sam::createTransitions() {
	uint8_t mem49 = 0;
	uint8_t pos	  = 0;
	while (true) {
		auto phoneme	  = phonemeIndexOutput[pos];
		auto next_phoneme = phonemeIndexOutput[pos + 1];

		if (next_phoneme == phonemeEndMarker) {
			break;
		}

		auto next_rank = sam::render_tables::blendRank[next_phoneme];
		auto rank	   = sam::render_tables::blendRank[phoneme];

		uint8_t phase1 = 0;
		uint8_t phase2 = 0;
		if (rank == next_rank) {
			phase1 = sam::render_tables::outBlendLength[phoneme];
			phase2 = sam::render_tables::outBlendLength[next_phoneme];
		} else if (rank < next_rank) {
			phase1 = sam::render_tables::inBlendLength[next_phoneme];
			phase2 = sam::render_tables::outBlendLength[next_phoneme];
		} else {
			phase1 = sam::render_tables::outBlendLength[phoneme];
			phase2 = sam::render_tables::inBlendLength[phoneme];
		}

		mem49 += phonemeLengthOutput[pos];

		auto speedCounter = mem49 + phase2;
		auto phase3		  = mem49 - phase1;
		auto transition	  = phase1 + phase2;

		if (((transition - 2) & 128) == 0) {
			uint8_t table = 169;
			interpolatePitch(pos, mem49, phase3);
			while (table < 175) {
				char value = static_cast<char>(readTable(table, speedCounter) - readTable(table, phase3));
				interpolate(transition, table, phase3, value);
				table++;
			}
		}
		++pos;
	}

	return mem49 + phonemeLengthOutput[pos];
}

void Sam::render() {
	if (phonemeIndexOutput[0] == phonemeEndMarker) {
		return;
	}

	createFrames();
	auto transitions = createTransitions();

	if (mode == Mode::Speech) {
		assignPitchContour();
	}
	rescaleAmplitude();

	processFrames(transitions);
}

void Sam::writeOutput(int index, uint8_t outputSample) {
	static constexpr int timetable[5][5] = {{162, 167, 167, 127, 128},
											{226, 60, 60, 0, 0},
											{225, 60, 59, 0, 0},
											{200, 0, 0, 54, 55},
											{199, 0, 0, 54, 54}};
	writePosition += timetable[timetableIndex][index];
	timetableIndex = index;
	for (int k = 0; k < 5; k++)
		outBuffer[writePosition / 50 + k] = (outputSample & 15) * 16;
}

void Sam::combineGlottalAndFormants(uint8_t phase1, uint8_t phase2, uint8_t phase3, uint8_t Y) {
	auto result = static_cast<unsigned int>(
		sam::render_tables::multtable[sam::render_tables::sinus[phase1] | amplitude1[Y]]);
	result += sam::render_tables::multtable[sam::render_tables::sinus[phase2] | amplitude2[Y]];
	result += result > phonemeEndMarker ? 1 : 0;
	result += sam::render_tables::multtable[sam::render_tables::rectangle[phase3] | amplitude3[Y]];
	result += 136;
	result >>= 4;
	writeOutput(0, result & 0xf);
}

uint8_t Sam::renderVoicedSample(unsigned short hi, uint8_t off, uint8_t phase1) {
	do {
		uint8_t bit	   = 8;
		uint8_t sample = sam::render_tables::sampleTable[hi + off];
		do {
			if ((sample & 128) != 0) {
				writeOutput(3, 26);
			} else {
				writeOutput(4, 6);
			}
			sample <<= 1;
		} while (--bit != 0);
		off++;
	} while (++phase1 != 0);
	return off;
}

void Sam::renderUnvoicedSample(unsigned short hi, uint8_t off, uint8_t mem53) {
	do {
		uint8_t bit	   = 8;
		uint8_t sample = sam::render_tables::sampleTable[hi + off];
		do {
			if ((sample & 128) != 0) {
				writeOutput(2, 5);
			} else {
				writeOutput(1, mem53);
			}
			sample <<= 1;
		} while (--bit != 0);
	} while (++off != 0);
}

void Sam::renderSample(uint8_t *mem66, uint8_t consonantFlag, uint8_t mem49) {
	// mem49 == current phoneme's index

	// mask low three bits and subtract 1 get value to
	// convert 0 bits on unvoiced samples.
	uint8_t hibyte = (consonantFlag & 7) - 1;

	// determine which offset to use from table { 0x18, 0x1A, 0x17, 0x17, 0x17 }
	// T, S, Z                0          0x18
	// CH, J, SH, ZH          1          0x1A
	// P, F*, V, TH, DH       2          0x17
	// /H                     3          0x17
	// /X                     4          0x17

	unsigned short hi = hibyte * 256;
	// voiced sample?
	uint8_t pitchl = consonantFlag & 248;
	if (pitchl == 0) {
		// voiced phoneme: Z*, ZH, V*, DH
		pitchl = pitches[mem49] >> 4;
		*mem66 = renderVoicedSample(hi, *mem66, pitchl ^ phonemeEndMarker);
	} else {
		renderUnvoicedSample(hi, pitchl ^ phonemeEndMarker, sam::render_tables::tab48426[hibyte]);
	}
}

// PROCESS THE FRAMES
//
// In traditional vocal synthesis, the glottal pulse drives filters, which
// are attenuated to the frequencies of the formants.
//
// SAM generates these formants directly with sin and rectangular waves.
// To simulate them being driven by the glottal pulse, the waveforms are
// reset at the beginning of each glottal pulse.
//
void Sam::processFrames(uint8_t mem48) {
	uint8_t speedcounter = 72;
	uint8_t phase1		 = 0;
	uint8_t phase2		 = 0;
	uint8_t phase3		 = 0;
	uint8_t mem66		 = 0;

	uint8_t Y = 0;

	uint8_t glottal_pulse = pitches[0];
	uint8_t mem38		  = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

	while (mem48) {
		uint8_t flags = sampledConsonantFlag[Y];

		// unvoiced sampled phoneme?
		if (flags & 248) {
			renderSample(&mem66, flags, Y);
			// skip ahead two in the phoneme buffer
			Y += 2;
			mem48 -= 2;
			speedcounter = speed;
		} else {
			combineGlottalAndFormants(phase1, phase2, phase3, Y);

			speedcounter--;
			if (speedcounter == 0) {
				Y++; //go to next amplitude
				// decrement the frame count
				mem48--;
				if (mem48 == 0) return;
				speedcounter = speed;
			}

			--glottal_pulse;

			if (glottal_pulse != 0) {
				// not finished with a glottal pulse

				--mem38;
				// within the first 75% of the glottal pulse?
				// is the count non-zero and the sampled flag is zero?
				if ((mem38 != 0) || (flags == 0)) {
					// reset the phase of the formants to match the pulse
					phase1 += frequency1[Y];
					phase2 += frequency2[Y];
					phase3 += frequency3[Y];
					continue;
				}

				// voiced sampled phonemes interleave the sample with the
				// glottal pulse. The sample flag is non-zero, so render
				// the sample for the phoneme.
				renderSample(&mem66, flags, Y);
			}
		}

		glottal_pulse = pitches[Y];
		mem38		  = glottal_pulse - (glottal_pulse >> 2); // mem44 * 0.75

		// reset the formant wave generators to keep them in
		// sync with the glottal pulse
		phase1 = 0;
		phase2 = 0;
		phase3 = 0;
	}
}

void Sam::prepareOutput() {
	uint8_t srcpos	= 0; // Position in source
	uint8_t destpos = 0; // Position in output

	while (true) {
		uint8_t A					= phonemeIndex[srcpos];
		phonemeIndexOutput[destpos] = A;
		switch (A) {
			case END: render(); return;
			case BREAK:
				phonemeIndexOutput[destpos] = END;
				render();
				destpos = 0;
				break;
			case 0: break;
			default:
				phonemeLengthOutput[destpos] = phonemeLength[srcpos];
				stressOutput[destpos]		 = stress[srcpos];
				++destpos;
				break;
		}
		++srcpos;
	}
}

void Sam::insertBreath() {
	uint8_t mem54 = phonemeEndMarker;
	uint8_t len	  = 0;
	uint8_t index; //variable Y

	uint8_t pos = 0;

	while ((index = phonemeIndex[pos]) != END) {
		len += phonemeLength[pos];
		if (len < 232) {
			if (index == BREAK) {
			} else if (!(sam::sam_tables::flags[index] & sam::sam_tables::FLAG_PUNCT)) {
				if (index == 0) mem54 = pos;
			} else {
				len = 0;
				insert(++pos, BREAK, 0, 0);
			}
		} else {
			pos				   = mem54;
			phonemeIndex[pos]  = 31; // 'Q*' glottal stop
			phonemeLength[pos] = 4;
			stress[pos]		   = 0;

			len = 0;
			insert(++pos, BREAK, 0, 0);
		}
		++pos;
	}
}

// Iterates through the phoneme buffer, copying the stress value from
// the following phoneme under the following circumstance:

//     1. The current phoneme is voiced, excluding plosives and fricatives
//     2. The following phoneme is voiced, excluding plosives and fricatives, and
//     3. The following phoneme is stressed
//
//  In those cases, the stress value+1 from the following phoneme is copied.
//
// For example, the word LOITER is represented as LOY5TER, with as stress
// of 5 on the dipthong OY. This routine will copy the stress value of 6 (5+1)
// to the L that precedes it.

void Sam::copyStress() {
	// loop thought all the phonemes to be output
	uint8_t pos = 0; //mem66
	uint8_t Y;
	while ((Y = phonemeIndex[pos]) != END) {
		// if CONSONANT_FLAG set, skip - only vowels get stress
		if (sam::sam_tables::flags[Y] & 64) {
			Y = phonemeIndex[pos + 1];

			// if the following phoneme is the end, or a vowel, skip
			if (Y != END && (sam::sam_tables::flags[Y] & 128) != 0) {
				// get the stress value at the next position
				Y = stress[pos + 1];
				if (Y && !(Y & 128)) {
					// if next phoneme is stressed, and a VOWEL OR ER
					// copy stress from next phoneme to this one
					stress[pos] = Y + 1;
				}
			}
		}

		++pos;
	}
}

void Sam::insert(uint8_t position /*var57*/, uint8_t mem60, uint8_t mem59, uint8_t mem58) {
	int i;
	for (i = 253; i >= position; i--) // ML : always keep last safe-guarding 255
	{
		phonemeIndex[i + 1]	 = phonemeIndex[i];
		phonemeLength[i + 1] = phonemeLength[i];
		stress[i + 1]		 = stress[i];
	}

	phonemeIndex[position]	= mem60;
	phonemeLength[position] = mem59;
	stress[position]		= mem58;
}

signed int Sam::fullMatch(uint8_t sign1, uint8_t sign2) {
	uint8_t Y = 0;
	do {
		// GET FIRST CHARACTER AT POSITION Y IN signInputTable
		// --> should change name to PhonemeNameTable1
		uint8_t A = sam::sam_tables::signInputTable1[Y];

		if (A == sign1) {
			A = sam::sam_tables::signInputTable2[Y];
			// NOT A SPECIAL AND MATCHES SECOND CHARACTER?
			if ((A != '*') && (A == sign2)) return Y;
		}
	} while (++Y != 81);
	return -1;
}

signed int Sam::wildMatch(uint8_t sign1) {
	signed int Y = 0;
	do {
		if (sam::sam_tables::signInputTable2[Y] == '*') {
			if (sam::sam_tables::signInputTable1[Y] == sign1) return Y;
		}
	} while (++Y != 81);
	return -1;
}

// The input[] buffer contains a string of phonemes and stress markers along
// the lines of:
//
//     DHAX KAET IHZ AH5GLIY. <0x9B>
//
// The byte 0x9B marks the end of the buffer. Some phonemes are 2 bytes
// long, such as "DH" and "AX". Others are 1 byte long, such as "T" and "Z".
// There are also stress markers, such as "5" and ".".
//
// The first character of the phonemes are stored in the table signInputTable1[].
// The second character of the phonemes are stored in the table signInputTable2[].
// The stress characters are arranged in low to high stress order in stressInputTable[].
//
// The following process is used to parse the input[] buffer:
//
// Repeat until the <0x9B> character is reached:
//
//        First, a search is made for a 2 character match for phonemes that do not
//        end with the '*' (wildcard) character. On a match, the index of the phoneme
//        is added to phonemeIndex[] and the buffer position is advanced 2 bytes.
//
//        If this fails, a search is made for a 1 character match against all
//        phoneme names ending with a '*' (wildcard). If this succeeds, the
//        phoneme is added to phonemeIndex[] and the buffer position is advanced
//        1 byte.
//
//        If this fails, search for a 1 character match in the stressInputTable[].
//        If this succeeds, the stress value is placed in the last stress[] table
//        at the same index of the last added phoneme, and the buffer position is
//        advanced by 1 byte.
//
//        If this fails, return a 0.
//
// On success:
//
//    1. phonemeIndex[] will contain the index of all the phonemes.
//    2. The last index in phonemeIndex[] will be 255.
//    3. stress[] will contain the stress value for each phoneme

// input[] holds the string of phonemes, each two bytes wide
// signInputTable1[] holds the first character of each phoneme
// signInputTable2[] holds te second character of each phoneme
// phonemeIndex[] holds the indexes of the phonemes after parsing input[]
//
// The parser scans through the input[], finding the names of the phonemes
// by searching signInputTable1[] and signInputTable2[]. On a match, it
// copies the index of the phoneme into the phonemeIndexTable[].
//
// The character <0x9B> marks the end of text in input[]. When it is reached,
// the index 255 is placed at the end of the phonemeIndexTable[], and the
// function returns with a 1 indicating success.
int Sam::parser1() {
	uint8_t sign1;
	uint8_t position = 0;
	uint8_t srcpos	 = 0;

	memset(stress, 0, phonemeSize);

	while ((sign1 = input[srcpos]) != 155) { // 155 (\233) is end of line marker
		signed int match;
		uint8_t sign2 = input[++srcpos];
		if ((match = fullMatch(sign1, sign2)) != -1) {
			// Matched both characters (no wildcards)
			phonemeIndex[position++] = (uint8_t) match;
			++srcpos; // Skip the second character of the input as we've matched it
		} else if ((match = wildMatch(sign1)) != -1) {
			// Matched just the first character (with second character matching '*'
			phonemeIndex[position++] = (uint8_t) match;
		} else {
			// Should be a stress character. Search through the
			// stress table backwards.
			match = 8; // End of stress table. FIXME: Don't hardcode.
			while ((sign1 != sam::sam_tables::stressInputTable[match]) && (match > 0))
				--match;

			if (match == 0) return 0; // failure

			stress[position - 1] = (uint8_t) match; // Set stress for prior phoneme
		}
	} //while

	phonemeIndex[position] = END;
	return 1;
}

//change phonemelength depedendent on stress
void Sam::setPhonemeLength() {
	int position = 0;
	while (phonemeIndex[position] != phonemeEndMarker) {
		uint8_t A = stress[position];
		if ((A == 0) || ((A & 128) != 0)) {
			phonemeLength[position] = sam::sam_tables::phonemeLengthTable[phonemeIndex[position]];
		} else {
			phonemeLength[position] = sam::sam_tables::phonemeStressedLengthTable[phonemeIndex[position]];
		}
		position++;
	}
}

void Sam::code41240() {
	uint8_t pos = 0;

	while (phonemeIndex[pos] != END) {
		uint8_t index = phonemeIndex[pos];

		if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_STOPCONS)) {
			if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_PLOSIVE)) {
				uint8_t A;
				uint8_t X = pos;
				while (!phonemeIndex[++X])
					; /* Skip pause */
				A = phonemeIndex[X];
				if (A != END) {
					if ((sam::sam_tables::flags[A] & 8) || (A == 36) || (A == 37)) {
						++pos;
						continue;
					} // '/H' '/X'
				}
			}
			insert(pos + 1, index + 1, sam::sam_tables::phonemeLengthTable[index + 1], stress[pos]);
			insert(pos + 2, index + 2, sam::sam_tables::phonemeLengthTable[index + 2], stress[pos]);
			pos += 2;
		}
		++pos;
	}
}

void Sam::changeRule(uint8_t position, uint8_t mem60, const char *descr) {
	phonemeIndex[position] = 13; //rule;
	insert(position + 1, mem60, 0, stress[position]);
}

// Rewrites the phonemes using the following rules:
//
//       <DIPTHONG ENDING WITH WX> -> <DIPTHONG ENDING WITH WX> WX
//       <DIPTHONG NOT ENDING WITH WX> -> <DIPTHONG NOT ENDING WITH WX> YX
//       UL -> AX L
//       UM -> AX M
//       <STRESSED VOWEL> <SILENCE> <STRESSED VOWEL> -> <STRESSED VOWEL> <SILENCE> Q <VOWEL>
//       T R -> CH R
//       D R -> J R
//       <VOWEL> R -> <VOWEL> RX
//       <VOWEL> L -> <VOWEL> LX
//       G S -> G Z
//       K <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPTHONG NOT ENDING WITH IY>
//       G <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPTHONG NOT ENDING WITH IY>
//       S P -> S B
//       S T -> S D
//       S K -> S G
//       S KX -> S GX
//       <ALVEOLAR> UW -> <ALVEOLAR> UX
//       CH -> CH CH' (CH requires two phonemes to represent it)
//       J -> J J' (J requires two phonemes to represent it)
//       <UNSTRESSED VOWEL> T <PAUSE> -> <UNSTRESSED VOWEL> DX <PAUSE>
//       <UNSTRESSED VOWEL> D <PAUSE>  -> <UNSTRESSED VOWEL> DX <PAUSE>

void Sam::applyUWAlveolarRule(uint8_t X) {
	// ALVEOLAR flag set?
	if (sam::sam_tables::flags[phonemeIndex[X - 1]] & sam::sam_tables::FLAG_ALVEOLAR) {
		phonemeIndex[X] = 16;
	}
}

void Sam::applyCHRule(uint8_t X) {
	insert(X + 1, 43, 0, stress[X]);
}

void Sam::applyJRule(uint8_t X) {
	insert(X + 1, 45, 0, stress[X]);
}

void Sam::applyGRule(uint8_t pos) {
	// G <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> GX <VOWEL OR DIPTHONG NOT ENDING WITH IY>
	// Example: GO

	uint8_t index = phonemeIndex[pos + 1];

	// If dipthong ending with YX, move continue processing next phoneme
	if ((index != phonemeEndMarker) && ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_DIP_YX) == 0)) {
		// replace G with GX and continue processing next phoneme
		phonemeIndex[pos] = 63; // 'GX'
	}
}

void Sam::change(uint8_t pos, uint8_t val, const char *rule) {
	phonemeIndex[pos] = val;
}

void Sam::applyDipthongRule(uint8_t p, unsigned short pf, uint8_t pos) {
	// <DIPTHONG ENDING WITH WX> -> <DIPTHONG ENDING WITH WX> WX
	// <DIPTHONG NOT ENDING WITH WX> -> <DIPTHONG NOT ENDING WITH WX> YX
	// Example: OIL, COW

	// If ends with IY, use YX, else use WX
	uint8_t A = (pf & sam::sam_tables::FLAG_DIP_YX) ? 21 : 20; // 'WX' = 20 'YX' = 21

	// Insert at WX or YX following, copying the stress
	insert(pos + 1, A, 0, stress[pos]);

	if (p == 53) applyUWAlveolarRule(pos); // Example: NEW, DEW, SUE, ZOO, THOO, TOO
	else if (p == 42) applyCHRule(pos); // Example: CHEW
	else if (p == 44) applyJRule(pos); // Example: JAY
}

void Sam::parser2() {
	uint8_t pos = 0; //mem66;
	uint8_t p;

	while ((p = phonemeIndex[pos]) != END) {
		unsigned short pf;
		uint8_t prior;

		if (p == 0) { // Is phoneme pause?
			++pos;
			continue;
		}

		pf	  = sam::sam_tables::flags[p];
		prior = phonemeIndex[pos - 1];

		if ((pf & sam::sam_tables::FLAG_DIPTHONG)) applyDipthongRule(p, pf, pos);
		else if (p == 78) changeRule(pos, 24, "UL -> AX L"); // Example: MEDDLE
		else if (p == 79) changeRule(pos, 27, "UM -> AX M"); // Example: ASTRONOMY
		else if (p == 80) changeRule(pos, 28, "UN -> AX N"); // Example: FUNCTION
		else if ((pf & sam::sam_tables::FLAG_VOWEL) && stress[pos]) {
			// RULE:
			//       <STRESSED VOWEL> <SILENCE> <STRESSED VOWEL> -> <STRESSED VOWEL> <SILENCE> Q <VOWEL>
			// EXAMPLE: AWAY EIGHT
			if (!phonemeIndex[pos + 1]) { // If following phoneme is a pause, get next
				p = phonemeIndex[pos + 2];
				if (p != END && (sam::sam_tables::flags[p] & sam::sam_tables::FLAG_VOWEL) && stress[pos + 2]) {
					insert(pos + 2, 31, 0, 0); // 31 = 'Q'
				}
			}
		} else if (p == pR) { // RULES FOR PHONEMES BEFORE R
			if (prior == pT) change(pos - 1, 42, "T R -> CH R"); // Example: TRACK
			else if (prior == pD) change(pos - 1, 44, "D R -> J R"); // Example: DRY
			else if (sam::sam_tables::flags[prior] & sam::sam_tables::FLAG_VOWEL)
				change(pos, 18, "<VOWEL> R -> <VOWEL> RX"); // Example: ART
		} else if (p == 24 && (sam::sam_tables::flags[prior] & sam::sam_tables::FLAG_VOWEL))
			change(pos, 19, "<VOWEL> L -> <VOWEL> LX"); // Example: ALL
		else if (prior == 60 && p == 32) { // 'G' 'S'
			// Can't get to fire -
			//       1. The G -> GX rule intervenes
			//       2. Reciter already replaces GS -> GZ
			change(pos, 38, "G S -> G Z");
		} else if (p == 60) applyGRule(pos);
		else {
			if (p == 72) { // 'K'
				// K <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPTHONG NOT ENDING WITH IY>
				// Example: COW
				uint8_t Y = phonemeIndex[pos + 1];
				// If at end, replace current phoneme with KX
				if ((sam::sam_tables::flags[Y] & sam::sam_tables::FLAG_DIP_YX) == 0
					|| Y == END) { // VOWELS AND DIPTHONGS ENDING WITH IY SOUND flag set?
					change(
						pos,
						75,
						"K <VOWEL OR DIPTHONG NOT ENDING WITH IY> -> KX <VOWEL OR DIPTHONG NOT ENDING WITH IY>");
					p  = 75;
					pf = sam::sam_tables::flags[p];
				}
			}

			// Replace with softer version?
			if ((sam::sam_tables::flags[p] & sam::sam_tables::FLAG_PLOSIVE) && (prior == 32)) { // 'S'
				// RULE:
				//      S P -> S B
				//      S T -> S D
				//      S K -> S G
				//      S KX -> S GX
				// Examples: SPY, STY, SKY, SCOWL

				phonemeIndex[pos] = p - 12;
			} else if (!(pf & sam::sam_tables::FLAG_PLOSIVE)) {
				p = phonemeIndex[pos];
				if (p == 53) applyUWAlveolarRule(pos); // Example: NEW, DEW, SUE, ZOO, THOO, TOO
				else if (p == 42) applyCHRule(pos); // Example: CHEW
				else if (p == 44) applyJRule(pos); // Example: JAY
			}

			if (p == 69 || p == 57) { // 'T', 'D'
				// RULE: Soften T following vowel
				// NOTE: This rule fails for cases such as "ODD"
				//       <UNSTRESSED VOWEL> T <PAUSE> -> <UNSTRESSED VOWEL> DX <PAUSE>
				//       <UNSTRESSED VOWEL> D <PAUSE>  -> <UNSTRESSED VOWEL> DX <PAUSE>
				// Example: PARTY, TARDY
				if (sam::sam_tables::flags[phonemeIndex[pos - 1]] & sam::sam_tables::FLAG_VOWEL) {
					p = phonemeIndex[pos + 1];
					if (!p) p = phonemeIndex[pos + 2];
					if ((sam::sam_tables::flags[p] & sam::sam_tables::FLAG_VOWEL) && !stress[pos + 1])
						change(pos, 30, "Soften T or D following vowel or ER and preceding a pause -> DX");
				}
			}
		}
		pos++;
	} // while
}

// Applies various rules that adjust the lengths of phonemes
//
//         Lengthen <FRICATIVE> or <VOICED> between <VOWEL> and <PUNCTUATION> by 1.5
//         <VOWEL> <RX | LX> <CONSONANT> - decrease <VOWEL> length by 1
//         <VOWEL> <UNVOICED PLOSIVE> - decrease vowel by 1/8th
//         <VOWEL> <UNVOICED CONSONANT> - increase vowel by 1/2 + 1
//         <NASAL> <STOP CONSONANT> - set nasal = 5, consonant = 6
//         <VOICED STOP CONSONANT> {optional silence} <STOP CONSONANT> - shorten both to 1/2 + 1
//         <LIQUID CONSONANT> <DIPTHONG> - decrease by 2
//
void Sam::adjustLengths() {
	// LENGTHEN VOWELS PRECEDING PUNCTUATION
	//
	// Search for punctuation. If found, back up to the first vowel, then
	// process all phonemes between there and up to (but not including) the punctuation.
	// If any phoneme is found that is a either a fricative or voiced, the duration is
	// increased by (length * 1.5) + 1

	// loop index
	{
		uint8_t X = 0;
		uint8_t index;

		while ((index = phonemeIndex[X]) != END) {
			uint8_t loopIndex;

			// not punctuation?
			if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_PUNCT) == 0) {
				++X;
				continue;
			}

			loopIndex = X;

			while (--X && !(sam::sam_tables::flags[phonemeIndex[X]] & sam::sam_tables::FLAG_VOWEL))
				; // back up while not a vowel
			if (X == 0) break;

			do {
				// test for vowel
				index = phonemeIndex[X];

				// test for fricative/unvoiced or not voiced
				if (!(sam::sam_tables::flags[index] & sam::sam_tables::FLAG_FRICATIVE)
					|| (sam::sam_tables::flags[index] & sam::sam_tables::FLAG_VOICED)) {
					uint8_t length	 = phonemeLength[X];
					phonemeLength[X] = (length >> 1) + length + 1;
				}
			} while (++X != loopIndex);
			X++;
		} // while
	}

	// Similar to the above routine, but shorten vowels under some circumstances

	// Loop through all phonemes
	uint8_t loopIndex = 0;
	uint8_t index;

	while ((index = phonemeIndex[loopIndex]) != END) {
		uint8_t X = loopIndex;

		if (sam::sam_tables::flags[index] & sam::sam_tables::FLAG_VOWEL) {
			index = phonemeIndex[loopIndex + 1];
			if (!(sam::sam_tables::flags[index] & sam::sam_tables::FLAG_CONSONANT)) {
				if ((index == 18) || (index == 19)) { // 'RX', 'LX'
					index = phonemeIndex[loopIndex + 2];
					if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_CONSONANT)) {
						phonemeLength[loopIndex]--;
					}
				}
			} else { // Got here if not <VOWEL>
				unsigned short flag = (index == END) ? 65 : sam::sam_tables::flags[index]; // 65 if end marker

				if (!(flag & sam::sam_tables::FLAG_VOICED)) { // Unvoiced
					// *, .*, ?*, ,*, -*, DX, S*, SH, F*, TH, /H, /X, CH, P*, T*, K*, KX
					if ((flag & sam::sam_tables::FLAG_PLOSIVE)) { // unvoiced plosive
						// RULE: <VOWEL> <UNVOICED PLOSIVE>
						// <VOWEL> <P*, T*, K*, KX>
						phonemeLength[loopIndex] -= (phonemeLength[loopIndex] >> 3);
					}
				} else {
					auto length				 = phonemeLength[loopIndex];
					phonemeLength[loopIndex] = (length >> 2) + length + 1; // 5/4*A + 1
				}
			}
		} else if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_NASAL) != 0) { // nasal?
			// RULE: <NASAL> <STOP CONSONANT>
			//       Set punctuation length to 6
			//       Set stop consonant length to 5
			index = phonemeIndex[++X];
			if (index != END && (sam::sam_tables::flags[index] & sam::sam_tables::FLAG_STOPCONS)) {
				phonemeLength[X]	 = 6; // set stop consonant length to 6
				phonemeLength[X - 1] = 5; // set nasal length to 5
			}
		} else if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_STOPCONS)) { // (voiced) stop consonant?
			// RULE: <VOICED STOP CONSONANT> {optional silence} <STOP CONSONANT>
			//       Shorten both to (length/2 + 1)

			// move past silence
			while ((index = phonemeIndex[++X]) == 0)
				;

			if (index != END && (sam::sam_tables::flags[index] & sam::sam_tables::FLAG_STOPCONS)) {
				phonemeLength[X]		 = (phonemeLength[X] >> 1) + 1;
				phonemeLength[loopIndex] = (phonemeLength[loopIndex] >> 1) + 1;
				X						 = loopIndex;
			}
		} else if ((sam::sam_tables::flags[index] & sam::sam_tables::FLAG_LIQUIC)) { // liquic consonant?
			// RULE: <VOICED NON-VOWEL> <DIPTHONG>
			//       Decrease <DIPTHONG> by 2
			index = phonemeIndex[X - 1]; // prior phoneme;
			phonemeLength[X] -= 2; // 20ms
		}

		++loopIndex;
	}
}
