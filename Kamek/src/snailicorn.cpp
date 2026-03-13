#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <sfx.h>
#include <profile.h>
#include "boss.h"

const char* SCileList[] = {"snailicorn", NULL};

class dSnailicorn_c : public dEn_c {
	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();
	
	mHeapAllocator_c allocator;
	
	nw4r::g3d::ResFile resFile;
	nw4r::g3d::ResFile anmFile;
	m3d::mdl_c bodyModel;
	m3d::anmChr_c chrAnimation;
	
	nw4r::g3d::ResAnmTexPat anmPat;
	m3d::anmTexPat_c patAnimation;
	
	ActivePhysics chasePhysics;

	int timer;
	int walkPatTimer;
	int lookdownPatTimer;
	int texState;
	int Baseline;
	u32 cmgr_returnValue;
	bool run;
	float animSpeed;
	bool groundPounded;
	
	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate);
	void texPat_bindAnimChr_and_setUpdateRate(const char* name);
	
	void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther); 
	bool collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther);
    bool collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther);
	
	bool calculateTileCollisions();
	bool willWalkOntoSuitableGround(float delta = 2.5);
	void _vf148();
	void _vf14C();
	bool CreateIceActors();
	void _vf5C();

	lineSensor_s adjacentSensor;
	lineSensor_s belowSensor;

	void collectNearbyPickups();
	float nearestPlayerDistance();

	bool loaded;
	bool canChase;
	
	public: static dActor_c *build();
	public: dAcPy_c *daPlayer;
	
	void syncHitboxXDistToDirection();

	void addScoreWhenHit(void *other);
	void spawnHitEffectAtPosition(Vec2 pos);
	void doSomethingWithHardHitAndSoftHitEffects(Vec pos);
	void playEnemyDownSound2();
	void playHpdpSound1(); // plays PLAYER_SE_EMY/GROUP_BOOT/SE_EMY_DOWN_HPDP_S or _H
	void playEnemyDownSound1();
	void playEnemyDownComboSound(void *player); // AcPy_c/daPlBase_c?
	void playHpdpSound2(); // plays PLAYER_SE_EMY/GROUP_BOOT/SE_EMY_DOWN_HPDP_S or _H
	void _vf260(void *other); // AcPy/PlBase? plays the SE_EMY_FUMU_%d sounds based on some value
	void _vf264(dStageActor_c *other); // if other is player or yoshi, do Wm_en_hit and a few other things
	void _vf268(void *other); // AcPy/PlBase? plays the SE_EMY_DOWN_SPIN_%d sounds based on some value
	void _vf278(void *other); // AcPy/PlBase? plays the SE_EMY_YOSHI_FUMU_%d sounds based on some value
	void powBlockActivated(bool isNotMPGP);

	USING_STATES(dSnailicorn_c);
	DECLARE_STATE(Walk);
	DECLARE_STATE(Turn);
    DECLARE_STATE(Chase);
	DECLARE_STATE(Slide);
	DECLARE_STATE(Jump);
};

CREATE_STATE(dSnailicorn_c, Walk);
CREATE_STATE(dSnailicorn_c, Turn);
CREATE_STATE(dSnailicorn_c, Chase);
CREATE_STATE(dSnailicorn_c, Slide);
CREATE_STATE(dSnailicorn_c, Jump);

const SpriteData SnailicornData = {ProfileId::snailicorn, 8, 0, 0, 0, 0x100, 0x100, 0x40, 0x40, 0, 0, 0};
Profile SnailicornProfile(&dSnailicorn_c::build, SpriteId::snailicorn, &SnailicornData, ProfileId::snailicorn, ProfileId::snailicorn, "snailicorn", SCileList);

dActor_c* dSnailicorn_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dSnailicorn_c));
	return new(buf) dSnailicorn_c;
}

void dSnailicorn_c::syncHitboxXDistToDirection() {
	this->aPhysics.info.xDistToCenter = (this->direction == 0) ? -6.0f : 6.0f;
	this->chasePhysics.info.xDistToCenter = (this->direction == 0) ? 50.0f : -50.0f;
}


extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);
extern "C" void changePosAngle(VEC3 *, S16Vec *, int);

