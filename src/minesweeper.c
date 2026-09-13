/**
 * @file minesweeper.c
 * Orx shell: MENU (difficulty) → PLAYING (camera + gesture → mines_* only).
 */

#include "orx.h"
#include "orxExtensions.h"
#include "mines.h"
#include "board_camera.h"
#include "gesture.h"
#include "touch_bridge.h"

#ifdef __orxMSVC__
__declspec(dllexport) unsigned long NvOptimusEnablement        = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

#define CELL_MAX (MINES_MAX_W * MINES_MAX_H)

/* Portrait phone frustum base (Galaxy S22); keep in sync with minesweeper.ini MainCamera. */
#define FRUSTUM_W 720.0f
#define FRUSTUM_H 1280.0f

typedef enum AppPhase {
  APP_MENU = 0,
  APP_PLAYING = 1
} AppPhase;

typedef enum Difficulty {
  DIFF_BEGINNER = 0,
  DIFF_INTERMEDIATE = 1,
  DIFF_EXPERT = 2
} Difficulty;

static MinesBoard   sstBoard;
static BoardCamera  sstCamera;
static GestureSM    sstGesture;
static orxVIEWPORT *spstViewport = orxNULL;
static orxCAMERA   *spstCamera   = orxNULL;
static orxOBJECT   *spstCells[CELL_MAX];
static orxBOOL      sbLeftWas    = orxFALSE;
static orxBOOL      sbRightWas   = orxFALSE;
static TouchBridge  sstTouch;
static orxBOOL      sbTouchActive = orxFALSE; /* Android: suppress mouse mirror */
static orxOBJECT   *spstModeHud   = orxNULL;
static char         sacLastCellChar[CELL_MAX];
static orxBOOL      sbHudArmed    = orxFALSE;

static const orxSTRING ModeLabel(GestureMode eMode)
{
  switch(eMode)
  {
    case GESTURE_MODE_FLAG:     return "F"; /* flag */
    case GESTURE_MODE_QUESTION: return "?";
    case GESTURE_MODE_REVEAL:
    default:                    return "*"; /* mine / reveal */
  }
}

static void SyncModeHud(void)
{
  if(spstModeHud)
  {
    orxObject_SetTextString(spstModeHud, ModeLabel(gesture_get_mode(&sstGesture)));
  }
}

/** Screen-space HUD hit (bottom-right). */
static orxBOOL HitModeHudScreen(orxFLOAT fSX, orxFLOAT fSY)
{
  orxFLOAT fW = orxFLOAT_0, fH = orxFLOAT_0;
  orxFLOAT fPad = 24.0f;
  orxFLOAT fBox = 140.0f;
  orxDisplay_GetScreenSize(&fW, &fH);
  if(fW <= orxFLOAT_0 || fH <= orxFLOAT_0)
  {
    return orxFALSE;
  }
  return (fSX >= fW - fBox - fPad) && (fSX <= fW - fPad)
      && (fSY >= fH - fBox - fPad) && (fSY <= fH - fPad);
}

static AppPhase     sePhase = APP_MENU;
static orxOBJECT   *spstTitle = orxNULL;
static orxOBJECT   *spstBtnBeginner = orxNULL;
static orxOBJECT   *spstBtnIntermediate = orxNULL;
static orxOBJECT   *spstBtnExpert = orxNULL;
static orxOBJECT   *spstBtnPlay = orxNULL;
/* Menu confirm: -1 = none; else Difficulty. Play calls StartGame. */
static orxS32       siMenuDiff = -1;

static orxBOOL ScreenXYToWorld(orxFLOAT fX, orxFLOAT fY, orxVECTOR *_pvOut)
{
  orxVECTOR vScreen;
  if(!_pvOut || !spstViewport)
  {
    return orxFALSE;
  }
  orxVector_Set(&vScreen, fX, fY, orxFLOAT_0);
  return orxRender_GetWorldPosition(&vScreen, spstViewport, _pvOut) ? orxTRUE : orxFALSE;
}

