#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "GiantMask.h"
#include "Misc.h"
#include "controller.h"
#include "Reloc.h"
#include "Camera.h"
#include "z64scene.h"
#include "controller.h"
#include "z64actor_dlftbls.h"
#include "z64camera.h"

#define z2_PlayerWaitForGiantMask   Reloc_ResolvePlayerActorFunc(z2_PlayerWaitForGiantMask)
#define z2_PlayerWaitForGiantMask_VRAM   0x80838A20
#define s801D0B70                        (*(struct_801D0B70*)        0x801D0B70)

bool isGiant;

typedef struct {
    /* 0x00 */ PlayerOverlay kaleidoScope; // VRAM: [0x808160A0, 0x8082DA90)
    /* 0x1C */ PlayerOverlay playerActor; // VRAM: [0x8082DA90, 0x80862B70)
    /* 0x38 */ void* start; // RAM start address to use.
    /* 0x3C */ PlayerOverlay* selected; // Points to selected overlay.
} struct_801D0B70; // size = 0x40

typedef struct {
    /* 0x000 */ s16 frameCount;
    /* 0x002 */ s16 unk02;
} GenericAnimationHeader; // size = 0x4

typedef struct {
    /* 0x000 */ GenericAnimationHeader genericHeader;
    /* 0x004 */ u32 animationSegAddress;
} LinkAnimetionEntry; // size = 0x8

typedef struct {
    /* 0x00 */ f32 unk_00; // ceiling collision height
    /* 0x04 */ f32 unk_04; // initial shape scale // probably dont need to multiply
    /* 0x08 */ f32 unk_08; // draw offset for some reason?
    /* 0x0C */ f32 unk_0C; // ledge grab height
    /* 0x10 */ f32 unk_10; // ledge climb distance out of water from swimming
    /* 0x14 */ f32 unk_14; // ledge climb distance out of water from standing
    /* 0x18 */ f32 unk_18; // ? related to climbing while in water ?
    /* 0x1C */ f32 unk_1C; // ledge jump height
    /* 0x20 */ f32 unk_20;
    /* 0x24 */ f32 unk_24; // distance to floor when surfacing
    /* 0x28 */ f32 unk_28; // water floating height
    /* 0x2C */ f32 unk_2C; // water collision height
    /* 0x30 */ f32 unk_30; // distance to re-surface?
    /* 0x34 */ f32 unk_34; // drop off grab ledge height
    /* 0x38 */ f32 unk_38; // terrain collision distance
    /* 0x3C */ f32 unk_3C; // ? related to climbing
    /* 0x40 */ f32 unk_40; // ? related to climbing
    /* 0x44 */ Vec3s unk_44;
    /* 0x4A */ Vec3s unk_4A[4];
    /* 0x62 */ Vec3s unk_62[4];
    /* 0x7A */ Vec3s unk_7A[4];
    /* 0x92 */ u16 unk_92;
    /* 0x94 */ u16 unk_94;
    /* 0x98 */ f32 unk_98;
    /* 0x9C */ f32 unk_9C;
    /* 0xA0 */ LinkAnimetionEntry* unk_A0;
    /* 0xA4 */ LinkAnimetionEntry* unk_A4;
    /* 0xA8 */ LinkAnimetionEntry* unk_A8;
    /* 0xAC */ LinkAnimetionEntry* unk_AC;
    /* 0xB0 */ LinkAnimetionEntry* unk_B0;
    /* 0xB4 */ LinkAnimetionEntry* unk_B4[4];
    /* 0xC4 */ LinkAnimetionEntry* unk_C4[2];
    /* 0xCC */ LinkAnimetionEntry* unk_CC[2];
    /* 0xD4 */ LinkAnimetionEntry* unk_D4[2];
} PlayerageProperties; // size = 0xDC

