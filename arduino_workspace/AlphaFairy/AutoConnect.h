bool app_poll();
void app_waitAllRelease();
void app_waitAllReleaseConnecting();
void app_waitAllReleaseUnsupported();
void app_sleep(uint32_t, bool);
void settings_save();
void pwr_lcdUndim();
void pwr_sleepCheck();
void pwr_tick(bool);
bool btnSide_hasPressed();
bool btnBig_hasPressed();
bool btnPwr_hasPressed();
bool btnBoth_hasPressed();
bool btnAny_hasPressed();
bool btnSide_isPressed();
bool btnBig_isPressed();
bool btnPwr_isPressed();
bool btnBoth_isPressed();
bool btnAll_isPressed();
void btnSide_clrPressed();
void btnBig_clrPressed();
void btnPwr_clrPressed();
void btnBoth_clrPressed();
void btnAny_clrPressed();
void autoconnect_poll();
void setup_autoconnect();