static orxBOOL ScreenToWorld(orxVECTOR *_pvOut)
{
  orxVECTOR vScreen;
  if(!_pvOut || !spstViewport)
  {
    return orxFALSE;
  }
  orxMouse_GetPosition(&vScreen);
  return orxRender_GetWorldPosition(&vScreen, spstViewport, _pvOut) ? orxTRUE : orxFALSE;
}

static void SyncOrxCamera(void)
{
  orxFLOAT fW, fH;
  if(!spstCamera)
  {
    return;
  }
  if(sePhase == APP_MENU)
  {
    /* Fixed portrait frustum for UI (ParentCamera children). */
    orxCamera_SetFrustum(spstCamera, FRUSTUM_W, FRUSTUM_H, orxFLOAT_0, orx2F(2.0f));
    {
      orxVECTOR v = {orxFLOAT_0, orxFLOAT_0, orxFLOAT_0};
      orxCamera_SetPosition(spstCamera, &v);
    }
    return;
  }
  /* Portrait world frustum from base_view / zoom — uniform scale, no backbuffer stretch. */
  board_camera_frustum(&sstCamera, &fW, &fH);
  orxCamera_SetFrustum(spstCamera, fW, fH, orxFLOAT_0, orx2F(2.0f));
  board_camera_set_view_half(&sstCamera, 0.5f * fW, 0.5f * fH);
  {
    orxVECTOR v = {sstCamera.center_x, sstCamera.center_y, orxFLOAT_0};
    orxCamera_SetPosition(spstCamera, &v);
  }
}

static char CellChar(const MinesCell *_pstCell)
{
  if(!_pstCell)
  {
    return '?';
  }
  if(_pstCell->state == MINES_FLAGGED)
  {
    return 'F';
  }
  if(_pstCell->state == MINES_QUESTION)
  {
    return '?';
  }
  if(_pstCell->state == MINES_CLOSED)
  {
    return '#';
  }
  if(_pstCell->is_mine)
  {
    return '*';
  }
  if(_pstCell->adjacent == 0)
  {
    return ' ';
  }
  return (char)('0' + _pstCell->adjacent);
}

static void ClearBoardVisuals(void)
{
  orxS32 i;
  for(i = 0; i < CELL_MAX; i++)
  {
    if(spstCells[i])
    {
      orxObject_Delete(spstCells[i]);
      spstCells[i] = orxNULL;
    }
    sacLastCellChar[i] = 0;
  }
}

static void RebuildBoardVisuals(void)
{
  orxS32 i, x, y, idx;
  orxVECTOR vPos;
  char zBuf[2] = {0, 0};

  ClearBoardVisuals();

  for(y = 0; y < sstBoard.height; y++)
  {
    for(x = 0; x < sstBoard.width; x++)
    {
      orxOBJECT *pstObj;
      idx = y * sstBoard.width + x;
      if(idx >= CELL_MAX)
      {
        continue;
      }
      pstObj = orxObject_CreateFromConfig("BoardCell");
      if(!pstObj)
      {
        continue;
      }
      board_camera_cell_center(&sstCamera, x, y, &vPos.fX, &vPos.fY);
      vPos.fZ = orxFLOAT_0;
      orxObject_SetPosition(pstObj, &vPos);
      zBuf[0] = CellChar(mines_at_c(&sstBoard, x, y));
      sacLastCellChar[idx] = zBuf[0];
      orxObject_SetTextString(pstObj, zBuf);
      spstCells[idx] = pstObj;
    }
  }
}

static void RefreshBoardVisuals(void)
{
  orxS32 x, y, idx;
  char zBuf[2] = {0, 0};
  char ch;
  for(y = 0; y < sstBoard.height; y++)
  {
    for(x = 0; x < sstBoard.width; x++)
    {
      idx = y * sstBoard.width + x;
      if(idx >= CELL_MAX || !spstCells[idx])
      {
        continue;
      }
      ch = CellChar(mines_at_c(&sstBoard, x, y));
      if(ch == sacLastCellChar[idx])
      {
        continue;
      }
      sacLastCellChar[idx] = ch;
      zBuf[0] = ch;
      orxObject_SetTextString(spstCells[idx], zBuf);
    }
  }
  SyncModeHud();
}