enum PlayerState1 {
    PLAYER_STATE1_GROTTO_IN   = 0x80000000, // Link is entering a grotto. // Also voiding.
    PLAYER_STATE1_TARGET_FAR  = 0x40000000, // Target is too far away, auto-untargetting.
    PLAYER_STATE1_TIME_STOP   = 0x20000000, // Time is stopped but Link & NPC animations continue.
    PLAYER_STATE1_SPECIAL_2   = 0x10000000, // Form transition, using ocarina.
    PLAYER_STATE1_SWIM        = 0x08000000, // Swimming.
    PLAYER_STATE1_DAMAGED     = 0x04000000, // Damaged.
    PLAYER_STATE1_ZORA_FINS2  = 0x02000000, // Camera locked to thrown zora fins
    PLAYER_STATE1_ZORA_WEAPON = 0x01000000, // Zora fins are out, "Put Away" may be prompted.
    PLAYER_STATE1_EPONA       = 0x00800000, // On Epona.
    PLAYER_STATE1_SHIELD      = 0x00400000, // Shielding.
    PLAYER_STATE1_LADDER      = 0x00200000, // Climbing a ladder.
    PLAYER_STATE1_AIM         = 0x00100000, // Aiming bow, hookshot, Zora fins, etc.
    PLAYER_STATE1_FALLING     = 0x00080000, // In the air (without jumping beforehand).
    PLAYER_STATE1_AIR         = 0x00040000, // In the air (with jumping beforehand).
    PLAYER_STATE1_Z_VIEW      = 0x00020000, // In Z-target view.
    PLAYER_STATE1_Z_CHECK     = 0x00010000, // Z-target check-able or speak-able.
    PLAYER_STATE1_Z_ON        = 0x00008000, // Z-target enabled.
    PLAYER_STATE1_CLIMB_UP    = 0x00004000, // Getting up from climbing wall (not ladder)
    PLAYER_STATE1_LEDGE_HANG  = 0x00002000, // Hanging from ledge.
    PLAYER_STATE1_CHARGE_SPIN = 0x00001000, // Charging spin attack.
    PLAYER_STATE1_HOLD        = 0x00000800, // Hold above head.
    PLAYER_STATE1_GET_ITEM    = 0x00000400, // Hold new item over head.
    PLAYER_STATE1_TIME_STOP_2 = 0x00000200, // Time is stopped (does not affect Tatl, HUD animations).
    PLAYER_STATE1_GIANT_MASK  = 0x00000100, // Equipping Giant's Mask
    PLAYER_STATE1_MOVE_SCENE  = 0x00000020, // When walking in a cutscene? Used during Postman's minigame.
    PLAYER_STATE1_BARRIER     = 0x00000010, // Zora electric barrier.
    PLAYER_STATE1_ITEM_OUT    = 0x00000008, // Item is out, may later prompt "Put Away." Relevant to Bow, Hookshot, not Great Fairy Sword.
    PLAYER_STATE1_LEDGE_CLIMB = 0x00000004, // Climbing ledge.
    PLAYER_STATE1_TIME_STOP_3 = 0x00000002, // Everything stopped and Link is stuck in place.
    PLAYER_STATE1_GROTTO_OUT  = 0x00000001, // Link is exiting a grotto
};

static void GiantMask_ageProperties_Grow(PlayerAgeProperties* ageProperties) {
    ageProperties->ceilingCheckHeight *= 10.0;
    ageProperties->unk_0C *= 10.0;
    ageProperties->unk_10 *= 10.0;
    ageProperties->unk_14 *= 10.0;
    ageProperties->unk_18 *= 10.0;
    ageProperties->unk_1C *= 10.0;
    ageProperties->unk_24 *= 10.0;
    ageProperties->unk_28 *= 10.0;
    ageProperties->unk_2C *= 10.0;
    ageProperties->unk_30 *= 10.0;
    ageProperties->unk_34 *= 10.0;
    ageProperties->wallCheckRadius *= 10.0;
    ageProperties->unk_3C *= 10.0;
    ageProperties->unk_40 *= 10.0;
}