static inline bool isPickup(u16 name) {
	return (
		name == EN_COIN || name == EN_EATCOIN || name == AC_BLOCK_COIN || name == EN_COIN_JUGEM || name == EN_COIN_ANGLE ||
		name == EN_COIN_JUMP || name == EN_COIN_FLOOR || name == EN_COIN_VOLT || name == EN_COIN_WIND ||
		name == EN_BLUE_COIN || name == EN_COIN_WATER || name == EN_REDCOIN || name == EN_GREENCOIN ||
		name == EN_JUMPDAI || name == EN_ITEM || name == EN_STAR_COIN
	);
}

void dSnailicorn_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	if (apThis == &this->chasePhysics) 
	{
		if(this->canChase && !this->run)
		{
			this->run = true;

			this->syncHitboxXDistToDirection();
			doStateChange(&StateID_Jump);
		}

		return;
	}
	 
	apOther->someFlagByte |= 2;

	char hitType = usedForDeterminingStatePress_or_playerCollision(this, apThis, apOther, 0);
	
	daPlayer = (dAcPy_c*)apOther->owner;

	// figure out which way to slide
	this->direction = (((dEn_c*)apOther->owner)->pos.x > this->pos.x) ? 1 : 0;

	if(hitType == 1 | hitType == 3)
	{
		this->counter_504[apOther->owner->which_player] = 0xA;

		this->speed.x = (this->direction) ? -3.0 : 3.0;
        this->max_speed.x = (this->direction) ? -3.0 : 3.0;
        
		this->groundPounded = false;

		doStateChange(&StateID_Slide);
	}
	else if(hitType == 0) // Take damage
	{
		dEn_c::playerCollision(apThis, apOther);
		this->_vf220(apOther->owner);
	}
}
void dSnailicorn_c::_vf278(void *other) {
	PlaySound(this, SE_EMY_HANACHAN_STOMP);
}
void dSnailicorn_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther)
{
	if (apThis == &this->chasePhysics) return;

	this->playerCollision(apThis, apOther);
}
void dSnailicorn_c::spriteCollision(ActivePhysics *apThis, ActivePhysics *apOther) 
{
	if (apThis == &this->chasePhysics) return;

	u16 name = ((dEn_c*)apOther->owner)->name;

	if (isPickup(name)) {
		dEn_c* pickup = (dEn_c*)apOther->owner;
		if (pickup && daPlayer) {
			pickup->playerCollision(apOther, &daPlayer->aPhysics);
		}
		return;
	}
	
	if (acState.getCurrentState() == &StateID_Slide || acState.getCurrentState() == &StateID_Chase) 
	{
		// Get our enemy
		dEn_c* enemy = (dEn_c*)apOther->owner;
		if (!enemy) return;

		dAcPy_c *player = dAcPy_c::findByID(0);

		// Pretend we hit them with mario's star-man
		enemy->collisionCat3_StarPower(apOther, &player->aPhysics);
		return;
	}

	pos.x = ((pos.x - ((dEn_c*)apOther->owner)->pos.x) > 0) ? pos.x + 1.5 : pos.x - 1.5;
	doStateChange(&StateID_Turn); 
}

bool dSnailicorn_c::collisionCat14_YoshiFire(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (apThis == &this->chasePhysics) return;

    return BigHanaFireball(this, apThis, apOther);
}
bool dSnailicorn_c::collisionCat3_StarPower(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (apThis == &this->chasePhysics) return;
	
	bool hit = dEn_c::collisionCat3_StarPower(apThis, apOther);
	//doStateChange(&StateID_Die);
	
	return hit;
}
bool dSnailicorn_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (apThis == &this->chasePhysics) return;
	
	this->collisionCat9_RollingObject(apThis, apOther);
	//return true;
}
bool dSnailicorn_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (apThis == &this->chasePhysics) return;

	// figure out which way to slide
	this->direction = (((dEn_c*)apOther->owner)->pos.x > this->pos.x) ? 1 : 0;
	
	daPlayer = (dAcPy_c*)apOther->owner;

	this->speed.x = (this->direction) ? -4.0 : 4.0;
    this->max_speed.x = (this->direction) ? -4.0 : 4.0;
    
	this->groundPounded = true;

	doStateChange(&StateID_Slide);

	return true;
}
bool dSnailicorn_c::collisionCat1_Fireball_E_Explosion(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (apThis == &this->chasePhysics) return;

	return BigHanaFireball(this, apThis, apOther);
}
bool dSnailicorn_c::collisionCat2_IceBall_15_YoshiIce(ActivePhysics *apThis, ActivePhysics *apOther) {
	if (apThis == &this->chasePhysics) return;

	return BigHanaIceball(this, apThis, apOther);
}

