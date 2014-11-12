//
//  GameLayer.cpp
//  TweeJumpCocos2dx
//
//  Modified by Sanjeev Dwivedi, Dale Stammen and Eric Mitelette
//  Created by Carlos Pinan on 13/10/13.
//
//

#include "GameLayer.h"
#include "SimpleAudioEngine.h"

using namespace cocos2d;

CCScene* GameLayer::scene()
{
    CCScene* scene = CCScene::create();
    GameLayer* gameLayer = new GameLayer();
    gameLayer->autorelease();
    scene->addChild(gameLayer);
    return scene;
}

GameLayer::GameLayer()
{
    gameSuspended = true;

    CCSpriteBatchNode* batchNode = dynamic_cast<CCSpriteBatchNode*>(getChildByTag(kSpriteManager));

    // Initialize all the platforms
    _initPlatforms();

    // Load the rocketman sprite and call the startGame function which simply draws the rocketman in the middle of the screen
    CCSprite* rocketman = CCSprite::createWithTexture(batchNode->getTexture(), CCRectMake(608, 16, 50, 60));
    batchNode->addChild(rocketman, 4, kRocketMan);

    _startGame();

    scheduleUpdate();

    // We don't want touch interaction
    setTouchEnabled(false);

    setAccelerometerEnabled(true);

#if K_PLAY_BACKGROUND_MUSIC
    // play and loop background music during game
    auto soundEngine = CocosDenshion::SimpleAudioEngine::sharedEngine();
    soundEngine->playBackgroundMusic("Sounds/background.wav", true);
    soundEngine->setBackgroundMusicVolume(K_PLAY_BACKGROUND_MUSIC_VOLUME);
#endif
}

void GameLayer::_startGame()
{
    _resetPlatforms();
    _resetRocketMan();
}

// RocketMan logic
void GameLayer::_resetRocketMan()
{
    CCSpriteBatchNode* batchNode = dynamic_cast<CCSpriteBatchNode*>(getChildByTag(kSpriteManager));
    CCSprite* rocketMan = dynamic_cast<CCSprite*>(batchNode->getChildByTag(kRocketMan));

    rm_position.x = SCREEN_WIDTH * 0.5f;
    rm_position.y = SCREEN_WIDTH * 0.5f;
    rocketMan->setPosition(rm_position);

    rm_velocity.x = 0;
    rm_velocity.y = 0;

    rm_acceleration.x = 0;
    rm_acceleration.y = -550.0f;

    rm_lookingRight = true;
    rocketMan->setScaleX(1.0f);
}


void GameLayer::update(float dt)
{
    if (gameSuspended)
        return;

    CCSpriteBatchNode* batchNode = dynamic_cast<CCSpriteBatchNode*>(getChildByTag(kSpriteManager));
    CCSprite* rocketMan = dynamic_cast<CCSprite*>(batchNode->getChildByTag(kRocketMan));

    rm_position.x += rm_velocity.x * dt;
    // rm_lookingRight/Left is used to flip RocketMan in the right direction i.e. direction of the velocity
    // so RocketMan does not travel backwards
    if (rm_velocity.x < -30.0f && rm_lookingRight)
    {
        rm_lookingRight = false;

        // what is the point of setting scaleX?
        rocketMan->setScaleX(-1.0f);
    }
    else if (rm_velocity.x > 30.0f && !rm_lookingRight)
    {
        rm_lookingRight = true;
        rocketMan->setScaleX(1.0f);
    }

    CCSize rm_size = rocketMan->getContentSize();
    float max_x = SCREEN_WIDTH + rm_size.width * 0.5f;
    float min_x = -rm_size.width * 0.5f;

    if (rm_position.x > max_x)
        rm_position.x = min_x;

    if (rm_position.x < min_x)
        rm_position.x = max_x;

    rm_velocity.y += rm_acceleration.y * dt;
    rm_position.y += rm_velocity.y * dt;

    // Add the collision logic between the rocketman and the struss/asteroid. The rocketman collides only 
    // when he is falling down.
    int platformTag;
    if (rm_velocity.y < 0)
    {
        for (platformTag = kPlatformsStartTag; platformTag < kPlatformsStartTag + K_NUM_PLATFORMS; platformTag++)
        {
            CCSprite* platform = dynamic_cast<CCSprite*>(batchNode->getChildByTag(platformTag));
            CCSize platform_size = platform->getContentSize();
            CCPoint platform_position = platform->getPosition();

            max_x = platform_position.x - platform_size.width * 0.5f - 10;
            min_x = platform_position.x + platform_size.width * 0.5f + 10;
            float min_y = platform_position.y + (platform_size.height + rm_size.height) * 0.5f - K_PLATFORM_TOP_PADDING;

            // check if RocketMan and the platform is colliding, if so, make the Rocketman jump
            if (rm_position.x > max_x && rm_position.x < min_x &&
                rm_position.y > platform_position.y && rm_position.y < min_y)
                _jump();

            if (rm_position.y < -rm_size.height)
            {
                // TODO: (exit the game here)
            }
        }

    }
    else if (rm_position.y > SCREEN_HEIGHT * 0.5f)
    {
        // If the rocketman is going past half the screen, we move the platforms down
        // and add new platforms at the top to make it feel like the rocketman is moving up
        // Obviously, the platforms cannot go down, can they :)
        float delta = rm_position.y - SCREEN_HEIGHT * 0.5f;
        rm_position.y = SCREEN_HEIGHT * 0.5f;
        currentPlatformY -= delta;

        for (platformTag = kPlatformsStartTag; platformTag < kPlatformsStartTag + K_NUM_PLATFORMS; platformTag++)
        {
            CCSprite* platform = dynamic_cast<CCSprite*>(batchNode->getChildByTag(platformTag));
            CCPoint position = platform->getPosition();
            position = ccp(position.x, position.y - delta);

            // If the platform just became invisible, reset it to just above the screen
            if (position.y < -platform->getContentSize().height * 0.5f)
            {
                currentPlatformTag = platformTag;
                _resetPlatform();
            }
            else
            {
                // If the platform is still visible, decrease its Y coordinates so it looks like the scene is scrolling
                platform->setPosition(position);
            }
        }
    }

    // draw RocketMan at its new position
    rocketMan->setPosition(rm_position);
}

