#include <common.h>
#include <game.h>
#include <g3dhax.h>
#include <profile.h>

const char *ManholeCoverArcList[] = {"obj_manhole", NULL};

class dManholeCover_c : public dEn_c {
public:
	static dActor_c *build();

	int onCreate();
	int onExecute();
	int onDelete();
	int onDraw();

	void updateModelMatrices();
	void bindAnimChr_and_setUpdateRate(const char *name, float rate);

	void enableCollision();
	void disableCollision();

	void tryTriggerStepAnim();
	void tryTriggerSpinUpAnim();
	void startSpinDown();
	void startSpinUp();

	int updatePlayersOnBlock();

	void playerCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	void yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther);
	bool collisionCat11_PipeCannon(ActivePhysics *apThis, ActivePhysics *apOther);

	mHeapAllocator_c allocator;
	nw4r::g3d::ResFile resFile;
	m3d::mdl_c model;
	m3d::anmChr_c chrAnimation;

	StandOnTopCollider topCollider;
	bool colliderInList;

	bool isAnimating;
	bool wasPlayerOnTop;
};

const SpriteData ManholeCoverSpriteData = {ProfileId::manholecover, 8, 0, 0, 0, 0x100, 0x100, 0, 0, 0, 0, 0};
Profile ManholeCoverProfile(&dManholeCover_c::build, SpriteId::manholecover, &ManholeCoverSpriteData, ProfileId::manholecover, ProfileId::manholecover, "manholecover", ManholeCoverArcList);

dActor_c *dManholeCover_c::build() {
	void *buf = AllocFromGameHeap1(sizeof(dManholeCover_c));
	return new(buf) dManholeCover_c;
}

void dManholeCover_c::bindAnimChr_and_setUpdateRate(const char *name, float rate) {
	nw4r::g3d::ResAnmChr anmChr = this->resFile.GetResAnmChr(name);
	this->chrAnimation.bind(&this->model, anmChr, 1);
	this->model.bindAnim(&this->chrAnimation, 0.0f);
	this->chrAnimation.setUpdateRate(rate);
}

void dManholeCover_c::enableCollision() {
	if (!this->colliderInList) {
		this->topCollider.addToList();
		this->aPhysics.addToList();
		this->colliderInList = true;
	}
}

void dManholeCover_c::disableCollision() {
	if (this->colliderInList) {
		this->topCollider.removeFromList();
		this->aPhysics.removeFromList();
		this->colliderInList = false;
	}
}

void dManholeCover_c::startSpinDown() {
	this->disableCollision();
	this->bindAnimChr_and_setUpdateRate("spin_down", 1.0f);
	this->isAnimating = true;
}

void dManholeCover_c::startSpinUp() {
	if (this->isAnimating)
		return;

	this->disableCollision();
	this->bindAnimChr_and_setUpdateRate("spin_up", 1.0f);
	this->isAnimating = true;
}

int dManholeCover_c::updatePlayersOnBlock() {
    bool anyPlayersGoUp = false;
    
    for (int i = 0; i < 4; i++) {
        if (dAcPy_c *player = dAcPy_c::findByID(i)) {
            // Slightly wider X range to catch straddling across adjacent tiles
            if(player->pos.x >= pos.x - 14 && player->pos.x <= pos.x + 14) {
                if(player->pos.y >= pos.y - 5 && player->pos.y <= pos.y + 12) {
                    anyPlayersGoUp = true;
                }
            }
        }
    }
    
    return anyPlayersGoUp ? 1 : 0;
}

void dManholeCover_c::tryTriggerStepAnim() {
	if (!this->colliderInList || this->isAnimating)
		return;

	/* const float left = this->pos.x - 16.0f;
	const float right = this->pos.x + 16.0f;
	const float topY = this->pos.y + 6.0f;

	bool isPlayerOnTop = false;

	for (int i = 0; i < 4; i++) {
		dAcPy_c *player = dAcPy_c::findByID(i);
		if (!player)
			continue;

		const float centerX = player->pos.x + player->aPhysics.info.xDistToCenter;
		const float centerY = player->pos.y + player->aPhysics.info.yDistToCenter;
		const float pLeft = centerX - player->aPhysics.info.xDistToEdge;
		const float pRight = centerX + player->aPhysics.info.xDistToEdge;
		const float pBottom = centerY - player->aPhysics.info.yDistToEdge;

		const bool overlapsX = !(pRight < left || pLeft > right);
		const bool touchingTop = (pBottom >= topY - 2.0f && pBottom <= topY + 6.0f && player->speed.y <= 0.0f);

		if (overlapsX && touchingTop) {
			isPlayerOnTop = true;
			break;
		}
	} */

	bool isPlayerOnTop = updatePlayersOnBlock();

	if (isPlayerOnTop && !this->wasPlayerOnTop)
		this->bindAnimChr_and_setUpdateRate("step", 1.0f);

	this->wasPlayerOnTop = isPlayerOnTop;
}

