#pragma once

#include <cstdint>

class Card;

class TargetSurface {
private:
    // Forbidding object copying
    TargetSurface(const TargetSurface& other) = delete;
    TargetSurface& operator = (const TargetSurface& other) = delete;

public:
	const Card& card;

	virtual uint32_t getWidth() const = 0;
	virtual uint32_t getHeight() const = 0;
	virtual uint32_t getStride() const = 0;
	virtual uint32_t getHandle() const = 0;

	TargetSurface(const Card& card) : card(card) { }
	virtual void makeCurrent() = 0;
	virtual void lock() = 0;
	virtual void swap() = 0;
	virtual void unlock() = 0;
    virtual void create(int width, int height) = 0;
    virtual void destroy() = 0;
	virtual ~TargetSurface() = default;
};