static void GiantMask_AgeProperties_Shrink(PlayerAgeProperties* ageProperties) {
    ageProperties->ceilingCheckHeight *= 0.1;
    ageProperties->unk_0C *= 0.1;
    ageProperties->unk_10 *= 0.1;
    ageProperties->unk_14 *= 0.1;
    ageProperties->unk_18 *= 0.1;
    ageProperties->unk_1C *= 0.1;
    ageProperties->unk_24 *= 0.1;
    ageProperties->unk_28 *= 0.1;
    ageProperties->unk_2C *= 0.1;
    ageProperties->unk_30 *= 0.1;
    ageProperties->unk_34 *= 0.1;
    ageProperties->wallCheckRadius *= 0.1;
    ageProperties->unk_3C *= 0.1;
    ageProperties->unk_40 *= 0.1;
}

static void GiantMask_Reg_Grow() {
    //REG(27);        // turning circle
    REG(48) *= 10; // slow backwalk animation threshold
    REG(19) *= 10;  // run acceleration // deceleration needs hook 806F9FE8
    // REG(30) /= 10; // base sidewalk animation speed
    REG(32) /= 10; // sidewalk animation speed multiplier
    // REG(34) *= 10; // ? unused ?
    // REG(35) *= 10; // base slow backwalk
    REG(36) /= 10; // slow backwalk animation speed multiplier
    REG(37) /= 10; // walk speed threshold
    REG(38) /= 10; // walk animation speed multiplier
    // REG(39) *= 10; // base walk animation speed
    REG(43) *= 10;  // idle deceleration
    REG(45) *= 10;  // running speed
    REG(68) *= 10;  // gravity
    // REG(69) *= 10;  // jump strength
    IREG(66) *= 10; // baby jump threshold
    // IREG(67) *= 10; // normal jump speed
    // IREG(68) *= 10; // baby jump base speed
    IREG(69) /= 10; // baby jump speed multiplier
    MREG(95) /= 10; // bow sidewalk animation?
}

static void GiantMask_SetTransformationFlash(PlayState* globalCtx, u8 r, u8 g, u8 b, u8 a) {
    MREG(64) = 1;
    MREG(65) = r;
    MREG(66) = g;
    MREG(67) = b;
    MREG(68) = a;
}

static void GiantMask_SetTransformationFlashAlpha(PlayState* globalCtx, u8 alpha) {
    MREG(68) = alpha;
}

static void GiantMask_DisableTransformationFlash(PlayState* globalCtx) {
    MREG(64) = 0;
}

typedef void (*z2_PlayerWaitForGiantMask_Func)(PlayState* ctxt, Player* player);

/* 0x1D14 */ static u32 sGiantsMaskCsTimer;
/* 0x1D18 */ static s16 sGiantsMaskCsState;
/* 0x1D22 */ static s16 sSubCamId;
/* 0x1D24 */ static Vec3f sSubCamEye;
/* 0x1D30 */ static Vec3f sSubCamAt;
/* 0x1D3C */ static Vec3f sSubCamUp;
/* 0x1D54 */ static f32 sSubCamUpRotZ;
/* 0x1D58 */ static f32 sSubCamUpRotZScale;
/* 0x1D5C */ static f32 sSubCamAtVel;
/* 0x1D64 */ static f32 sSubCamDistZFromPlayer;
/* 0x1D68 */ const static f32 sSubCamEyeOffsetY = 10.0f;
/* 0x1D6C */ static f32 sSubCamAtOffsetY;
             static f32 sSubCamAtOffsetTargetY;
/* 0x1D70 */ static f32 sPlayerScale = 0.01f;
/* 0x1D78 */ static u8 sGiantsMaskCsFlashState;
/* 0x1D7A */ static s16 sGiantsMaskCsFlashAlpha;