static void SetMenuVisible(orxBOOL bShow)
{
  if(spstTitle)
  {
    orxObject_Enable(spstTitle, bShow);
  }
  if(spstBtnBeginner)
  {
    orxObject_Enable(spstBtnBeginner, bShow);
  }
  if(spstBtnIntermediate)
  {
    orxObject_Enable(spstBtnIntermediate, bShow);
  }
  if(spstBtnExpert)
  {
    orxObject_Enable(spstBtnExpert, bShow);
  }
  if(spstBtnPlay)
  {
    orxObject_Enable(spstBtnPlay, bShow);
  }
  if(spstModeHud)
  {
    orxObject_Enable(spstModeHud, bShow ? orxFALSE : orxTRUE);
  }
}

/** Highlight selected difficulty; dim Play until a difficulty is chosen. */
static void SyncMenuSelectionVisual(void)
{
  orxVECTOR vScaleSel, vScaleOff, vPlaySc;
  orxVECTOR vColSel, vColOff, vPlayOn, vPlayOff;
  orxOBJECT *apst[3];
  orxS32 i;

  orxVector_Set(&vScaleSel, orx2F(0.65f), orx2F(0.65f), orxFLOAT_1);
  orxVector_Set(&vScaleOff, orx2F(0.55f), orx2F(0.55f), orxFLOAT_1);
  orxVector_Set(&vPlaySc, orx2F(0.75f), orx2F(0.75f), orxFLOAT_1);
  orxVector_Set(&vColSel, orx2F(1.0f), orx2F(0.95f), orx2F(0.35f));
  orxVector_Set(&vColOff, orx2F(0.70f), orx2F(0.82f), orx2F(1.0f));
  orxVector_Set(&vPlayOn, orx2F(0.45f), orx2F(0.90f), orx2F(0.50f));
  orxVector_Set(&vPlayOff, orx2F(0.35f), orx2F(0.45f), orx2F(0.40f));

  apst[0] = spstBtnBeginner;
  apst[1] = spstBtnIntermediate;
  apst[2] = spstBtnExpert;
  for(i = 0; i < 3; i++)
  {
    if(!apst[i])
    {
      continue;
    }
    if(siMenuDiff == i)
    {
      orxObject_SetScale(apst[i], &vScaleSel);
      orxObject_SetRGB(apst[i], &vColSel);
    }
    else
    {
      orxObject_SetScale(apst[i], &vScaleOff);
      orxObject_SetRGB(apst[i], &vColOff);
    }
  }
  if(spstBtnPlay)
  {
    orxObject_SetScale(spstBtnPlay, &vPlaySc);
    orxObject_SetRGB(spstBtnPlay, (siMenuDiff >= 0) ? &vPlayOn : &vPlayOff);
  }
}

/**
 * Default spawn: ~9 cells across portrait view at zoom=1 (tappable).
 * Uses Senior base_view API: base_view_w ≈ visible_cells * cell_size;
 * effective frustum = base_view / zoom. Not fit-all-board.
 * Zoom limits left at Senior defaults (0.5–4); default zoom=1 is below max.
 */
static void SetDefaultPlayableZoom(void)
{
  const orxFLOAT fVisibleCells = 9.0f;
  orxFLOAT fCs, fViewW, fViewH, fAspect;

  fCs = sstCamera.cell_size > orxFLOAT_0 ? sstCamera.cell_size : orxFLOAT_1;
  fAspect = FRUSTUM_H / FRUSTUM_W;
  fViewW = fVisibleCells * fCs;
  fViewH = fViewW * fAspect;

  sstCamera.center_x = (orxFLOAT)sstBoard.width * fCs * orx2F(0.5f);
  sstCamera.center_y = (orxFLOAT)sstBoard.height * fCs * orx2F(0.5f);

  board_camera_set_base_view(&sstCamera, fViewW, fViewH);
  /* Do not rewrite Senior min/max beyond ensuring default is below max. */
  if(sstCamera.zoom_max < orx2F(1.1f))
  {
    board_camera_set_zoom_limits(&sstCamera, sstCamera.zoom_min, orx2F(4.0f));
  }
  sstCamera.zoom = orxFLOAT_1;
  board_camera_frustum(&sstCamera, &fViewW, &fViewH);
  board_camera_set_view_half(&sstCamera, 0.5f * fViewW, 0.5f * fViewH);
}