extern "C" bool BigHanaFireball(dEn_c* t, ActivePhysics *apThis, ActivePhysics *apOther);
extern "C" bool BigHanaIceball(dEn_c* t, ActivePhysics *apThis, ActivePhysics *apOther);

/*	Ice Physics	*/
void dSnailicorn_c::_vf148()
{
	dEn_c::_vf148();
	this->Delete(1);
}
void dSnailicorn_c::_vf14C()
{
	dEn_c::_vf14C();
	this->Delete(1);
}

extern "C" void sub_80024C20(void);
extern "C" void __destroy_arr(void *, void(*)(void), int, int);
extern "C" int SmoothRotation(short* rot, u16 amt, int unk2);
extern "C" char usedForDeterminingStatePress_or_playerCollision(dEn_c* t, ActivePhysics *apThis, ActivePhysics *apOther, int unk1);
extern "C" int SomeStrangeModification(dStageActor_c* actor);
extern "C" void DoStuffAndMarkDead(dStageActor_c *actor, Vec vector, float unk);
extern "C" bool HandlesEdgeTurns(dEn_c* actor);
extern "C" bool SpawnEffect(const char*, int, Vec*, S16Vec*, Vec*);

void dSnailicorn_c::addScoreWhenHit(void *other) { }

void dSnailicorn_c::spawnHitEffectAtPosition(Vec2 pos) { }
void dSnailicorn_c::doSomethingWithHardHitAndSoftHitEffects(Vec pos) { }
void dSnailicorn_c::playEnemyDownSound2() { }
void dSnailicorn_c::playHpdpSound1() { } // plays PLAYER_SE_EMY/GROUP_BOOT/SE_EMY_DOWN_HPDP_S or _H
void dSnailicorn_c::playEnemyDownSound1() { }
void dSnailicorn_c::playEnemyDownComboSound(void *player) { } // AcPy_c/daPlBase_c?
void dSnailicorn_c::playHpdpSound2() { } // plays PLAYER_SE_EMY/GROUP_BOOT/SE_EMY_DOWN_HPDP_S or _H
void dSnailicorn_c::_vf260(void *other) { } // AcPy/PlBase? plays the SE_EMY_FUMU_%d sounds based on some value
void dSnailicorn_c::_vf264(dStageActor_c *other) { } // if other is player or yoshi, do Wm_en_hit and a few other things
void dSnailicorn_c::_vf268(void *other) { } // AcPy/PlBase? plays the SE_EMY_DOWN_SPIN_%d sounds based on some value

void dSnailicorn_c::powBlockActivated(bool isNotMPGP) {
}

void dSnailicorn_c::_vf5C() {
    // this is something EVERYONE gets wrong
    // newer does a crappy version of this in a "updateModelMatricies" function
    // you're supposed to do it like this in finalUpdate/_vf5C
    Mtx someMatrix;
    Mtx thirdMatrix;

    changePosAngle(&pos, &rot, 1);
    PSMTXTrans(matrix, pos.x, pos.y, pos.z);
    matrix.applyRotationY(&rot.y);
    
    PSMTXTrans(someMatrix, 0.0, pos_delta2.y, 0.0);
    PSMTXConcat(matrix, someMatrix, matrix);
    matrix.applyRotationX(&rot.x);

    PSMTXTrans(thirdMatrix, 0.0, -pos_delta2.y, 0.0);
    PSMTXConcat(matrix, thirdMatrix, matrix);
    matrix.applyRotationZ(&rot.z);

    bodyModel.setDrawMatrix(matrix);
    bodyModel.setScale(initialScale.x, initialScale.y, initialScale.z);
    bodyModel.calcWorld(false);
    return;
}