static bool sHasSeenGrowCutscene;
static bool sHasSeenShrinkCutscene;
static f32 sNextScaleFactor = 10.0f;

static bool sShouldReset = false;

void Matrix_RotateY(s16 y, MatrixMode mode) {
    Matrix_RotateYS(y, MTXMODE_NEW);
    Matrix_RotateYF(y, MTXMODE_NEW);
}

void GiantMask_Handle(Player* player, PlayState* globalCtx, Input* input) {

    if (globalCtx->sceneId == SCENE_INISIE_BS) {
        return;
    }

    s16 i;
    Vec3f subCamEyeOffset;
    bool performMainSwitch = false;
    s16 alpha;

    sGiantsMaskCsTimer++;

    switch (sGiantsMaskCsState) {
        case 0:
            if (player->stateFlags1 & PLAYER_STATE1_GIANT_MASK) {
                bool isShielding = player->stateFlags1 & PLAYER_STATE1_400000;
                if (!(player->stateFlags1 & PLAYER_STATE1_TIME_STOP) || isShielding) {
                    if (isShielding) {
                        player->heldItemAction = 0;
                    }
                    z2_PlayerWaitForGiantMask(globalCtx, player);
                    if (isShielding) {
                        player->heldItemAction = -1;
                    }
                }
                // z2_800EA0D4(globalCtx, &globalCtx->csCtx);
                sSubCamId = Play_CreateSubCamera(globalCtx);
                Camera_ChangeStatus(0, 1); // CAM_ID_MAIN, CAM_STATUS_WAIT
                Camera_ChangeStatus(0, 7); // CAM_STATUS_ACTIVE
                Play_EnableMotionBlur(150); // enable motion blur
                sGiantsMaskCsTimer = 0;
                sSubCamAtVel = 0.0f;
                sSubCamUpRotZScale = 0.0f;
                f32 playerHeight = Player_GetHeight(player);
                if (!GiantMask_IsGiant()) {
                    sGiantsMaskCsState = 1;
                    sSubCamDistZFromPlayer = 60.0f;
                    sSubCamAtOffsetY = playerHeight * 0.53f; // 23.0f;
                    sSubCamAtOffsetTargetY = playerHeight * 6.2f; // 273.0f;
                    sPlayerScale = 0.01f;
                    goto maskOn;
                } else {
                    sGiantsMaskCsState = 10;
                    sSubCamDistZFromPlayer = 200.0f;
                    sSubCamAtOffsetY = playerHeight * 0.62f; // 273.0f;
                    sSubCamAtOffsetTargetY = playerHeight * 0.053f; // 23.0f;
                    sPlayerScale = 0.1f;
                    goto maskOff;
                }
            }
            break;

        case 1:
            if ((sGiantsMaskCsTimer < 80U) && sHasSeenGrowCutscene &&
                CHECK_BTN_ANY(CONTROLLER1(&globalCtx->state)->press.button,
                              BTN_A | BTN_B | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT)) {
                sGiantsMaskCsState++;
                sGiantsMaskCsFlashState = 1;
                sGiantsMaskCsTimer = 0;
            } else {
            maskOn:
                if (sGiantsMaskCsTimer >= 50U) {
                    if (sGiantsMaskCsTimer == (u32)(BREG(43) + 60)) {
                        Audio_PlaySfx(0x9C5); // NA_SE_PL_TRANSFORM_GIANT
                    }
                    Math_ApproachF(&sSubCamDistZFromPlayer, 200.0f, 0.1f, sSubCamAtVel * 640.0f);
                    Math_ApproachF(&sSubCamAtOffsetY, sSubCamAtOffsetTargetY, 0.1f, sSubCamAtVel * 150.0f);
                    Math_ApproachF(&sPlayerScale, 0.1f, 0.2f, sSubCamAtVel * 0.1f);
                    Math_ApproachF(&sSubCamAtVel, 1.0f, 1.0f, 0.001f);
                } else {
                    Math_ApproachF(&sSubCamDistZFromPlayer, 30.0f, 0.1f, 1.0f);
                }

                if (sGiantsMaskCsTimer > 50U) {
                    Math_ApproachZeroF(&sSubCamUpRotZScale, 1.0f, 0.06f);
                } else {
                    Math_ApproachF(&sSubCamUpRotZScale, 0.4f, 1.0f, 0.02f);
                }

                if (sGiantsMaskCsTimer == 107U) {
                    sGiantsMaskCsFlashState = 1;
                }

                if (sGiantsMaskCsTimer < 121U) {
                    break;
                }

                performMainSwitch = true;
                sHasSeenGrowCutscene = true;
                goto done;
            }
            break;

        case 2:
            if (sGiantsMaskCsTimer < 8U) {
                break;
            }
            performMainSwitch = true;
            goto done;

        case 10:
            if ((sGiantsMaskCsTimer < 30U) && sHasSeenShrinkCutscene &&
                CHECK_BTN_ANY(CONTROLLER1(&globalCtx->state)->press.button,
                              BTN_A | BTN_B | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT)) {
                sGiantsMaskCsState++;
                sGiantsMaskCsFlashState = 1;
                sGiantsMaskCsTimer = 0;
                break;
            }

        maskOff:
            if (sGiantsMaskCsTimer != 0U) {
                if (sGiantsMaskCsTimer == (u32)(BREG(44) + 10)) {
                    Audio_PlaySfx(0x9C6); // NA_SE_PL_TRANSFORM_NORMAL
                }
                Math_ApproachF(&sSubCamDistZFromPlayer, 60.0f, 0.1f, sSubCamAtVel * 640.0f);
                Math_ApproachF(&sSubCamAtOffsetY, sSubCamAtOffsetTargetY, 0.1f, sSubCamAtVel * 150.0f);
                Math_ApproachF(&sPlayerScale, 0.01f, 0.1f, 0.003f);
                Math_ApproachF(&sSubCamAtVel, 2.0f, 1.0f, 0.01f);
            }

            if (sGiantsMaskCsTimer == 42U) {
                sGiantsMaskCsFlashState = 1;
            }

            if (sGiantsMaskCsTimer > 50U) {
                performMainSwitch = true;
                sHasSeenShrinkCutscene = true;
                goto done;
            }
            break;

        case 11:
            if (sGiantsMaskCsTimer < 8U) {
                break;
            }
            performMainSwitch = true;

        done:
        case 20:
            // sGiantsMaskCsState = 0;
            sGiantsMaskCsState = 21;
            func_80169AFC(globalCtx, sSubCamId, 0);
            sSubCamId = 0;
            // z2_800EA0EC(globalCtx, &globalCtx->csCtx);
            //actor.flags |= 1;
            player->stateFlags1 &= ~PLAYER_STATE1_GIANT_MASK;
            //sPlayerScale = 0.01f;
            Play_DisableMotionBlur();
            player->meleeWeaponState = 0;
            break;
        case 21:
            sGiantsMaskCsState = 0;
            if (GiantMask_IsGiant()) {
                GiantMask_Reg_Grow();
            }
            break;
    }

    if (performMainSwitch) {
        GiantMask_SetIsGiant(!GiantMask_IsGiant());
        if (!GiantMask_IsGiant()) {
            sPlayerScale = 0.01f;
            sNextScaleFactor = 10.0f;
        } else {
            sPlayerScale = 0.1f;
            sNextScaleFactor = 0.1f;
        }
    }

    if (player->currentMask == 0x14 && player->currentBoots == 1) {
        gSaveContext.magicState = 0xC;
        player->currentBoots = 2;
        func_80123140(globalCtx, player);
    } else if (player->currentMask != 0x14 && player->currentBoots == 2) {
        player->currentBoots = 1;
        func_80123140(globalCtx, player);
    }

    if (GiantMask_IsGiant()) {
        if (player->ageProperties->ceilingCheckHeight < 200.0) {
            GiantMask_ageProperties_Grow(player->ageProperties);
            player->actor.flags |= (1 << 17); // ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCH
        }
        if (REG(68) > -200) {
            GiantMask_Reg_Grow();
        }
    }
    else if (player->ageProperties->ceilingCheckHeight >= 200.0) {
        GiantMask_AgeProperties_Shrink(player->ageProperties);
        player->actor.flags &= ~(1 << 17); // ~ACTOR_FLAG_CAN_PRESS_HEAVY_SWITCH
    }

    if (player->transformation == PLAYER_FORM_FIERCE_DEITY) {
        Actor_SetScale(&player->actor, sPlayerScale * 1.5f);
    } else {
        Actor_SetScale(&player->actor, sPlayerScale);
    }

    switch (sGiantsMaskCsFlashState) {
        case 0:
            break;

        case 1:
            sGiantsMaskCsFlashAlpha = 0;
            GiantMask_SetTransformationFlash(globalCtx, 255, 255, 255, 0);
            sGiantsMaskCsFlashState = 2;
            Audio_PlaySfx(0x484F); // NA_SE_SY_TRANSFORM_MASK_FLASH

        case 2:
            sGiantsMaskCsFlashAlpha += 40;
            if (sGiantsMaskCsFlashAlpha >= 400) {
                sGiantsMaskCsFlashState = 3;
            }
            alpha = sGiantsMaskCsFlashAlpha;
            if (alpha > 255) {
                alpha = 255;
            }
            GiantMask_SetTransformationFlashAlpha(globalCtx, alpha);
            break;

        case 3:
            sGiantsMaskCsFlashAlpha -= 40;
            if (sGiantsMaskCsFlashAlpha <= 0) {
                sGiantsMaskCsFlashAlpha = 0;
                sGiantsMaskCsFlashState = 0;
                GiantMask_DisableTransformationFlash(globalCtx);
            } else {
                alpha = sGiantsMaskCsFlashAlpha;
                if (alpha > 255) {
                    alpha = 255;
                }
                GiantMask_SetTransformationFlashAlpha(globalCtx, alpha);
            }
            break;
    }

    if ((sGiantsMaskCsState != 0) && (sSubCamId != 0)) {
        // prevent all other cutscenes from starting
        CutsceneManager_ClearWaiting();

        Matrix_RotateY(player->actor.shape.rot.y, 0); // MTXMODE_NEW
        // Matrix_GetStateTranslationAndScaledZ(sSubCamDistZFromPlayer, &subCamEyeOffset);

        sSubCamEye.x = ((player->actor.world.pos.x) && (player->actor.shape.rot.x)) + subCamEyeOffset.x;
        sSubCamEye.y = ((player->actor.world.pos.y) && (player->actor.shape.rot.y)) + subCamEyeOffset.y + sSubCamEyeOffsetY;
        sSubCamEye.z = ((player->actor.world.pos.z) && (player->actor.shape.rot.z)) + subCamEyeOffset.z;

        sSubCamAt.x = ((player->actor.world.pos.x) && (player->actor.shape.rot.x));
        sSubCamAt.y = ((player->actor.world.pos.y) && (player->actor.shape.rot.y)) + sSubCamAtOffsetY;
        sSubCamAt.z = ((player->actor.world.pos.z) && (player->actor.shape.rot.z));

        sSubCamUpRotZ = Math_SinS(sGiantsMaskCsTimer * 1512) * sSubCamUpRotZScale;
        // Matrix_InsertZRotation_f(sSubCamUpRotZ, 1); // MTXMODE_APPLY
        // Matrix_GetStateTranslationAndScaledY(1.0f, &sSubCamUp);
        Play_SetCameraAtEyeUp(globalCtx, sSubCamId, &sSubCamAt, &sSubCamEye, &sSubCamUp);
        // ShrinkWindow_SetLetterboxTarget(0x1B);
    }
}