static void StartGame(Difficulty eDiff)
{
  int w = 9, h = 9, mines = 10;
  uint32_t uSeed;

  switch(eDiff)
  {
    case DIFF_INTERMEDIATE:
      w = 16; h = 16; mines = 40;
      break;
    case DIFF_EXPERT:
      /* Portrait-friendly classic expert: 16 wide × 30 tall, 99 mines. */
      w = 16; h = 30; mines = 99;
      break;
    case DIFF_BEGINNER:
    default:
      w = 9; h = 9; mines = 10;
      break;
  }

  if(mines_init(&sstBoard, w, h) != 0)
  {
    orxLOG("StartGame: mines_init failed");
    return;
  }
  uSeed = (uint32_t)orxSystem_GetRealTime();
  if(uSeed == 0u)
  {
    uSeed = 1u;
  }
  if(mines_place_random(&sstBoard, mines, uSeed) != 0)
  {
    orxLOG("StartGame: mines_place_random failed");
    return;
  }

  board_camera_init(&sstCamera, sstBoard.width, sstBoard.height);
  SetDefaultPlayableZoom();
  gesture_init(&sstGesture, &sstBoard, &sstCamera);
  touch_bridge_init(&sstTouch, &sstGesture);

  SetMenuVisible(orxFALSE);
  RebuildBoardVisuals();
  if(!spstModeHud)
  {
    spstModeHud = orxObject_CreateFromConfig("ModeHud");
  }
  if(spstModeHud)
  {
    orxObject_Enable(spstModeHud, orxTRUE);
  }
  SyncModeHud();
  sePhase = APP_PLAYING;
  sbLeftWas = orxFALSE;
  sbRightWas = orxFALSE;
  sbTouchActive = orxFALSE;
  SyncOrxCamera();

  orxLOG("StartGame: %dx%d %d mines — HUD Reveal/Flag/Question; tap-N chord; camera clamped.",
         w, h, mines);
}

/**
 * ParentCamera UseParentSpace hit-test: screen → normalized parent space
 * (pivot center ⇒ 0 at mid, ~±0.5 at edges). Tiny half-extents — never
 * treat 40 world units as a minimum (that covered the whole frustum).
 */
static void ScreenToParentNorm(orxFLOAT fSX, orxFLOAT fSY, orxFLOAT *_pfNX, orxFLOAT *_pfNY)
{
  orxFLOAT fW = orxFLOAT_0, fH = orxFLOAT_0;
  orxDisplay_GetScreenSize(&fW, &fH);
  if(fW <= orxFLOAT_0 || fH <= orxFLOAT_0)
  {
    *_pfNX = orxFLOAT_0;
    *_pfNY = orxFLOAT_0;
    return;
  }
  *_pfNX = (fSX / fW) - orx2F(0.5f);
  *_pfNY = (fSY / fH) - orx2F(0.5f);
}

static orxBOOL PointHitsMenuBtn(orxOBJECT *_pstObj, orxFLOAT fNX, orxFLOAT fNY,
                                orxFLOAT fHalfW, orxFLOAT fHalfH)
{
  orxVECTOR vPos;
  if(!_pstObj || !orxObject_IsEnabled(_pstObj))
  {
    return orxFALSE;
  }
  /* Local parent-space position (ParentCamera children), not world AABB. */
  orxObject_GetPosition(_pstObj, &vPos);
  return (fNX >= vPos.fX - fHalfW && fNX <= vPos.fX + fHalfW
       && fNY >= vPos.fY - fHalfH && fNY <= vPos.fY + fHalfH);
}

