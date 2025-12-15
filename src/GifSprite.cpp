#include "GifSprite.hpp"
#include <Geode/cocos/platform/CCGL.h>

using namespace geode::prelude;

GifSprite* GifSprite::create(const std::string& path) {
    auto ret = new GifSprite();
    if (ret && ret->init(path)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

GifSprite* GifSprite::createFromUrl(const std::string& url, const std::string& filename) {
    auto ret = new GifSprite();
    std::string fname = filename;
    if (fname.empty()) {
        // Generate filename from URL hash if not provided
        fname = std::to_string(std::hash<std::string>{}(url)) + ".gif";
    }
    
    if (ret && ret->initFromUrl(url, fname)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GifSprite::init(const std::string& path) {
    // 1. Open GIF stream
    m_gif = gd_open_gif(path.c_str());
    if (!m_gif) {
        log::error("Failed to open GIF: {}", path);
        return false;
    }

    // 2. Allocate buffer (RGB)
    m_buffer = new uint8_t[m_gif->width * m_gif->height * 3];

    // 3. Decode first frame
    gd_get_frame(m_gif);
    gd_render_frame(m_gif, m_buffer);

    // 4. Create texture
    auto texture = new CCTexture2D();
    texture->initWithData(
        m_buffer, 
        kCCTexture2DPixelFormat_RGB888, 
        m_gif->width, 
        m_gif->height, 
        CCSize(m_gif->width, m_gif->height)
    );
    
    if (!this->initWithTexture(texture)) {
        texture->release();
        return false;
    }
    texture->release();

    this->scheduleUpdate();
    m_running = true;
    return true;
}

bool GifSprite::initFromUrl(const std::string& url, const std::string& filename) {
    if (!CCSprite::init()) return false;

    auto savePath = Mod::get()->getSaveDir() / filename;

    // Show loading spinner
    auto loading = CCSprite::createWithSpriteFrameName("loadingCircle.png");
    if (loading) {
        loading->setID("loading-spinner"_spr);
        loading->setPosition(this->getContentSize() / 2);
        loading->runAction(CCRepeatForever::create(CCRotateBy::create(1.0f, 360.0f)));
        this->addChild(loading);
    }

    // Check if cached
    if (std::filesystem::exists(savePath)) {
        // Load immediately if cached
        Loader::get()->queueInMainThread([this, savePath] {
            if (this->getChildByID("loading-spinner"_spr)) {
                this->removeChildByID("loading-spinner"_spr);
            }
            
            // Re-init with local file
            // We need to manually call the logic from init(path) because we are already initialized as a node
            m_gif = gd_open_gif(savePath.string().c_str());
            if (m_gif) {
                m_buffer = new uint8_t[m_gif->width * m_gif->height * 3];
                gd_get_frame(m_gif);
                gd_render_frame(m_gif, m_buffer);
                
                auto texture = new CCTexture2D();
                texture->initWithData(m_buffer, kCCTexture2DPixelFormat_RGB888, m_gif->width, m_gif->height, CCSize(m_gif->width, m_gif->height));
                this->setTexture(texture);
                this->setTextureRect(CCRect(0, 0, m_gif->width, m_gif->height));
                texture->release();
                
                this->scheduleUpdate();
                m_running = true;
            }
        });
        return true;
    }

    // Download
    web::WebRequest()
        .get(url)
        .listen([this, savePath](web::WebResponse* res) {
            if (res && res->ok()) {
                auto data = res->data();
                // Save to disk
                std::ofstream file(savePath, std::ios::binary);
                file.write(reinterpret_cast<const char*>(data.data()), data.size());
                file.close();

                Loader::get()->queueInMainThread([this, savePath] {
                    if (this->getChildByID("loading-spinner"_spr)) {
                        this->removeChildByID("loading-spinner"_spr);
                    }

                    // Initialize from the saved file
                    m_gif = gd_open_gif(savePath.string().c_str());
                    if (m_gif) {
                        m_buffer = new uint8_t[m_gif->width * m_gif->height * 3];
                        gd_get_frame(m_gif);
                        gd_render_frame(m_gif, m_buffer);
                        
                        auto texture = new CCTexture2D();
                        texture->initWithData(m_buffer, kCCTexture2DPixelFormat_RGB888, m_gif->width, m_gif->height, CCSize(m_gif->width, m_gif->height));
                        this->setTexture(texture);
                        this->setTextureRect(CCRect(0, 0, m_gif->width, m_gif->height));
                        texture->release();
                        
                        this->scheduleUpdate();
                        m_running = true;
                    }
                });
            } else {
                log::error("Failed to download GIF: {}", res ? res->code() : -1);
                if (this->getChildByID("loading-spinner"_spr)) {
                    this->removeChildByID("loading-spinner"_spr);
                }
            }
        });

    return true;
}

void GifSprite::update(float dt) {
    if (!m_running || !m_gif) return;

    m_timeAccumulator += dt * 100.0f; // Convert to centiseconds

    int delay = m_gif->gce.delay;
    if (delay < 2) delay = 10; // Fix for bad GIFs

    if (m_timeAccumulator >= delay) {
        m_timeAccumulator -= delay;

        int result = gd_get_frame(m_gif);
        if (result == 1) {
            gd_render_frame(m_gif, m_buffer);
            this->updateTexture();
        } else if (result == 0) {
            // End of GIF, rewind
            gd_rewind(m_gif);
            gd_get_frame(m_gif);
            gd_render_frame(m_gif, m_buffer);
            this->updateTexture();
        }
    }
}

void GifSprite::updateTexture() {
    if (!this->getTexture()) return;
    
    ccGLBindTexture2D(this->getTexture()->getName());
    
    // FAST PATH: Update existing texture memory
    glTexSubImage2D(
        GL_TEXTURE_2D, 
        0, 
        0, 0, 
        m_gif->width, 
        m_gif->height, 
        GL_RGB, 
        GL_UNSIGNED_BYTE, 
        m_buffer
    );
}

GifSprite::~GifSprite() {
    if (m_gif) gd_close_gif(m_gif);
    if (m_buffer) delete[] m_buffer;
}
