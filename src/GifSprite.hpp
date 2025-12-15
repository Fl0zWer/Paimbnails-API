#pragma once

#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <fstream>

extern "C" {
#include "gifdec.h"
}

class GifSprite : public geode::prelude::CCSprite {
public:
    static GifSprite* create(const std::string& path);
    static GifSprite* createFromUrl(const std::string& url, const std::string& filename = "");

    bool init(const std::string& path);
    bool initFromUrl(const std::string& url, const std::string& filename);

    void update(float dt) override;
    void updateTexture();

    ~GifSprite();

protected:
    gd_GIF* m_gif = nullptr;
    uint8_t* m_buffer = nullptr;
    float m_timeAccumulator = 0.0f;
    bool m_running = false;
};