/**
 * Two-step menu: tap Beginner/Intermediate/Expert → highlight selection;
 * tap Play → StartGame. Difficulty alone never starts the game.
 */
static void TryMenuPick(orxFLOAT fScreenX, orxFLOAT fScreenY)
{
  orxFLOAT fNX, fNY;
  const orxFLOAT fDiffHalfW = orx2F(0.42f);
  const orxFLOAT fDiffHalfH = orx2F(0.055f);
  const orxFLOAT fPlayHalfW = orx2F(0.32f);
  const orxFLOAT fPlayHalfH = orx2F(0.06f);

  ScreenToParentNorm(fScreenX, fScreenY, &fNX, &fNY);

  if(PointHitsMenuBtn(spstBtnBeginner, fNX, fNY, fDiffHalfW, fDiffHalfH))
  {
    siMenuDiff = (orxS32)DIFF_BEGINNER;
    SyncMenuSelectionVisual();
    orxLOG("Menu: selected Beginner — tap Play to start");
    return;
  }
  if(PointHitsMenuBtn(spstBtnIntermediate, fNX, fNY, fDiffHalfW, fDiffHalfH))
  {
    siMenuDiff = (orxS32)DIFF_INTERMEDIATE;
    SyncMenuSelectionVisual();
    orxLOG("Menu: selected Intermediate — tap Play to start");
    return;
  }
  if(PointHitsMenuBtn(spstBtnExpert, fNX, fNY, fDiffHalfW, fDiffHalfH))
  {
    siMenuDiff = (orxS32)DIFF_EXPERT;
    SyncMenuSelectionVisual();
    orxLOG("Menu: selected Expert — tap Play to start");
    return;
  }
  if(PointHitsMenuBtn(spstBtnPlay, fNX, fNY, fPlayHalfW, fPlayHalfH))
  {
    if(siMenuDiff < 0)
    {
      orxLOG("Menu: pick a difficulty before Play");
      return;
    }
    StartGame((Difficulty)siMenuDiff);
    return;
  }
}

static orxSTATUS orxFASTCALL TouchEventHandler(const orxEVENT *_pstEvent)
{
  orxSYSTEM_EVENT_PAYLOAD *pstPayload;
  orxVECTOR vWorld;

  if(!_pstEvent || _pstEvent->eType != orxEVENT_TYPE_SYSTEM)
  {
    return orxSTATUS_SUCCESS;
  }

  pstPayload = (orxSYSTEM_EVENT_PAYLOAD *)_pstEvent->pstPayload;
  if(!pstPayload)
  {
    return orxSTATUS_SUCCESS;
  }

  if(sePhase == APP_MENU)
  {
    /* Menu only — never route into touch_bridge / board gestures. */
    if(_pstEvent->eID == orxSYSTEM_EVENT_TOUCH_END)
    {
      TryMenuPick(pstPayload->stTouch.fX, pstPayload->stTouch.fY);
    }
    return orxSTATUS_SUCCESS;
  }

  if(_pstEvent->eID == orxSYSTEM_EVENT_TOUCH_BEGIN
     && HitModeHudScreen(pstPayload->stTouch.fX, pstPayload->stTouch.fY))
  {
    gesture_cycle_mode(&sstGesture);
    SyncModeHud();
    sbHudArmed = orxTRUE;
    return orxSTATUS_SUCCESS;
  }
  if(sbHudArmed && _pstEvent->eID == orxSYSTEM_EVENT_TOUCH_END)
  {
    sbHudArmed = orxFALSE;
    return orxSTATUS_SUCCESS;
  }

  if(!ScreenXYToWorld(pstPayload->stTouch.fX, pstPayload->stTouch.fY, &vWorld))
  {
    return orxSTATUS_SUCCESS;
  }

  sbTouchActive = orxTRUE;

  switch(_pstEvent->eID)
  {
    case orxSYSTEM_EVENT_TOUCH_BEGIN:
      touch_bridge_begin(&sstTouch, pstPayload->stTouch.u32ID, vWorld.fX, vWorld.fY);
      break;
    case orxSYSTEM_EVENT_TOUCH_MOVE:
      touch_bridge_move(&sstTouch, pstPayload->stTouch.u32ID, vWorld.fX, vWorld.fY);
      break;
    case orxSYSTEM_EVENT_TOUCH_END:
      touch_bridge_end(&sstTouch, pstPayload->stTouch.u32ID, vWorld.fX, vWorld.fY);
      if(!sstTouch.has0 && !sstTouch.has1)
      {
        sbTouchActive = orxFALSE;
      }
      break;
    default:
      break;
  }
  return orxSTATUS_SUCCESS;
}

