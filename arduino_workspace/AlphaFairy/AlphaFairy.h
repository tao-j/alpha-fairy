#ifndef _ALPHAFAIRY_H_
#define _ALPHAFAIRY_H_

#include <stdint.h>
#include <stdbool.h>

#include "alfy_conf.h"
#include "alfy_types.h"
#include "alfy_defs.h"

#include <M5StickCPlus.h>
#include <M5DisplayExt.h>
#include <SpriteMgr.h>
#include <FS.h>
#include <SPIFFS.h>
#include <PtpIpCamera.h>
#include <PtpIpSonyAlphaCamera.h>
#include <SonyHttpCamera.h>
#include "AlphaFairyCamera.h"
#include <AlphaFairy_NetMgr.h>
#include <AlphaFairyImu.h>
#include <FairyKeyboard.h>
#include <FairyEncoder.h>
#include <SerialCmdLine.h>
#include <SonyCameraInfraredRemote.h>
#include <DebuggingSerial.h>

// This list can be reduced
#include <CpuFreq.h>
#include <AppUtils.h>
#include <DrawingUtils.h>
#include <FairyMenu.h>
#include <WifiUtils.h>
#include <CamUtils.h>

#ifdef ENABLE_BUILD_LEPTON
#include <Lepton.h>
#endif

extern PtpIpSonyAlphaCamera ptpcam;
extern SonyHttpCamera       httpcam;
extern AlphaFairyCamera     fairycam;
extern SerialCmdLine        cmdline;
extern configsettings_t     config_settings;
extern AlphaFairyImu        imu;
extern FairyEncoder         fencoder;
extern SpriteMgr*           sprites;

extern uint32_t             gpio_time;
extern int wifi_err_reason;


extern
#ifdef DISABLE_ALL_MSG
    DebuggingSerialDisabled
#else
    DebuggingSerial
#endif
                            dbg_ser;

extern bool app_poll(void);

extern void app_waitAllRelease(void);
extern void app_waitAllReleaseConnecting(void);
extern void app_waitAllReleaseUnsupported(void);
extern void app_sleep(uint32_t x, bool forget_btns);

extern void settings_save(void);

extern void pwr_lcdUndim(void);
extern void pwr_sleepCheck(void);
extern void pwr_tick(bool);

extern bool btnSide_hasPressed(void);
extern bool btnBig_hasPressed(void);
extern bool btnPwr_hasPressed(void);
extern bool btnBoth_hasPressed(void);
extern bool btnAny_hasPressed(void);
extern bool btnSide_isPressed(void);
extern bool btnBig_isPressed(void);
extern bool btnPwr_isPressed(void);
extern bool btnBoth_isPressed(void);
extern bool btnAll_isPressed(void);
extern void btnSide_clrPressed(void);
extern void btnBig_clrPressed(void);
extern void btnPwr_clrPressed(void);
extern void btnBoth_clrPressed(void);
extern void btnAny_clrPressed(void);

extern void critical_error(const char *msg);

#endif
