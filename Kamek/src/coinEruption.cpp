#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>

const char* CoinEruptionList[] = {"kuribo", NULL};

class daCoinEruption_c : public dEn_c {
	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();
	
	int timer;
	int timerSecond;
	int amountOfCoins;
	int spread;
	int dir;
	bool spawned;
	u64 triggerEventOverride;
	
	dStageActor_c* daCoin;
	u32 coinID;
	
	public: static dActor_c *build();
	
	USING_STATES(daCoinEruption_c);
	DECLARE_STATE(Wait);
	DECLARE_STATE(Spawn);
};

CREATE_STATE(daCoinEruption_c, Wait);
CREATE_STATE(daCoinEruption_c, Spawn);

const SpriteData CoinEruptionData = {ProfileId::coinEruption, 8, -0x10, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile CoinEruptionProfile(&daCoinEruption_c::build, SpriteId::coinEruption, &GalaxyShellSpawnerData, ProfileId::coinEruption, ProfileId::coinEruption, "coinEruption", CoinEruptionList);

dActor_c* daCoinEruption_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(daCoinEruption_c));
	return new(buf) daCoinEruption_c;
}

int daCoinEruption_c::onCreate()
{
	char triggerEvent = (settings & 0xFF);
	this->triggerEventOverride = (triggerEvent == 0) ? 0 : ((u64)1 << (triggerEvent - 1));
	
	this->amountOfCoins = (settings & 0xF00) >> 8;
	switch(this->amountOfCoins)
	{
		case 0:
			this->amountOfCoins = 10;
			break;
		case 1:
			this->amountOfCoins = 50;
			break;
		case 2:
			this->amountOfCoins = 100;
			break;
		case 3:
			this->amountOfCoins = 200;
			break;
		case 4:
			this->amountOfCoins = 500;
			break;
	}
	
	this->spread = (settings & 0x3000) >> 12;
	switch(this->spread)
	{
		case 0:
			this->spread = 4;
			break;
		case 1:
			this->spread = 11;
			break;
		case 2:
			this->spread = 21;
			break;
		case 3:
			this->spread = 41;
			break;
	}
	
	this->dir = ((settings & 0xF0000) >> 16); // down, up
	this->spawned = false;
	this->timerSecond = 0;
	
	doStateChange(&StateID_Wait);
	
	this->onExecute();
	return true;
}

int daCoinEruption_c::onDraw() {
	return true;
}

int daCoinEruption_c::onExecute() 
{
	u64 effectiveFlag = triggerEventOverride | spriteFlagMask;
	
	if (dFlagMgr_c::instance->flags & effectiveFlag) 
		if(!this->spawned)
		{
			doStateChange(&StateID_Spawn);
			this->spawned = true;
		}
	
	acState.execute();
	return true;
}

int daCoinEruption_c::onDelete() {
	return true;
}

/*	Wait		*/
void daCoinEruption_c::beginState_Wait() 
{
	this->timer = 0;
}
void daCoinEruption_c::executeState_Wait() 
{
	if(!this->spawned) return;
	if(this->timerSecond > this->amountOfCoins) return;
	
	if(this->timer >= 12)
		doStateChange(&StateID_Spawn);
	
	this->timer++;
}
void daCoinEruption_c::endState_Wait() 
{}

/*	Respawn		*/
void daCoinEruption_c::beginState_Spawn() 
{
	this->timer = 0;
}
void daCoinEruption_c::executeState_Spawn() 
{
	// Leave if done
	/*if(this->timerSecond > this->amountOfCoins)
	{
		doStateChange(&StateID_Wait);
		return;
	}*/
	
	// Get our random speed
	float rndSpeed = GenerateRandomNumber(this->spread);
	rndSpeed -= 1;
	rndSpeed = (rndSpeed == 0) ? rndSpeed : rndSpeed * 0.1f;
	
	// Left or right conditions
	int isNeg = GenerateRandomNumber(2);
	rndSpeed = (isNeg == 1) ? rndSpeed : rndSpeed * -1;
	
	// Spawn our coin!
	daCoin = (dStageActor_c*)CreateActor(402, 0, this->pos, 0, 0);
	daCoin->speed.x = rndSpeed;
	
	if(this->dir == 1)
	{
		daCoin->speed.y = 6.0f;
	}
	else
	{
		daCoin->speed.y = -6.0f;
	}
	
	this->timerSecond += 1;
	doStateChange(&StateID_Wait);
}
void daCoinEruption_c::endState_Spawn() 
{}