static void HandleMenuInput(void)
{
  orxBOOL bLeft;
  orxVECTOR vScreen;

  if(orxInput_HasBeenActivated("Quit"))
  {
    orxEvent_SendShort(orxEVENT_TYPE_SYSTEM, orxSYSTEM_EVENT_CLOSE);
    return;
  }

  bLeft = orxMouse_IsButtonPressed(orxMOUSE_BUTTON_LEFT);
  if(bLeft && !sbLeftWas)
  {
    orxMouse_GetPosition(&vScreen);
    TryMenuPick(vScreen.fX, vScreen.fY);
  }
  sbLeftWas = bLeft;
}

static void HandlePendingInput(void)
{
  orxVECTOR vWorld;
  orxBOOL bLeft, bRight;

  if(orxInput_HasBeenActivated("Quit"))
  {
    orxEvent_SendShort(orxEVENT_TYPE_SYSTEM, orxSYSTEM_EVENT_CLOSE);
    return;
  }

  /* Touch path owns input while fingers are down (avoids Android mouse mirror double-fire). */
  if(sbTouchActive)
  {
    return;
  }

  if(!ScreenToWorld(&vWorld))
  {
    return;
  }

  bLeft  = orxMouse_IsButtonPressed(orxMOUSE_BUTTON_LEFT);
  bRight = orxMouse_IsButtonPressed(orxMOUSE_BUTTON_RIGHT);

  if(bLeft && !sbLeftWas)
  {
    orxVECTOR vScreen;
    orxMouse_GetPosition(&vScreen);
    if(HitModeHudScreen(vScreen.fX, vScreen.fY))
    {
      gesture_cycle_mode(&sstGesture);
      SyncModeHud();
      sbHudArmed = orxTRUE;
    }
    else
    {
      gesture_button_down(&sstGesture, GESTURE_BTN_LEFT, vWorld.fX, vWorld.fY);
    }
  }
  if(bRight && !sbRightWas)
  {
    gesture_button_down(&sstGesture, GESTURE_BTN_RIGHT, vWorld.fX, vWorld.fY);
  }

  if(bLeft || sstGesture.panning || (sstGesture.left_down && sstGesture.right_down))
  {
    gesture_pointer_move(&sstGesture, vWorld.fX, vWorld.fY);
  }

  if(!bLeft && sbLeftWas)
  {
    if(sbHudArmed)
    {
      sbHudArmed = orxFALSE;
    }
    else
    {
      gesture_button_up(&sstGesture, GESTURE_BTN_LEFT, vWorld.fX, vWorld.fY);
    }
  }
  if(!bRight && sbRightWas)
  {
    gesture_button_up(&sstGesture, GESTURE_BTN_RIGHT, vWorld.fX, vWorld.fY);
  }

  if(orxMouse_IsButtonPressed(orxMOUSE_BUTTON_WHEEL_UP)
     || orxInput_HasBeenActivated("ZoomIn"))
  {
    gesture_wheel(&sstGesture, 1.1f);
  }
  if(orxMouse_IsButtonPressed(orxMOUSE_BUTTON_WHEEL_DOWN)
     || orxInput_HasBeenActivated("ZoomOut"))
  {
    gesture_wheel(&sstGesture, 1.0f / 1.1f);
  }

  sbLeftWas  = bLeft;
  sbRightWas = bRight;
}