void dManholeCover_c::tryTriggerSpinUpAnim() {
	if (!this->colliderInList || this->isAnimating)
		return;

	const float left = this->pos.x - 16.0f;
	const float right = this->pos.x + 16.0f;
	const float topY = this->pos.y + 6.0f;

	for (int i = 0; i < 4; i++) {
		dAcPy_c *player = dAcPy_c::findByID(i);
		if (!player)
			continue;

		const float centerX = player->pos.x + player->aPhysics.info.xDistToCenter;
		const float centerY = player->pos.y + player->aPhysics.info.yDistToCenter;
		const float pLeft = centerX - player->aPhysics.info.xDistToEdge;
		const float pRight = centerX + player->aPhysics.info.xDistToEdge;
		const float pTop = centerY + player->aPhysics.info.yDistToEdge;
		const float pBottom = centerY - player->aPhysics.info.yDistToEdge;

		const bool overlapsX = !(pRight < left || pLeft > right);
		const bool crossingFromBelow = (player->speed.y > 0.0f && pBottom < (topY - 8.0f) && pTop > (topY - 2.0f));

		if (overlapsX && crossingFromBelow) {
			this->startSpinUp();
			return;
		}
	}
}

void dManholeCover_c::playerCollision(ActivePhysics *apThis, ActivePhysics *apOther) {
	(void)apThis;
	(void)apOther;
}

void dManholeCover_c::yoshiCollision(ActivePhysics *apThis, ActivePhysics *apOther) {
	this->playerCollision(apThis, apOther);
}

bool dManholeCover_c::collisionCat7_GroundPound(ActivePhysics *apThis, ActivePhysics *apOther) {
    OSReport("Yess hello we are groundpounding\n");

	this->startSpinDown();
	return true;
}

bool dManholeCover_c::collisionCat7_GroundPoundYoshi(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCat7_GroundPound(apThis, apOther);
}

bool dManholeCover_c::collisionCatD_Drill(ActivePhysics *apThis, ActivePhysics *apOther) {
	return this->collisionCat7_GroundPound(apThis, apOther);
}

bool dManholeCover_c::collisionCat9_RollingObject(ActivePhysics *apThis, ActivePhysics *apOther) {
	(void)apThis;
	(void)apOther;
	return true;
}

bool dManholeCover_c::collisionCat13_Hammer(ActivePhysics *apThis, ActivePhysics *apOther) {
	(void)apThis;
	(void)apOther;
	return true;
}

bool dManholeCover_c::collisionCatA_PenguinMario(ActivePhysics *apThis, ActivePhysics *apOther) {
	(void)apThis;
	(void)apOther;
	return false;
}

bool dManholeCover_c::collisionCat5_Mario(ActivePhysics *apThis, ActivePhysics *apOther) {
	(void)apThis;
	(void)apOther;
	return false;
}

bool dManholeCover_c::collisionCat11_PipeCannon(ActivePhysics *apThis, ActivePhysics *apOther) {
	(void)apThis;
	(void)apOther;
	return false;
}

int dManholeCover_c::onCreate() {
	this->allocator.link(-1, GameHeaps[0], 0, 0x20);

	this->resFile.data = getResource("obj_manhole", "g3d/obj_manhole.brres");
	nw4r::g3d::ResMdl mdl = this->resFile.GetResMdl("obj_manhole");
	this->model.setup(mdl, &this->allocator, 0x224, 1, 0);
	SetupTextures_MapObj(&this->model, 0);

	nw4r::g3d::ResAnmChr anmChr = this->resFile.GetResAnmChr("step");
	this->chrAnimation.setup(mdl, anmChr, &this->allocator, 0);
	this->bindAnimChr_and_setUpdateRate("step", 0.0f);

	this->allocator.unlink();

	ActivePhysics::Info apInfo;
	apInfo.xDistToCenter = 0.0f;
	apInfo.yDistToCenter = 8.0f;
	apInfo.xDistToEdge = 16.0f;
	apInfo.yDistToEdge = 6.0f;
	apInfo.category1 = 0x3;
	apInfo.category2 = 0x0;
	apInfo.bitfield1 = 0x6F;
	apInfo.bitfield2 = 0xffbafffe;
	apInfo.unkShort1C = 0;
	apInfo.callback = &dEn_c::collisionCallback;
	this->aPhysics.initWithStruct(this, &apInfo);
    this->aPhysics.addToList();

	this->topCollider.init(this,
		/*xOffset=*/0.0f, /*yOffset=*/1.0f,
		/*topYOffset=*/0.0f,
		/*rightSize=*/16.0f, /*leftSize=*/-16.0f,
		/*rotation=*/0, /*unk_45=*/1);
	this->topCollider._47 = 0xA;
	this->topCollider.flags = 0x80180 | 0xC00;

	this->isAnimating = false;
	this->wasPlayerOnTop = false;
	this->colliderInList = false;
	this->enableCollision();

	return true;
}

int dManholeCover_c::onExecute() {
	this->model._vf1C();

	if (this->colliderInList) {
		this->topCollider.update();
	}

	this->tryTriggerSpinUpAnim();
	this->tryTriggerStepAnim();

	this->chrAnimation.process();

	if (this->isAnimating && this->chrAnimation.isAnimationDone()) {
		this->isAnimating = false;
		this->enableCollision();
		this->bindAnimChr_and_setUpdateRate("step", 0.0f);
	}

	this->updateModelMatrices();
	return true;
}

int dManholeCover_c::onDraw() {
	this->model.scheduleForDrawing();
	return true;
}

int dManholeCover_c::onDelete() {
	this->disableCollision();
	return true;
}

void dManholeCover_c::updateModelMatrices() {
	matrix.translation(pos.x, pos.y, pos.z);
	matrix.applyRotationYXZ(&rot.x, &rot.y, &rot.z);

	model.setDrawMatrix(matrix);
	model.setScale(&scale);
	model.calcWorld(false);
}
