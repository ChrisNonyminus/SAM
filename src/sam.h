#pragma once

#include <string>
#include <array>

#include "tables.h"

class Sam {
public:
	enum class Mode { Speech, Singing };
	static constexpr auto sampleRate		 = 22050;
	static constexpr auto maxLengthInSeconds = 10;
	static constexpr auto bufferLength		 = sampleRate * maxLengthInSeconds;

	Sam();

	void setSpeed(uint8_t speed);
	void setPitch(uint8_t pitch);
	void setMouth(uint8_t mouth);
	void setThroat(uint8_t throat);
	void setMode(Mode mode);

	[[nodiscard]] bool process(const std::string &input);

	[[nodiscard]] const std::array<char, bufferLength> &getBuffer() const;
	[[nodiscard]] size_t getBufferSize() const;

private:
	static constexpr auto phonemeSize		= 256;
	static constexpr auto outputSize		= 60;
	static constexpr auto phonemePeriod		= 1;
	static constexpr auto phonemeQuestion	= 2;
	static constexpr auto risingInflection	= 1;
	static constexpr auto fallingInflection = 255;

	enum { pR = 23, pD = 57, pT = 69, BREAK = 254, END = 255 };

	void reset();
	void prepareOutput();
	void insertBreath();
	void copyStress();
	void insert(unsigned char position /*var57*/, unsigned char mem60, unsigned char mem59, unsigned char mem58);
	signed int full_match(unsigned char sign1, unsigned char sign2);
	signed int wild_match(unsigned char sign1);
	int parser1();
	void setPhonemeLength();
	void code41240();
	void changeRule(unsigned char position, unsigned char mem60, const char *descr);
	void drule(const char *str);
	void drule_pre(const char *descr, unsigned char X);
	void drule_post(unsigned char X);
	void rule_alveolar_uw(unsigned char X);
	void rule_ch(unsigned char X);
	void rule_j(unsigned char X);
	void rule_g(unsigned char pos);
	void change(unsigned char pos, unsigned char val, const char *rule);
	void rule_dipthong(unsigned char p, unsigned short pf, unsigned char pos);
	void parser2();
	void adjustLengths();
	void updateFrequencyTables();
	static unsigned char translate(unsigned char a, unsigned char b);
	void checkForPhonemeEnd();

	void Render();

	void processFrames(unsigned char mem48);
	void renderSample(unsigned char *mem66, unsigned char consonantFlag, unsigned char mem49);
	unsigned char renderVoicedSample(unsigned short hi, unsigned char off, unsigned char phase1);
	void renderUnvoicedSample(unsigned short hi, unsigned char off, unsigned char mem53);
	unsigned char createTransitions();
	void addInflection(unsigned char inflection, unsigned char pos);
	void rescaleAmplitude();
	void assignPitchContour();
	void createFrames();
	void combineGlottalAndFormants(uint8_t phase1, uint8_t phase2, uint8_t phase3, uint8_t Y);
	void writeOutput(int index, uint8_t outputSample);
	uint8_t readTable(uint8_t table, uint8_t index);
	void writeTable(uint8_t table, uint8_t index, unsigned char value);
	void interpolate(unsigned char width, unsigned char table, unsigned char frame, char mem53);
	void interpolatePitch(unsigned char pos, unsigned char mem49, unsigned char phase3);

	std::string input;
	uint8_t speed {72};
	uint8_t pitch {64};
	uint8_t mouth {128};
	uint8_t throat {128};
	Mode mode {Mode::Speech};
	std::array<char, sampleRate * maxLengthInSeconds> outBuffer;
	size_t writePosition {0};
	size_t timetableIndex {0};

	uint8_t stress[phonemeSize]				  = {0};
	uint8_t phonemeLength[phonemeSize]		  = {0};
	uint8_t phonemeIndex[phonemeSize]		  = {0};
	uint8_t pitches[phonemeSize]			  = {0};
	uint8_t sampledConsonantFlag[phonemeSize] = {0};

	uint8_t phonemeIndexOutput[outputSize]	= {0};
	uint8_t stressOutput[outputSize]		= {0};
	uint8_t phonemeLengthOutput[outputSize] = {0};

	uint8_t frequency1[phonemeSize] = {0};
	uint8_t frequency2[phonemeSize] = {0};
	uint8_t frequency3[phonemeSize] = {0};

	uint8_t amplitude1[phonemeSize] = {0};
	uint8_t amplitude2[phonemeSize] = {0};
	uint8_t amplitude3[phonemeSize] = {0};

	uint8_t freq1Data[sam::render_tables::freq1DataSize] = {0};
	uint8_t freq2Data[sam::render_tables::freq2DataSize] = {0};
};