void orxFASTCALL Update(const orxCLOCK_INFO *_pstClockInfo, void *_pContext)
{
  (void)_pstClockInfo;
  (void)_pContext;

  if(sePhase == APP_MENU)
  {
    HandleMenuInput();
    SyncOrxCamera();
    return;
  }

  HandlePendingInput();
  SyncOrxCamera();
  RefreshBoardVisuals();
}

void orxFASTCALL CameraUpdate(const orxCLOCK_INFO *_pstClockInfo, void *_pContext)
{
  (void)_pstClockInfo;
  (void)_pContext;
  SyncOrxCamera();
}

orxSTATUS orxFASTCALL Init()
{
  InitExtensions();
  orxConfig_PushSection("Main");

  for(orxS32 i = 0, iCount = orxConfig_GetListCount("ViewportList"); i < iCount; i++)
  {
    orxVIEWPORT *pstVP = orxViewport_CreateFromConfig(orxConfig_GetListString("ViewportList", i));
    if(!spstViewport)
    {
      spstViewport = pstVP;
      if(pstVP)
      {
        spstCamera = orxViewport_GetCamera(pstVP);
      }
    }
  }
  orxConfig_PopSection();

  (void)orxObject_CreateFromConfig("Scene");
  sePhase = APP_MENU;
  ClearBoardVisuals();

  /* ParentCamera UI: create explicitly (reparented to MainCamera, not Scene). */
  spstTitle = orxObject_CreateFromConfig("Title");
  spstBtnBeginner = orxObject_CreateFromConfig("BeginnerBtn");
  spstBtnIntermediate = orxObject_CreateFromConfig("IntermediateBtn");
  spstBtnExpert = orxObject_CreateFromConfig("ExpertBtn");
  spstBtnPlay = orxObject_CreateFromConfig("PlayBtn");
  siMenuDiff = -1;

  SetMenuVisible(orxTRUE);
  SyncMenuSelectionVisual();
  SyncOrxCamera();

  orxEvent_AddHandler(orxEVENT_TYPE_SYSTEM, TouchEventHandler);
  orxEvent_SetHandlerIDFlags(TouchEventHandler, orxEVENT_TYPE_SYSTEM, orxNULL,
                             orxEVENT_GET_FLAG(orxSYSTEM_EVENT_TOUCH_BEGIN)
                               | orxEVENT_GET_FLAG(orxSYSTEM_EVENT_TOUCH_MOVE)
                               | orxEVENT_GET_FLAG(orxSYSTEM_EVENT_TOUCH_END),
                             orxEVENT_KU32_MASK_ID_ALL);

  orxClock_Register(orxClock_Get(orxCLOCK_KZ_CORE), Update, orxNULL, orxMODULE_ID_MAIN, orxCLOCK_PRIORITY_NORMAL);
  orxClock_Register(orxClock_Get(orxCLOCK_KZ_CORE), CameraUpdate, orxNULL, orxMODULE_ID_MAIN, orxCLOCK_PRIORITY_LOWER);

  orxLOG("Minesweeper MENU: select difficulty, then Play. Portrait frustum %.0fx%.0f.",
         FRUSTUM_W, FRUSTUM_H);

  return orxSTATUS_SUCCESS;
}

orxSTATUS orxFASTCALL Run()
{
  return orxSTATUS_SUCCESS;
}

void orxFASTCALL Exit()
{
  orxEvent_RemoveHandler(orxEVENT_TYPE_SYSTEM, TouchEventHandler);
  ClearBoardVisuals();
  ExitExtensions();
}

orxSTATUS orxFASTCALL Bootstrap()
{
  BootstrapExtensions();
  return orxSTATUS_SUCCESS;
}

int main(int argc, char **argv)
{
  orxConfig_SetBootstrap(Bootstrap);
  orx_Execute(argc, argv, Init, Run, Exit);
  return EXIT_SUCCESS;
}
