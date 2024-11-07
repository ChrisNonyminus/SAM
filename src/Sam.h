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

	Sam() = default;

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
	static constexpr auto phonemeEndMarker	= phonemeSize - 1;
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
	void insert(uint8_t position, uint8_t mem60, uint8_t mem59, uint8_t mem58);
	[[nodiscard]] signed int fullMatch(uint8_t sign1, uint8_t sign2);
	[[nodiscard]] signed int wildMatch(uint8_t sign1);
	[[nodiscard]] int parser1();
	void setPhonemeLength();
	void code41240();
	void changeRule(uint8_t position, uint8_t mem60, const char *descr);
	void applyUWAlveolarRule(uint8_t X);
	void applyCHRule(uint8_t X);
	void applyJRule(uint8_t X);
	void applyGRule(uint8_t pos);
	void change(uint8_t pos, uint8_t val, const char *rule);
	void applyDipthongRule(uint8_t p, unsigned short pf, uint8_t pos);
	void parser2();
	void adjustLengths();
	void updateFrequencyTables();
	static uint8_t translate(uint8_t a, uint8_t b);
	void checkForPhonemeEnd();
	void render();
	void processFrames(uint8_t mem48);
	void renderSample(uint8_t *mem66, uint8_t consonantFlag, uint8_t mem49);
	[[nodiscard]] uint8_t renderVoicedSample(unsigned short hi, uint8_t off, uint8_t phase1);
	void renderUnvoicedSample(unsigned short hi, uint8_t off, uint8_t mem53);
	[[nodiscard]] uint8_t createTransitions();
	void addInflection(uint8_t inflection, uint8_t pos);
	void rescaleAmplitude();
	void assignPitchContour();
	void createFrames();
	void combineGlottalAndFormants(uint8_t phase1, uint8_t phase2, uint8_t phase3, uint8_t Y);
	void writeOutput(int index, uint8_t outputSample);
	[[nodiscard]] uint8_t readTable(uint8_t table, uint8_t index);
	void writeTable(uint8_t table, uint8_t index, uint8_t value);
	void interpolate(uint8_t width, uint8_t table, uint8_t frame, char mem53);
	void interpolatePitch(uint8_t pos, uint8_t mem49, uint8_t phase3);

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