f32 GiantMask_GetScaleModifier() {
    return sPlayerScale * 100.0f;
}

f32 GiantMask_GetSimpleScaleModifier() {
    return GiantMask_IsGiant() ? 10.0f : 1.0f;
}

f32 GiantMask_GetSimpleInvertedScaleModifier() {
    return GiantMask_IsGiant() ? 0.1f : 1.0f;
}

f32 GiantMask_GetNextScaleFactor() {
    return sNextScaleFactor;
}

f32 GiantMask_GetFloorHeightCheckDelta(PlayState* globalCtx, Actor* actor, Vec3f* pos, s32 flags) {
    // Displaced code:
    f32 result = (flags & 0x800) ? 10.0f : 50.0f;
    // End displaced code

    if (actor->id == ACTOR_PLAYER) {
        result *= GiantMask_GetSimpleScaleModifier();
    }
    return result;
}

f32 GiantMask_GetLedgeWalkOffHeight(Actor* actor) {
    // Displaced code:
    f32 result = -11.0;
    // End displaced code

    if (actor->id == ACTOR_PLAYER) {
        result *= GiantMask_GetSimpleScaleModifier();
    }

    return result;
}

f32 GiantMask_Math_SmoothStepToF(f32* pValue, f32 target, f32 fraction, f32 step, f32 minStep) {
    const f32 modifier = GiantMask_GetSimpleScaleModifier();

    target *= modifier;
    step *= modifier;
    minStep *= modifier;

    return Math_SmoothStepToF(pValue, target, fraction, step, minStep);
}