bool dSnailicorn_c::CreateIceActors()
{
	struct DoSomethingCool my_struct = { 0, this->pos, {1.5, 1.5, 1.5}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	this->frzMgr.Create_ICEACTORs( (void*)&my_struct, 1 );
	__destroy_arr( (void*)&my_struct, sub_80024C20, 0x3C, 1 );
	chrAnimation.setUpdateRate(0.0f);
	return true;
}

bool dSnailicorn_c::calculateTileCollisions()
{
	HandleXSpeed();
	HandleYSpeed();
	doSpriteMovement();
	
	cmgr_returnValue = collMgr.isOnTopOfTile();
	collMgr.calculateBelowCollisionWithSmokeEffect();
	
	float xDelta = pos.x - last_pos.x;
	if (xDelta >= 0.0f)
		direction = 0;
	else
		direction = 1;
	
	if(collMgr.isOnTopOfTile())
	{
		speed.y = 0.0f;
		max_speed.x = (direction == 1) ? -speed.x : speed.x;
	}
	else
		x_speed_inc = 0.0f;
	
	collMgr.calculateAdjacentCollision(0);
	
	if (collMgr.outputMaybe & (0x15 << direction)) {
		/*if (collMgr.isOnTopOfTile()) {
			isBouncing = true;
		}*/
		return true;
	}
	return false;
}


//Stolen from newer shyguy
bool dSnailicorn_c::willWalkOntoSuitableGround(float delta) {
	static const float deltas[] = {delta, -delta};
	VEC3 checkWhere = {
			pos.x + deltas[direction],
			4.0f + pos.y,
			pos.z};

	u32 props = collMgr.getTileBehaviour2At(checkWhere.x, checkWhere.y, currentLayerID);

	if (((props >> 16) & 0xFF) == 8)
		return false;

	float someFloat = 0.0f;
	if (collMgr.sub_800757B0(&checkWhere, &someFloat, currentLayerID, 1, -1)) {
		if (someFloat < checkWhere.y && someFloat > (pos.y - 5.0f))
			return true;
	}

	return false;
}

void dSnailicorn_c::updateModelMatrices() {
	return;

	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	bodyModel.setDrawMatrix(matrix);
	bodyModel.setScale(&scale);
	bodyModel.calcWorld(false);
}
 
void dSnailicorn_c::bindAnimChr_and_setUpdateRate(const char* name, int unk, float unk2, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->bodyModel, anmChr, unk);
	this->bodyModel.bindAnim(&this->chrAnimation, unk2);
	this->chrAnimation.setUpdateRate(rate);
}

int dSnailicorn_c::onCreate()
{
	allocator.link(-1, GameHeaps[0], 0, 0x20);
	
	this->resFile.data = getResource("snailicorn", "g3d/snailicorn.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("snailicorn");
	bodyModel.setup(mdl, &allocator, 0x227, 1, 0);

	SetupTextures_Enemy(&bodyModel, 0);

	this->anmFile.data = getResource("snailicorn", "g3d/snailicorn.brres");
	nw4r::g3d::ResAnmChr anmChr = this->anmFile.GetResAnmChr("walk");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);
	
	allocator.unlink();
	
	ActivePhysics::Info HitMeBaby;
	
	// Dist from center
	HitMeBaby.xDistToCenter = 6.0;
	HitMeBaby.yDistToCenter = 20.0;
	
	// Size
	HitMeBaby.xDistToEdge = 13.0;
	HitMeBaby.yDistToEdge = 15.0;
	
	HitMeBaby.category1 = 0x3;
	HitMeBaby.category2 = 0x0;
	HitMeBaby.bitfield1 = 0x4F;
	HitMeBaby.bitfield2 = 0xFFBAFFFE;
	HitMeBaby.unkShort1C = 0;
	HitMeBaby.callback = &dEn_c::collisionCallback;
	
	ActivePhysics::Info chaseInfo = HitMeBaby;
	chaseInfo.xDistToEdge = 30;
	chaseInfo.yDistToEdge = 10;

	this->aPhysics.initWithStruct(this, &HitMeBaby);
	this->aPhysics.addToList();
	this->chasePhysics.initWithStruct(this, &chaseInfo);
	this->chasePhysics.addToList();
	
	this->scale = (Vec){0.55f, 0.55f, 0.55f};

	pos_delta2.x = 0.0;
    pos_delta2.y = 8.0;
    pos_delta2.z = 0.0;
	
	this->rot.x = 0;
	this->rot.y = 0xD800;
	this->rot.z = 0; 
	this->direction = 1; // facing left
	
	this->speed.x = 0.0;
	this->speed.y = 0.0;
	this->max_speed.x = 0.6;
	this->x_speed_inc = 0.15;
	
	this->walkPatTimer = 0;
	this->lookdownPatTimer = 0;
	this->texState = 0;
	
	this->Baseline = this->pos.y;
	loaded = true;
	this->run = false;
	this->animSpeed = 1.7f;
	this->groundPounded = false;
	
	// STOLEN
	spriteSomeRectX = 28.0f;
	spriteSomeRectY = 32.0f;
	_320 = 0.0f;
	_324 = 16.0f;

	// "These structs tell stupid collider what to collide with - these are from koopa troopa" - Very well spoken ~
	static const lineSensor_s below(-5<<12, 5<<12, 0<<12);
	static const pointSensor_s above(0<<12, 12<<12);

	// SIDE SENSOR (member, not local!)
	adjacentSensor.flags =
		SENSOR_LINE |
		SENSOR_10000000; // breaking enabled dynamically

	adjacentSensor.lineA =  10 << 12;
	adjacentSensor.lineB = 20 << 12; // height of body
	adjacentSensor.distanceFromCenter = 6 << 12; // side offset

	collMgr.init(this, &below, &above, &adjacentSensor);
	
	collMgr.calculateBelowCollisionWithSmokeEffect();

	bindAnimChr_and_setUpdateRate("walk", 1, 0.0, 1.7); 
	doStateChange(&StateID_Walk);
	
	this->onExecute();
	return true;
}