// when RocketMan is jumping, this is  its velocity
void GameLayer::_jump()
{
    // play sound effect when player jumps
#if K_PLAY_SOUND_EFFECTS
    CocosDenshion::SimpleAudioEngine::sharedEngine()->playEffect("Sounds/jump.wav");
#endif
    rm_velocity.y = 350.0f + fabsf(rm_velocity.x);
}


// When we exit the layer, stop all background music and any other audio effects being played
GameLayer::~GameLayer()
{
    CocosDenshion::SimpleAudioEngine::sharedEngine()->stopBackgroundMusic();
    CocosDenshion::SimpleAudioEngine::sharedEngine()->stopAllEffects();
}

void GameLayer::_initPlatform()
{
    CCRect rect;
    switch ((int)(CCRANDOM_0_1() * 2))
    {
    case 0:
        rect = CCRectMake(608, 90, 102, 22);
        break;
    case 1:
        rect = CCRectMake(608, 140, 102, 22);
        break;

    default:
        return;
    }
    CCSpriteBatchNode* batchNode = dynamic_cast<CCSpriteBatchNode*>(getChildByTag(kSpriteManager));
    CCSprite* platform = CCSprite::createWithTexture(batchNode->getTexture(), rect);
    batchNode->addChild(platform, 3, currentPlatformTag);
}

void GameLayer::_initPlatforms()
{
    currentPlatformTag = kPlatformsStartTag;
    while (currentPlatformTag < kPlatformsStartTag + K_NUM_PLATFORMS)
    {
        _initPlatform();
        currentPlatformTag++;
    }
}
void GameLayer::_resetPlatform()
{
    if (currentPlatformY < 0)
        currentPlatformY = 30.0f;
    else
    {
        currentPlatformY += CCRANDOM_0_1() * (int)(currentMaxPlatformStep - K_MIN_PLATFORM_STEP) + K_MIN_PLATFORM_STEP;
        if (currentMaxPlatformStep < K_MAX_PLATFORM_STEP)
            currentMaxPlatformStep += 0.5f;
    }

    CCSpriteBatchNode* batchNode = dynamic_cast<CCSpriteBatchNode*>(getChildByTag(kSpriteManager));
    CCSprite* platform = dynamic_cast<CCSprite*>(batchNode->getChildByTag(currentPlatformTag));

    if (CCRANDOM_0_1() * 2 == 1)
        platform->setScaleX(-1.0f);

    float x;

    CCSize size = platform->getContentSize();

    if (currentPlatformY == 30.0f)
        x = SCREEN_WIDTH * 0.5f;
    else
        x = CCRANDOM_0_1() * (SCREEN_WIDTH - (int)size.width) + size.width * 0.5f;

    platform->setPosition(ccp(x, currentPlatformY));
    platformCount++;
}

void GameLayer::_resetPlatforms()
{
    currentPlatformY = -1;
    currentPlatformTag = kPlatformsStartTag;
    currentMaxPlatformStep = 60.0f;

    platformCount = 0;

    while (currentPlatformTag < kPlatformsStartTag + K_NUM_PLATFORMS)
    {
        _resetPlatform();
        currentPlatformTag++;
    }

    gameSuspended = false;
}

void GameLayer::didAccelerate(CCAcceleration *pAccelerationValue)
{
    if (gameSuspended)
        return;

    // RocketMan's acceleration, left and right
    float accel_filter = 0.1f;
    rm_velocity.x = rm_velocity.x * accel_filter + pAccelerationValue->x * (1.0f - accel_filter) * 500.0f;
}
