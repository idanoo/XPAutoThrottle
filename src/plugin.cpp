#include "XPLMPlugin.h"
#include <cstring>

PLUGIN_API int XPluginStart(char* outName, char* outSignature, char* outDescription) {
    strcpy(outName, "XPAutothrottle");
    strcpy(outSignature, "nz.m2.xpautothrottle");
    strcpy(outDescription, "X-Plane AutoThrottle plugin");
    
    return 1;
}

PLUGIN_API void XPluginStop(void) {
    // Nothing to do
}

PLUGIN_API int XPluginEnable(void) {
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    // Nothing to do
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMessage, void* inParam) {
    // Nothing to do
}