int dSnailicorn_c::onDraw() {
	bodyModel.scheduleForDrawing();
	return true;
}

int dSnailicorn_c::onExecute() {
	acState.execute();
	updateModelMatrices();
	bodyModel._vf1C();

	this->syncHitboxXDistToDirection();
	
	// :)...
	
	/*dStateBase_c* currentState = this->acState.getCurrentState();
	OSReport("Current State: %s\n", currentState->getName());*/
	
	this->animSpeed = (this->run) ? 3.4f : 1.7f;

	if(acState.getCurrentState() == &StateID_Slide)
	{	
		collectNearbyPickups();
	
		adjacentSensor.flags |= SENSOR_BREAK_BLOCK | SENSOR_BREAK_BRICK;
	}
	else
		adjacentSensor.flags &= ~(SENSOR_BREAK_BLOCK | SENSOR_BREAK_BRICK);

	return true;
}

int dSnailicorn_c::onDelete() {
	return true;
}

static inline void awarddaCoins(int playerIndex, int amount) {
	if (playerIndex < 0 || playerIndex > 3)
		return;

	Player_Coins[playerIndex] += amount;

	nw4r::snd::SoundHandle handle;
	if (Player_Coins[playerIndex] > MaxCoins) {
		PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_SYS_100COIN_ONE_UP, 1);

		if (Player_Lives[playerIndex] >= 99)
			Player_Lives[playerIndex] = 99;
		else
			Player_Lives[playerIndex] += 1;

		int difference = Player_Coins[playerIndex] - MaxCoins;
		Player_Coins[playerIndex] = (difference - 1);
	}

	PlaySoundWithFunctionB4(SoundRelatedClass, &handle, SE_OBJ_GET_COIN, 1);
}