f32 GiantMask_Math_StepToF(f32* pValue, f32 target, f32 step) {
    return Math_StepToF(pValue, target, step * GiantMask_GetSimpleScaleModifier());
}

f32 GiantMask_GetScaledFloat(f32 value) {
    return value * GiantMask_GetSimpleScaleModifier();
}

f32 GiantMask_GetInvertedScaledFloat(f32 value) {
    return value * GiantMask_GetSimpleInvertedScaleModifier();
}

void GiantMask_AdjustSpinAttackHeight(Actor* actor, ColliderCylinder* collider) {
    collider->dim.height *= GiantMask_GetSimpleScaleModifier();

    // Displaced code:
    Collider_UpdateCylinder(actor, collider);
    // End displaced code
}

bool GiantMask_IsGiant() {
    return isGiant;
}

void GiantMask_SetIsGiant(bool value) {
    isGiant = value;
}

void GiantMask_MarkReset() {
    sShouldReset = true;
}

void GiantMask_TryReset() {
    if (sShouldReset) {
        sPlayerScale = 0.01f;
        GiantMask_SetIsGiant(false);
        sNextScaleFactor = 10.0f;
        sShouldReset = false;
    }
}

void GiantMask_ClearState() {
    sGiantsMaskCsState = 0;
    sSubCamId = 0;
    sPlayerScale = GiantMask_IsGiant() ? 0.1f : 0.01f;
}

f32 GiantMask_GetHitDistance(Vec3f* position, Actor* hittingActor) {
    if (hittingActor->id == ACTOR_PLAYER) {
        if (GiantMask_IsGiant()) {
            return 0.0f;
        }
    }
    return Math3D_Vec3fDistSq(position, &hittingActor->projectedPos);
}

/*
C Buttons Anywhere
E9738CE0 595A
E9738CDE 595A
*/

// 80382530 REG

// TODO
// fall height damage // function at 806F43A0
// climb out of water 80700C50 1770, 80701248 43FA, 80701260 4348

RECOMP_HOOK("Player_UpdateCommon") void DoGiantsMask(Player* player, PlayState* globalCtx, Input* input) {
    GiantMask_Handle(player, globalCtx, input);
}
