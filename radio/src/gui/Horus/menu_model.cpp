/*
 * Authors (alphabetical order)
 * - Andre Bernet <bernet.andre@gmail.com>
 * - Andreas Weitl
 * - Bertrand Songis <bsongis@gmail.com>
 * - Bryan J. Rentoul (Gruvin) <gruvin@gmail.com>
 * - Cameron Weeks <th9xer@gmail.com>
 * - Erez Raviv
 * - Gabriel Birkus
 * - Jean-Pierre Parisy
 * - Karl Szmutny
 * - Michael Blandford
 * - Michal Hlavinka
 * - Pat Mackenzie
 * - Philip Moss
 * - Rob Thomson
 * - Thomas Husterer
 *
 * opentx is based on code named
 * gruvin9x by Bryan J. Rentoul: http://code.google.com/p/gruvin9x/,
 * er9x by Erez Raviv: http://code.google.com/p/er9x/,
 * and the original (and ongoing) project by
 * Thomas Husterer, th9x: http://code.google.com/p/th9x/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "../../opentx.h"

enum EnumTabModel {
  // e_ModelSelect,
  e_ModelSetup,
  CASE_HELI(e_Heli)
  CASE_FLIGHT_MODES(e_FlightModesAll)
  e_InputsAll,
  e_MixAll,
  e_Limits,
  CASE_CURVES(e_CurvesAll)
  CASE_GVARS(e_GVars)
  e_LogicalSwitches,
  e_CustomFunctions,
#if defined(LUA_MODEL_SCRIPTS)
  e_CustomScripts,
#endif
  CASE_FRSKY(e_Telemetry)
};

bool menuModelSelect(evt_t event);
bool menuModelSetup(evt_t event);
bool menuModelHeli(evt_t event);
bool menuModelFlightModesAll(evt_t event);
bool menuModelExposAll(evt_t event);
bool menuModelMixAll(evt_t event);
bool menuModelLimits(evt_t event);
bool menuModelCurvesAll(evt_t event);
bool menuModelCurveOne(evt_t event);
bool menuModelGVars(evt_t event);
bool menuModelLogicalSwitches(evt_t event);
bool menuModelCustomFunctions(evt_t event);
bool menuModelCustomScripts(evt_t event);
bool menuModelTelemetry(evt_t event);
bool menuModelExpoOne(evt_t event);

extern uint8_t s_curveChan;

#define FlightModesType uint16_t

void editCurveRef(coord_t x, coord_t y, CurveRef & curve, evt_t event, uint8_t attr);

#define MIXES_2ND_COLUMN    100

uint8_t editDelay(const coord_t x, const coord_t y, const evt_t event, const uint8_t attr, uint8_t delay)
{
  lcdDrawNumber(x+MIXES_2ND_COLUMN, y, (10/DELAY_STEP)*delay, attr|PREC1|LEFT);
  if (attr) CHECK_INCDEC_MODELVAR_ZERO(event, delay, DELAY_MAX);
  return delay;
}

const MenuFuncP menuTabModel[] = {
//   menuModelSelect,
  menuModelSetup,
  CASE_HELI(menuModelHeli)
  CASE_FLIGHT_MODES(menuModelFlightModesAll)
  menuModelExposAll,
  menuModelMixAll,
  menuModelLimits,
  CASE_CURVES(menuModelCurvesAll)
#if defined(GVARS) && defined(FLIGHT_MODES)
  CASE_GVARS(menuModelGVars)
#endif
  menuModelLogicalSwitches,
  menuModelCustomFunctions,
#if defined(LUA_MODEL_SCRIPTS)
  menuModelCustomScripts,
#endif
  CASE_FRSKY(menuModelTelemetry)
  CASE_MAVLINK(menuTelemetryMavlinkSetup)
  CASE_TEMPLATES(menuModelTemplates)
};

#define COPY_MODE 1
#define MOVE_MODE 2
static uint8_t s_copyMode = 0;
static int8_t s_copySrcRow;
static int8_t s_copyTgtOfs;

static uint8_t editNameCursorPos = 0;

void editName(coord_t x, coord_t y, char * name, uint8_t size, evt_t event, uint8_t active)
{
  uint8_t flags = 0;
  if (active && s_editMode <= 0) {
    flags = INVERS;
  }

  if (!active || s_editMode <= 0) {
    uint8_t len = zlen(name, size);
    if (len == 0) {
      char tmp[] = "---";
      lcdDrawTextWithLen(x, y, tmp, size, flags);
    }
    else {
      lcdDrawTextWithLen(x, y, name, len, ZCHAR | flags);
    }
  }

  if (active) {
    if (s_editMode > 0) {
      int8_t c = name[editNameCursorPos];
      int8_t v = c;

      if (event==EVT_ROTARY_RIGHT || event==EVT_ROTARY_LEFT) {
        v = checkIncDec(event, abs(v), 0, ZCHAR_MAX, 0);
        if (c <= 0) v = -v;
      }

      switch (event) {
        case EVT_KEY_BREAK(KEY_LEFT):
          if (editNameCursorPos>0) editNameCursorPos--;
          break;

        case EVT_KEY_BREAK(KEY_RIGHT):
          if (editNameCursorPos<size-1) editNameCursorPos++;
          break;

        case EVT_KEY_BREAK(KEY_ENTER):
          if (s_editMode == EDIT_MODIFY_FIELD) {
            s_editMode = EDIT_MODIFY_STRING;
            editNameCursorPos = 0;
          }
          else if (editNameCursorPos<size-1)
            editNameCursorPos++;
          else
            s_editMode = 0;
          break;

        case EVT_KEY_LONG(KEY_ENTER):
          if (v==0) {
            s_editMode = 0;
            killEvents(event);
            break;
          }
          // no break

        case EVT_KEY_LONG(KEY_LEFT):
        case EVT_KEY_LONG(KEY_RIGHT):
          if (v>=-26 && v<=26) {
            v = -v; // toggle case
            if (event==EVT_KEY_LONG(KEY_LEFT))
              killEvents(KEY_LEFT);
          }
          break;
      }

      if (c != v) {
        name[editNameCursorPos] = v;
        storageDirty(g_menuPos[0] == 0 ? EE_MODEL : EE_GENERAL);
      }

      lcdDrawTextWithLen(x, y, name, size, ZCHAR | flags);
      coord_t w = (editNameCursorPos == 0 ? 0 : getTextWidth(name, editNameCursorPos, ZCHAR));
      char s[] = { idx2char(name[editNameCursorPos]), '\0' };
      lcdDrawSolidFilledRect(x+w-1, y-INVERT_VERT_MARGIN, getTextWidth(s, 1)+1, INVERT_LINE_HEIGHT, TEXT_INVERTED_BGCOLOR);
      lcdDrawText(x+w, y, s, TEXT_INVERTED_COLOR);
    }
    else {
      editNameCursorPos = 0;
    }
  }
}

static uint8_t s_currIdx;