void dSnailicorn_c::collectNearbyPickups()
{
	if (!daPlayer)
		return;

	const ActivePhysics::Info &megaInfo = this->aPhysics.info;
	float megaLeft = this->pos.x + megaInfo.xDistToCenter - megaInfo.xDistToEdge;
	float megaRight = this->pos.x + megaInfo.xDistToCenter + megaInfo.xDistToEdge;
	float megaTop = this->pos.y + megaInfo.yDistToCenter + megaInfo.yDistToEdge;
	float megaBottom = this->pos.y + megaInfo.yDistToCenter - megaInfo.yDistToEdge;

	ActivePhysics *ap = ActivePhysics::globalListHead;
	while (ap) {
		dStageActor_c *ac = ap->owner;
		if (!ac || ac == this) {
			ap = ap->listPrev;
			continue;
		}

		if (!isPickup(ac->name)) {
			ap = ap->listPrev;
			continue;
		}

		const ActivePhysics::Info &info = ap->info;
		float otherLeft = ac->pos.x + info.xDistToCenter - info.xDistToEdge;
		float otherRight = ac->pos.x + info.xDistToCenter + info.xDistToEdge;
		float otherTop = ac->pos.y + info.yDistToCenter + info.yDistToEdge;
		float otherBottom = ac->pos.y + info.yDistToCenter - info.yDistToEdge;

		if (megaRight < otherLeft || megaLeft > otherRight) {
			ap = ap->listPrev;
			continue;
		}
		if (megaTop < otherBottom || megaBottom > otherTop) {
			ap = ap->listPrev;
			continue;
		}

		dEn_c *pickup = (dEn_c*)ac;
		pickup->playerCollision(ap, &daPlayer->aPhysics);

		ap = ap->listPrev;
	}

	int playerIndex = Player_ID[daPlayer->which_player];

	static const u16 manualCoinNames[] = {
		EN_COIN, EN_COIN_JUGEM, EN_COIN_JUMP, EN_COIN_FLOOR,
		EN_COIN_VOLT, EN_COIN_WIND, EN_COIN_WATER, EN_COIN_ANGLE
	};

	for (int i = 0; i < (int)(sizeof(manualCoinNames) / sizeof(manualCoinNames[0])); i++) {
		fBase_c *fb = 0;
		while ((fb = fBase_c::search((Actors)manualCoinNames[i], fb))) {
			dStageActor_c *ac = (dStageActor_c*)fb;
			if (!ac || ac == this)
				continue;

			if (ac->aPhysics.isLinkedIntoList)
				continue;

			float otherHalfX = (ac->aPhysics.info.xDistToEdge != 0.0f) ? ac->aPhysics.info.xDistToEdge : 8.0f;
			float otherHalfY = (ac->aPhysics.info.yDistToEdge != 0.0f) ? ac->aPhysics.info.yDistToEdge : 8.0f;
			otherHalfX *= ac->scale.x;
			otherHalfY *= ac->scale.y;

			float otherLeft = ac->pos.x - otherHalfX;
			float otherRight = ac->pos.x + otherHalfX;
			float otherTop = ac->pos.y + otherHalfY;
			float otherBottom = ac->pos.y - otherHalfY;

			if (megaRight < otherLeft || megaLeft > otherRight)
				continue;
			if (megaTop < otherBottom || megaBottom > otherTop)
				continue;

			awarddaCoins(playerIndex, 1);
			SpawnEffect("Wm_ob_coinkira", 0, &ac->pos, &(S16Vec){0,0,0}, &(Vec){1.0, 1.0, 1.0});
			ac->Delete(1);
		}
	}
}

void dSnailicorn_c::beginState_Walk() 
{
	this->timer = 0;
	this->rot.y = (direction) ? 0xD800 : 0x2800;
	
	this->max_speed.x = (direction) ? -0.5f : 0.5f;
	this->speed.x = (direction) ? -0.5f : 0.5f;

	this->max_speed.y = -4.0;
	this->speed.y = -4.0;
	this->y_speed_inc = -0.1875;

	this->run = false;
	this->canChase = false;

	this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 1.0f);
}

/* 		Walk 		*/
void dSnailicorn_c::executeState_Walk() 
{
	bool turn = (calculateTileCollisions() || (!willWalkOntoSuitableGround(2.5f) && collMgr.isOnTopOfTile()));
	if(turn) {
		this->run = false;
		this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 1.0f);
		doStateChange(&StateID_Turn);
	}
	
	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);

	this->timer++;

    if(this->timer >= 60)
		this->canChase = true;
}
void dSnailicorn_c::endState_Walk() 
{}

/* 		Turn 		*/
void dSnailicorn_c::beginState_Turn()
{
	this->direction ^= 1;
	this->speed.x = 0.0;
}
void dSnailicorn_c::executeState_Turn()
{
	this->canChase = false;
	this->run = false;

	collMgr.calculateBelowCollisionWithSmokeEffect();

	u16 rotationAmount = (this->direction) ? 0xD800 : 0x2800;
	int weDoneHere = SmoothRotation(&this->rot.y, rotationAmount, 0x150);
	
	if(weDoneHere)
		//if(this->run)
			doStateChange(&StateID_Walk);
		//else
		//	doStateChange(&StateID_Chase);
	
	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);
}
void dSnailicorn_c::endState_Turn()
{}

/*      Chase       */
void dSnailicorn_c::beginState_Chase() 
{
	this->run = true;

	this->timer = 0;
	
	this->rot.y = (direction) ? 0xD800 : 0x2800;
	
	this->max_speed.x = (direction) ? -0.5f : 0.5f;
	this->speed.x = (direction) ? -0.5f : 0.5f;
	
	this->max_speed.x *= 3;
	this->speed.x *= 3;

	this->max_speed.y = -4.0;
	this->speed.y = -4.0;
	this->y_speed_inc = -0.1875;

	this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 2.0f);
}

