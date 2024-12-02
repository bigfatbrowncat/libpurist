#pragma once

#include <memory>

class FrameBuffer;

class DisplayContents {
public:
    virtual ~DisplayContents() = default;

    virtual void drawIntoBuffer(FrameBuffer* buf) = 0;
};

class DisplayContentsFactory {
public:
    virtual ~DisplayContentsFactory() = default;

    virtual std::shared_ptr<DisplayContents> createDisplayContents() = 0;
};
