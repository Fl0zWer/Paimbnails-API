#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "GifSprite.hpp"

using namespace geode::prelude;

class $modify(MyMenuLayer, MenuLayer) {
	bool init() {
		if (!MenuLayer::init()) {
			return false;
		}

		log::debug("Hello from my MenuLayer::init hook! This layer has {} children.", this->getChildrenCount());

		auto myButton = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png"),
			this,
			menu_selector(MyMenuLayer::onMyButton)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);

		myButton->setID("my-button"_spr);

		menu->updateLayout();

		return true;
	}

	void onMyButton(CCObject*) {
		// Example usage of GifSprite
		// You can use a local path:
		// std::string gifPath = (Mod::get()->getConfigDir() / "test.gif").string();
		// auto gifSprite = GifSprite::create(gifPath);

		// Or a URL:
		std::string gifUrl = "https://media.tenor.com/g9a8Xm7t9tIAAAAi/geometry-dash.gif";
		log::info("Creating GifSprite from URL: {}", gifUrl);

		auto gifSprite = GifSprite::createFromUrl(gifUrl);
		
		if (gifSprite) {
			// Center it on screen
			auto winSize = CCDirector::get()->getWinSize();
			gifSprite->setPosition({winSize.width / 2, winSize.height / 2});
			
			// Optional: Set a fixed size if you want
			// gifSprite->setSize({100, 100});

			auto scene = CCDirector::get()->getRunningScene();
			scene->addChild(gifSprite);
			
			FLAlertLayer::create("Success", "GifSprite created! It will appear once downloaded and loaded.", "OK")->show();
		} else {
			FLAlertLayer::create("Error", "Failed to create GifSprite.", "OK")->show();
		}
	}
};