void dSnailicorn_c::executeState_Chase() 
{
	this->canChase = false;

	bool turn = (
                    calculateTileCollisions() || 
					(!willWalkOntoSuitableGround(2.5f) && collMgr.isOnTopOfTile()) //||
                    /*(this->direction != dSprite_c__getXDirectionOfFurthestPlayerRelativeToVEC3(this, this->pos))*/
                );
    
	if(turn && this->timer > 5) {
		doStateChange(&StateID_Turn);
	}
	
	this->timer++;

    if(this->timer >= 180)
	{   
		this->run = false;
		this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 1.0f);
		doStateChange(&StateID_Jump);
	}

	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);
}
void dSnailicorn_c::endState_Chase() 
{}

/* 		Slide 		*/
void dSnailicorn_c::beginState_Slide()
{
	this->rot.y = (this->direction) ? 0xD800 : 0x2800;
    this->timer = 0;

	this->bindAnimChr_and_setUpdateRate("hit", 1, 0.0f, 1.0f);

	this->max_speed.y = -4.0;
	this->speed.y = -4.0;
	this->y_speed_inc = -0.1875;
}
void dSnailicorn_c::executeState_Slide()
{
	this->canChase = false;
	this->run = false;
	
    if(this->timer > 0)
    {
        this->timer += 1;

        if(this->timer >= 60)
        {
			this->syncHitboxXDistToDirection();
			
			if(this->nearestPlayerDistance() <= 50.0f && this->timer >= 60)
			{
				this->run = true;
				this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 2.0f);
			}
			else
			{
				this->run = false;
				this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 1.0f);
			}

			doStateChange(&StateID_Jump);
		}

        return;
    }

	// Move & handle collisions
	bool hitWall = calculateTileCollisions();

	// If we hit a wall, flip direction and reflect horizontal speed
	if (hitWall) {
		this->direction ^= 1;
		this->speed.x = -this->speed.x;
	}

	if((!willWalkOntoSuitableGround(2.5f) && collMgr.isOnTopOfTile()))
	{
		if(!this->groundPounded && this->speed.x <= 2.5f) // We don't have enough speed to fly off and we were not ground-punded
			this->speed.x = 0.0f; // So don't fly off the ledge
	}

	// Apply simple horizontal friction
	const float friction = 0.08f;
	if (this->speed.x > 0.0f) {
		this->speed.x -= friction;
		if (this->speed.x < 0.0f)
			this->speed.x = 0.0f;
	}
	else if (this->speed.x < 0.0f) {
		this->speed.x += friction;
		if (this->speed.x > 0.0f)
			this->speed.x = 0.0f;
	}

	// When we've come to a stop on the ground, go back to walking
	if (this->speed.x == 0.0f && collMgr.isOnTopOfTile()) {
		this->timer = 1;
	}
}
void dSnailicorn_c::endState_Slide()
{}

/* Jump */
void dSnailicorn_c::beginState_Jump()
{
	this->timer = 0;

	this->rot.y = (direction) ? 0xD800 : 0x2800;

	// little hop
	this->speed.y = 2.0f;
	this->max_speed.y = 2.0f;

	this->y_speed_inc = -0.1875f;

	this->max_speed.x = (direction) ? -0.1f : 0.1f;
	this->speed.x = (direction) ? -0.1f : 0.1f;

	this->bindAnimChr_and_setUpdateRate("walk", 1, 0.0f, 2.0f);
}

void dSnailicorn_c::executeState_Jump()
{
	this->timer++;

	calculateTileCollisions();

	this->speed.y = this->speed.y - 0.3; 
	this->max_speed.y = this->max_speed.y - 0.3;

	// when we land, start chasing
	if (collMgr.isOnTopOfTile() && this->timer > 5) {
		if(!this->run)
			doStateChange(&StateID_Walk);
		else
			doStateChange(&StateID_Chase);
	}

	if(this->chrAnimation.isAnimationDone())
		this->chrAnimation.setCurrentFrame(0.0);
}

void dSnailicorn_c::endState_Jump()
{
}

float dSnailicorn_c::nearestPlayerDistance() {
	float bestSoFar = 10000.0f;

	for (int i = 0; i < 4; i++) {
		if (dAcPy_c *player = dAcPy_c::findByID(i)) {
			if (strcmp(player->states2.getCurrentState()->getName(), "dAcPy_c::StateID_Balloon")) {
				
				float thisDist = abs(player->pos.x - pos.x);

				if (thisDist < bestSoFar)
					if(abs(player->pos.y - pos.y) < 25)
						bestSoFar = thisDist;
			}
		}
	}

	return bestSoFar;
}