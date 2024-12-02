#pragma once

#include "Card.h"

class DumbBuffer {
private:
    bool created = false;

public:
	const Card& card;

	const uint32_t stride;
	const uint32_t size;
	const uint32_t handle;

    const int width, height;

	DumbBuffer(const Card& card);
    void create(int width, int height);
    void destroy();
	virtual ~DumbBuffer();
};
