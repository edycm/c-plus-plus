#pragma once
#include <memory.h>
#include <stdlib.h>
#include <math.h>

class BitMap {
public:
	BitMap() : m_bitMap(nullptr), m_start(0), m_size(0) {}

	bool Init(int number, int start = 0) {
		m_start = start;
		m_size = number / 8 + 1;
		//m_bitMap = (unsigned char*)malloc(sizeof(unsigned char) * m_size);
		//memset(m_bitMap, 0, sizeof(unsigned char) * m_size);
		m_bitMap = new unsigned char[sizeof(unsigned char) * m_size];
		memset(m_bitMap, 0, sizeof(unsigned char) * m_size);
		return true;
	}

	bool Set(int number) {
		if (number > m_start + m_size * 8 || number < m_start) {
			return false;
		}
		
		int tmpValue = number - m_start;
		unsigned char x = 1 << tmpValue % 8;
		m_bitMap[tmpValue / 8] |= x;

		return true;
	}

	bool Get(int number) {
		if (number > m_start + m_size * 8 || number < m_start) {
			return false;
		}

		int tmpValue = number - m_start;
		unsigned char x = 1 << tmpValue % 8;
		int value = m_bitMap[tmpValue / 8] & x;
		return value > 0;
	}

	void UnInit() {
		if (m_bitMap) {
			//free(m_bitMap);
			delete m_bitMap;
		}
	}

	~BitMap() {}
private:
	unsigned char* m_bitMap;
	int m_start;
	int m_size;
};
